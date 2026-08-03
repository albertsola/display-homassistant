#pragma once
// Drawing helpers for the Dashboard Solar display lambda.
// Same watch-face style as round_dashboard.h (geometry + theme as named
// functions; the YAML lambda stays a short layout that reads Home Assistant
// state and calls these). Self-contained so the solar track evolves
// independently of the round-dashboard track.
//
// Layout (see dashboard-solar/README.md):
//   outer ring (r96-110) = Solar production magnitude (amber)
//   inner ring (r78-92)  = stacked FLOW ring on the SAME scale, so its
//                          blue+green+purple always overlay the outer arc
//   left sub-dial  = Battery SoC % (+ charge/discharge)
//   right sub-dial = Grid net (import red / export purple)
//   centre = Home usage headline; top = weather icon; bezel = seconds
//            (turns green while exporting)

#include "esphome.h"
#include <cmath>
#include <cstdio>
#include <string>

namespace ds {

using esphome::Color;

static constexpr int CENTER = 120;   // face centre (240x240 panel)

// ---- theme palette ----
struct Palette {
  Color background, text, text_dim, text_off;
  Color bezel_major, bezel_minor, dial_well;
  Color solar, solar_track;                 // outer ring (solar magnitude)
  Color f_house, f_batt, f_grid;            // solar-origin flow: blue / green / purple
  Color f_from_batt, f_from_grid;           // deficit flow: orange / dark red
  Color inner_track;                        // inner ring idle
  Color battery, grid_imp, grid_exp, sub_track;  // sub-dials
  Color value, seconds, press_ring, presence;
};

inline Palette make_palette(bool dark) {
  Palette p;
  p.background   = dark ? Color(8, 14, 24)     : Color(233, 239, 247);
  p.text         = dark ? Color(242, 247, 255) : Color(20, 30, 46);
  p.text_dim     = dark ? Color(159, 178, 204) : Color(90, 106, 128);
  p.text_off     = dark ? Color(120, 140, 165) : Color(150, 165, 185);
  p.bezel_major  = dark ? Color(150, 175, 205) : Color(120, 140, 165);
  p.bezel_minor  = dark ? Color(80, 100, 125)  : Color(184, 198, 214);
  p.dial_well    = dark ? Color(8, 16, 29)     : Color(214, 224, 236);
  p.solar        = dark ? Color(255, 179, 0)   : Color(210, 140, 0);   // amber
  p.solar_track  = dark ? Color(110, 90, 30)   : Color(230, 205, 150);
  p.f_house      = dark ? Color(56, 150, 248)  : Color(2, 110, 190);   // blue
  p.f_batt       = dark ? Color(76, 200, 120)  : Color(20, 150, 80);   // green
  p.f_grid       = dark ? Color(185, 120, 235) : Color(130, 60, 180);  // purple
  p.f_from_batt  = dark ? Color(255, 150, 60)  : Color(210, 110, 20);  // orange
  p.f_from_grid  = dark ? Color(220, 70, 70)   : Color(180, 30, 30);   // dark red
  p.inner_track  = dark ? Color(45, 55, 70)    : Color(200, 210, 222);
  p.battery      = dark ? Color(76, 200, 120)  : Color(20, 150, 80);
  p.grid_imp     = dark ? Color(220, 70, 70)   : Color(180, 30, 30);
  p.grid_exp     = dark ? Color(185, 120, 235) : Color(130, 60, 180);
  p.sub_track    = dark ? Color(60, 75, 90)    : Color(200, 210, 222);
  p.value        = dark ? Color(210, 225, 245) : Color(30, 45, 70);
  p.seconds      = dark ? Color(255, 95, 95)   : Color(205, 45, 45);
  p.press_ring   = dark ? Color(50, 200, 110)  : Color(30, 160, 80);   // BOOT-pressed ring (green)
  p.presence     = dark ? Color(235, 30, 30)   : Color(205, 40, 40);   // presence ring (red)
  return p;
}

// ---- small math / format helpers ----
inline float deg2rad(float d) { return d * 3.14159265f / 180.0f; }
inline float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
inline float nz(float v) { return std::isnan(v) ? 0.0f : v; }   // NaN -> 0

// Map a value onto 0..1 given a full scale.
inline float scale_value(float v, float lo, float hi) {
  if (std::isnan(v) || std::isnan(lo) || std::isnan(hi) || hi <= lo) return 0.0f;
  return clamp01((v - lo) / (hi - lo));
}

// Format a power in W as "123 W" or "1.23 kW" (magnitude; caller adds sign).
inline void fmt_power(char *buf, size_t n, float w) {
  if (std::isnan(w)) { snprintf(buf, n, "-- W"); return; }
  float a = fabsf(w);
  if (a >= 1000.0f) snprintf(buf, n, "%.2f kW", w / 1000.0f);
  else              snprintf(buf, n, "%.0f W", w);
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

// Filled annular sector between r_in and r_out across [a0..a1] degrees — SOLID, no gaps:
// it tiles the band with quads (two filled_triangles each) whose edges are shared, so
// there are no radial gaps at any radius. This is the gap-free primitive every filled
// band (rings, gauges, alert band, seconds tick) is built from.
template<class It>
void fill_arc(It &it, int cx, int cy, float r_in, float r_out, float a0_deg, float a1_deg, Color col) {
  if (a1_deg - a0_deg < 0.01f) return;
  float a0 = deg2rad(a0_deg), a1 = deg2rad(a1_deg);
  int n = (int)ceilf((a1_deg - a0_deg) / 2.0f);   // ~2deg facets
  if (n < 1) n = 1;
  int pxi = cx + (int)roundf(cosf(a0) * r_in),  pyi = cy + (int)roundf(sinf(a0) * r_in);
  int pxo = cx + (int)roundf(cosf(a0) * r_out), pyo = cy + (int)roundf(sinf(a0) * r_out);
  for (int i = 1; i <= n; i++) {
    float a = a0 + (a1 - a0) * i / n;
    int xi = cx + (int)roundf(cosf(a) * r_in),  yi = cy + (int)roundf(sinf(a) * r_in);
    int xo = cx + (int)roundf(cosf(a) * r_out), yo = cy + (int)roundf(sinf(a) * r_out);
    it.filled_triangle(pxi, pyi, pxo, pyo, xo, yo, col);
    it.filled_triangle(pxi, pyi, xo, yo, xi, yi, col);
    pxi = xi; pyi = yi; pxo = xo; pyo = yo;
  }
}

// Outer bezel: 60 base ticks (major every 5). Seconds tick drawn separately, last.
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

// Alert band: the whole outer ring becomes a solid colour (green on BOOT press, red on presence).
template<class It>
void draw_alert_ring(It &it, Color col) {
  fill_arc(it, CENTER, CENTER, 108, 118, 0.0f, 360.0f, col);
}

// The current-second highlight tick (longer + thicker), drawn last so it sits on top.
template<class It>
void draw_seconds_tick(It &it, int second, Color color) {
  float a = second * 6 - 90;   // degrees
  fill_arc(it, CENTER, CENTER, 103, 118, a - 1.2f, a + 1.2f, color);
}

// Concentric single-value gauge. Sweep -220deg..40deg (bottom gap).
// ticked=false -> one solid band; ticked=true -> discrete 4deg blocks (each a filled
// polygon, NOT lines, so no black-pixel gaps). Idle portion uses the track colour.
template<class It>
void draw_ring_gauge(It &it, int inner, int outer, float pct, Color active, Color track, bool ticked) {
  float start = -220.0f, end = 40.0f;
  float limit = start + (end - start) * clamp01(pct);
  if (!ticked) {
    fill_arc(it, CENTER, CENTER, inner, outer, start, end, track);
    if (limit > start + 0.5f) fill_arc(it, CENTER, CENTER, inner, outer, start, limit, active);
  } else {
    for (float a = start; a <= end + 1e-3f; a += 4.0f)
      fill_arc(it, CENTER, CENTER, inner, outer, a - 1.2f, a + 1.2f, a <= limit ? active : track);
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

// Connector arc across the bottom gap, leaving `half_gap_px` clearance each side of the
// centred label so the line stops before the text (drawn under each ring's value label).
template<class It>
void draw_connector(It &it, float radius, Color color, float half_gap_px) {
  float ratio = half_gap_px / radius;
  if (ratio > 1.0f) ratio = 1.0f;
  float edge = acosf(ratio) * 180.0f / 3.14159265f;   // gap edge angle
  arc_line(it, radius, 40.0f, edge, color);
  arc_line(it, radius, 180.0f - edge, 140.0f, color);
}

// Stacked flow ring: solid coloured segments on the shared `power_max` scale (same sweep
// as draw_ring_gauge). Each `watts` maps to an arc; segments stack from the start and are
// filled solid (no gaps). Because the solar segments sum to PV they overlay the outer arc.
struct Seg { float watts; Color color; };

// which segment covers cumulative-watts `w` (track colour if past the total)
inline Color seg_color_at(const Seg *segs, int n, float w, Color track) {
  float cum = 0.0f;
  for (int i = 0; i < n; i++) {
    if (segs[i].watts <= 0.0f) continue;
    if (w >= cum && w < cum + segs[i].watts) return segs[i].color;
    cum += segs[i].watts;
  }
  return track;
}

template<class It>
void draw_stacked_ring(It &it, int inner, int outer, const Seg *segs, int n,
                       float power_max, Color track, bool ticked) {
  if (power_max <= 0.0f) power_max = 1.0f;
  float start = -220.0f, end = 40.0f, sweep = end - start;
  if (!ticked) {
    fill_arc(it, CENTER, CENTER, inner, outer, start, end, track);   // idle remainder
    float cum = 0.0f;
    for (int i = 0; i < n; i++) {
      float w = segs[i].watts;
      if (w > 0.0f) {
        float a0 = start + (cum / power_max) * sweep;
        float a1 = start + ((cum + w) / power_max) * sweep;
        if (a1 > end) a1 = end;
        if (a0 < end) fill_arc(it, CENTER, CENTER, inner, outer, a0, a1, segs[i].color);
      }
      cum += w;
      if (start + (cum / power_max) * sweep >= end) break;   // ring full
    }
  } else {
    // discrete 4deg blocks, coloured by the segment at each block (filled polygons)
    for (float a = start; a <= end + 1e-3f; a += 4.0f) {
      float w = (a - start) / sweep * power_max;
      fill_arc(it, CENTER, CENTER, inner, outer, a - 1.2f, a + 1.2f,
               seg_color_at(segs, n, w, track));
    }
  }
}

// Small sub-dial gauge. Sweep 150deg..390deg (240deg). ticked=false -> solid arc;
// ticked=true -> 16 discrete blocks (filled polygons). Idle portion uses the track colour.
template<class It>
void draw_sub_gauge(It &it, int cx, int cy, float pct, Color active, Color track, bool ticked) {
  const int inner = 16, outer = 22;
  float start = 150.0f, sweep = 240.0f;
  if (!ticked) {
    fill_arc(it, cx, cy, inner, outer, start, start + sweep, track);
    float lim = sweep * clamp01(pct);
    if (lim > 0.5f) fill_arc(it, cx, cy, inner, outer, start, start + lim, active);
  } else {
    const int N = 16;
    for (int i = 0; i < N; i++) {
      float a = start + sweep * i / (N - 1);
      bool on = pct >= (i + 0.5f) / N;
      fill_arc(it, cx, cy, inner, outer, a - 2.5f, a + 2.5f, on ? active : track);
    }
  }
}

// Third ring, split BAT-left / KIT-right, both rising toward the top (ticked polygons):
//   battery arc 140deg..250deg (lower-left -> upper-left), fills from 140
//   tesla   arc  40deg..-70deg (lower-right -> upper-right), fills from 40
// leaves a ~40deg gap at the top (weather icon) and a wide gap at the bottom (value labels).
template<class It>
void draw_split_gauge(It &it, int r_in, int r_out, float bat, float tes,
                      Color bat_col, Color tes_col, Color track) {
  const int N = 16;                 // 16 ticks per side
  const float span = 110.0f, half = 2.2f;
  for (int i = 0; i < N; i++) {      // battery: 140deg..250deg, fills from 140 (lower-left up)
    float a = 140.0f + span * (i + 0.5f) / N;
    bool on = clamp01(bat) >= (i + 0.5f) / N;
    fill_arc(it, CENTER, CENTER, r_in, r_out, a - half, a + half, on ? bat_col : track);
  }
  for (int i = 0; i < N; i++) {      // tesla: 40deg..-70deg, fills from 40 (lower-right up)
    float a = 40.0f - span * (i + 0.5f) / N;
    bool on = clamp01(tes) >= (i + 0.5f) / N;
    fill_arc(it, CENTER, CENTER, r_in, r_out, a - half, a + half, on ? tes_col : track);
  }
}

}  // namespace ds
