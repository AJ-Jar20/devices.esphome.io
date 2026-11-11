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

Both need Node.js, which the Homebrew step below installs.

## Setup on your Mac mini (about 10 minutes)

1. Install **Homebrew** (https://brew.sh), then run in Terminal:
   ```bash
   brew install node python
   npm install -g @google/gemini-cli      # free engine
   gemini                                  # sign in with your Google account once, then /quit
   ```
   Optional, if you have a Claude subscription: `npm install -g @anthropic-ai/claude-code`, then run `claude` once to sign in.
2. Copy this `pc-agent` folder to your home folder, so it's at `~/pc-agent`.
3. `cd ~/pc-agent && cp config.example.json config.json`, then edit `config.json`:
   - `token`: a long random password, from
     `python3 -c "import secrets; print(secrets.token_urlsafe(32))"`
   - `workdir`: the folder the AI may work in (default `~/JarvisWork`)
   - `command`: the engine (see below)
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

## Safety

- Every request must carry the token. Without it the agent answers `401` and does nothing.
- Keep port 8765 **inside your home network**. Never forward it on your router. (Tailscale is fine, see the main README.)
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
