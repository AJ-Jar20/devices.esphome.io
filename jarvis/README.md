# J.A.R.V.I.S.: your personal AI assistant

**Hardware: just two things**
1. **Raspberry Pi 5 (8 GB) + NVMe SSD HAT**, running Home Assistant OS
2. **Guition 7" display JC1060P470C_I_W_Y** (ESP32-P4 + ESP32-C6), with a speaker plugged into
   its speaker connector. The microphone is built in.

Optional: **your computer**, so Jarvis can hand it tasks (step 7).

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
| "On my computer, write a script that sorts my downloads" | Hands the task to the AI on your PC (Gemini CLI or Claude Code), then **announces the result** |
| "Who won the match last night?" / "What's the price of gold?" | Searches the web |
| "Explain black holes" / any general question | General knowledge |
| "Set a timer for 10 minutes" | Timer on the display, with an alarm and a surprised face |
| "Add milk to the shopping list" | Home Assistant to-do list |

Tap the face to talk without the wake word.

## Honest limits

- **The brain is Google Gemini (free tier, cloud).** A Pi 5 can't run an AI model big enough to write
  automations, design DIY projects and answer anything at conversational speed. Everything else is
  local: wake word, face, speech-to-text, voice, smart-home control. Simple commands like "lights
  off" never leave your home. Free-tier limits are plenty for home use, and Google may use free-tier
  chats to improve its models.
- **No camera for now.** The display has a camera, but ESPHome can't use that type of camera
  (MIPI-CSI) on the ESP32-P4 yet. Vision can be added once it's supported.
- **Computer tasks** need your computer switched on and the small Jarvis PC agent running. Gemini
  CLI is free; Claude Code needs a Claude subscription.
- **The face is tested in a simulation,** not yet on a real display. The firmware compiles, and the
  previews above are rendered from the real face code with the display's graphics library.

## How it fits together

```
 Display (ESP32-P4)              Raspberry Pi 5 + SSD (Home Assistant OS)       Your computer (optional)
 ──────────────────              ────────────────────────────────────────       ────────────────────────
 "Hey Jarvis" wake word   ──▶    Whisper add-on  (speech → text, local)
 mic + speaker                   Piper add-on    (text → voice, local)
 animated eyes + subtitles ◀──   Brain: Google Gemini (free)
 timers                          Jarvis tools (jarvis_package.yaml):
                                   set mood · create automation · send guide
                                   draft ESPHome config · web search
                                   computer task  ─────────────────────────▶   jarvis_pc_agent.py
                                 Your smart devices                            → Gemini CLI / Claude Code
```

## Files

| File | What it is |
|------|------------|
| `jarvis.yaml` | ESPHome firmware for the display |
| `jarvis_face.h` | The animated eyes: blinking, glancing, 9 emotions |
| `sounds/timer.wav` | Timer alarm |
| `secrets.example.yaml` | Template for your Wi-Fi details |
| `homeassistant/jarvis_package.yaml` | The Jarvis tools for Home Assistant |
| `pc-agent/` | The optional agent for your computer, with its own README |

---

## Step 1: Home Assistant OS on the Pi 5 + SSD

**Hardware:** fit the SSD HAT and the SSD, a cooler or fan, and use the **official 27 W USB-C power
supply**. An SSD on a weaker charger causes random reboots.

**Install:**
- **With a USB NVMe adapter:** plug the SSD into your PC, open **Raspberry Pi Imager**, choose
  *Raspberry Pi 5* → *Other specific-purpose OS → Home assistants and home automation →
  Home Assistant OS (RPi 5)* → the SSD, and write it. Then fit the SSD in the HAT.
- **Without an adapter:** write Raspberry Pi OS to a microSD card, boot the Pi 5 from it, run
  Raspberry Pi Imager *on the Pi* to write Home Assistant OS to the SSD, then shut down and
  **remove the SD card**.

If it won't boot from the SSD: boot Raspberry Pi OS from the SD card, run `sudo raspi-config`, choose
**Advanced Options → Boot Order → NVMe/USB Boot**, and update the bootloader.

**First start:** connect Ethernet, open `http://homeassistant.local:8123` and create your account.

- **Moving from your Pi 4?** First take a full backup there (**Settings → System → Backups**) and
  download it. On the Pi 5's welcome screen, choose **Restore from backup**. Turn the Pi 4 off first.
- Give the Pi 5 a **fixed IP** in your router.

## Step 2: Local speech-to-text and voice

Go to **Settings → Add-ons → Add-on store**, then install, start, and turn on *Start on boot* for:

1. **Whisper.** Language `en`, model `base-int8` (fast, about 1-2 s on a Pi 5). If it mishears
   you, try `small-int8` (more accurate, roughly twice as slow).
2. **Piper.** Voice `en_GB-alan-medium` (British, butler-like).

Then go to **Settings → Devices & services**: both appear as discovered **Wyoming** integrations.
Click **Configure** on each.

## Step 3: Jarvis's brain (Google Gemini, free)

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
- "Jarvis computer task": when asked to do something on the user's computer, in Claude or in
  Gemini, or anything that needs files. Confirm first if it deletes, sends, buys or publishes.
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

## Step 4: The "Jarvis" assistant

Go to **Settings → Voice assistants → Add assistant**:

| Setting | Value |
|---|---|
| Name | Jarvis |
| Language | English |
| Conversation agent | Google Generative AI (the one with Assist control) |
| Prefer handling commands locally | **On**, so "lights off" is instant and stays local |
| Speech-to-text | faster-whisper |
| Text-to-speech | piper, voice `en_GB-alan-medium` |

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
   - your computer's IP (only if you do step 7)
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

1. Install the **ESPHome Device Builder** add-on.
2. Copy `jarvis.yaml`, `jarvis_face.h` and the `sounds` folder into `/config/esphome/`.
3. In ESPHome's **Secrets** (top-right menu), add your Wi-Fi details and keys, following
   `secrets.example.yaml`.
4. Connect the display to your computer with USB-C (the **Full-speed Type-C** port, #7). In the
   ESPHome dashboard (open it from that computer), click **Jarvis → Install → Plug into this computer**.
   The first build takes 10-20 minutes on the Pi 5. Later updates go over Wi-Fi.
5. Home Assistant discovers **Jarvis**. Click **Configure**.
6. On the device page (**Settings → Devices → Jarvis**), set **Assistant** to *Jarvis*. Try the
   **Face Mood** dropdown to see the eyes react.

Say **"Hey Jarvis, what can you do?"**

## Step 7 (optional): Let Jarvis use your computer

Run the small **Jarvis PC agent** on your computer. Jarvis can then hand it tasks, for example
"write a script", "summarise the PDFs in my work folder", "draft an email to my landlord", and it
announces the result when it's done. It uses **Gemini CLI (free)** or **Claude Code**.

Full setup and safety notes: [`pc-agent/README.md`](pc-agent/README.md)

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Won't boot; log says `chip revision: v1.x` | Add `engineering_sample: true` under `esp32:` in `jarvis.yaml` |
| No Wi-Fi | The ESP32-C6 Wi-Fi chip may need a firmware update (see the `esp32_hosted` docs), or use the Ethernet port |
| Eyes stay grey ("Waiting for Home Assistant") | The device isn't adopted yet, or the API key doesn't match |
| Hears you but no reply sound | Check the speaker connector (#3) and the *Speaker Amplifier* switch |
| Face never shows moods | Expose the *Jarvis set mood* script and check the instructions. Jarvis still guesses a mood from its reply |
| Automation not created | `jarvis_ha_token` must start with `Bearer `. If Home Assistant uses SSL, change the URL in the package to `https://` |
| "Could not reach the computer" | Is the PC agent running, is the IP right, and does the firewall allow it on private networks? |
| Slow answers | Use Whisper `base-int8` and a Gemini **Flash** model. Check the Pi 5 isn't overheating |
| Pi 5 reboots randomly | Use the official 27 W power supply |
| Choppy animation | Lower `GLOW` in `jarvis_face.h` to 0, or set the face `interval:` to 50ms |
| Wake word too sensitive or deaf | Change `mic_gain` in `audio_dac:` (18DB-30DB) |

## Customising the face

All in `jarvis_face.h`: eye size and position (`EYE_W`, `EYE_H`, `EYE_SPACING`, `EYE_CY`),
colours (`mood_color()`), emotion shapes (`target_shape()`), and how long an emotion stays
(`EMOTION_HOLD_MS`).

## Credits

- Hardware pin mapping: [p1ngb4ck/unofficial_guition_esp32p4_repo](https://github.com/p1ngb4ck/unofficial_guition_esp32p4_repo/tree/main/JC1060P470_I_W_Y)
- [ESPHome voice assistant](https://esphome.io/components/voice_assistant/), [micro_wake_word](https://esphome.io/components/micro_wake_word/), [LVGL](https://esphome.io/components/lvgl/)
