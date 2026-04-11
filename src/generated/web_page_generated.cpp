#include <Arduino.h>
#include "web_page.h"

const char INDEX_HTML[] PROGMEM = R"HTML_ASSET(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Knee Rehab Tracker</title>
  <link rel="stylesheet" href="/styles.css">
</head>
<body>
  <div class="shell">
    <div class="ambient ambient-a"></div>
    <div class="ambient ambient-b"></div>
    <div class="ambient ambient-c"></div>
    <section class="hero">
      <div class="panel hero-panel">
        <p class="eyebrow">Rehab Tracker</p>
        <h1 class="headline"><span id="today-average">0.0</span>/100</h1>
        <p class="subline">Daily average step quality with a goal of <strong id="goal-inline">85</strong>.</p>
        <div class="hero-ribbon">
          <span class="signal-dot" id="hero-signal-dot"></span>
          <span id="hero-signal-copy">Observable live telemetry active</span>
        </div>
      </div>
      <div class="panel control-panel">
        <div class="status-row">
          <div class="tabs" id="tabs">
            <button data-view="live" class="active" type="button">Live</button>
            <button data-view="history" type="button">History</button>
            <button data-view="variables" type="button">Variables</button>
          </div>
          <div id="sync-pill" class="status-pill">Phone time synced for daily tracking</div>
        </div>
        <div class="card-grid">
          <div class="metric-card telemetry-card" data-telemetry-card>
            <div class="metric-label">Current Reading</div>
            <div class="metric-value" id="reading">0</div>
            <div class="metric-detail">Raw potentiometer value</div>
            <div class="metric-trace"><span></span></div>
          </div>
          <div class="metric-card telemetry-card" data-telemetry-card>
            <div class="metric-label">Straightness</div>
            <div class="metric-value" id="percent-straight">0%</div>
            <div class="metric-detail">Mapped knee position</div>
            <div class="metric-trace"><span></span></div>
          </div>
          <div class="metric-card telemetry-card" data-telemetry-card>
            <div class="metric-label">Step Score</div>
            <div class="metric-value" id="last-score">0.0</div>
            <div class="metric-detail">Most recent completed step</div>
            <div class="metric-trace"><span></span></div>
          </div>
          <div class="metric-card telemetry-card" data-telemetry-card>
            <div class="metric-label">Steps Logged</div>
            <div class="metric-value" id="step-count">0</div>
            <div class="metric-detail">Stored in device history</div>
            <div class="metric-trace"><span></span></div>
          </div>
        </div>
      </div>
    </section>

    <section id="live-view" class="view-surface">
      <div class="live-layout">
        <div class="panel meter-card">
          <p class="eyebrow">Knee Position</p>
          <svg viewBox="0 0 280 320" aria-label="Knee joint animation" id="knee-meter">
            <line class="guide" x1="140" y1="64" x2="140" y2="174"></line>
            <line class="limb limb-shadow" x1="140" y1="64" x2="140" y2="174"></line>
            <line class="limb limb-shadow" id="lower-leg-shadow" x1="140" y1="174" x2="140.0" y2="274.0"></line>
            <line class="limb" x1="140" y1="64" x2="140" y2="174"></line>
            <line class="limb" id="lower-leg" x1="140" y1="174" x2="140.0" y2="274.0"></line>
            <line class="limb-highlight" x1="140" y1="64" x2="140" y2="174"></line>
            <line class="limb-highlight" id="lower-leg-highlight" x1="140" y1="174" x2="140.0" y2="274.0"></line>
            <line class="stride-echo" id="stride-echo" x1="140" y1="174" x2="140.0" y2="274.0"></line>
            <circle class="joint joint-glow" cx="140" cy="64" r="18"></circle>
            <circle class="joint joint-glow" cx="140" cy="174" r="20"></circle>
            <circle class="joint joint-core" cx="140" cy="64" r="14"></circle>
            <circle class="joint joint-core" cx="140" cy="174" r="16"></circle>
            <circle class="joint joint-glow" id="ankle-glow" cx="140.0" cy="274.0" r="18"></circle>
            <circle class="joint joint-core" id="ankle" cx="140.0" cy="274.0" r="14"></circle>
            <circle class="pulse-ring" id="ankle-ring" cx="140.0" cy="274.0" r="22"></circle>
          </svg>
          <div class="joint-labels">
            <span>Bent</span>
            <span>Straight</span>
          </div>
          <div class="meter-demo-copy" id="meter-demo-copy">Waiting for live knee movement</div>
        </div>
        <div class="panel">
          <div class="status-row">
            <div>
              <p class="eyebrow">Live Movement Quality</p>
              <div id="step-status" class="status-pill warn">Waiting for a full movement cycle</div>
            </div>
            <div>
              <div class="metric-label">Speed</div>
              <div class="metric-value" id="speed">0.0</div>
              <div class="metric-detail">% straight per second</div>
            </div>
          </div>
          <div class="quality-grid">
            <div class="quality-card" data-quality-card>
              <div class="metric-label">Shaky Movement</div>
              <strong id="live-shaky">0.0</strong>
              <span>From reversals and extra path during a step</span>
            </div>
            <div class="quality-card" data-quality-card>
              <div class="metric-label">Uncontrolled Descent</div>
              <strong id="live-descent">0.0</strong>
              <span>Penalizes overly fast or spiky lowering</span>
            </div>
            <div class="quality-card" data-quality-card>
              <div class="metric-label">Compensation</div>
              <strong id="live-compensation">0.0</strong>
              <span>Flags phase imbalance and incomplete return</span>
            </div>
          </div>
        </div>
      </div>
    </section>

    <section id="history-view" class="hidden view-surface">
      <div class="history-layout">
        <div class="panel diagnostics-panel">
          <p class="eyebrow">History Diagnostics</p>
          <div class="diagnostics-grid">
            <div class="diagnostic-card" data-diagnostic-card>
              <div class="metric-label">Best Step</div>
              <strong id="best-step-score">0.0</strong>
              <span id="best-step-detail">No steps yet</span>
            </div>
            <div class="diagnostic-card" data-diagnostic-card>
              <div class="metric-label">Worst Step</div>
              <strong id="worst-step-score">0.0</strong>
              <span id="worst-step-detail">No steps yet</span>
            </div>
            <div class="diagnostic-card" data-diagnostic-card>
              <div class="metric-label">Best Day</div>
              <strong id="best-day-score">0.0</strong>
              <span id="best-day-detail">No days yet</span>
            </div>
            <div class="diagnostic-card" data-diagnostic-card>
              <div class="metric-label">Worst Day</div>
              <strong id="worst-day-score">0.0</strong>
              <span id="worst-day-detail">No days yet</span>
            </div>
            <div class="diagnostic-card" data-diagnostic-card>
              <div class="metric-label">Average Step Score</div>
              <strong id="average-step-score">0.0</strong>
              <span id="average-step-detail">Across all logged steps</span>
            </div>
            <div class="diagnostic-card" data-diagnostic-card>
              <div class="metric-label">Average Step Duration</div>
              <strong id="average-step-duration">0 ms</strong>
              <span id="average-step-duration-detail">Across all logged steps</span>
            </div>
          </div>
        </div>
        <div class="table-panel">
          <p class="eyebrow">Daily Averages</p>
          <div class="table-wrap">
            <table>
              <thead>
                <tr><th>Day</th><th>Average Score</th><th>Steps</th><th>Goal</th></tr>
              </thead>
              <tbody id="daily-average-body">
                <tr><td class="empty" colspan="4">No synced history yet.</td></tr>
              </tbody>
            </table>
          </div>
        </div>
        <div class="table-panel table-panel-history">
          <p class="eyebrow">Step History</p>
          <div class="table-wrap">
            <table>
              <thead>
                <tr>
                  <th>Time</th><th>Score</th><th>Shaky</th><th>Descent</th><th>Compensation</th>
                  <th>Duration</th><th>Range</th><th>Down Speed</th><th>Up Speed</th><th>Oscillations</th>
                </tr>
              </thead>
              <tbody id="history-body">
                <tr><td class="empty" colspan="10">No steps recorded yet.</td></tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>
    </section>

    <section id="variables-view" class="hidden view-surface">
      <div class="panel">
        <p class="eyebrow">Calibration Variables</p>
        <div class="hint">Open this directly at <strong>/variables</strong>. Saving applies immediately to live tracking.</div>
        <div id="variables-status" class="status-pill warn" style="margin-top:14px;">Variables not loaded yet</div>
        <div class="variables-grid">
          <div>
            <div class="field" data-variable-field><label for="minReading">Min Reading</label><input id="minReading" name="minReading" type="number"></div>
            <div class="field" data-variable-field><label for="maxReading">Max Reading</label><input id="maxReading" name="maxReading" type="number"></div>
            <div class="field" data-variable-field><label for="bentAngle">Bent Angle</label><input id="bentAngle" name="bentAngle" type="number" step="0.1"></div>
            <div class="field" data-variable-field><label for="straightAngle">Straight Angle</label><input id="straightAngle" name="straightAngle" type="number" step="0.1"></div>
            <div class="field" data-variable-field><label for="sampleIntervalMs">Sample Interval Ms</label><input id="sampleIntervalMs" name="sampleIntervalMs" type="number"></div>
            <div class="field" data-variable-field><label for="filterAlpha">Filter Alpha</label><input id="filterAlpha" name="filterAlpha" type="number" step="0.001"></div>
          </div>
          <div>
            <div class="field" data-variable-field><label for="motionThreshold">Motion Threshold</label><input id="motionThreshold" name="motionThreshold" type="number" step="0.001"></div>
            <div class="field" data-variable-field><label for="stepRangeThreshold">Step Range Threshold</label><input id="stepRangeThreshold" name="stepRangeThreshold" type="number" step="0.001"></div>
            <div class="field" data-variable-field><label for="startReadyThreshold">Start Ready Threshold</label><input id="startReadyThreshold" name="startReadyThreshold" type="number" step="0.001"></div>
            <div class="field" data-variable-field><label for="returnMargin">Return Margin</label><input id="returnMargin" name="returnMargin" type="number" step="0.001"></div>
            <div class="field" data-variable-field><label for="maxStepDurationMs">Max Step Duration Ms</label><input id="maxStepDurationMs" name="maxStepDurationMs" type="number"></div>
            <div class="field" data-variable-field><label for="sampleHistoryEnabled">Sample History</label><input id="sampleHistoryEnabled" name="sampleHistoryEnabled" type="text"></div>
          </div>
        </div>
        <div class="actions">
          <button id="save-variables" type="button">Save Variables</button>
          <button id="reload-variables" class="secondary" type="button">Reload</button>
        </div>
      </div>
    </section>
  </div>
  <script src="/app.js"></script>
</body>
</html>

)HTML_ASSET";

const char STYLES_CSS[] PROGMEM = R"CSS_ASSET(
:root {
  color-scheme: light;
  --bg: #f4eee6;
  --paper: rgba(255, 250, 245, 0.9);
  --paper-strong: #fffaf4;
  --line: rgba(63, 46, 32, 0.12);
  --text: #221912;
  --muted: #6e5748;
  --accent: #bf5b2c;
  --good: #2f7d53;
  --warn: #b85f18;
  --bad: #a5302d;
  --shadow: 0 24px 60px rgba(47, 29, 18, 0.12);
  --glow: rgba(191, 91, 44, 0.18);
  --surface-shift: 0px;
}

* { box-sizing: border-box; }

body {
  margin: 0;
  min-height: 100vh;
  background:
    radial-gradient(circle at top left, rgba(240, 195, 159, 0.9), transparent 28%),
    radial-gradient(circle at bottom right, rgba(191, 91, 44, 0.12), transparent 30%),
    linear-gradient(165deg, #f8f2eb, var(--bg));
  color: var(--text);
  font-family: "Segoe UI", Tahoma, Geneva, Verdana, sans-serif;
}

.shell {
  position: relative;
  overflow: hidden;
  width: min(1180px, calc(100vw - 24px));
  margin: 20px auto;
  padding: 20px;
  border-radius: 28px;
  background: var(--paper);
  border: 1px solid var(--line);
  box-shadow: var(--shadow);
  backdrop-filter: blur(10px);
}

.ambient {
  position: absolute;
  border-radius: 999px;
  pointer-events: none;
  filter: blur(18px);
  opacity: 0.7;
  animation: ambientFloat 16s ease-in-out infinite;
}
.ambient-a {
  width: 220px;
  height: 220px;
  top: -40px;
  right: -20px;
  background: radial-gradient(circle, rgba(191, 91, 44, 0.3), transparent 70%);
}
.ambient-b {
  width: 180px;
  height: 180px;
  bottom: 80px;
  left: -40px;
  background: radial-gradient(circle, rgba(47, 125, 83, 0.14), transparent 72%);
  animation-duration: 20s;
}
.ambient-c {
  width: 140px;
  height: 140px;
  top: 45%;
  right: 36%;
  background: radial-gradient(circle, rgba(255, 255, 255, 0.38), transparent 70%);
  animation-duration: 13s;
}

.hero, .card-grid, .history-layout, .variables-grid { display: grid; gap: 18px; }
.hero { grid-template-columns: minmax(320px, 1.2fr) minmax(260px, 0.8fr); align-items: stretch; }
.panel, .metric-card, .table-panel {
  border-radius: 24px;
  border: 1px solid var(--line);
  background: #fffaf4;
}
.panel {
  position: relative;
  padding: 22px;
  transition: transform 320ms ease, box-shadow 320ms ease, border-color 320ms ease, opacity 320ms ease;
}
.panel::before,
.metric-card::before,
.diagnostic-card::before,
.quality-card::before {
  content: "";
  position: absolute;
  inset: 0;
  border-radius: inherit;
  background: linear-gradient(135deg, rgba(255,255,255,0.24), transparent 40%, rgba(191, 91, 44, 0.06));
  pointer-events: none;
  opacity: 0.55;
}
.hero-panel,
.control-panel,
.meter-card,
.diagnostics-panel,
.table-panel,
.variables-grid,
.field {
  animation: riseFade 720ms ease both;
}
.eyebrow {
  margin: 0 0 10px;
  font-size: 0.78rem;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  color: var(--muted);
}
.headline {
  margin: 0;
  font-size: clamp(2.6rem, 7vw, 4.8rem);
  line-height: 0.95;
  color: var(--accent);
  text-shadow: 0 10px 30px rgba(191, 91, 44, 0.1);
}
.subline { margin: 10px 0 0; color: var(--muted); font-size: 1rem; }
.hero-ribbon {
  display: inline-flex;
  align-items: center;
  gap: 10px;
  margin-top: 18px;
  padding: 10px 14px;
  border-radius: 999px;
  background: rgba(255, 247, 240, 0.9);
  border: 1px solid rgba(191, 91, 44, 0.12);
  color: var(--muted);
  font-size: 0.92rem;
}
.signal-dot {
  width: 11px;
  height: 11px;
  border-radius: 999px;
  background: var(--good);
  box-shadow: 0 0 0 0 rgba(47, 125, 83, 0.3);
  animation: signalPulse 2.4s ease-out infinite;
}
.card-grid { grid-template-columns: repeat(4, minmax(0, 1fr)); margin-top: 18px; }
.metric-card {
  position: relative;
  padding: 18px;
  overflow: hidden;
}
.metric-label {
  font-size: 0.8rem;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  color: var(--muted);
}
.metric-value {
  margin-top: 10px;
  font-size: clamp(1.8rem, 4vw, 2.5rem);
  font-weight: 700;
  transition: transform 220ms ease, opacity 220ms ease, filter 220ms ease;
}
.metric-detail, .hint { margin-top: 8px; color: var(--muted); font-size: 0.95rem; }
.metric-trace {
  margin-top: 14px;
  height: 4px;
  border-radius: 999px;
  background: rgba(63, 46, 32, 0.08);
  overflow: hidden;
}
.metric-trace span {
  display: block;
  width: 42%;
  height: 100%;
  border-radius: inherit;
  background: linear-gradient(90deg, rgba(191, 91, 44, 0.15), rgba(191, 91, 44, 0.8), rgba(255,255,255,0.2));
  transform: translateX(-180%);
  opacity: 0;
}
.telemetry-card.is-live .metric-trace span {
  opacity: 1;
  animation: traceRun 1.35s linear infinite;
}
.tabs {
  display: inline-flex;
  gap: 8px;
  padding: 6px;
  border-radius: 999px;
  background: rgba(191, 91, 44, 0.08);
  border: 1px solid rgba(191, 91, 44, 0.15);
  flex-wrap: wrap;
}
.tabs button {
  border: 0;
  padding: 10px 16px;
  border-radius: 999px;
  background: transparent;
  color: var(--muted);
  font: inherit;
  cursor: pointer;
  transition: transform 180ms ease, background 180ms ease, color 180ms ease, box-shadow 180ms ease;
}
.tabs button.active {
  background: var(--accent);
  color: #fff8f4;
  box-shadow: 0 10px 24px rgba(191, 91, 44, 0.24);
}
.tabs button:hover { transform: translateY(-1px); }
.status-row {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 16px;
}
.status-pill {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  border-radius: 999px;
  font-size: 0.92rem;
  background: rgba(47, 125, 83, 0.1);
  color: var(--good);
  transition: transform 220ms ease, background 220ms ease, color 220ms ease, opacity 220ms ease;
}
.status-pill.warn { background: rgba(184, 95, 24, 0.12); color: var(--warn); }
.hidden { display: none !important; }
.view-surface {
  transform: translateY(var(--surface-shift));
  transition: transform 380ms ease, opacity 380ms ease, filter 380ms ease;
}
.view-surface.is-transitioning {
  animation: viewReveal 420ms ease;
}
.live-layout {
  display: grid;
  grid-template-columns: minmax(320px, 420px) minmax(0, 1fr);
  gap: 18px;
  margin-top: 18px;
}
.meter-card {
  padding: 20px;
  text-align: center;
  background: linear-gradient(180deg, rgba(255,255,255,0.92), rgba(246, 232, 220, 0.92));
  overflow: hidden;
}
.joint-labels, .quality-grid { display: grid; gap: 12px; }
.joint-labels {
  grid-template-columns: repeat(2, minmax(0, 1fr));
  margin-top: 8px;
  color: var(--muted);
  text-transform: uppercase;
  letter-spacing: 0.08em;
  font-size: 0.8rem;
}
svg { width: min(100%, 320px); height: auto; overflow: visible; }
.limb {
  fill: none;
  stroke: var(--accent);
  stroke-width: 22;
  stroke-linecap: round;
  filter: drop-shadow(0 12px 18px rgba(191, 91, 44, 0.18));
  transition: filter 220ms ease, transform 220ms ease;
}
.limb-shadow {
  stroke: rgba(34, 25, 18, 0.16);
  stroke-width: 30;
  filter: blur(5px);
}
.limb-highlight {
  fill: none;
  stroke: rgba(255, 250, 245, 0.82);
  stroke-width: 7;
  stroke-linecap: round;
  stroke-opacity: 0.88;
}
.stride-echo {
  fill: none;
  stroke: rgba(191, 91, 44, 0.18);
  stroke-width: 10;
  stroke-linecap: round;
  stroke-dasharray: 10 12;
  opacity: 0;
  transition: opacity 180ms ease;
}
.guide {
  fill: none;
  stroke: rgba(34, 25, 18, 0.12);
  stroke-width: 12;
  stroke-linecap: round;
  stroke-dasharray: 10 14;
}
.joint {
  transition: transform 220ms ease, filter 220ms ease;
}
.joint-glow {
  fill: rgba(191, 91, 44, 0.22);
  filter: blur(2px);
}
.joint-core {
  fill: var(--text);
}
.pulse-ring {
  fill: none;
  stroke: rgba(191, 91, 44, 0.42);
  stroke-width: 3;
  stroke-dasharray: 14 10;
  transform-origin: center;
  opacity: 0;
  transform: scale(0.92);
  transition: opacity 260ms ease, transform 260ms ease, stroke-width 260ms ease;
}
.meter-card.is-energized .joint {
  filter: drop-shadow(0 0 16px rgba(191, 91, 44, 0.2));
}
.meter-card.is-energized .limb {
  filter: drop-shadow(0 16px 22px rgba(191, 91, 44, 0.28));
}
.meter-card.is-energized .stride-echo {
  opacity: 1;
}
.pulse-ring.is-moving {
  opacity: 0.82;
  stroke-width: 4;
  animation: anklePulse 1.35s ease-out 1;
}
.meter-demo-copy {
  margin-top: 10px;
  color: var(--muted);
  font-size: 0.9rem;
  opacity: 0.82;
}
.quality-grid { grid-template-columns: repeat(3, minmax(0, 1fr)); }
.quality-card {
  position: relative;
  padding: 18px;
  border-radius: 20px;
  border: 1px solid var(--line);
  background: rgba(255, 248, 241, 0.86);
  transition: transform 220ms ease, border-color 220ms ease, box-shadow 220ms ease;
}
.quality-card strong, .quality-card span { display: block; }
.quality-card strong { font-size: 1.7rem; margin: 10px 0 6px; }
.quality-card span { color: var(--muted); font-size: 0.92rem; }
.history-layout { grid-template-columns: minmax(0, 1fr); margin-top: 18px; }
.table-panel { padding: 18px; overflow: hidden; }
.table-wrap {
  overflow: auto;
  border-radius: 18px;
  border: 1px solid var(--line);
  background: linear-gradient(180deg, rgba(255,255,255,0.88), rgba(249, 241, 233, 0.88));
}
table {
  width: 100%;
  border-collapse: collapse;
  min-width: 880px;
  background: #fffdf9;
}
th, td {
  padding: 12px 14px;
  text-align: left;
  border-bottom: 1px solid rgba(63, 46, 32, 0.08);
  font-size: 0.93rem;
  white-space: nowrap;
}
th {
  position: sticky;
  top: 0;
  background: #fcf4ed;
  color: var(--muted);
  text-transform: uppercase;
  letter-spacing: 0.06em;
  font-size: 0.76rem;
}
.score-chip {
  display: inline-block;
  min-width: 58px;
  padding: 6px 10px;
  border-radius: 999px;
  text-align: center;
  font-weight: 700;
  color: #fff;
  background: var(--good);
}
.score-chip.warn { background: var(--warn); }
.score-chip.bad { background: var(--bad); }
.empty { padding: 32px 18px; text-align: center; color: var(--muted); }
.diagnostics-panel {
  margin-bottom: 18px;
}
.diagnostics-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 14px;
  margin-top: 14px;
}
.diagnostic-card {
  position: relative;
  padding: 18px;
  border-radius: 18px;
  background: rgba(255, 250, 245, 0.92);
  border: 1px solid rgba(63, 46, 32, 0.08);
  box-shadow: inset 0 1px 0 rgba(255,255,255,0.7);
  display: grid;
  gap: 6px;
  transition: transform 240ms ease, opacity 240ms ease, border-color 240ms ease, box-shadow 240ms ease;
}
.diagnostic-card strong {
  font-size: 1.8rem;
  line-height: 1;
}
.diagnostic-card span {
  color: var(--muted);
  font-size: 0.92rem;
}
.variables-grid {
  grid-template-columns: repeat(2, minmax(0, 1fr));
  margin-top: 18px;
}
.field {
  display: grid;
  gap: 6px;
  margin-bottom: 14px;
  opacity: 0;
  transform: translateY(12px);
  animation: riseFade 680ms ease forwards;
}
.field label {
  font-size: 0.82rem;
  color: var(--muted);
  text-transform: uppercase;
  letter-spacing: 0.06em;
}
.field input {
  width: 100%;
  padding: 12px 14px;
  border-radius: 14px;
  border: 1px solid rgba(63, 46, 32, 0.16);
  background: #fffdf9;
  font: inherit;
  color: var(--text);
  transition: border-color 180ms ease, box-shadow 180ms ease, transform 180ms ease;
}
.field input:focus {
  outline: none;
  border-color: rgba(191, 91, 44, 0.42);
  box-shadow: 0 0 0 4px rgba(191, 91, 44, 0.12);
  transform: translateY(-1px);
}
.actions {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin-top: 10px;
}
.actions button {
  border: 0;
  padding: 12px 16px;
  border-radius: 999px;
  cursor: pointer;
  font: inherit;
  background: var(--accent);
  color: #fffaf4;
  transition: transform 180ms ease, box-shadow 180ms ease, opacity 180ms ease;
}
.actions button:hover {
  transform: translateY(-1px);
  box-shadow: 0 12px 24px rgba(191, 91, 44, 0.18);
}
.actions button.secondary {
  background: rgba(63, 46, 32, 0.12);
  color: var(--text);
}
.table-row-enter {
  animation: rowIn 420ms ease both;
}

@keyframes ambientFloat {
  0%, 100% { transform: translate3d(0, 0, 0) scale(1); }
  50% { transform: translate3d(0, -14px, 0) scale(1.06); }
}
@keyframes signalPulse {
  0% { box-shadow: 0 0 0 0 rgba(47, 125, 83, 0.3); }
  70% { box-shadow: 0 0 0 12px rgba(47, 125, 83, 0); }
  100% { box-shadow: 0 0 0 0 rgba(47, 125, 83, 0); }
}
@keyframes traceRun {
  0% { transform: translateX(-160%); }
  100% { transform: translateX(320%); }
}
@keyframes riseFade {
  0% { opacity: 0; transform: translateY(18px) scale(0.98); }
  100% { opacity: 1; transform: translateY(0) scale(1); }
}
@keyframes viewReveal {
  0% { opacity: 0.4; transform: translateY(10px); filter: blur(4px); }
  100% { opacity: 1; transform: translateY(0); filter: blur(0); }
}
@keyframes rowIn {
  0% { opacity: 0; transform: translateY(8px); }
  100% { opacity: 1; transform: translateY(0); }
}
@keyframes anklePulse {
  0% { transform: scale(0.94); opacity: 0.72; }
  45% { transform: scale(1.1); opacity: 0.46; }
  100% { transform: scale(1.28); opacity: 0; }
}
@media (max-width: 900px) {
  .hero, .live-layout, .variables-grid { grid-template-columns: 1fr; }
  .card-grid, .quality-grid, .diagnostics-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); }
}
@media (max-width: 640px) {
  .shell {
    width: min(100vw - 12px, 100%);
    margin: 6px auto;
    padding: 14px;
    border-radius: 20px;
  }
  .card-grid, .quality-grid, .diagnostics-grid { grid-template-columns: 1fr; }
  .headline { font-size: 2.7rem; }
  .status-row { align-items: stretch; }
}

)CSS_ASSET";

const char APP_JS[] PROGMEM = R"JS_ASSET(
(function () {
  function ngLog() {
    var args = Array.prototype.slice.call(arguments);
    args.unshift('[ng-runtime]');
    console.log.apply(console, args);
  }

  function ngFetchJson(path, options) {
    ngLog('fetch', path, options && options.method ? options.method : 'GET');
    return fetch(path, options).then(function (response) {
      return response.json();
    });
  }

  function ngResolveHook(name) {
    return typeof window[name] === 'function' ? window[name] : null;
  }

  function ngNormalizeName(name) {
    return name && name.charAt(name.length - 1) === '$' ? name.slice(0, -1) : name;
  }

  function ngExtractValue(value, path) {
    if (!path) {
      return value;
    }
    return String(path).split('.').reduce(function (current, segment) {
      if (current === null || current === undefined) {
        return undefined;
      }
      return current[segment];
    }, value);
  }

  function ngRenderTemplate(text, value, app, spec) {
    var rendered = String(text || '');
    var alias = ngNormalizeName(spec.name);
    var scalar = (value !== null && typeof value === 'object') ? '' : String(value === undefined || value === null ? '' : value);
    rendered = rendered.replace(/\{\{value\}\}/g, scalar);
    rendered = rendered.replace(/\{\{alias\}\}/g, alias);
    return rendered;
  }

  function ngResolveNodes(selectorTemplate, value, app, spec) {
    var selector = ngRenderTemplate(selectorTemplate, value, app, spec);
    try {
      return Array.prototype.slice.call(document.querySelectorAll(selector));
    } catch (error) {
      ngLog('invalid selector', selector, error && error.message ? error.message : error);
      return [];
    }
  }

  function ngApplyTemporaryClass(element, className, durationMs) {
    if (!element) {
      return;
    }
    element.classList.remove(className);
    void element.offsetWidth;
    element.classList.add(className);
    window.setTimeout(function () {
      element.classList.remove(className);
    }, durationMs || 420);
  }

  function ngEffectStateKey(spec, suffix) {
    return spec.name + '::' + suffix;
  }

  function ngHasChanged(app, spec, suffix, nextValue) {
    var key = ngEffectStateKey(spec, suffix);
    var previous = app.effectState[key];
    app.effectState[key] = nextValue;
    return previous !== nextValue;
  }

  function ngEffectConditionMet(current, step, app, spec, kindIndex, valueIndex, suffix) {
    var args = step.args || [];
    var conditionKind = args[kindIndex] || 'always';
    var conditionPath = args[valueIndex] || '';
    var observed = conditionPath ? ngExtractValue(current, conditionPath) : current;
    if (conditionKind === 'always') {
      return true;
    }
    if (conditionKind === 'truthy') {
      return !!observed;
    }
    if (conditionKind === 'changed') {
      return ngHasChanged(app, spec, suffix + ':' + conditionPath, observed);
    }
    ngLog('unknown effect condition', conditionKind, step.kind);
    return true;
  }

  function ngApplyEffectStep(current, step, app, spec) {
    var args = step.args || [];
    var nodes;
    var targetIndex;
    var durationMs;
    var staggerMs;
    var selector;
    var cssValue;
    if (step.kind === 'effect-class') {
      selector = args[0] || '';
      durationMs = Number(args[2] || 420);
      if (!ngEffectConditionMet(current, step, app, spec, 3, 4, step.kind + ':' + selector)) {
        return current;
      }
      nodes = ngResolveNodes(selector, current, app, spec);
      nodes.forEach(function (node) { ngApplyTemporaryClass(node, args[1], durationMs); });
      return current;
    }
    if (step.kind === 'effect-class-at') {
      selector = args[0] || '';
      targetIndex = Number(args[1] || 0);
      durationMs = Number(args[3] || 420);
      if (!ngEffectConditionMet(current, step, app, spec, 4, 5, step.kind + ':' + selector + ':' + targetIndex)) {
        return current;
      }
      nodes = ngResolveNodes(selector, current, app, spec);
      if (nodes[targetIndex]) {
        ngApplyTemporaryClass(nodes[targetIndex], args[2], durationMs);
      }
      return current;
    }
    if (step.kind === 'effect-stagger-class') {
      selector = args[0] || '';
      durationMs = Number(args[2] || 420);
      staggerMs = Number(args[3] || 24);
      nodes = ngResolveNodes(selector, current, app, spec);
      nodes.forEach(function (node, index) {
        window.setTimeout(function () {
          ngApplyTemporaryClass(node, args[1], durationMs);
        }, index * staggerMs);
      });
      return current;
    }
    if (step.kind === 'effect-style-var') {
      selector = args[0] || ':root';
      nodes = ngResolveNodes(selector, current, app, spec);
      if (!nodes.length) {
        return current;
      }
      if ((args[2] || 'value') === 'ternary-bool') {
        cssValue = ngExtractValue(current, args[3]) ? args[4] : args[5];
      } else if ((args[2] || 'value') === 'field') {
        cssValue = ngExtractValue(current, args[3]);
      } else {
        cssValue = args[2];
      }
      nodes.forEach(function (node) {
        node.style.setProperty(args[1], String(cssValue));
      });
      return current;
    }
    return current;
  }

  function ngApplyPipeChain(value, steps, app, spec) {
    return (steps || []).reduce(function (current, step) {
      var hook;
      ngLog('pipe', spec.name, step.kind, step.argument);
      if (step.kind === 'prop') {
        if (current && typeof current === 'object') {
          return current[step.argument];
        }
        return undefined;
      }
      if (step.kind === 'map') {
        hook = ngResolveHook(step.argument);
        if (!hook) {
          ngLog('missing map hook', step.argument);
          return current;
        }
        if (Array.isArray(current)) {
          return current.map(function (item, index) { return hook(item, app, index, spec); });
        }
        return hook(current, app, 0, spec);
      }
      if (step.kind === 'reduce') {
        hook = ngResolveHook(step.argument);
        if (!hook) {
          ngLog('missing reduce hook', step.argument);
          return current;
        }
        return hook(current, app, spec);
      }
      if (step.kind === 'tap') {
        hook = ngResolveHook(step.argument);
        if (hook) {
          hook(current, app, spec);
        } else {
          ngLog('missing tap hook', step.argument);
        }
        return current;
      }
      if (step.kind.indexOf('effect-') === 0) {
        return ngApplyEffectStep(current, step, app, spec);
      }
      return current;
    }, value);
  }

  function ngCreateApp() {
    var app = {
      streams: {},
      values: {},
      intervals: [],
      effectState: {}
    };

    app.getValue = function (name) {
      return app.values[name];
    };

    app.setValue = function (name, value) {
      ngLog('setValue', name, value);
      app.values[name] = value;
      return value;
    };

    app.applyObservableValue = function (spec, rawValue) {
      var processed = ngApplyPipeChain(rawValue, spec.steps, app, spec);
      var alias = ngNormalizeName(spec.name);
      ngLog('applyObservableValue', spec.name, alias, processed);
      app.values[spec.name] = processed;
      app.values[alias] = processed;
      return processed;
    };

    app.registerObservable = function (spec) {
      ngLog('registerObservable', spec.name, spec.kind, spec.path || spec.seed || '');
      app.streams[spec.name] = spec;
      if (spec.kind === 'state') {
        app.applyObservableValue(spec, spec.seed);
      }
    };

    app.setObservable = function (name, value) {
      var spec = app.streams[name];
      if (!spec || spec.kind !== 'state') {
        ngLog('setObservable ignored', name);
        return;
      }
      app.applyObservableValue(spec, value);
    };

    app.refreshObservable = function (name) {
      var spec = app.streams[name];
      if (!spec || spec.kind !== 'poll') {
        ngLog('refreshObservable ignored', name);
        return Promise.resolve(null);
      }
      return ngFetchJson(spec.path).then(function (payload) {
        return app.applyObservableValue(spec, payload);
      });
    };

    app.runObservable = function (name, payload) {
      var spec = app.streams[name];
      if (!spec || spec.kind !== 'post') {
        ngLog('runObservable ignored', name);
        return Promise.resolve(null);
      }
      return ngFetchJson(spec.path, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload || {})
      }).then(function (responsePayload) {
        return app.applyObservableValue(spec, responsePayload);
      });
    };

    app.start = function () {
      Object.keys(app.streams).forEach(function (name) {
        var spec = app.streams[name];
        if (spec.kind === 'poll') {
          app.refreshObservable(name);
          app.intervals.push(window.setInterval(function () {
            app.refreshObservable(name);
          }, spec.intervalMs));
        }
      });
    };

    return app;
  }
function setText(id, value) {
  var element = document.getElementById(id);
  if (element) {
    element.textContent = value;
  }
}

function setHtml(id, value) {
  var element = document.getElementById(id);
  if (element) {
    element.innerHTML = value;
  }
}

function setDemoCopy(text) {
  setText('meter-demo-copy', text);
}

function showView(view) {
  console.log('[app] showView', view);
  ['live', 'history', 'variables'].forEach(function (name) {
    var section = document.getElementById(name + '-view');
    if (section) {
      section.classList.toggle('hidden', name !== view);
    }
  });
  var tabs = document.getElementById('tabs');
  if (tabs) {
    Array.prototype.forEach.call(tabs.querySelectorAll('button[data-view]'), function (button) {
      button.classList.toggle('active', button.getAttribute('data-view') === view);
    });
  }
  window.history.replaceState({}, '', view === 'variables' ? '/variables' : '/');
}

function updateKnee(percentStraight) {
  var normalized = Math.max(0, Math.min(1, Number(percentStraight || 0) / 100));
  var angle = 78 + (0 - 78) * normalized;
  var radians = angle * Math.PI / 180;
  var strideLift = Math.sin(normalized * Math.PI) * 6.5;
  var shinLength = 100 - strideLift;
  var ankleX = 140 + Math.sin(radians) * shinLength;
  var ankleY = 174 + Math.cos(radians) * shinLength;
  var lowerLeg = document.getElementById('lower-leg');
  var lowerLegShadow = document.getElementById('lower-leg-shadow');
  var lowerLegHighlight = document.getElementById('lower-leg-highlight');
  var strideEcho = document.getElementById('stride-echo');
  var ankle = document.getElementById('ankle');
  var ankleGlow = document.getElementById('ankle-glow');
  var ankleRing = document.getElementById('ankle-ring');
  var previousAnkleX = ankle ? Number(ankle.getAttribute('cx') || 0) : ankleX;
  var previousAnkleY = ankle ? Number(ankle.getAttribute('cy') || 0) : ankleY;
  var movedDistance = Math.sqrt(Math.pow(ankleX - previousAnkleX, 2) + Math.pow(ankleY - previousAnkleY, 2));
  if (lowerLeg) {
    lowerLeg.setAttribute('x2', ankleX.toFixed(1));
    lowerLeg.setAttribute('y2', ankleY.toFixed(1));
  }
  if (lowerLegShadow) {
    lowerLegShadow.setAttribute('x2', ankleX.toFixed(1));
    lowerLegShadow.setAttribute('y2', ankleY.toFixed(1));
  }
  if (lowerLegHighlight) {
    lowerLegHighlight.setAttribute('x2', ankleX.toFixed(1));
    lowerLegHighlight.setAttribute('y2', ankleY.toFixed(1));
  }
  if (strideEcho) {
    strideEcho.setAttribute('x2', (ankleX - 10 + normalized * 20).toFixed(1));
    strideEcho.setAttribute('y2', (ankleY - 4).toFixed(1));
    strideEcho.style.opacity = String(0.18 + normalized * 0.42);
  }
  if (ankle) {
    ankle.setAttribute('cx', ankleX.toFixed(1));
    ankle.setAttribute('cy', ankleY.toFixed(1));
  }
  if (ankleGlow) {
    ankleGlow.setAttribute('cx', ankleX.toFixed(1));
    ankleGlow.setAttribute('cy', ankleY.toFixed(1));
  }
  if (ankleRing) {
    ankleRing.setAttribute('cx', ankleX.toFixed(1));
    ankleRing.setAttribute('cy', ankleY.toFixed(1));
    ankleRing.setAttribute('r', (18 + normalized * 8).toFixed(1));
    ankleRing.classList.remove('is-moving');
    if (movedDistance > 1.2) {
      void ankleRing.offsetWidth;
      ankleRing.classList.add('is-moving');
    }
  }
  var meter = document.getElementById('knee-meter');
  if (meter) {
    meter.style.transform = 'translate3d(' + ((normalized - 0.5) * 8).toFixed(1) + 'px, ' + (5 - normalized * 9).toFixed(1) + 'px, 0)';
  }
}

function scoreChip(score) {
  if (score < 55) { return 'score-chip bad'; }
  if (score < 75) { return 'score-chip warn'; }
  return 'score-chip';
}

function formatTime(timestampMs) {
  return new Date(timestampMs).toLocaleString();
}

function formatHistoryRow(item) {
  return '<tr><td>' + formatTime(item.timestampMs) + '</td><td><span class="' + scoreChip(item.score) + '">' + Number(item.score).toFixed(1) + '</span></td><td>' + Number(item.shakiness).toFixed(1) + '</td><td>' + Number(item.uncontrolledDescent).toFixed(1) + '</td><td>' + Number(item.compensation).toFixed(1) + '</td><td>' + Math.round(item.durationMs) + ' ms</td><td>' + Number(item.range).toFixed(1) + '%</td><td>' + Number(item.descentAvgSpeed).toFixed(1) + '</td><td>' + Number(item.ascentAvgSpeed).toFixed(1) + '</td><td>' + item.oscillations + '</td></tr>';
}

function formatDailyAverageRow(item, app) {
  var goal = app && typeof app.getValue === 'function' ? Number(app.getValue('historyGoal') || app.getValue('goal') || 0) : 0;
  return '<tr><td>' + formatTime(item.dayStartMs).split(',')[0] + '</td><td><span class="' + scoreChip(item.averageScore) + '">' + Number(item.averageScore).toFixed(1) + '</span></td><td>' + item.count + '</td><td>' + goal + '</td></tr>';
}

function renderHistoryRows(rows) {
  console.log('[app] renderHistoryRows count=' + ((rows && rows.length) || 0));
  setHtml('history-body', (rows || []).join('') || '<tr><td class="empty" colspan="10">No steps recorded yet.</td></tr>');
}

function renderDailyAverageRows(rows) {
  console.log('[app] renderDailyAverageRows count=' + ((rows && rows.length) || 0));
  setHtml('daily-average-body', (rows || []).join('') || '<tr><td class="empty" colspan="4">No synced history yet.</td></tr>');
}

function summarizeHistory(history) {
  var rows = Array.isArray(history) ? history : [];
  var totalScore = 0;
  var totalDuration = 0;
  var bestStep = null;
  var worstStep = null;

  rows.forEach(function (item) {
    totalScore += Number(item.score || 0);
    totalDuration += Number(item.durationMs || 0);
    if (!bestStep || Number(item.score || 0) > Number(bestStep.score || 0)) {
      bestStep = item;
    }
    if (!worstStep || Number(item.score || 0) < Number(worstStep.score || 0)) {
      worstStep = item;
    }
  });

  console.log('[app] summarizeHistory count=' + rows.length);
  return {
    count: rows.length,
    averageScore: rows.length ? totalScore / rows.length : 0,
    averageDurationMs: rows.length ? totalDuration / rows.length : 0,
    bestStep: bestStep,
    worstStep: worstStep
  };
}

function summarizeDailyAverages(days) {
  var rows = Array.isArray(days) ? days : [];
  var bestDay = null;
  var worstDay = null;

  rows.forEach(function (item) {
    if (!bestDay || Number(item.averageScore || 0) > Number(bestDay.averageScore || 0)) {
      bestDay = item;
    }
    if (!worstDay || Number(item.averageScore || 0) < Number(worstDay.averageScore || 0)) {
      worstDay = item;
    }
  });

  console.log('[app] summarizeDailyAverages count=' + rows.length);
  return {
    count: rows.length,
    bestDay: bestDay,
    worstDay: worstDay
  };
}

function renderHistoryDiagnostics(summary) {
  var bestStep = summary && summary.bestStep;
  var worstStep = summary && summary.worstStep;
  console.log('[app] renderHistoryDiagnostics count=' + (summary && summary.count));
  setText('best-step-score', bestStep ? Number(bestStep.score || 0).toFixed(1) : '0.0');
  setText('best-step-detail', bestStep ? formatTime(bestStep.timestampMs) + ' • ' + Math.round(bestStep.durationMs) + ' ms' : 'No steps yet');
  setText('worst-step-score', worstStep ? Number(worstStep.score || 0).toFixed(1) : '0.0');
  setText('worst-step-detail', worstStep ? formatTime(worstStep.timestampMs) + ' • ' + Math.round(worstStep.durationMs) + ' ms' : 'No steps yet');
  setText('average-step-score', summary ? Number(summary.averageScore || 0).toFixed(1) : '0.0');
  setText('average-step-detail', summary ? String(summary.count || 0) + ' logged steps' : 'Across all logged steps');
  setText('average-step-duration', summary ? Math.round(summary.averageDurationMs || 0) + ' ms' : '0 ms');
  setText('average-step-duration-detail', 'Across all logged steps');
}

function renderDailyDiagnostics(summary) {
  var bestDay = summary && summary.bestDay;
  var worstDay = summary && summary.worstDay;
  console.log('[app] renderDailyDiagnostics count=' + (summary && summary.count));
  setText('best-day-score', bestDay ? Number(bestDay.averageScore || 0).toFixed(1) : '0.0');
  setText('best-day-detail', bestDay ? formatTime(bestDay.dayStartMs).split(',')[0] + ' • ' + bestDay.count + ' steps' : 'No days yet');
  setText('worst-day-score', worstDay ? Number(worstDay.averageScore || 0).toFixed(1) : '0.0');
  setText('worst-day-detail', worstDay ? formatTime(worstDay.dayStartMs).split(',')[0] + ' • ' + worstDay.count + ' steps' : 'No days yet');
}

function applyHistoryGoal(goal, app) {
  console.log('[app] applyHistoryGoal', goal);
  if (app && typeof app.setValue === 'function') {
    app.setValue('historyGoal', Number(goal || 0));
  }
}

function applyLivePayload(payload, app) {
  var nextPercent = Number(payload.percentStraight || 0);
  console.log('[app] applyLivePayload reading=' + payload.reading + ' percent=' + payload.percentStraight);
  if (app && typeof app.setValue === 'function') {
    app.setValue('goal', payload.goal || 0);
  }
  setText('reading', String(payload.reading));
  setText('percent-straight', Number(payload.percentStraight || 0).toFixed(1) + '%');
  setText('last-score', Number(payload.lastScore || 0).toFixed(1));
  setText('step-count', String(payload.stepCount || 0));
  setText('today-average', Number(payload.todayAverage || 0).toFixed(1));
  setText('goal-inline', String(payload.goal || 0));
  setText('speed', Number(payload.speed || 0).toFixed(1));
  setText('live-shaky', Number(payload.shaky || 0).toFixed(1));
  setText('live-descent', Number(payload.uncontrolledDescent || 0).toFixed(1));
  setText('live-compensation', Number(payload.compensation || 0).toFixed(1));
  updateKnee(nextPercent);
  var syncPill = document.getElementById('sync-pill');
  if (syncPill) {
    syncPill.textContent = payload.timeSynced ? 'Phone time synced for daily tracking' : 'Phone time not synced yet';
    syncPill.classList.toggle('warn', !payload.timeSynced);
  }
  var statusPill = document.getElementById('step-status');
  if (statusPill) {
    statusPill.textContent = payload.inStep ? 'Step in progress with live observable updates' : 'Waiting for a full movement cycle';
    statusPill.classList.toggle('warn', !payload.inStep);
  }
  var signalCopy = document.getElementById('hero-signal-copy');
  if (signalCopy) {
    signalCopy.textContent = payload.inStep ? 'Live observable pulse detecting active movement' : 'Observable live telemetry active';
  }
  setDemoCopy(payload.inStep ? 'Tracking real movement from the live sensor' : 'Waiting for live knee movement');
}

function applyVariablesPayload(payload) {
  console.log('[app] applyVariablesPayload keys=' + Object.keys(payload).join(','));
  Object.keys(payload).forEach(function (key) {
    var input = document.getElementById(key);
    if (input) {
      input.value = payload[key];
    }
  });
  setText('variables-status', payload.status || 'Variables loaded');
}

function applyVariablesSaveResponse(payload) {
  console.log('[app] applyVariablesSaveResponse', payload && payload.status);
  setText('variables-status', payload.status || 'Variables saved');
}

function collectVariablesPayload() {
  var payload = {};
  Array.prototype.forEach.call(document.querySelectorAll('#variables-view input'), function (input) {
    payload[input.name] = input.value;
  });
  return payload;
}

function ngInitializeApp(app) {
  console.log('[app] ngInitializeApp');
  var tabs = document.getElementById('tabs');
  if (tabs) {
    tabs.addEventListener('click', function (event) {
      var button = event.target.closest('button[data-view]');
      if (button) {
        app.setObservable('selectedView$', button.getAttribute('data-view'));
      }
    });
  }

  var saveButton = document.getElementById('save-variables');
  if (saveButton) {
    saveButton.addEventListener('click', function () {
      app.runObservable('saveVariables$', collectVariablesPayload());
    });
  }

  var reloadButton = document.getElementById('reload-variables');
  if (reloadButton) {
    reloadButton.addEventListener('click', function () {
      app.refreshObservable('variablesState$');
    });
  }

  app.setObservable('selectedView$', window.location.pathname === '/variables' ? 'variables' : 'live');
  setDemoCopy('Waiting for live knee movement');
}

window.setText = setText;
window.setHtml = setHtml;
window.showView = showView;
window.updateKnee = updateKnee;
window.scoreChip = scoreChip;
window.formatTime = formatTime;
window.formatHistoryRow = formatHistoryRow;
window.formatDailyAverageRow = formatDailyAverageRow;
window.renderHistoryRows = renderHistoryRows;
window.renderDailyAverageRows = renderDailyAverageRows;
window.summarizeHistory = summarizeHistory;
window.summarizeDailyAverages = summarizeDailyAverages;
window.renderHistoryDiagnostics = renderHistoryDiagnostics;
window.renderDailyDiagnostics = renderDailyDiagnostics;
window.applyHistoryGoal = applyHistoryGoal;
window.applyLivePayload = applyLivePayload;
window.applyVariablesPayload = applyVariablesPayload;
window.applyVariablesSaveResponse = applyVariablesSaveResponse;
window.collectVariablesPayload = collectVariablesPayload;
window.ngInitializeApp = ngInitializeApp;


  var ngApp = ngCreateApp();
  window.ngApp = ngApp;
  ngApp.registerObservable({ name: 'selectedView$', alias: 'selectedView', kind: 'state', path: '', seed: 'live', intervalMs: 0, steps: [{ kind: 'tap', argument: 'showView', args: ['showView'] }, { kind: 'effect-class', argument: 'is-transitioning', args: ['#{{value}}-view', 'is-transitioning', '460'] }] });
  ngApp.registerObservable({ name: 'liveState$', alias: 'liveState', kind: 'poll', path: '/api/live', seed: null, intervalMs: 500, steps: [{ kind: 'tap', argument: 'applyLivePayload', args: ['applyLivePayload'] }, { kind: 'effect-class-at', argument: '0', args: ['[data-telemetry-card]', '0', 'is-live', '1000', 'changed', 'reading'] }, { kind: 'effect-class-at', argument: '1', args: ['[data-telemetry-card]', '1', 'is-live', '1000', 'changed', 'percentStraight'] }, { kind: 'effect-class-at', argument: '2', args: ['[data-telemetry-card]', '2', 'is-live', '1000', 'changed', 'lastScore'] }, { kind: 'effect-class-at', argument: '3', args: ['[data-telemetry-card]', '3', 'is-live', '1000', 'changed', 'stepCount'] }, { kind: 'effect-class', argument: 'is-energized', args: ['.meter-card', 'is-energized', '360', 'changed', 'percentStraight'] }, { kind: 'effect-style-var', argument: '--surface-shift', args: [':root', '--surface-shift', 'ternary-bool', 'inStep', '-2px', '0px'] }] });
  ngApp.registerObservable({ name: 'historyRows$', alias: 'historyRows', kind: 'poll', path: '/api/history', seed: null, intervalMs: 3000, steps: [{ kind: 'prop', argument: 'history', args: ['history'] }, { kind: 'map', argument: 'formatHistoryRow', args: ['formatHistoryRow'] }, { kind: 'tap', argument: 'renderHistoryRows', args: ['renderHistoryRows'] }, { kind: 'effect-stagger-class', argument: 'table-row-enter', args: ['#history-body tr', 'table-row-enter', '420', '16'] }] });
  ngApp.registerObservable({ name: 'dailyAverageRows$', alias: 'dailyAverageRows', kind: 'poll', path: '/api/history', seed: null, intervalMs: 3000, steps: [{ kind: 'prop', argument: 'dailyAverages', args: ['dailyAverages'] }, { kind: 'map', argument: 'formatDailyAverageRow', args: ['formatDailyAverageRow'] }, { kind: 'tap', argument: 'renderDailyAverageRows', args: ['renderDailyAverageRows'] }, { kind: 'effect-stagger-class', argument: 'table-row-enter', args: ['#daily-average-body tr', 'table-row-enter', '420', '24'] }] });
  ngApp.registerObservable({ name: 'historyGoal$', alias: 'historyGoal', kind: 'poll', path: '/api/history', seed: null, intervalMs: 3000, steps: [{ kind: 'prop', argument: 'goal', args: ['goal'] }, { kind: 'tap', argument: 'applyHistoryGoal', args: ['applyHistoryGoal'] }] });
  ngApp.registerObservable({ name: 'historyDiagnostics$', alias: 'historyDiagnostics', kind: 'poll', path: '/api/history', seed: null, intervalMs: 3000, steps: [{ kind: 'prop', argument: 'history', args: ['history'] }, { kind: 'reduce', argument: 'summarizeHistory', args: ['summarizeHistory'] }, { kind: 'tap', argument: 'renderHistoryDiagnostics', args: ['renderHistoryDiagnostics'] }] });
  ngApp.registerObservable({ name: 'dailyDiagnostics$', alias: 'dailyDiagnostics', kind: 'poll', path: '/api/history', seed: null, intervalMs: 3000, steps: [{ kind: 'prop', argument: 'dailyAverages', args: ['dailyAverages'] }, { kind: 'reduce', argument: 'summarizeDailyAverages', args: ['summarizeDailyAverages'] }, { kind: 'tap', argument: 'renderDailyDiagnostics', args: ['renderDailyDiagnostics'] }] });
  ngApp.registerObservable({ name: 'variablesState$', alias: 'variablesState', kind: 'poll', path: '/api/variables', seed: null, intervalMs: 8000, steps: [{ kind: 'tap', argument: 'applyVariablesPayload', args: ['applyVariablesPayload'] }] });
  ngApp.registerObservable({ name: 'saveVariables$', alias: 'saveVariables', kind: 'post', path: '/api/variables', seed: null, intervalMs: 0, steps: [{ kind: 'tap', argument: 'applyVariablesSaveResponse', args: ['applyVariablesSaveResponse'] }, { kind: 'effect-class', argument: 'warn', args: ['#variables-status', 'warn', '520'] }] });
  if (typeof window.ngInitializeApp === 'function') {
    window.ngInitializeApp(ngApp);
  }
  ngApp.start();
}());

)JS_ASSET";

