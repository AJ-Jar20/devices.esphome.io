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

## Recommended setup for your hardware

Your mini PC (Intel i7 6th/7th gen, 8 GB RAM, no graphics card) is a good Home Assistant
server. **You don't need to buy anything.** It's stronger than a Raspberry Pi 4.

| Job | Runs on | Choice | Why |
|-----|---------|--------|-----|
| Wake word | Display | "Hey Jarvis" = English, "Okay Nabu" = Arabic | Runs on the board itself |
| Speech-to-text | Mini PC | Whisper `small-int8` | Understands both Arabic and English, 2-4 s on that CPU |
| AI brain | Cloud (free) | **Google Gemini** free API | A local AI on 8 GB without a GPU would be slow, and weak at Arabic |
| Voice (English) | Mini PC | Piper `en_GB-alan-medium` | British butler voice, local |
| Voice (Arabic) | Mini PC or cloud | Piper `ar_JO-kareem-medium`, or Google Translate TTS (free, more natural) | |

Optional upgrade: 16 GB RAM (cheap) and an SSD instead of an HDD make Home Assistant faster.
Neither is needed.

## What you need

- The **JC1060P470C_I_W_Y** display, with a speaker plugged into the 2-pin speaker
  connector (#3 on the board diagram). The microphone is built in.
- Home Assistant (you already have it) with its **Add-ons** menu. That menu comes with Home Assistant OS.
  On Docker ("Container"), use the Docker notes below instead.

---

## Step 1: Speech-to-text and text-to-speech (free)

Go to **Settings → Add-ons → Add-on store** and install and start:

1. **Whisper.** In its Configuration, set the model to `small-int8` and the language to `auto`, then
   restart it. One model serves both languages, because each assistant (step 3) tells Whisper
   which language to expect.
2. **Piper.** Voices download automatically when you pick them in step 3.
3. **openWakeWord** is *not* needed. The display hears its wake words by itself.

Then go to **Settings → Devices & services**. Whisper and Piper show up as discovered **Wyoming**
integrations; click **Configure** on each.

Optional, for a more natural Arabic voice: **Add integration → Google Translate text-to-speech**
(free, needs internet).

> On Docker, run the `rhasspy/wyoming-whisper` and `rhasspy/wyoming-piper` containers and add
> them with the **Wyoming Protocol** integration.

## Step 2: Give Jarvis a brain (Google Gemini, free)

1. Get a free API key at https://aistudio.google.com/apikey. Sign in with a Google account; no card
   is needed.
2. In Home Assistant, go to **Settings → Devices & services → Add integration → Google Generative AI**
   and paste the key.
3. Open its options, choose a **Flash** model (fast, with generous free limits), and enable
   **Control Home Assistant** (Assist), so Jarvis can switch your devices.
4. Set the **Instructions** (this is Jarvis's personality):

```
You are J.A.R.V.I.S., a witty, loyal and highly capable AI butler.
Always reply in the same language the user spoke: Arabic (Modern Standard or Gulf/Omani dialect,
matching the user) or English with a British tone.
Address the user as "sir" in English and "سيدي" in Arabic.
Keep spoken answers short (1-3 sentences). Never use emojis, markdown or lists - your answers are spoken aloud.
You can control the smart home through the tools you are given.
```

Notes on the free tier: there are daily request limits (plenty for home use), and Google may use
free-tier conversations to improve its models. Smart-home commands like "turn on the lights" are
handled locally by Home Assistant first, so they're instant and don't use Gemini at all.

> **A fully local alternative, later:** install Ollama on a PC with 16 GB+ RAM, ideally with an
> NVIDIA GPU, and add the **Ollama** integration instead. On your current 8 GB mini PC it would
> take 10-30 seconds per answer.

## Step 3: Create two assistants (English and Arabic)

Go to **Settings → Voice assistants → Add assistant** and make two:

| Setting | **Jarvis (English)** | **Jarvis (Arabic)** |
|---------|----------------------|---------------------|
| Language | English | العربية (Arabic) |
| Conversation agent | Google Generative AI | Google Generative AI |
| Prefer handling commands locally | On | On |
| Speech-to-text | faster-whisper | faster-whisper |
| Text-to-speech | Piper: `en_GB-alan-medium` | Piper: `ar_JO-kareem-medium` (or Google Translate, Arabic) |

## Step 4: Flash the display

1. Install the **ESPHome Device Builder** add-on (Settings → Add-ons), or on your PC run `pip install esphome`.
2. Copy this folder over, rename `secrets.example.yaml` to `secrets.yaml`, and fill it in.
3. Connect the display to your PC with USB-C. Use the **Full-speed Type-C** port (#7); if your PC
   doesn't detect it, try the other port.
4. Run:
   ```bash
   esphome run jarvis.yaml
   ```
   Later updates go over Wi-Fi.
5. Home Assistant will discover **Jarvis**. Click **Configure**.

## Step 5: One wake word per language

Open the display's device page: **Settings → Devices → Jarvis → Configuration**.

- **Wake word:** `Hey Jarvis`, with **Assistant:** `Jarvis (English)`
- **Wake word 2:** `Okay Nabu`, with **Assistant 2:** `Jarvis (Arabic)`

(Recent Home Assistant versions show these second wake word and assistant options for ESPHome devices
that have two wake words. If yours doesn't, update Home Assistant.)

Try it:
- **"Hey Jarvis, what's the weather like?"**
- **"Okay Nabu, شغّل الإضاءة في الصالة"** ("turn on the living room lights")

You can also tap the arc reactor to talk without a wake word. That uses the first assistant.

> **On-screen text:** the display shows English text. Arabic replies are **spoken** in full, but the
> screen shows a short note instead of the Arabic text, because the ESPHome display library can't yet
> join Arabic letters correctly. Unjoined letters would look like broken text.

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Won't boot after flashing; log says `chip revision: v1.x` | Your board has an early ESP32-P4. Add `engineering_sample: true` under `esp32:` |
| No Wi-Fi | The ESP32-C6 Wi-Fi chip may need a firmware update, see the `esp32_hosted` docs. Or plug in Ethernet (port #13): replace the `wifi:` block with the `ethernet:` block from the community config linked below |
| Hears you but no voice reply | Check the speaker is in connector #3, and that the "Speaker Amplifier" switch is on in Home Assistant |
| Wake word triggers too often or not at all | Change `mic_gain` (try `18DB` or `30DB`) in `audio_dac:` |
| Screen says OFFLINE | Home Assistant hasn't connected yet. Check the device was adopted in Settings → Devices |
| Arabic answers are slow | Make sure the Whisper model is `small-int8`, not `medium`, and that Home Assistant isn't running from a slow HDD |
| Arabic is misheard | Speak a little closer to the display, or try Whisper `medium-int8` (more accurate, about 2x slower) |

## Customising

- **Colours:** change the `c_*` substitutions at the top of `jarvis.yaml`.
- **Time zone:** the `timezone` substitution (default `Asia/Muscat`).
- **Swap the Arabic wake word:** replace `okay_nabu` with `hey_mycroft` in `jarvis.yaml`.
  There's no ready-made Arabic wake word yet.

## Credits

- Hardware pin mapping: [p1ngb4ck/unofficial_guition_esp32p4_repo](https://github.com/p1ngb4ck/unofficial_guition_esp32p4_repo/tree/main/JC1060P470_I_W_Y)
- [ESPHome voice assistant](https://esphome.io/components/voice_assistant/), [micro_wake_word](https://esphome.io/components/micro_wake_word/), [LVGL](https://esphome.io/components/lvgl/)
