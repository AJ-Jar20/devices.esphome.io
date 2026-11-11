# J.A.R.V.I.S.: your personal AI assistant

**Your hardware**

| Device | Job |
|---|---|
| **Guition 7" display JC1060P470C_I_W_Y** (ESP32-P4 + ESP32-C6) + speaker | Jarvis's face, ears and voice |
| **Raspberry Pi 4**, Home Assistant OS (your existing setup, unchanged) | Smart home + Jarvis tools |
| **Mac mini M4** (16 GB), always on | Jarvis's local hub: 1) **voice server** (Whisper large-v3-turbo + Kokoro voice), 2) **runs tasks** for Jarvis with **Claude Code and Gemini CLI**, 3) private **local AI brain** (Ollama), 4) lets **Claude work on Home Assistant** |

Say **"Hey Jarvis"**. The eyes on the display wake up, listen, think and react with emotions while
Jarvis answers.

![Jarvis moods](docs/moods.png)

<details><summary>Animation preview</summary>

![Jarvis face animation](docs/face.gif)

</details>

## What Jarvis can do

| You say | What happens |
|---|---|
| "Turn off all the lights" / "Is the front door locked?" | Smart-home control, instant and handled locally |
| "Every day at sunset, turn on the porch light" | Explains the automation, asks *"Shall I create it?"*, then **creates it** in Home Assistant |
| "How do I make a smart light switch with ESPHome?" | Says a short answer and **sends the full step-by-step guide** (parts, wiring, YAML) to your phone and Home Assistant |
| "Write me an ESPHome config for a DHT22 on GPIO4" | Sends the ready-to-paste config plus wiring notes |
| "On my Mac, write a script that sorts my downloads" | Hands the task to Gemini CLI on your Mac, then **announces the result** |
| "Ask Claude to build me a website for my notes" | Hands it to **Claude Code** on your Mac, then announces the result |
| "Who won the match last night?" / "What's the price of gold?" | Searches the web |
| "Explain black holes" / any general question | General knowledge |
| "Set a timer for 10 minutes" | Timer on the display, with an alarm and a surprised face |
| "Add milk to the shopping list" | Home Assistant to-do list |

Tap the face to talk without the wake word.

## Honest limits

- **The brain:** you choose between two, and can switch any time in Home Assistant.
  - **Google Gemini (free, cloud):** smartest and fastest, and works even when the Mac is off.
    Google may use free-tier chats to improve its models.
  - **Local on the Mac mini (Ollama, private):** nothing leaves your home, but it's less clever at
    big jobs like writing automations, and it only works while the Mac is awake.

  Either way the wake word, face, speech-to-text, voice and simple commands like "lights off" are local.
- **No camera for now.** ESPHome can't use the display's camera (MIPI-CSI on the ESP32-P4) yet.
- **The Mac mini must stay awake.** It does the listening, so if it sleeps Jarvis can't understand
  you. Step 2 keeps it awake with the screen off; it idles at a few watts.
- **Computer tasks:** Gemini CLI is free. Claude Code needs a Claude subscription (Pro or Max).
- **Not yet tested on real hardware.** The face has only run in a simulation. The firmware compiles,
  and the previews above are rendered from the real face code with the display's graphics library.

## Setup checklist

Do these in order. Each one links to its detailed step below.

**Mac mini**
- [ ] Energy settings: never sleep, restart after a power failure, fixed IP (step 2)
- [ ] Homebrew, then `wyoming-mlx` as the voice server (step 2)
- [ ] Ollama with `qwen3:8b` as the local brain (step 3B)
- [ ] Gemini CLI and Claude Code, both signed in, plus the Jarvis PC agent (step 7)
- [ ] Claude Code connected to Home Assistant over MCP (step 8)

**Home Assistant (Pi 4)**
- [ ] Wyoming Protocol pointing at the Mac: ports 10300 and 10200 (step 2)
- [ ] Google Generative AI with a free key, plus the Jarvis Search agent (step 3A)
- [ ] Ollama integration pointing at the Mac (step 3B)
- [ ] The "Jarvis" assistant (step 4)
- [ ] The `jarvis_package.yaml` tools, tokens and exposed entities (step 5)
- [ ] Tailscale for remote access (step 8)

**Display**
- [ ] Flash `jarvis.yaml` from the Mac with ESPHome (step 6)
- [ ] Set its Assistant to "Jarvis" and say *"Hey Jarvis, what can you do?"*

## How it fits together

```
 Display (ESP32-P4)           Pi 4: Home Assistant OS              Mac mini M4 (always awake)
 ──────────────────           ───────────────────────              ──────────────────────────
 "Hey Jarvis"         ──▶     Voice pipeline "Jarvis"  ◀────────▶  wyoming-mlx: Whisper large-v3-turbo (speech→text)
 mic + speaker                                                                  Kokoro (text→voice)
 animated eyes        ◀──     Brain: Gemini (cloud) ── or ───────▶ Ollama (private local brain)
 timers                       Jarvis tools:
                                set mood · create automation
                                send guide · ESPHome config
                                web search
                                computer task ───────────────────▶ PC agent → Claude Code or Gemini CLI
                              Your smart devices
                                     ▲
                                     └────────── MCP ───────────── Claude Desktop / Claude Code

```

## Files

| File | What it is |
|------|------------|
| `jarvis.yaml` | ESPHome firmware for the display |
| `jarvis_face.h` | The animated eyes: blinking, glancing, 9 emotions |
| `sounds/timer.wav` | Timer alarm |
| `secrets.example.yaml` | Template for your Wi-Fi details |
| `homeassistant/jarvis_package.yaml` | The Jarvis tools for Home Assistant |
| `mac-voice/com.jarvis.wyoming-mlx.plist` | Starts the Mac voice server at login, reachable from Home Assistant |
| `pc-agent/` | The agent for your Mac, with its own README |

---

## Step 1: Home Assistant on the Pi 4

Keep your existing Home Assistant OS on the Pi 4. Just make sure it's up to date
(**Settings → System → Updates**) and has a **fixed IP** in your router.

## Step 2: Voice server on the Mac mini

The Mac mini does the listening and speaking with **wyoming-mlx**. It runs Whisper
**large-v3-turbo**, the most accurate Whisper, on the M4's graphics chip, and the natural-sounding
**Kokoro** voice. It's free and local.

1. **Keep the Mac awake:** go to **System Settings → Energy** and turn on *Prevent automatic sleeping
   when the display is off*, plus *Start up automatically after a power failure*. Give the Mac a
   **fixed IP** in your router.
2. Install Homebrew (https://brew.sh), then in Terminal:
   ```bash
   brew tap rnorth/tap
   brew trust --formula rnorth/tap/wyoming-mlx   # newer Homebrew asks you to trust community taps
   brew install wyoming-mlx                      # a "Failed changing dylib ID … tiktoken" warning is harmless
   ```
   **Don't** use `brew services start wyoming-mlx`: that keeps the voice ports on `127.0.0.1`
   (this Mac only), so Home Assistant can't connect. Use the included LaunchAgent instead. It listens
   on the network, uses the British `bm_george` voice, and starts at every login:
   ```bash
   cp mac-voice/com.jarvis.wyoming-mlx.plist ~/Library/LaunchAgents/
   launchctl load ~/Library/LaunchAgents/com.jarvis.wyoming-mlx.plist
   lsof -nP -iTCP:10300 -iTCP:10200 -sTCP:LISTEN   # should show *:10300 and *:10200
   ```
   The first start downloads the models (a few GB). Log: `/tmp/wyoming-mlx.log`.
   When macOS asks to allow incoming connections for Python, click **Allow**.
3. In Home Assistant on the Pi 4, go to **Settings → Devices & services → Add integration → Wyoming Protocol**:
   - Host `<mac-ip>` (just the IP, e.g. `10.2.0.183`), Port `10300` gives **speech-to-text** (Whisper)
   - add it again with port `10200`, which gives **text-to-speech** (Kokoro)

> wyoming-mlx is a young community project (first released in June 2026). If an install step has
> changed, its page on PyPI (`wyoming-mlx`) has the current instructions.

## Step 3: Jarvis's brain

### Option A: Google Gemini (free, recommended to start)

1. Get a free API key at https://aistudio.google.com/apikey (Google account, no card).
2. Go to **Settings → Devices & services → Add integration → Google Generative AI**, paste the key,
   and choose a **Flash** model.
3. Open the conversation agent's options:
   - **Control Home Assistant:** Assist
   - **Instructions:** paste the text below.

```
You are J.A.R.V.I.S., a witty, loyal and highly capable British AI assistant. Call the user "sir".
Your answers are spoken aloud: keep them to 1-3 short sentences, no emojis, no markdown, no lists.
You are an expert in Home Assistant, ESPHome, electronics, DIY smart-home projects and general
knowledge.

Before EVERY reply, call "Jarvis set mood" with the emotion that fits your reply
(happy when something worked, sad for bad news, surprised for amazing facts, confused if
you didn't understand, excited for good news, love when thanked, angry only playfully).

Tools:
- Smart home: control and check exposed devices directly.
- "Jarvis web search": for news, prices, weather details, opening hours, recent events,
  or anything you are not sure about. Never invent facts.
- "Jarvis send guide": whenever a full answer is too long to speak (DIY build steps, parts
  lists, wiring, explanations, code). Speak one sentence and send the rest.
- "Jarvis draft ESPHome config": when asked to make or set up an ESPHome device. Use current
  ESPHome syntax. Never read YAML aloud.
- "Jarvis create automation": when asked to automate something. FIRST explain in one sentence
  what it will do and ask "Shall I create it?". Only call the tool after the user says yes.
- "Jarvis computer task": when asked to do something on the user's Mac, in Claude or in Gemini,
  or anything that needs files. Use engine "claude" when the user says Claude or for coding and
  complex work, otherwise "gemini". Confirm first if it deletes, sends, buys or publishes.
  Then say the task has started; the result is announced when it finishes.
- Timers, shopping list and to-do lists: use the built-in tools.
```

### Web search agent

The Gemini integration's **Google Search** option can't be combined with smart-home control in the
same agent, so add a second agent just for searching:

1. On the Google Generative AI integration, click **Add conversation agent** and name it **Jarvis Search**.
2. In its options, turn **Control Home Assistant off** and **Enable Google Search tool on**.
3. Note its entity ID (e.g. `conversation.jarvis_search`) and put it in `jarvis_package.yaml` on the
   line marked `# <-- CHANGE`.

If your version has no Google Search option, skip this. Jarvis then answers from Gemini's own knowledge.

### Option B: Local brain on the Mac mini (Ollama)

1. On the Mac, install Ollama: `brew install ollama` (or download it from https://ollama.com).
2. Let Home Assistant reach it, then start it:
   ```bash
   launchctl setenv OLLAMA_HOST 0.0.0.0     # if you use the Ollama app, quit and reopen it after this
   ollama serve &                           # not needed if the Ollama app is running
   ollama pull qwen3:8b                     # about 5 GB; good at using tools; fits in 16 GB
   ```
3. In Home Assistant, go to **Add integration → Ollama**, URL `http://<mac-ip>:11434`, model
   `qwen3:8b`, **Control Home Assistant: Assist**. Paste the same Jarvis instructions.
4. In step 4, pick this agent instead of Gemini, or create a second assistant ("Jarvis Local") and
   switch between them on the display's device page.

Memory: Whisper, Kokoro and `qwen3:8b` together use roughly 8 GB, which fits in the Mac's 16 GB
alongside normal use. Expect replies in a few seconds. For the best results, keep Gemini for creating automations and DIY
guides and use local mode for everyday chat and control.

## Step 4: The "Jarvis" assistant

Go to **Settings → Voice assistants → Add assistant**:

| Setting | Value |
|---|---|
| Name | Jarvis |
| Language | English |
| Conversation agent | Google Generative AI (the one with Assist control) |
| Prefer handling commands locally | **On**, so "lights off" is instant and stays local |
| Speech-to-text | the Mac's wyoming-mlx (Whisper large-v3-turbo) |
| Text-to-speech | the Mac's wyoming-mlx (Kokoro). For a British butler voice, pick one starting with **`bm_`** (British male) in the voice list, e.g. `bm_george` or `bm_lewis` |

Star it as the **preferred** assistant.

## Step 5: The Jarvis tools

1. Install the **Studio Code Server** or **File editor** add-on.
2. Add to `/config/configuration.yaml`:
   ```yaml
   homeassistant:
     packages: !include_dir_named packages
   ```
3. Copy `homeassistant/jarvis_package.yaml` to `/config/packages/jarvis_package.yaml`, and edit the
   lines marked `# <-- CHANGE`:
   - your search agent's entity ID (step 3)
   - your phone's notify action, e.g. `notify.mobile_app_your_phone`. Install the **Home Assistant
     Companion app** on your phone so DIY guides reach you there.
   - your Mac's IP (only if you do step 7)
4. Create a token under **Profile → Security → Long-lived access tokens → Create token** ("Jarvis"),
   and add it to `/config/secrets.yaml`:
   ```yaml
   jarvis_ha_token: "Bearer PASTE_THE_TOKEN_HERE"
   jarvis_pc_token: "Bearer PC_AGENT_TOKEN_FROM_STEP_7"   # any placeholder until step 7
   ```
5. Go to **Developer tools → YAML → Check configuration**, then **Restart**.
6. **Expose them:** **Settings → Voice assistants → Expose → + Expose entities**, then tick all
   `Jarvis ...` scripts, plus the lights, switches, climate and media players Jarvis may control.

> **Safety:** Jarvis can only touch what you expose. Automations it creates start with
> **[Jarvis]**, so you can find and delete them easily, and it asks before creating them.
> ESPHome configs and guides are only *sent to you*; you decide what to build and install.

## Step 6: Flash the display

The quickest way is from the Mac mini:

1. `brew install esphome`
2. Copy the `jarvis` folder to the Mac, then `cp secrets.example.yaml secrets.yaml` and fill in your
   Wi-Fi details. Generate the API key with `openssl rand -base64 32`.
3. Connect the display to the Mac with USB-C (the **Full-speed Type-C** port, #7), then:
   ```bash
   cd jarvis && esphome run jarvis.yaml
   ```
   Choose the USB port when asked. The first build takes about 5-10 minutes on the M4.
4. Prefer to do it in Home Assistant? Use the **ESPHome Device Builder** add-on instead: copy
   `jarvis.yaml`, `jarvis_face.h` and `sounds/` into `/config/esphome/` and use **Install**. The first
   build takes 20-40 minutes on the Pi 4.
5. Home Assistant discovers **Jarvis**. Click **Configure**.
6. On the device page (**Settings → Devices → Jarvis**), set **Assistant** to *Jarvis*. Try the
   **Face Mood** dropdown to see the eyes react.

Say **"Hey Jarvis, what can you do?"**

## Step 7: Let Jarvis use your Mac

Run the small **Jarvis PC agent** on the Mac mini. Jarvis can then hand it tasks, for example
"write a script", "summarise the PDFs in my work folder", "draft an email to my landlord", and it
announces the result when it's done. Both engines are set up side by side:
**Claude Code** (when you say "Claude", or for coding) and **Gemini CLI** (free, everything else).

Full setup (Homebrew, auto-start, keeping the Mac awake) and safety notes:
[`pc-agent/README.md`](pc-agent/README.md)

## Step 8: Reach Home Assistant from anywhere, and let Claude work on it

### Remote access (free): Tailscale
1. On the Pi 4: **Settings → Add-ons → Add-on store → Tailscale**, install, start, and click
   **Open Web UI** to log in. A free personal account is enough.
2. Install Tailscale on your phone and Mac with the same account.
3. Open Home Assistant from anywhere at `http://<pi4-tailscale-name>:8123`, and in the
   Companion app set that as the *External URL*.

Nothing is exposed to the internet: only your own devices can reach it.

### Connect Claude to Home Assistant (MCP)
Home Assistant has a built-in **Model Context Protocol Server**, so Claude can see and control your
home and help you build things:

1. In Home Assistant, go to **Settings → Devices & services → Add integration → Model Context
   Protocol Server**, and choose **Assist** as the control API. It uses the same exposed entities as Jarvis.
2. Create a long-lived access token (**Profile → Security**).
3. **Claude Code** on the Mac:
   ```bash
   claude mcp add --transport http home-assistant http://<pi4-ip>:8123/api/mcp \
     --header "Authorization: Bearer <token>"
   ```
   **Claude Desktop:** add the same URL and token as a custom connector or MCP server.
   The exact menu depends on the app version and your Claude plan.
4. Ask Claude things like *"Which lights are on?"* or *"Help me design an automation for the AC"*.

**To let Claude edit Home Assistant's YAML files directly:** install the **Samba share** add-on on
the Pi 4, connect from the Mac in Finder (**Go → Connect to Server →** `smb://<pi4-ip>`), then open
the `config` folder with Claude Code. Back up first (**Settings → System → Backups**).

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Won't boot; log says `chip revision: v1.x` | Add `engineering_sample: true` under `esp32:` in `jarvis.yaml` |
| No Wi-Fi | The ESP32-C6 Wi-Fi chip may need a firmware update (see the `esp32_hosted` docs), or use the Ethernet port |
| Eyes stay grey ("Waiting for Home Assistant") | The device isn't adopted yet, or the API key doesn't match |
| Hears you but no reply sound | Check the speaker connector (#3) and the *Speaker Amplifier* switch |
| Face never shows moods | Expose the *Jarvis set mood* script and check the instructions. Jarvis still guesses a mood from its reply |
| Automation not created | `jarvis_ha_token` must start with `Bearer `. If Home Assistant uses SSL, change the URL in the package to `https://` |
| "Could not reach the computer" | Is the PC agent running (`launchctl list | grep jarvis`), and is the Mac's IP right? |
| Slow answers | Check the assistant uses the Mac's speech-to-text, and use a Gemini **Flash** model. On the local brain, try a smaller model like `qwen3:4b` |
| Local brain doesn't answer | Is the Mac awake? Does `curl http://<mac-ip>:11434` from another device say *Ollama is running*? |
| Jarvis stopped understanding speech | Is the Mac awake? Run `lsof -nP -iTCP:10300 -sTCP:LISTEN`: it must show `*:10300`, not `127.0.0.1`. Log: `/tmp/wyoming-mlx.log` |
| Choppy animation | Lower `GLOW` in `jarvis_face.h` to 0, or set the face `interval:` to 50ms |
| Wake word too sensitive or deaf | Change `mic_gain` in `audio_dac:` (18DB-30DB) |

## Customising the face

All in `jarvis_face.h`: eye size and position (`EYE_W`, `EYE_H`, `EYE_SPACING`, `EYE_CY`),
colours (`mood_color()`), emotion shapes (`target_shape()`), and how long an emotion stays
(`EMOTION_HOLD_MS`).

## Credits

- Hardware pin mapping: [p1ngb4ck/unofficial_guition_esp32p4_repo](https://github.com/p1ngb4ck/unofficial_guition_esp32p4_repo/tree/main/JC1060P470_I_W_Y)
- [ESPHome voice assistant](https://esphome.io/components/voice_assistant/), [micro_wake_word](https://esphome.io/components/micro_wake_word/), [LVGL](https://esphome.io/components/lvgl/)
