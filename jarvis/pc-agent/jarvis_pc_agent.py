#!/usr/bin/env python3
"""
Jarvis PC agent - lets Jarvis (Home Assistant) hand tasks to an AI on your computer.

  Jarvis -> Home Assistant -> POST http://<this-pc>:8765/task -> runs your AI CLI
  (Gemini CLI, Claude Code, ...) in a work folder -> reports back to Home Assistant.

Only the Python standard library is used. Works on Windows, macOS and Linux.

    python jarvis_pc_agent.py            # uses config.json next to this file

See README.md in this folder for setup.
"""

from __future__ import annotations

import hmac
import json
import logging
import shutil
import subprocess
import sys
import threading
import time
import urllib.request
import uuid
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

HERE = Path(__file__).resolve().parent
CONFIG_PATH = HERE / "config.json"

SUMMARY_HINT = (
    "\n\nWhen you have finished, end your reply with one line that starts with "
    "'SUMMARY:' followed by one short sentence, suitable to be spoken aloud, "
    "saying what you did or what you found."
)

log = logging.getLogger("jarvis-pc-agent")
jobs: dict[str, dict] = {}
jobs_lock = threading.Lock()
busy = threading.Semaphore(1)  # one AI task at a time


def load_config() -> dict:
    if not CONFIG_PATH.exists():
        sys.exit(f"Missing {CONFIG_PATH}. Copy config.example.json to config.json and edit it.")
    cfg = json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
    token = cfg.get("token", "")
    if len(token) < 24 or token.startswith("CHANGE"):
        sys.exit("Set a long random 'token' in config.json (at least 24 characters).")
    workdir = Path(cfg.get("workdir", HERE / "workspace")).expanduser()
    workdir.mkdir(parents=True, exist_ok=True)
    cfg["workdir"] = str(workdir)
    cfg.setdefault("host", "0.0.0.0")
    cfg.setdefault("port", 8765)
    cfg.setdefault("timeout_seconds", 900)
    cfg.setdefault("command", ["gemini", "--approval-mode", "auto_edit", "-p", "{task}"])
    cfg.setdefault("callback_url", "")
    return cfg


def build_command(template: list[str], task: str) -> list[str]:
    cmd = [part.replace("{task}", task) for part in template]
    exe = shutil.which(cmd[0])  # finds gemini.cmd / claude.cmd on Windows too
    if not exe:
        raise FileNotFoundError(f"'{cmd[0]}' was not found on PATH")
    cmd[0] = exe
    return cmd


def extract_summary(output: str, ok: bool) -> str:
    for line in reversed(output.strip().splitlines()):
        line = line.strip().strip("*").strip()
        if line.upper().startswith("SUMMARY:"):
            return line.split(":", 1)[1].strip()[:300]
    if not ok:
        return "The task on your computer did not finish successfully."
    tail = " ".join(output.strip().split())[-280:]
    return tail or "The task on your computer is finished."


def notify(cfg: dict, job: dict) -> None:
    url = cfg.get("callback_url")
    if not url:
        return
    body = json.dumps({
        "job_id": job["id"],
        "task": job["task"],
        "ok": job["ok"],
        "summary": job["summary"],
        "output": job["output"][-3500:],
        "seconds": job["seconds"],
    }).encode()
    req = urllib.request.Request(url, data=body, headers={"Content-Type": "application/json"})
    try:
        urllib.request.urlopen(req, timeout=15).read()
    except Exception as exc:  # Home Assistant may be restarting - not fatal
        log.warning("Could not reach Home Assistant callback: %s", exc)


def run_job(cfg: dict, job: dict) -> None:
    with busy:
        job["state"] = "running"
        started = time.time()
        try:
            cmd = build_command(cfg["command"], job["task"] + SUMMARY_HINT)
            log.info("Job %s: %s", job["id"], job["task"])
            proc = subprocess.run(
                cmd,
                cwd=cfg["workdir"],
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
                timeout=cfg["timeout_seconds"],
                stdin=subprocess.DEVNULL,
            )
            output = (proc.stdout or "") + (("\n" + proc.stderr) if proc.returncode and proc.stderr else "")
            job["ok"] = proc.returncode == 0
        except subprocess.TimeoutExpired:
            output, job["ok"] = "Timed out.", False
        except Exception as exc:
            output, job["ok"] = f"Error: {exc}", False
        job["output"] = output
        job["summary"] = extract_summary(output, job["ok"])
        job["seconds"] = round(time.time() - started)
        job["state"] = "done"
        log.info("Job %s finished ok=%s in %ss: %s", job["id"], job["ok"], job["seconds"], job["summary"])
        notify(cfg, job)


class Handler(BaseHTTPRequestHandler):
    cfg: dict = {}

    def _send(self, code: int, payload: dict) -> None:
        data = json.dumps(payload).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _authorised(self) -> bool:
        expected = "Bearer " + self.cfg["token"]
        given = self.headers.get("Authorization", "")
        return hmac.compare_digest(given.encode(), expected.encode())

    def do_GET(self) -> None:  # noqa: N802
        if self.path == "/health":
            return self._send(200, {"ok": True})
        if not self._authorised():
            return self._send(401, {"error": "unauthorised"})
        if self.path.startswith("/status/"):
            with jobs_lock:
                job = jobs.get(self.path.rsplit("/", 1)[-1])
            if not job:
                return self._send(404, {"error": "unknown job"})
            return self._send(200, {k: job[k] for k in ("id", "state", "ok", "summary", "seconds")})
        self._send(404, {"error": "not found"})

    def do_POST(self) -> None:  # noqa: N802
        if not self._authorised():
            return self._send(401, {"error": "unauthorised"})
        if self.path != "/task":
            return self._send(404, {"error": "not found"})
        try:
            length = min(int(self.headers.get("Content-Length", 0)), 20000)
            body = json.loads(self.rfile.read(length) or b"{}")
            task = str(body.get("task", "")).strip()
        except (ValueError, json.JSONDecodeError):
            return self._send(400, {"error": "invalid JSON"})
        if not task:
            return self._send(400, {"error": "missing 'task'"})
        job = {"id": uuid.uuid4().hex[:8], "task": task, "state": "queued",
               "ok": None, "summary": "", "output": "", "seconds": 0}
        with jobs_lock:
            jobs[job["id"]] = job
        threading.Thread(target=run_job, args=(self.cfg, job), daemon=True).start()
        self._send(202, {"job_id": job["id"], "state": "queued"})

    def log_message(self, fmt: str, *args) -> None:
        log.debug("%s - %s", self.address_string(), fmt % args)


def main() -> None:
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(message)s",
        handlers=[logging.StreamHandler(), logging.FileHandler(HERE / "jarvis_pc_agent.log", encoding="utf-8")],
    )
    cfg = load_config()
    Handler.cfg = cfg
    server = ThreadingHTTPServer((cfg["host"], int(cfg["port"])), Handler)
    log.info("Jarvis PC agent listening on %s:%s, work folder %s", cfg["host"], cfg["port"], cfg["workdir"])
    log.info("AI command: %s", " ".join(cfg["command"]))
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
