# J.A.R.V.I.S. on the Guition 7" ESP32-P4 display (free and local)

You say **"Hey Jarvis"**, the screen lights up and listens, and Jarvis answers out loud with the
reply shown on screen. It's free: no subscriptions, and all software is open source.

```
 ┌──────────── Display (ESP32-P4) ────────────┐        ┌────────── Home Assistant (your PC / Pi) ──────────┐
 │ mic → "Hey Jarvis" wake word (on device)   │  Wi-Fi │ Whisper  : your voice → text                       │
 │ streams your voice ───────────────────────────────▶ │ AI brain : Ollama (local) or Gemini (free API)     │
 │ speaker ◀── spoken reply ◀────────────────────────── │ Piper    : reply text → voice                      │
 │ screen  : arc-reactor UI, transcript, clock │        │ + controls your lights, AC, media...               │
 └─────────────────────────────────────────────┘        └────────────────────────────────────────────────────┘
```

## Files

| File | What it is |
|------|------------|
| `jarvis.yaml` | ESPHome firmware for the display: audio, wake word, voice assistant, Jarvis UI |
| `secrets.example.yaml` | Copy to `secrets.yaml` and put in your Wi-Fi details |

## What you need

- The **JC1060P470C_I_W_Y** display, with a speaker plugged into the 2-pin speaker
  connector (#3 on the board diagram). The microphone is built in.
- A **computer that stays on** to run Home Assistant: a Raspberry Pi 4/5, an old laptop, a mini PC,
  or your desktop.
  - To run the AI brain **locally** (Ollama), you need a PC with 16 GB RAM, ideally with an NVIDIA GPU.
  - On a weak machine or a Pi, use the free **Gemini** brain instead (step 3b).

---

## Step 1: Install Home Assistant (free)

Pick one:

- **Raspberry Pi / spare PC:** flash *Home Assistant OS*. See https://www.home-assistant.io/installation/
- **Your Windows/Linux PC:** run Home Assistant OS in VirtualBox, or use Docker (`homeassistant/home-assistant`).

Open `http://homeassistant.local:8123` and create your account.

## Step 2: Speech-to-text and text-to-speech (local, free)

On Home Assistant OS, go to **Settings → Add-ons → Add-on store** and install and start:

1. **Whisper** turns speech into text. Model: `base-int8` (fast) or `small-int8` (more accurate).
   Set the language to `en`, or `ar` for Arabic.
2. **Piper** turns text into speech. For a British "butler" voice, try `en_GB-alan-medium`
   or `en_GB-northern_english_male-medium`. For Arabic, use `ar_JO-kareem-medium`.
3. **openWakeWord** is *not* needed. The display hears "Hey Jarvis" by itself.

Then go to **Settings → Devices & services**. Whisper and Piper show up as discovered **Wyoming**
integrations; click **Configure** on each.

> On Docker instead of Home Assistant OS, run the `rhasspy/wyoming-whisper` and `rhasspy/wyoming-piper`
> containers and add them with the **Wyoming Protocol** integration.

## Step 3: Give Jarvis a brain (choose one)

### 3a. Ollama: fully local, private, unlimited

1. Install Ollama on your PC: https://ollama.com/download
2. Download a model, for example `ollama pull qwen3:8b` (or `llama3.1:8b`; on a weak PC use `qwen3:4b`).
3. Let Home Assistant reach it. Start Ollama with `OLLAMA_HOST=0.0.0.0`.
4. In Home Assistant, go to **Settings → Devices & services → Add integration → Ollama**, enter
   `http://<PC-IP>:11434`, pick the model, and enable **Control Home Assistant** (Assist).

### 3b. Google Gemini: free API tier, smarter, needs internet

1. Get a free API key at https://aistudio.google.com/apikey
2. In Home Assistant, go to **Add integration → Google Generative AI**, paste the key, and enable
   **Control Home Assistant**.
3. The free tier has rate limits, and Google may use free-tier conversations to improve its models.

### Jarvis's personality

In the Ollama or Gemini integration options, set the **Instructions** to something like:

```
You are J.A.R.V.I.S., a witty, loyal and highly capable British AI butler.
Address the user as "sir". Keep spoken answers short (1-3 sentences), clear and a little dry-humoured.
You can control the smart home through the tools you are given. Never use emojis or markdown -
your answers are spoken aloud.
```

## Step 4: Create the "Jarvis" voice pipeline

Go to **Settings → Voice assistants → Add assistant**:

- Name: `Jarvis`
- Conversation agent: your Ollama or Gemini agent. Turn on **Prefer handling commands locally**,
  so "turn on the lights" is instant.
- Speech-to-text: `faster-whisper`
- Text-to-speech: `piper`, with the voice you chose

Star it to make it the **preferred** assistant.

## Step 5: Flash the display

1. Install the **ESPHome Device Builder** add-on (Settings → Add-ons), or on your PC run `pip install esphome`.
2. Copy this folder over, rename `secrets.example.yaml` to `secrets.yaml`, and fill it in.
3. Connect the display to your PC with USB-C. Use the **Full-speed Type-C** port (#7); if your PC
   doesn't detect it, try the other port.
4. Run:
   ```bash
   esphome run jarvis.yaml
   ```
   Later updates go over Wi-Fi.
5. Home Assistant will discover **Jarvis**. Click **Configure**, then open the device page and set
   its **Assistant** to `Jarvis`.

Say **"Hey Jarvis... what's the weather like?"** You can also tap the arc reactor to talk without
the wake word.

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Won't boot after flashing; log says `chip revision: v1.x` | Your board has an early ESP32-P4. Add `engineering_sample: true` under `esp32:` |
| No Wi-Fi | The ESP32-C6 Wi-Fi chip may need a firmware update, see the `esp32_hosted` docs. Or plug in Ethernet (port #13): replace the `wifi:` block with the `ethernet:` block from the community config linked below |
| Hears you but no voice reply | Check the speaker is in connector #3, and that the "Speaker Amplifier" switch is on in Home Assistant |
| Wake word triggers too often or not at all | Change `mic_gain` (try `18DB` or `30DB`) in `audio_dac:` |
| Screen says OFFLINE | Home Assistant hasn't connected yet. Check the device was adopted in Settings → Devices |
| Arabic text shows boxes | The fonts only include Latin letters. Add an Arabic Google font (e.g. `Noto Sans Arabic`) with Arabic glyphs |

## Customising

- **Colours:** change the `c_*` substitutions at the top of `jarvis.yaml`.
- **Time zone:** the `timezone` substitution (default `Asia/Muscat`).
- **More wake words:** add `- model: okay_nabu` under `micro_wake_word: models:`.

## Credits

- Hardware pin mapping: [p1ngb4ck/unofficial_guition_esp32p4_repo](https://github.com/p1ngb4ck/unofficial_guition_esp32p4_repo/tree/main/JC1060P470_I_W_Y)
- [ESPHome voice assistant](https://esphome.io/components/voice_assistant/), [micro_wake_word](https://esphome.io/components/micro_wake_word/), [LVGL](https://esphome.io/components/lvgl/)
