#pragma once
// Drawing helpers for the Round Dashboard display lambda.
// Geometry + theme live here as named functions; the YAML lambda stays a short
// layout that reads Home Assistant state and calls these. Behaviour matches the
// original inline lambda exactly.

#include "esphome.h"
#include <cmath>
#include <string>

namespace rd {

using esphome::Color;

static constexpr int CENTER = 120;   // face centre (240x240 panel)

// ---- theme palette ----
// Colours are NOT hardcoded here: they live as `color_*` substitutions at the
// top of round-dashboard.yaml (one dark/light pair per element), and the display
// lambda fills this struct from them. To add an element, add a field here plus
// its two substitutions together.
struct Palette {
  Color background, text, text_dimmed, text_off;
  Color bezel_major, bezel_minor, dial_well;
  Color electricity, gas, temperature, humidity;        // gauge active colours
  Color electricity_track, gas_track, sub_dial_track;   // idle track colours
  Color electricity_value, gas_value, seconds;          // value text + seconds highlight
};

// ---- small math helpers ----
inline float deg2rad(float d) { return d * 3.14159265f / 180.0f; }
inline float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

// Map a value onto 0..1 given HA-configured min/max (0 if unusable).
inline float scale_value(float v, float lo, float hi) {
  if (std::isnan(v) || std::isnan(lo) || std::isnan(hi) || hi <= lo) return 0.0f;
  return clamp01((v - lo) / (hi - lo));
}

// Current weather condition string -> MDI glyph (nullptr if unknown/empty).
inline const char *weather_glyph(const std::string &c) {
  if (c == "sunny")           return "\U000F0599";
  if (c == "clear-night")     return "\U000F0594";
  if (c == "partlycloudy")    return "\U000F0595";
  if (c == "cloudy")          return "\U000F0590";
  if (c == "fog")             return "\U000F0591";
  if (c == "hail")            return "\U000F0592";
  if (c == "lightning")       return "\U000F0593";
  if (c == "lightning-rainy") return "\U000F067E";
  if (c == "pouring")         return "\U000F0596";
  if (c == "rainy")           return "\U000F0597";
  if (c == "snowy")           return "\U000F0598";
  if (c == "snowy-rainy")     return "\U000F067F";
  if (c == "windy")           return "\U000F059D";
  if (c == "windy-variant")   return "\U000F059E";
  if (c == "exceptional")     return "\U000F0F2F";
  return nullptr;
}

// ---- geometry primitives (templated on the display type) ----

// Solid-fill the trapezoid between `inner` and `outer` radius spanning angles
// [a0, a1] (radians) about (cx, cy), as two filled triangles. Used for every
// filled block (ring ticks, seconds tick, sub-dial ticks) instead of sweeping
// many thin lines.
template<class It>
void fill_radial_quad(It &it, int cx, int cy, int inner, int outer,
                      float a0, float a1, Color col) {
  int ix0 = cx + (int)roundf(cosf(a0) * inner), iy0 = cy + (int)roundf(sinf(a0) * inner);
  int ox0 = cx + (int)roundf(cosf(a0) * outer), oy0 = cy + (int)roundf(sinf(a0) * outer);
  int ox1 = cx + (int)roundf(cosf(a1) * outer), oy1 = cy + (int)roundf(sinf(a1) * outer);
  int ix1 = cx + (int)roundf(cosf(a1) * inner), iy1 = cy + (int)roundf(sinf(a1) * inner);
  it.filled_triangle(ix0, iy0, ox0, oy0, ox1, oy1, col);
  it.filled_triangle(ix0, iy0, ox1, oy1, ix1, iy1, col);
}

// One radial tick at angle `a` (radians). When `wide`, it is a solid ~3px block
// (filled trapezoid); otherwise a single thin line.
template<class It>
void draw_tick(It &it, int cx, int cy, int inner, int outer, float a, Color col, bool wide) {
  if (wide) {
    fill_radial_quad(it, cx, cy, inner, outer, a - deg2rad(0.9f), a + deg2rad(0.9f), col);
  } else {
    it.line(cx + (int)(cosf(a) * inner), cy + (int)(sinf(a) * inner),
            cx + (int)(cosf(a) * outer), cy + (int)(sinf(a) * outer), col);
  }
}

// Thin 1px arc (polyline) at a given radius across [start,end] degrees, about CENTER.
template<class It>
void arc_line(It &it, float radius, float start_deg, float end_deg, Color col) {
  float start = deg2rad(start_deg), end = deg2rad(end_deg);
  int px = CENTER + (int)(cosf(start) * radius), py = CENTER + (int)(sinf(start) * radius);
  for (float a = start; a <= end + 1e-4f; a += deg2rad(2.0f)) {
    int x = CENTER + (int)(cosf(a) * radius), y = CENTER + (int)(sinf(a) * radius);
    it.line(px, py, x, y, col);
    px = x; py = y;
  }
}

// Outer bezel: 60 base ticks (major every 5). Seconds tick is drawn separately, last.
template<class It>
void draw_bezel(It &it, const Palette &p) {
  for (int i = 0; i < 60; i++) {
    float a = deg2rad(i * 6 - 90);
    bool major = (i % 5) == 0;
    int r_out = 118;
    int r_in = major ? 108 : 113;
    it.line(CENTER + (int)(cosf(a) * r_in), CENTER + (int)(sinf(a) * r_in),
            CENTER + (int)(cosf(a) * r_out), CENTER + (int)(sinf(a) * r_out),
            major ? p.bezel_major : p.bezel_minor);
  }
}

// Alert ring: the outer ring becomes a solid band in `color`.
// Red for presence, green while the BOOT button is held.
template<class It>
void draw_alert_ring(It &it, Color color) {
  it.filled_ring(CENTER, CENTER, 118, 108, color);
}

// Presence alert: solid red band.
template<class It>
void draw_presence_ring(It &it) {
  draw_alert_ring(it, Color(235, 30, 30));
}

// Button press: same solid band, green.
template<class It>
void draw_button_ring(It &it) {
  draw_alert_ring(it, Color(30, 210, 80));
}

// The current-second highlight tick (longer + thicker), drawn last so it sits on top.
template<class It>
void draw_seconds_tick(It &it, int second, Color color) {
  float a = deg2rad(second * 6 - 90);
  fill_radial_quad(it, CENTER, CENTER, 103, 118, a - deg2rad(0.9f), a + deg2rad(0.9f), color);
}

// Main concentric gauge (Electricity/Gas): discrete ~3px ticks, active solid-filled,
// idle a thin track. Sweep -220deg..40deg (bottom gap), step 4deg.
template<class It>
void draw_ring_gauge(It &it, int inner, int outer, float pct, Color active, Color track) {
  float start = deg2rad(-220.0f), end = deg2rad(40.0f);
  float limit = start + (end - start) * clamp01(pct);
  for (float a = start; a <= end + 0.0001f; a += deg2rad(4.0f)) {
    bool on = a <= limit;
    draw_tick(it, CENTER, CENTER, inner, outer, a, on ? active : track, on /*wide*/);
  }
}

// Connector arc across the bottom gap, leaving `half_gap_px` clearance on each side
// of the centred label so the line stops before the text.
template<class It>
void draw_connector(It &it, float radius, Color color, float half_gap_px) {
  float ratio = half_gap_px / radius;
  if (ratio > 1.0f) ratio = 1.0f;
  float edge = acosf(ratio) * 180.0f / 3.14159265f;   // gap edge angle
  arc_line(it, radius, 40.0f, edge, color);
  arc_line(it, radius, 180.0f - edge, 140.0f, color);
}

// Small sub-dial gauge: 16 ticks; active ticks solid ~3px, idle 1px.
template<class It>
void draw_sub_gauge(It &it, int cx, int cy, float pct, Color active, Color track) {
  const int inner = 16, outer = 22, N = 16;
  const float rmid = (inner + outer) / 2.0f;
  for (int i = 0; i < N; i++) {
    float a = deg2rad(150.0f + 240.0f * i / (N - 1));
    bool on = pct >= (i + 0.5f) / N;
    Color col = on ? active : track;
    if (on) {
      // solid block ~3px wide (±1.5px tangential half-width at the mid radius)
      fill_radial_quad(it, cx, cy, inner, outer, a - 1.5f / rmid, a + 1.5f / rmid, col);
    } else {
      it.line(cx + (int)roundf(cosf(a) * inner), cy + (int)roundf(sinf(a) * inner),
              cx + (int)roundf(cosf(a) * outer), cy + (int)roundf(sinf(a) * outer), col);
    }
  }
}

}  // namespace rd
