// =============================================================================
//  Jarvis animated eyes face (LVGL 9) for ESPHome
//
//  Two glowing eyes that blink, glance around and show emotions.
//  The face is drawn with plain LVGL objects:
//    eye   - rounded rectangle (the glowing part)
//    lid   - thick black line across the top of the eye (slant = sad/angry)
//    cheek - black circle pushed up from below (makes happy "^ ^" eyes)
//
//  Usage from YAML (see jarvis.yaml):
//    face::begin(parent_obj);            once, on boot
//    face::tick(millis());               every ~33 ms
//    face::set_activity(face::LISTENING) voice assistant state
//    face::set_emotion("happy")          from Home Assistant / the AI
// =============================================================================
#pragma once

#include "lvgl.h"
#include <cmath>
#include <cstdlib>
#include <string>
#include <algorithm>

namespace face {

enum Activity { IDLE, LISTENING, THINKING, SPEAKING, ERROR, OFFLINE };
enum Emotion { NEUTRAL, HAPPY, EXCITED, SAD, ANGRY, SURPRISED, CONFUSED, SLEEPY, LOVE };

// ---------------------------------------------------------------- tuning ----
static const int SCREEN_W = 1024;
static const int SCREEN_H = 600;
static const int EYE_SPACING = 205;   // distance from centre to each eye centre
static const int EYE_CY = 255;        // vertical centre of the eyes
static const int EYE_W = 190;
static const int EYE_H = 235;
static const int LID_W = 260;         // eyelid thickness, must exceed eye height
static const int GLOW = 24;           // shadow glow width (lower = faster drawing)
static const uint32_t EMOTION_HOLD_MS = 9000;  // how long an emotion stays after speaking

// ----------------------------------------------------------------- state ----
struct EyeParts {
  lv_obj_t *eye{nullptr};
  lv_obj_t *lid{nullptr};
  lv_obj_t *cheek{nullptr};
  lv_point_precise_t lid_pts[2];
};

struct Shape {
  float w, h, dy;     // eye size and vertical offset
  float lid_depth;    // how far the lid comes down into the eye (px)
  float lid_slant;    // + = inner corner lower (angry), - = outer lower (sad)
  float cheek;        // how far the cheek pushes up (px), 0 = none
  float l_scale;      // left eye size factor (confused)
  float r_scale;
};

static EyeParts g_left, g_right;
static Shape g_cur{EYE_W, EYE_H, 0, 0, 0, 0, 1, 1};
static Activity g_activity = OFFLINE;
static Emotion g_emotion = NEUTRAL;
static bool g_emotion_from_ai = false;
static uint32_t g_emotion_until = 0;
static uint32_t g_color = 0x00E5FF;
static uint32_t g_shown_color = 0;

static float g_gaze_x = 0, g_gaze_y = 0, g_gaze_tx = 0, g_gaze_ty = 0;
static uint32_t g_next_glance = 0;
static uint32_t g_next_blink = 0;
static uint32_t g_blink_start = 0;
static bool g_double_blink = false;
static bool g_ready = false;

static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
static inline uint32_t rnd(uint32_t lo, uint32_t hi) { return lo + (uint32_t) (rand() % (hi - lo + 1)); }

// ------------------------------------------------------------- creation ----
static lv_obj_t *make_box(lv_obj_t *parent, uint32_t color, int radius) {
  lv_obj_t *o = lv_obj_create(parent);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_border_width(o, 0, 0);
  lv_obj_set_style_pad_all(o, 0, 0);
  lv_obj_set_style_radius(o, radius, 0);
  lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  return o;
}

static void make_eye(lv_obj_t *parent, EyeParts &p) {
  p.eye = make_box(parent, g_color, 70);
  lv_obj_set_style_shadow_width(p.eye, GLOW, 0);
  lv_obj_set_style_shadow_spread(p.eye, 2, 0);
  lv_obj_set_style_shadow_opa(p.eye, LV_OPA_50, 0);

  p.cheek = make_box(parent, 0x000000, LV_RADIUS_CIRCLE);
  lv_obj_set_size(p.cheek, EYE_W + 120, EYE_H);

  p.lid = lv_line_create(parent);
  lv_obj_remove_flag(p.lid, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_pos(p.lid, 0, 0);
  lv_obj_set_style_line_width(p.lid, LID_W, 0);
  lv_obj_set_style_line_color(p.lid, lv_color_hex(0x000000), 0);
  lv_obj_set_style_line_rounded(p.lid, false, 0);
}

inline void begin(lv_obj_t *parent) {
  srand(lv_tick_get());
  make_eye(parent, g_left);
  make_eye(parent, g_right);
  g_ready = true;
}

// ------------------------------------------------------- mood -> shape ----
static Shape target_shape(uint32_t now) {
  Shape s{EYE_W, EYE_H, 0, 0, 0, 0, 1, 1};
  float t = now / 1000.0f;

  // What the face is "doing" wins over how it "feels", except while speaking.
  Emotion e = g_emotion;
  switch (g_activity) {
    case LISTENING:
      s.w = EYE_W * 1.08f;
      s.h = EYE_H * (1.10f + 0.03f * sinf(t * 5));   // gentle "breathing"
      return s;
    case THINKING:
      s.h = EYE_H * 0.80f;
      s.lid_depth = 40;
      s.lid_slant = -10;
      return s;
    case OFFLINE:
      e = SLEEPY;
      break;
    case ERROR:
      e = CONFUSED;
      break;
    default:
      break;
  }

  switch (e) {
    case HAPPY:     s.cheek = 105; break;
    case EXCITED:   s.cheek = 95; s.w *= 1.1f; s.dy = -10 + 8 * sinf(t * 12); break;
    case LOVE:      s.cheek = 80; s.w *= 1.15f; s.h *= 1.1f; break;
    case SAD:       s.lid_depth = 70; s.lid_slant = -55; s.dy = 18; break;
    case ANGRY:     s.lid_depth = 70; s.lid_slant = 60; s.h *= 0.9f; break;
    case SURPRISED: s.w *= 1.18f; s.h *= 1.18f; break;
    case CONFUSED:  s.l_scale = 0.75f; s.r_scale = 1.1f; s.lid_depth = 35; s.lid_slant = 25; break;
    case SLEEPY:    s.lid_depth = EYE_H * 0.55f; s.dy = 20; break;
    default: break;
  }
  if (g_activity == SPEAKING) {
    s.dy += 6 * sinf(t * 14);   // little bounce while talking
    s.h *= 1.0f + 0.04f * sinf(t * 9);
  }
  return s;
}

static uint32_t mood_color() {
  switch (g_activity) {
    case LISTENING: return 0x00FF9C;
    case THINKING:  return 0xFFB300;
    case OFFLINE:   return 0x37474F;
    case ERROR:     return 0xFF5252;
    default: break;
  }
  switch (g_emotion) {
    case ANGRY: return 0xFF1744;
    case SAD:   return 0x448AFF;
    case LOVE:  return 0xFF4FA3;
    case EXCITED: return 0x18FFFF;
    default:    return 0x00E5FF;
  }
}

// --------------------------------------------------------------- drawing ----
static void place_eye(EyeParts &p, int cx, int cy, float w, float h, const Shape &s, bool left) {
  int iw = std::max(8, (int) w), ih = std::max(6, (int) h);
  int x = cx - iw / 2, y = cy - ih / 2;
  lv_obj_set_pos(p.eye, x, y);
  lv_obj_set_size(p.eye, iw, ih);
  lv_obj_set_style_radius(p.eye, std::min(70, std::max(3, std::min(iw, ih) / 2 - 4)), 0);

  // Eyelid: thick line whose lower edge sits lid_depth px below the top of the eye.
  if (s.lid_depth > 1) {
    lv_obj_remove_flag(p.lid, LV_OBJ_FLAG_HIDDEN);
    float base = y - LID_W / 2 + s.lid_depth * (h / EYE_H);
    float inner = base + s.lid_slant / 2, outer = base - s.lid_slant / 2;
    float lx = x - 30, rx = x + iw + 30;
    p.lid_pts[0] = {(lv_value_precise_t) lx, (lv_value_precise_t) std::max(0.0f, left ? outer : inner)};
    p.lid_pts[1] = {(lv_value_precise_t) rx, (lv_value_precise_t) std::max(0.0f, left ? inner : outer)};
    lv_line_set_points(p.lid, p.lid_pts, 2);
  } else {
    lv_obj_add_flag(p.lid, LV_OBJ_FLAG_HIDDEN);
  }

  // Cheek: black circle rising from below (happy eyes).
  if (s.cheek > 1) {
    lv_obj_remove_flag(p.cheek, LV_OBJ_FLAG_HIDDEN);
    int cw = iw + 120;
    lv_obj_set_size(p.cheek, cw, ih);
    lv_obj_set_pos(p.cheek, cx - cw / 2, y + ih - (int) (s.cheek * (h / EYE_H)));
  } else {
    lv_obj_add_flag(p.cheek, LV_OBJ_FLAG_HIDDEN);
  }
}

inline void tick(uint32_t now) {
  if (!g_ready) return;

  // Emotions fade back to neutral a while after Jarvis stops talking.
  if (g_activity == IDLE && g_emotion != NEUTRAL && g_emotion_until && now > g_emotion_until) {
    g_emotion = NEUTRAL;
    g_emotion_until = 0;
  }

  // Glances: look around when idle, up-left when thinking, at you when listening.
  if (g_activity == THINKING) {
    g_gaze_tx = -45 + 6 * sinf(now / 300.0f);
    g_gaze_ty = -40;
  } else if (g_activity == LISTENING || g_activity == SPEAKING) {
    g_gaze_tx = 0;
    g_gaze_ty = 0;
  } else if (now > g_next_glance) {
    bool center = rand() % 3 == 0;
    g_gaze_tx = center ? 0 : (float) ((int) rnd(0, 120) - 60);
    g_gaze_ty = center ? 0 : (float) ((int) rnd(0, 60) - 30);
    g_next_glance = now + rnd(1200, 4000);
  }
  g_gaze_x = lerp(g_gaze_x, g_gaze_tx, 0.18f);
  g_gaze_y = lerp(g_gaze_y, g_gaze_ty, 0.18f);

  // Blinks: every few seconds, sometimes a double blink.
  if (g_next_blink == 0) g_next_blink = now + rnd(2000, 5000);
  if (now >= g_next_blink && g_blink_start == 0 && g_activity != OFFLINE) {
    g_blink_start = now;
    g_double_blink = rand() % 5 == 0;
  }
  float blink = 1.0f;
  if (g_blink_start) {
    uint32_t dt = now - g_blink_start;
    const uint32_t B = 170;
    if (dt < B) {
      blink = fabsf(1.0f - 2.0f * dt / (float) B);
      blink = std::max(0.06f, blink);
    } else if (g_double_blink && dt < 2 * B + 60) {
      if (dt > B + 60) blink = std::max(0.06f, fabsf(1.0f - 2.0f * (dt - B - 60) / (float) B));
    } else {
      g_blink_start = 0;
      g_next_blink = now + rnd(2200, 6000);
    }
  }

  // Ease the current shape towards the target shape.
  Shape t = target_shape(now);
  const float k = 0.22f;
  g_cur.w = lerp(g_cur.w, t.w, k);
  g_cur.h = lerp(g_cur.h, t.h, k);
  g_cur.dy = lerp(g_cur.dy, t.dy, 0.35f);
  g_cur.lid_depth = lerp(g_cur.lid_depth, t.lid_depth, k);
  g_cur.lid_slant = lerp(g_cur.lid_slant, t.lid_slant, k);
  g_cur.cheek = lerp(g_cur.cheek, t.cheek, k);
  g_cur.l_scale = lerp(g_cur.l_scale, t.l_scale, k);
  g_cur.r_scale = lerp(g_cur.r_scale, t.r_scale, k);

  int cy = EYE_CY + (int) (g_cur.dy + g_gaze_y);
  int gx = (int) g_gaze_x;
  place_eye(g_left, SCREEN_W / 2 - EYE_SPACING + gx, cy, g_cur.w * g_cur.l_scale,
            g_cur.h * g_cur.l_scale * blink, g_cur, true);
  place_eye(g_right, SCREEN_W / 2 + EYE_SPACING + gx, cy, g_cur.w * g_cur.r_scale,
            g_cur.h * g_cur.r_scale * blink, g_cur, false);

  uint32_t c = mood_color();
  if (c != g_shown_color) {
    g_shown_color = c;
    for (EyeParts *p : {&g_left, &g_right}) {
      lv_obj_set_style_bg_color(p->eye, lv_color_hex(c), 0);
      lv_obj_set_style_shadow_color(p->eye, lv_color_hex(c), 0);
    }
  }
}

// ------------------------------------------------------------ public API ----
inline Emotion parse_emotion(const std::string &raw) {
  std::string m = raw;
  std::transform(m.begin(), m.end(), m.begin(), ::tolower);
  if (m == "happy" || m == "joy" || m == "pleased") return HAPPY;
  if (m == "excited" || m == "amazed") return EXCITED;
  if (m == "sad" || m == "sorry" || m == "disappointed") return SAD;
  if (m == "angry" || m == "annoyed" || m == "determined") return ANGRY;
  if (m == "surprised" || m == "shocked" || m == "wow") return SURPRISED;
  if (m == "confused" || m == "unsure") return CONFUSED;
  if (m == "sleepy" || m == "tired" || m == "bored") return SLEEPY;
  if (m == "love" || m == "grateful") return LOVE;
  return NEUTRAL;
}

inline const char *emotion_name(Emotion e) {
  static const char *names[] = {"neutral", "happy", "excited", "sad", "angry",
                                "surprised", "confused", "sleepy", "love"};
  return names[e];
}

// Called by the AI (via Home Assistant) - takes priority over guessing.
inline void set_emotion(const std::string &mood) {
  g_emotion = parse_emotion(mood);
  g_emotion_from_ai = true;
  g_emotion_until = lv_tick_get() + EMOTION_HOLD_MS + 20000;
}

// Fallback when the AI didn't pick a mood: guess from the reply text.
inline void guess_emotion(const std::string &reply) {
  if (g_emotion_from_ai) return;
  std::string r = reply;
  std::transform(r.begin(), r.end(), r.begin(), ::tolower);
  auto has = [&](const char *w) { return r.find(w) != std::string::npos; };
  if (has("sorry") || has("unfortunately") || has("can't") || has("cannot") || has("unable"))
    g_emotion = SAD;
  else if (has("wow") || has("incredible") || has("really?") || has("whoa"))
    g_emotion = SURPRISED;
  else if (has("great") || has("done") || has("glad") || has("happy") || has("pleasure") || has("!"))
    g_emotion = HAPPY;
  else if (has("hmm") || has("not sure") || has("unclear"))
    g_emotion = CONFUSED;
  else
    g_emotion = NEUTRAL;
}

inline void set_activity(Activity a) {
  if (a == LISTENING) g_emotion_from_ai = false;   // new question, new mood
  if (a == IDLE && g_activity == SPEAKING) g_emotion_until = lv_tick_get() + EMOTION_HOLD_MS;
  g_activity = a;
}

inline Emotion emotion() { return g_emotion; }

}  // namespace face
