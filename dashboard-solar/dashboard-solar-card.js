/*
 * Dashboard Solar — Home Assistant custom Lovelace card
 * ----------------------------------------------------------------------------
 * Renders the same round energy display as the ESP32 firmware (dashboard_solar.h),
 * but in the HA UI, reading live entity states. Self-contained Canvas — no deps.
 *
 * Install:
 *   1) Copy this file to  <config>/www/dashboard-solar-card.js
 *   2) Settings → Dashboards → ⋮ → Resources → Add resource
 *        URL: /local/dashboard-solar-card.js   ·   Type: JavaScript module
 *   3) Add a card (YAML):
 *        type: custom:dashboard-solar-card
 *        # all entities below are optional — these are the defaults:
 *        pv: sensor.powermon_totalsolar
 *        load: sensor.consum_total
 *        battery_soc: sensor.byd_battery_box_premium_hv_state_of_charge
 *        battery_charge: sensor.solarnet_power_battery_charge
 *        battery_discharge: sensor.solarnet_power_battery_discharge
 *        grid_import: sensor.solarnet_power_grid_import
 *        grid_export: sensor.solarnet_power_grid_export
 *        ev_charge: sensor.kitt_charger_power
 *        ev_soc: sensor.kitt_battery
 *        weather: weather.ivallm3
 *        presence: binary_sensor.cam_entrada_moviment_3
 *        power_max: 8000          # number, or an entity id
 * ----------------------------------------------------------------------------
 */

const DEF = {
  pv: 'sensor.powermon_totalsolar',
  load: 'sensor.consum_total',
  battery_soc: 'sensor.byd_battery_box_premium_hv_state_of_charge',
  battery_charge: 'sensor.solarnet_power_battery_charge',
  battery_discharge: 'sensor.solarnet_power_battery_discharge',
  grid_import: 'sensor.solarnet_power_grid_import',
  grid_export: 'sensor.solarnet_power_grid_export',
  ev_charge: 'sensor.kitt_charger_power',
  ev_soc: 'sensor.kitt_battery',
  weather: 'weather.ivallm3',
  presence: 'binary_sensor.cam_entrada_moviment_3',
  power_max: 8000,
};

// dark palette — mirrors ds::make_palette(dark) in dashboard_solar.h
const P = {
  bg: '#060c15', text: '#eef4ff', text_dim: '#9fb2cc',
  bezel_major: 'rgb(150,175,205)', bezel_minor: 'rgb(80,100,125)',
  solar: '#ffd60a', solar_track: 'rgb(110,90,30)',
  f_house: '#3896f8', f_batt: '#4cc878', f_grid: '#bec8d6',      // grid export = light grey
  f_from_batt: '#cd5f19', f_from_grid: '#dc4646',                 // battery→house / import
  inner_track: 'rgb(45,55,70)', battery: '#4cc878',
  electric_vehicle: '#b978eb', sub_track: 'rgb(60,75,90)',
  seconds: '#ff5f5f', presence: '#eb1e1e',
};
const MONO = 'ui-monospace, Menlo, Consolas, monospace';
const WX = { sunny:'☀️', 'clear-night':'🌙', partlycloudy:'⛅', cloudy:'☁️', fog:'🌫️',
  hail:'🌨️', lightning:'⚡', 'lightning-rainy':'⛈️', pouring:'🌧️', rainy:'🌦️',
  snowy:'❄️', 'snowy-rainy':'🌨️', windy:'💨', 'windy-variant':'💨', exceptional:'⚠️' };

const C = 120;
const rad = d => d * Math.PI / 180;
const fmt = w => Math.abs(w) >= 1000 ? (w / 1000).toFixed(2) + ' kW' : (isNaN(w) ? '-- W' : w.toFixed(0) + ' W');

function fillArc(ctx, rin, rout, a0, a1, color) {
  ctx.beginPath(); ctx.arc(C, C, rout, rad(a0), rad(a1), false); ctx.arc(C, C, rin, rad(a1), rad(a0), true);
  ctx.closePath(); ctx.fillStyle = color; ctx.fill();
}
function segColorAt(segs, w, track) {
  let cum = 0; for (const s of segs) { if (s.w <= 0) continue; if (w >= cum && w < cum + s.w) return s.c; cum += s.w; } return track;
}
function stackedRing(ctx, rin, rout, segs, pmax, track) {
  const start = -220, end = 40, sweep = 260;
  for (let a = start; a <= end + 1e-3; a += 4) fillArc(ctx, rin, rout, a - 1.2, a + 1.2, segColorAt(segs, (a - start) / sweep * pmax, track));
}
function splitGauge(ctx, rin, rout, bat, tes, bc, tc, track) {
  const N = 16, span = 110, h = 2.2;
  for (let i = 0; i < N; i++) { const a = 140 + span * (i + 0.5) / N; fillArc(ctx, rin, rout, a - h, a + h, bat >= (i + 0.5) / N ? bc : track); }
  for (let i = 0; i < N; i++) { const a = 40 - span * (i + 0.5) / N; fillArc(ctx, rin, rout, a - h, a + h, tes >= (i + 0.5) / N ? tc : track); }
}
function bezel(ctx) {
  for (let i = 0; i < 60; i++) { const a = rad(i * 6 - 90), M = (i % 5) === 0, ro = 118, ri = M ? 108 : 113;
    ctx.beginPath(); ctx.moveTo(C + Math.cos(a) * ri, C + Math.sin(a) * ri); ctx.lineTo(C + Math.cos(a) * ro, C + Math.sin(a) * ro);
    ctx.strokeStyle = M ? P.bezel_major : P.bezel_minor; ctx.lineWidth = 1; ctx.stroke(); }
}
function alertRing(ctx, col) { fillArc(ctx, 108, 118, 0, 360, col); }
function secTick(ctx, sec) { fillArc(ctx, 103, 118, sec * 6 - 90 - 1.2, sec * 6 - 90 + 1.2, P.seconds); }
function arcLine(ctx, r, a0, a1, c) { ctx.beginPath(); ctx.arc(C, C, r, rad(a0), rad(a1), false); ctx.strokeStyle = c; ctx.lineWidth = 1; ctx.stroke(); }
function connector(ctx, r, c, hg) { const e = Math.acos(Math.min(1, hg / r)) * 180 / Math.PI; arcLine(ctx, r, 40, e, c); arcLine(ctx, r, 180 - e, 140, c); }

function paint(ctx, d) {
  ctx.setTransform(2, 0, 0, 2, 0, 0);
  ctx.clearRect(0, 0, 240, 240);
  ctx.fillStyle = P.bg; ctx.fillRect(0, 0, 240, 240);
  const self_gen = Math.max(0, d.pv - d.exp), house_ev = Math.max(0, d.load - d.ev);
  const now = new Date(), sec = now.getSeconds();

  if (d.pres) alertRing(ctx, P.presence); else bezel(ctx);
  // outer = sources: solar self-used, battery→house, import, exported (grey, last)
  stackedRing(ctx, 96, 110, [ { w: self_gen, c: P.solar }, { w: d.dis, c: P.f_from_batt },
    { w: d.imp, c: P.f_from_grid }, { w: d.exp, c: P.f_grid } ], d.pmax, P.solar_track);
  // inner = usage: house−EV, home battery charge, EV charge
  stackedRing(ctx, 78, 92, [ { w: house_ev, c: P.f_house }, { w: d.chg, c: P.f_batt },
    { w: d.ev, c: P.electric_vehicle } ], d.pmax, P.inner_track);
  // third ring = SoC: home battery (left) + EV (right)
  splitGauge(ctx, 52, 64, (isNaN(d.soc) ? 0 : d.soc) / 100, (isNaN(d.esoc) ? 0 : d.esoc) / 100,
    P.battery, P.electric_vehicle, P.sub_track);

  ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
  // bottom value labels + connector arcs: solar (pv) + house (load)
  const sB = fmt(d.pv), hB = fmt(d.load); ctx.font = 'bold 14px ' + MONO;
  connector(ctx, 103, P.solar, ctx.measureText(sB).width / 2 + 3);
  connector(ctx, 85, P.f_house, ctx.measureText(hB).width / 2 + 3);
  ctx.fillStyle = P.f_house; ctx.fillText(hB, 120, 202);
  ctx.fillStyle = P.solar;   ctx.fillText(sB, 120, 220);
  // weather icon + temperature/humidity
  ctx.font = '20px sans-serif'; ctx.fillText(WX[d.wcond] || '☀️', 120, 57);
  let wt = ''; if (!isNaN(d.wtemp)) wt += Math.round(d.wtemp) + '°';
  if (!isNaN(d.whum)) wt += (wt ? '  ' : '') + Math.round(d.whum) + '%';
  if (wt) { ctx.font = '14px ' + MONO; ctx.fillStyle = P.text_dim; ctx.fillText(wt, 120, 93); }
  // centre clock: date + HH:MM
  ctx.font = '11px sans-serif'; ctx.fillStyle = P.text_dim;
  ctx.fillText(now.toLocaleDateString(undefined, { weekday: 'short', day: 'numeric', month: 'short' }), 120, 111);
  ctx.font = '22px ' + MONO; ctx.fillStyle = P.text;
  ctx.fillText(String(now.getHours()).padStart(2, '0') + ':' + String(now.getMinutes()).padStart(2, '0'), 120, 133);
  if (!d.pres) secTick(ctx, sec);
  // partial-gauge icons + %
  ctx.font = '20px sans-serif'; ctx.fillText('🔋', 95, 167); ctx.fillText('🚗', 145, 167);
  ctx.font = '12px ' + MONO; ctx.fillStyle = P.text;
  ctx.fillText(isNaN(d.soc) ? '--' : Math.round(d.soc) + '%', 95, 182);
  ctx.fillText(isNaN(d.esoc) ? '--' : Math.round(d.esoc) + '%', 145, 182);
}

class DashboardSolarCard extends HTMLElement {
  setConfig(config) {
    this._config = Object.assign({}, DEF, config || {});
    if (!this._canvas) this._build();
  }
  _build() {
    const card = document.createElement('ha-card');
    const style = document.createElement('style');
    // Fill the tile: ha-card takes the grid cell's height, the circular canvas is
    // centred and object-fit:contain keeps it round at any (even non-square) size.
    style.textContent = `ha-card{height:100%;box-sizing:border-box;display:flex;
      align-items:center;justify-content:center;padding:4px;background:${P.bg}}
      canvas{width:100%;height:100%;object-fit:contain;display:block}`;
    this._canvas = document.createElement('canvas'); this._canvas.width = 480; this._canvas.height = 480;
    card.appendChild(style); card.appendChild(this._canvas); this.appendChild(card);
  }
  set hass(hass) { this._hass = hass; this._render(); }
  _num(id) { const e = id && this._hass && this._hass.states[id]; if (!e) return NaN; const v = parseFloat(e.state); return isNaN(v) ? NaN : v; }
  _nz(id) { const v = this._num(id); return isNaN(v) ? 0 : v; }
  _attr(id, a) { const e = id && this._hass && this._hass.states[id]; const v = e && e.attributes ? parseFloat(e.attributes[a]) : NaN; return isNaN(v) ? NaN : v; }
  _pmax() { const pm = this._config.power_max; if (typeof pm === 'number') return pm; const v = this._num(pm); return (isNaN(v) || v < 100) ? 8000 : v; }
  _render() {
    if (!this._hass || !this._canvas) return;
    const c = this._config, W = c.weather, pr = c.presence;
    paint(this._canvas.getContext('2d'), {
      pv: this._nz(c.pv), load: this._nz(c.load), imp: this._nz(c.grid_import), exp: this._nz(c.grid_export),
      chg: this._nz(c.battery_charge), dis: this._nz(c.battery_discharge), ev: this._nz(c.ev_charge),
      soc: this._num(c.battery_soc), esoc: this._num(c.ev_soc), pmax: this._pmax(),
      pres: !!(pr && this._hass.states[pr] && this._hass.states[pr].state === 'on'),
      wcond: (W && this._hass.states[W]) ? this._hass.states[W].state : '',
      wtemp: this._attr(W, 'temperature'), whum: this._attr(W, 'humidity'),
    });
  }
  connectedCallback() { this._timer = setInterval(() => this._render(), 1000); }  // live clock + seconds tick
  disconnectedCallback() { clearInterval(this._timer); }
  getCardSize() { return 3; }                                    // masonry dashboards
  // Sections dashboards: a small, square, resizable widget (≈ quarter/half a section).
  getGridOptions() { return { rows: 4, columns: 6, min_rows: 3, min_columns: 3 }; }
  static getConfigElement() { return document.createElement('dashboard-solar-card-editor'); }
  static getStubConfig() { return Object.assign({ type: 'custom:dashboard-solar-card' }, DEF); }
}

customElements.define('dashboard-solar-card', DashboardSolarCard);

// ── Visual config editor (entity pickers when adding/editing the card) ────────
// Built on HA's <ha-form> + selectors, so users pick entities from a dropdown
// instead of editing YAML. Every field is optional; leave one blank to disable it.
const EDITOR_SCHEMA = [
  { name: 'pv',                selector: { entity: { domain: 'sensor' } } },
  { name: 'load',             selector: { entity: { domain: 'sensor' } } },
  { name: 'battery_soc',      selector: { entity: { domain: 'sensor' } } },
  { name: 'battery_charge',   selector: { entity: { domain: 'sensor' } } },
  { name: 'battery_discharge',selector: { entity: { domain: 'sensor' } } },
  { name: 'grid_import',      selector: { entity: { domain: 'sensor' } } },
  { name: 'grid_export',      selector: { entity: { domain: 'sensor' } } },
  { name: 'ev_charge',        selector: { entity: { domain: 'sensor' } } },
  { name: 'ev_soc',           selector: { entity: { domain: 'sensor' } } },
  { name: 'weather',          selector: { entity: { domain: 'weather' } } },
  { name: 'presence',         selector: { entity: { domain: 'binary_sensor' } } },
  { name: 'power_max',        selector: { number: { min: 500, max: 30000, step: 100, unit_of_measurement: 'W', mode: 'box' } } },
];
const EDITOR_LABELS = {
  pv: 'Solar production (W)', load: 'House consumption (W)',
  battery_soc: 'Home battery SoC (%)', battery_charge: 'Home battery charge (W)',
  battery_discharge: 'Home battery discharge (W)', grid_import: 'Grid import (W)',
  grid_export: 'Grid export (W)', ev_charge: 'EV charge power (W)',
  ev_soc: 'EV battery SoC (%)', weather: 'Weather', presence: 'Presence (binary sensor)',
  power_max: 'Ring full-scale (W)',
};
const EDITOR_HELPERS = {
  pv: 'Total solar power being generated, in W. Shown as the amber label at the bottom and drives the outer ring.',
  load: 'Total house power draw, in W. Shown as the blue label at the bottom and drives the inner ring.',
  battery_soc: 'Home battery state of charge, in %. Fills the left half of the inner split gauge.',
  battery_charge: 'Power flowing INTO the home battery, in W. Green segment of the inner (usage) ring.',
  battery_discharge: 'Power flowing OUT of the home battery to the house, in W. Orange segment of the outer (sources) ring.',
  grid_import: 'Power drawn FROM the grid, in W. Red segment of the outer (sources) ring.',
  grid_export: 'Solar power sent back TO the grid, in W. Grey segment (last) of the outer (sources) ring.',
  ev_charge: 'Power going into the electric vehicle, in W. Purple segment of the inner (usage) ring.',
  ev_soc: 'EV state of charge, in %. Fills the right half of the inner split gauge.',
  weather: 'A weather entity — its condition icon, temperature and humidity show in the centre.',
  presence: 'Optional binary sensor. When "on", the outer bezel turns into a red alert ring.',
  power_max: 'Watts that equal a full ring (both rings share this scale). Set it near your peak solar/consumption, e.g. 8000.',
};

class DashboardSolarCardEditor extends HTMLElement {
  setConfig(config) { this._config = config; this._update(); }
  set hass(hass) { this._hass = hass; this._update(); }
  _update() {
    if (!this._hass || !this._config) return;
    if (!this._form) {
      this._form = document.createElement('ha-form');
      this._form.computeLabel = (s) => EDITOR_LABELS[s.name] || s.name;
      this._form.computeHelper = (s) => EDITOR_HELPERS[s.name] || '';
      this._form.addEventListener('value-changed', (e) => {
        // Merge to preserve `type` (and any keys not in the schema).
        const config = Object.assign({}, this._config, e.detail.value);
        this.dispatchEvent(new CustomEvent('config-changed',
          { detail: { config }, bubbles: true, composed: true }));
      });
      this.appendChild(this._form);
    }
    this._form.hass = this._hass;
    this._form.schema = EDITOR_SCHEMA;
    this._form.data = this._config;
  }
}
customElements.define('dashboard-solar-card-editor', DashboardSolarCardEditor);

window.customCards = window.customCards || [];
window.customCards.push({ type: 'dashboard-solar-card', name: 'Dashboard Solar',
  description: 'Round solar-energy display (sources/usage rings + battery/EV)' });
const VERSION = '1.0.0';
console.info('%c DASHBOARD-SOLAR-CARD %c v' + VERSION + ' ',
  'background:#ffd60a;color:#111;font-weight:700', 'background:#060c15;color:#ffd60a');
