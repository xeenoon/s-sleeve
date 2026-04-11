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
    <section class="hero">
      <div class="panel">
        <p class="eyebrow">Rehab Tracker</p>
        <h1 class="headline"><span id="today-average">0.0</span>/100</h1>
        <p class="subline">Daily average step quality with a goal of <strong id="goal-inline">85</strong>.</p>
      </div>
      <div class="panel">
        <div class="status-row">
          <div class="tabs" id="tabs">
            <button data-view="live" class="active" type="button">Live</button>
            <button data-view="history" type="button">History</button>
            <button data-view="variables" type="button">Variables</button>
          </div>
          <div id="sync-pill" class="status-pill">Phone time synced for daily tracking</div>
        </div>
        <div class="card-grid">
          <div class="metric-card">
            <div class="metric-label">Current Reading</div>
            <div class="metric-value" id="reading">0</div>
            <div class="metric-detail">Raw potentiometer value</div>
          </div>
          <div class="metric-card">
            <div class="metric-label">Straightness</div>
            <div class="metric-value" id="percent-straight">0%</div>
            <div class="metric-detail">Mapped knee position</div>
          </div>
          <div class="metric-card">
            <div class="metric-label">Step Score</div>
            <div class="metric-value" id="last-score">0.0</div>
            <div class="metric-detail">Most recent completed step</div>
          </div>
          <div class="metric-card">
            <div class="metric-label">Steps Logged</div>
            <div class="metric-value" id="step-count">0</div>
            <div class="metric-detail">Stored in device history</div>
          </div>
        </div>
      </div>
    </section>

    <section id="live-view">
      <div class="live-layout">
        <div class="panel meter-card">
          <p class="eyebrow">Knee Position</p>
          <svg viewBox="0 0 280 320" aria-label="Knee joint animation">
            <line class="guide" x1="140" y1="64" x2="140" y2="174"></line>
            <line class="limb" x1="140" y1="64" x2="140" y2="174"></line>
            <line class="limb" id="lower-leg" x1="140" y1="174" x2="140.0" y2="274.0"></line>
            <circle class="joint" cx="140" cy="64" r="14"></circle>
            <circle class="joint" cx="140" cy="174" r="16"></circle>
            <circle class="joint" id="ankle" cx="140.0" cy="274.0" r="14"></circle>
          </svg>
          <div class="joint-labels">
            <span>Bent</span>
            <span>Straight</span>
          </div>
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
            <div class="quality-card">
              <div class="metric-label">Shaky Movement</div>
              <strong id="live-shaky">0.0</strong>
              <span>From reversals and extra path during a step</span>
            </div>
            <div class="quality-card">
              <div class="metric-label">Uncontrolled Descent</div>
              <strong id="live-descent">0.0</strong>
              <span>Penalizes overly fast or spiky lowering</span>
            </div>
            <div class="quality-card">
              <div class="metric-label">Compensation</div>
              <strong id="live-compensation">0.0</strong>
              <span>Flags phase imbalance and incomplete return</span>
            </div>
          </div>
        </div>
      </div>
    </section>

    <section id="history-view" class="hidden">
      <div class="history-layout">
        <div class="panel diagnostics-panel">
          <p class="eyebrow">History Diagnostics</p>
          <div class="diagnostics-grid">
            <div class="diagnostic-card">
              <div class="metric-label">Best Step</div>
              <strong id="best-step-score">0.0</strong>
              <span id="best-step-detail">No steps yet</span>
            </div>
            <div class="diagnostic-card">
              <div class="metric-label">Worst Step</div>
              <strong id="worst-step-score">0.0</strong>
              <span id="worst-step-detail">No steps yet</span>
            </div>
            <div class="diagnostic-card">
              <div class="metric-label">Best Day</div>
              <strong id="best-day-score">0.0</strong>
              <span id="best-day-detail">No days yet</span>
            </div>
            <div class="diagnostic-card">
              <div class="metric-label">Worst Day</div>
              <strong id="worst-day-score">0.0</strong>
              <span id="worst-day-detail">No days yet</span>
            </div>
            <div class="diagnostic-card">
              <div class="metric-label">Average Step Score</div>
              <strong id="average-step-score">0.0</strong>
              <span id="average-step-detail">Across all logged steps</span>
            </div>
            <div class="diagnostic-card">
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
        <div class="table-panel">
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

    <section id="variables-view" class="hidden">
      <div class="panel">
        <p class="eyebrow">Calibration Variables</p>
        <div class="hint">Open this directly at <strong>/variables</strong>. Saving applies immediately to live tracking.</div>
        <div id="variables-status" class="status-pill warn" style="margin-top:14px;">Variables not loaded yet</div>
        <div class="variables-grid">
          <div>
            <div class="field"><label for="minReading">Min Reading</label><input id="minReading" name="minReading" type="number"></div>
            <div class="field"><label for="maxReading">Max Reading</label><input id="maxReading" name="maxReading" type="number"></div>
            <div class="field"><label for="bentAngle">Bent Angle</label><input id="bentAngle" name="bentAngle" type="number" step="0.1"></div>
            <div class="field"><label for="straightAngle">Straight Angle</label><input id="straightAngle" name="straightAngle" type="number" step="0.1"></div>
            <div class="field"><label for="sampleIntervalMs">Sample Interval Ms</label><input id="sampleIntervalMs" name="sampleIntervalMs" type="number"></div>
            <div class="field"><label for="filterAlpha">Filter Alpha</label><input id="filterAlpha" name="filterAlpha" type="number" step="0.001"></div>
          </div>
          <div>
            <div class="field"><label for="motionThreshold">Motion Threshold</label><input id="motionThreshold" name="motionThreshold" type="number" step="0.001"></div>
            <div class="field"><label for="stepRangeThreshold">Step Range Threshold</label><input id="stepRangeThreshold" name="stepRangeThreshold" type="number" step="0.001"></div>
            <div class="field"><label for="startReadyThreshold">Start Ready Threshold</label><input id="startReadyThreshold" name="startReadyThreshold" type="number" step="0.001"></div>
            <div class="field"><label for="returnMargin">Return Margin</label><input id="returnMargin" name="returnMargin" type="number" step="0.001"></div>
            <div class="field"><label for="maxStepDurationMs">Max Step Duration Ms</label><input id="maxStepDurationMs" name="maxStepDurationMs" type="number"></div>
            <div class="field"><label for="sampleHistoryEnabled">Sample History</label><input id="sampleHistoryEnabled" name="sampleHistoryEnabled" type="text"></div>
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
  width: min(1180px, calc(100vw - 24px));
  margin: 20px auto;
  padding: 20px;
  border-radius: 28px;
  background: var(--paper);
  border: 1px solid var(--line);
  box-shadow: var(--shadow);
  backdrop-filter: blur(10px);
}

.hero, .card-grid, .history-layout, .variables-grid { display: grid; gap: 18px; }
.hero { grid-template-columns: minmax(320px, 1.2fr) minmax(260px, 0.8fr); align-items: stretch; }
.panel, .metric-card, .table-panel {
  border-radius: 24px;
  border: 1px solid var(--line);
  background: #fffaf4;
}
.panel { padding: 22px; }
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
}
.subline { margin: 10px 0 0; color: var(--muted); font-size: 1rem; }
.card-grid { grid-template-columns: repeat(4, minmax(0, 1fr)); margin-top: 18px; }
.metric-card { padding: 18px; }
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
}
.metric-detail, .hint { margin-top: 8px; color: var(--muted); font-size: 0.95rem; }
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
}
.tabs button.active { background: var(--accent); color: #fff8f4; }
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
}
.status-pill.warn { background: rgba(184, 95, 24, 0.12); color: var(--warn); }
.hidden { display: none !important; }
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
.limb { fill: none; stroke: var(--accent); stroke-width: 22; stroke-linecap: round; }
.guide {
  fill: none;
  stroke: rgba(34, 25, 18, 0.12);
  stroke-width: 12;
  stroke-linecap: round;
  stroke-dasharray: 10 14;
}
.joint { fill: var(--text); }
.quality-grid { grid-template-columns: repeat(3, minmax(0, 1fr)); }
.quality-card {
  padding: 18px;
  border-radius: 20px;
  border: 1px solid var(--line);
  background: rgba(255, 248, 241, 0.86);
}
.quality-card strong, .quality-card span { display: block; }
.quality-card strong { font-size: 1.7rem; margin: 10px 0 6px; }
.quality-card span { color: var(--muted); font-size: 0.92rem; }
.history-layout { grid-template-columns: minmax(0, 1fr); margin-top: 18px; }
.table-panel { padding: 18px; overflow: hidden; }
.table-wrap { overflow: auto; border-radius: 18px; border: 1px solid var(--line); }
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
  padding: 18px;
  border-radius: 18px;
  background: rgba(255, 250, 245, 0.92);
  border: 1px solid rgba(63, 46, 32, 0.08);
  box-shadow: inset 0 1px 0 rgba(255,255,255,0.7);
  display: grid;
  gap: 6px;
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
}
.actions button.secondary {
  background: rgba(63, 46, 32, 0.12);
  color: var(--text);
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
          return current.map(function (item, itemIndex) { return hook(item, app, itemIndex, spec); });
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
      return current;
    }, value);
  }

  function ngCreateApp() {
    var app = {
      streams: {},
      values: {},
      intervals: []
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
  var ankleX = 140 + Math.sin(radians) * 100;
  var ankleY = 174 + Math.cos(radians) * 100;
  var lowerLeg = document.getElementById('lower-leg');
  var ankle = document.getElementById('ankle');
  if (lowerLeg) {
    lowerLeg.setAttribute('x2', ankleX.toFixed(1));
    lowerLeg.setAttribute('y2', ankleY.toFixed(1));
  }
  if (ankle) {
    ankle.setAttribute('cx', ankleX.toFixed(1));
    ankle.setAttribute('cy', ankleY.toFixed(1));
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
  updateKnee(payload.percentStraight || 0);
  var syncPill = document.getElementById('sync-pill');
  if (syncPill) {
    syncPill.textContent = payload.timeSynced ? 'Phone time synced for daily tracking' : 'Phone time not synced yet';
    syncPill.classList.toggle('warn', !payload.timeSynced);
  }
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
  ngApp.registerObservable({ name: 'selectedView$', alias: 'selectedView', kind: 'state', path: '', seed: 'live', intervalMs: 0, steps: [{ kind: 'tap', argument: 'showView' }] });
  ngApp.registerObservable({ name: 'liveState$', alias: 'liveState', kind: 'poll', path: '/api/live', seed: null, intervalMs: 500, steps: [{ kind: 'tap', argument: 'applyLivePayload' }] });
  ngApp.registerObservable({ name: 'historyRows$', alias: 'historyRows', kind: 'poll', path: '/api/history', seed: null, intervalMs: 3000, steps: [{ kind: 'prop', argument: 'history' }, { kind: 'map', argument: 'formatHistoryRow' }, { kind: 'tap', argument: 'renderHistoryRows' }] });
  ngApp.registerObservable({ name: 'dailyAverageRows$', alias: 'dailyAverageRows', kind: 'poll', path: '/api/history', seed: null, intervalMs: 3000, steps: [{ kind: 'prop', argument: 'dailyAverages' }, { kind: 'map', argument: 'formatDailyAverageRow' }, { kind: 'tap', argument: 'renderDailyAverageRows' }] });
  ngApp.registerObservable({ name: 'historyGoal$', alias: 'historyGoal', kind: 'poll', path: '/api/history', seed: null, intervalMs: 3000, steps: [{ kind: 'prop', argument: 'goal' }, { kind: 'tap', argument: 'applyHistoryGoal' }] });
  ngApp.registerObservable({ name: 'historyDiagnostics$', alias: 'historyDiagnostics', kind: 'poll', path: '/api/history', seed: null, intervalMs: 3000, steps: [{ kind: 'prop', argument: 'history' }, { kind: 'reduce', argument: 'summarizeHistory' }, { kind: 'tap', argument: 'renderHistoryDiagnostics' }] });
  ngApp.registerObservable({ name: 'dailyDiagnostics$', alias: 'dailyDiagnostics', kind: 'poll', path: '/api/history', seed: null, intervalMs: 3000, steps: [{ kind: 'prop', argument: 'dailyAverages' }, { kind: 'reduce', argument: 'summarizeDailyAverages' }, { kind: 'tap', argument: 'renderDailyDiagnostics' }] });
  ngApp.registerObservable({ name: 'variablesState$', alias: 'variablesState', kind: 'poll', path: '/api/variables', seed: null, intervalMs: 8000, steps: [{ kind: 'tap', argument: 'applyVariablesPayload' }] });
  ngApp.registerObservable({ name: 'saveVariables$', alias: 'saveVariables', kind: 'post', path: '/api/variables', seed: null, intervalMs: 0, steps: [{ kind: 'tap', argument: 'applyVariablesSaveResponse' }] });
  if (typeof window.ngInitializeApp === 'function') {
    window.ngInitializeApp(ngApp);
  }
  ngApp.start();
}());

)JS_ASSET";

