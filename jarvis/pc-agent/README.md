# Jarvis PC agent: let Jarvis use the AI on your computer

Say *"Hey Jarvis, on my computer, write a Python script that renames my holiday photos by date"*.
Jarvis passes the task to an AI coding agent on your computer, the agent does the work in a work
folder, and Jarvis **announces the result** when it's done. The full output appears in Home Assistant.

```
"Hey Jarvis..." ─▶ Home Assistant (Pi 4) ─▶ jarvis_pc_agent.py (Mac mini) ─▶ Claude Code or Gemini CLI
                         ▲                                                          │
                         └──────────── result: spoken summary + notification ◀──────┘
```

## Two engines, side by side

| Engine | Used when | Cost |
|---|---|---|
| **Claude Code** | You say "Claude", or for coding and complex multi-step work | Needs a Claude Pro/Max subscription (or API credits) |
| **Gemini CLI** | Everything else (the default) | **Free** with a personal Google account (daily limits) |

Jarvis picks the engine for each task. You can also just say it: *"Ask Claude to…"* or *"Use Gemini to…"*.

## Setup on your Mac mini (about 10 minutes)

1. Install **Homebrew** (https://brew.sh), then run in Terminal:
   ```bash
   brew install node python
   npm install -g @google/gemini-cli @anthropic-ai/claude-code
   gemini      # sign in with your Google account once, then type /quit
   claude      # sign in with your Claude account once, then type /exit
   ```
2. Copy this `pc-agent` folder to your home folder, so it's at `~/pc-agent`.
3. `cd ~/pc-agent && cp config.example.json config.json`, then edit `config.json`:
   - `token`: a long random password, from
     `python3 -c "import secrets; print(secrets.token_urlsafe(32))"`
   - `workdir`: the folder the AI may work in (default `~/JarvisWork`)
   - `engines` / `default_engine`: both engines are already set up (see below)
   - `callback_url`: `http://<Pi-4-IP>:8123/api/webhook/jarvis_pc_done`
4. Test it: `python3 jarvis_pc_agent.py`. You should see `listening on 0.0.0.0:8765`.
   When macOS asks *"Allow incoming network connections?"*, click **Allow**.
5. **Keep the Mac awake:** in **System Settings → Energy**, turn on *Prevent automatic sleeping
   when the display is off*. The screen can still turn off.
6. **Start automatically:** edit `com.jarvis.pcagent.plist` (replace `YOUR_USER` with the output of `whoami`), then:
   ```bash
   cp com.jarvis.pcagent.plist ~/Library/LaunchAgents/
   launchctl load ~/Library/LaunchAgents/com.jarvis.pcagent.plist
   ```
7. In Home Assistant (Pi 4):
   - `/config/secrets.yaml`: `jarvis_pc_token: "Bearer <the same token>"`
   - In `jarvis_package.yaml`, set the Mac's IP on the `jarvis_pc_task` line marked `# <-- CHANGE`.
     Give the Mac a fixed IP in your router.
   - Restart Home Assistant and expose the **Jarvis computer task** script to Assist.

### Engines in `config.json`

```json
"default_engine": "gemini",
"engines": {
  "gemini": ["gemini", "--approval-mode", "auto_edit", "-p", "{task}"],
  "claude": ["claude", "-p", "{task}", "--permission-mode", "acceptEdits"]
}
```

Both can create and edit files in the work folder, but they can't run other programs without
permission. `{task}` is replaced by what Jarvis asks for. Flag names can change between versions;
check `gemini --help` or `claude --help` if a command is refused. No Claude subscription? Delete the
`claude` line, and Jarvis uses Gemini for everything.

## Safety

- Every request must carry the token. Without it the agent answers `401` and does nothing.
- Keep port 8765 **inside your home network**. Never forward it on your router. (Tailscale is fine, see the main README.)
- The AI works only inside `workdir`, and the commands above don't allow running programs or
  deleting outside it. Only loosen that (e.g. Gemini `--approval-mode yolo`) if you fully understand
  the risk: anyone who can talk to Jarvis could then run commands on your Mac.
- Jarvis is told to confirm before tasks that delete, send, buy or publish anything.
- Every task is logged in `jarvis_pc_agent.log`.

## Test without Jarvis

```bash
curl -X POST http://<pc-ip>:8765/task -H "Authorization: Bearer <token>" \
     -d '{"task": "Create notes.txt with three fun facts about Oman", "engine": "claude"}'
```

This returns `{"job_id": "...", "engine": "claude", "state": "queued"}`. When the task finishes, Jarvis speaks the summary.
