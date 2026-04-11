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
  .card-grid, .quality-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); }
}
@media (max-width: 640px) {
  .shell {
    width: min(100vw - 12px, 100%);
    margin: 6px auto;
    padding: 14px;
    border-radius: 20px;
  }
  .card-grid, .quality-grid { grid-template-columns: 1fr; }
  .headline { font-size: 2.7rem; }
  .status-row { align-items: stretch; }
}

)CSS_ASSET";

const char APP_JS[] PROGMEM = R"JS_ASSET(
(function () {
  function fetchJson(path, options) {
    return fetch(path, options).then(function (response) { return response.json(); });
  }

  function setText(id, value) {
    var element = document.getElementById(id);
    if (element) {
      element.textContent = value;
    }
  }

  function showView(view) {
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

  function applyLive(payload) {
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
  }

  function applyHistory(payload) {
    var historyBody = document.getElementById('history-body');
    var dailyBody = document.getElementById('daily-average-body');
    if (dailyBody) {
      dailyBody.innerHTML = (payload.dailyAverages || []).map(function (item) {
        return '<tr><td>' + formatTime(item.dayStartMs).split(',')[0] + '</td><td><span class="' + scoreChip(item.averageScore) + '">' + Number(item.averageScore).toFixed(1) + '</span></td><td>' + item.count + '</td><td>' + payload.goal + '</td></tr>';
      }).join('') || '<tr><td class="empty" colspan="4">No synced history yet.</td></tr>';
    }
    if (historyBody) {
      historyBody.innerHTML = (payload.history || []).map(function (item) {
        return '<tr><td>' + formatTime(item.timestampMs) + '</td><td><span class="' + scoreChip(item.score) + '">' + Number(item.score).toFixed(1) + '</span></td><td>' + Number(item.shakiness).toFixed(1) + '</td><td>' + Number(item.uncontrolledDescent).toFixed(1) + '</td><td>' + Number(item.compensation).toFixed(1) + '</td><td>' + Math.round(item.durationMs) + ' ms</td><td>' + Number(item.range).toFixed(1) + '%</td><td>' + Number(item.descentAvgSpeed).toFixed(1) + '</td><td>' + Number(item.ascentAvgSpeed).toFixed(1) + '</td><td>' + item.oscillations + '</td></tr>';
      }).join('') || '<tr><td class="empty" colspan="10">No steps recorded yet.</td></tr>';
    }
  }

  function applyVariables(payload) {
    Object.keys(payload).forEach(function (key) {
      var input = document.getElementById(key);
      if (input) {
        input.value = payload[key];
      }
    });
    setText('variables-status', payload.status || 'Variables loaded');
  }

  function refreshAll() {
    return Promise.all([
      fetchJson('/api/live').then(applyLive),
      fetchJson('/api/history').then(applyHistory),
      fetchJson('/api/variables').then(applyVariables)
    ]);
  }

  document.getElementById('tabs').addEventListener('click', function (event) {
    var button = event.target.closest('button[data-view]');
    if (button) {
      showView(button.getAttribute('data-view'));
    }
  });

  document.getElementById('save-variables').addEventListener('click', function () {
    var form = new URLSearchParams();
    Array.prototype.forEach.call(document.querySelectorAll('#variables-view input'), function (input) {
      form.append(input.name, input.value);
    });
    fetchJson('/api/variables', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: form.toString()
    }).then(function (result) {
      setText('variables-status', result.status || 'Variables saved');
    });
  });

  document.getElementById('reload-variables').addEventListener('click', function () {
    fetchJson('/api/variables').then(applyVariables);
  });

  showView(window.location.pathname === '/variables' ? 'variables' : 'live');
  refreshAll();
  window.setInterval(function () { fetchJson('/api/live').then(applyLive); }, 500);
  window.setInterval(function () { fetchJson('/api/history').then(applyHistory); }, 3000);
}());

)JS_ASSET";

