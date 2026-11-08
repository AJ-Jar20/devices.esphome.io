# Jarvis PC agent: let Jarvis use the AI on your computer

Say *"Hey Jarvis, on my computer, write a Python script that renames my holiday photos by date"*.
Jarvis passes the task to an AI coding agent on your computer, the agent does the work in a work
folder, and Jarvis **announces the result** when it's done. The full output appears in Home Assistant.

```
"Hey Jarvis..." ─▶ Home Assistant (Pi 5) ─▶ jarvis_pc_agent.py (your PC) ─▶ Gemini CLI / Claude Code
                         ▲                                                          │
                         └──────────── result: spoken summary + notification ◀──────┘
```

## Which AI runs the tasks?

| Engine | Cost | Notes |
|---|---|---|
| **Gemini CLI** (default) | **Free** with a personal Google account (daily limits) | `npm install -g @google/gemini-cli`, then run `gemini` once to sign in |
| **Claude Code** | Needs a Claude Pro/Max subscription or API credits | `npm install -g @anthropic-ai/claude-code`, then run `claude` once to sign in |
| Any other CLI | - | Anything that takes the task as an argument and prints the answer |

Both need **Node.js 20+** (https://nodejs.org).

## Setup (about 10 minutes)

1. Install **Python 3.10+** (https://python.org; on Windows, tick *Add to PATH*).
2. Install and sign in to one engine from the table above.
3. Copy this `pc-agent` folder to your computer.
4. Copy `config.example.json` to `config.json` and edit it:
   - `token`: a long random password. Generate one with
     `python -c "import secrets; print(secrets.token_urlsafe(32))"`
   - `workdir`: the folder the AI is allowed to work in (default `~/JarvisWork`)
   - `command`: the engine (see below)
   - `callback_url`: your Home Assistant address, ending in `/api/webhook/jarvis_pc_done`
5. Start it: `python jarvis_pc_agent.py`
   You should see `Jarvis PC agent listening on 0.0.0.0:8765`.
6. Allow it through the firewall **on your home network only**. On Windows, click *Allow* on
   *Private networks* when asked.
7. In Home Assistant:
   - `/config/secrets.yaml`: `jarvis_pc_token: "Bearer <the same token>"`
   - In `jarvis_package.yaml`, set the computer's IP on the `jarvis_pc_task` line marked `# <-- CHANGE`.
     Give the PC a fixed IP in your router.
   - Restart Home Assistant and expose the **Jarvis computer task** script to Assist.

### Engine commands for `config.json`

Gemini CLI. It can create and edit files in the work folder, but it can't run shell commands:
```json
"command": ["gemini", "--approval-mode", "auto_edit", "-p", "{task}"]
```

Claude Code. Same idea: edits allowed, nothing else without permission:
```json
"command": ["claude", "-p", "{task}", "--permission-mode", "acceptEdits"]
```

`{task}` is replaced by what Jarvis asks for. Flag names can change between versions; check
`gemini --help` or `claude --help` if a command is refused.

### Start automatically

- **Windows:** press Win+R, type `shell:startup`, and put a shortcut there with
  target `pythonw.exe C:\path\to\jarvis_pc_agent.py`
- **macOS/Linux:** add `@reboot cd /path/to/pc-agent && python3 jarvis_pc_agent.py` with `crontab -e`

## Safety

- Every request must carry the token. Without it the agent answers `401` and does nothing.
- Keep port 8765 **inside your home network**. Never forward it on your router.
- The AI works only inside `workdir`, and the commands above don't allow running programs or
  deleting outside it. Only loosen that (e.g. Gemini `--approval-mode yolo`) if you fully understand
  the risk: anyone who can talk to Jarvis could then run commands on your PC.
- Jarvis is told to confirm before tasks that delete, send, buy or publish anything.
- Every task is logged in `jarvis_pc_agent.log`.

## Test without Jarvis

```bash
curl -X POST http://<pc-ip>:8765/task -H "Authorization: Bearer <token>" \
     -d '{"task": "Create notes.txt with three fun facts about Oman"}'
```

This returns `{"job_id": "...", "state": "queued"}`. When the task finishes, Jarvis speaks the summary.
