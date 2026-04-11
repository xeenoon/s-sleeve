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
  class RehabTrackerApp {
    constructor() {
      this.state = {
        selectedView: window.location.pathname === '/variables' ? 'variables' : 'live',
        goal: 85,
        live: { reading: 0, percentStraight: 0, speed: 0, inStep: false, stepCount: 0, lastScore: 0, todayAverage: 0, timeSynced: false },
        history: [],
        dailyAverages: [],
        latestStep: null,
        variables: {}
      };
      this.refs = {
        tabs: document.getElementById('tabs'),
        liveView: document.getElementById('live-view'),
        historyView: document.getElementById('history-view'),
        variablesView: document.getElementById('variables-view'),
        reading: document.getElementById('reading'),
        percentStraight: document.getElementById('percent-straight'),
        lastScore: document.getElementById('last-score'),
        stepCount: document.getElementById('step-count'),
        todayAverage: document.getElementById('today-average'),
        goalInline: document.getElementById('goal-inline'),
        speed: document.getElementById('speed'),
        syncPill: document.getElementById('sync-pill'),
        stepStatus: document.getElementById('step-status'),
        liveShaky: document.getElementById('live-shaky'),
        liveDescent: document.getElementById('live-descent'),
        liveCompensation: document.getElementById('live-compensation'),
        lowerLeg: document.getElementById('lower-leg'),
        ankle: document.getElementById('ankle'),
        historyBody: document.getElementById('history-body'),
        dailyAverageBody: document.getElementById('daily-average-body'),
        variablesStatus: document.getElementById('variables-status'),
        saveVariables: document.getElementById('save-variables'),
        reloadVariables: document.getElementById('reload-variables'),
        variableInputs: Array.from(document.querySelectorAll('#variables-view input'))
      };
    }
    async init() {
      this.attachEvents();
      this.renderView();
      await this.syncClock();
      await Promise.all([this.refreshLive(), this.refreshHistory(), this.refreshVariables()]);
      window.setInterval(() => this.refreshLive(), 250);
      window.setInterval(() => this.refreshHistory(), 3000);
    }
    attachEvents() {
      if (this.refs.tabs) {
        this.refs.tabs.addEventListener('click', (event) => {
          const button = event.target.closest('button[data-view]');
          if (!button) { return; }
          this.switchView(button.dataset.view);
        });
      }
      if (this.refs.saveVariables) {
        this.refs.saveVariables.addEventListener('click', () => this.saveVariables());
      }
      if (this.refs.reloadVariables) {
        this.refs.reloadVariables.addEventListener('click', () => this.refreshVariables());
      }
    }
    switchView(view) {
      this.state.selectedView = view;
      this.renderView();
      window.history.replaceState({}, '', view === 'variables' ? '/variables' : '/');
    }
    renderView() {
      const view = this.state.selectedView;
      if (this.refs.liveView) { this.refs.liveView.classList.toggle('hidden', view !== 'live'); }
      if (this.refs.historyView) { this.refs.historyView.classList.toggle('hidden', view !== 'history'); }
      if (this.refs.variablesView) { this.refs.variablesView.classList.toggle('hidden', view !== 'variables'); }
      if (this.refs.tabs) {
        Array.from(this.refs.tabs.querySelectorAll('button[data-view]')).forEach((button) => {
          button.classList.toggle('active', button.dataset.view === view);
        });
      }
    }
    async syncClock() {
      const epochMs = Date.now();
      const offsetMinutes = new Date().getTimezoneOffset();
      try {
        await fetch('/api/time-sync?epochMs=' + epochMs + '&offsetMinutes=' + offsetMinutes);
      } catch (error) {}
    }
    clamp(value, min, max) {
      return Math.min(max, Math.max(min, value));
    }
    mapPercentToAngle(percentStraight) {
      const normalized = this.clamp(percentStraight / 100, 0, 1);
      return 78 + (0 - 78) * normalized;
    }
    updateKnee(percentStraight) {
      const angle = this.mapPercentToAngle(percentStraight);
      const radians = angle * Math.PI / 180;
      const kneeX = 140;
      const kneeY = 174;
      const shinLength = 100;
      const ankleX = kneeX + Math.sin(radians) * shinLength;
      const ankleY = kneeY + Math.cos(radians) * shinLength;
      if (this.refs.lowerLeg) {
        this.refs.lowerLeg.setAttribute('x2', ankleX.toFixed(1));
        this.refs.lowerLeg.setAttribute('y2', ankleY.toFixed(1));
      }
      if (this.refs.ankle) {
        this.refs.ankle.setAttribute('cx', ankleX.toFixed(1));
        this.refs.ankle.setAttribute('cy', ankleY.toFixed(1));
      }
    }
    formatTimestamp(timestampMs) {
      if (!timestampMs) { return 'Unsynced'; }
      return new Date(timestampMs).toLocaleString();
    }
    scoreChipClass(score) {
      if (score < 55) { return 'score-chip bad'; }
      if (score < 75) { return 'score-chip warn'; }
      return 'score-chip';
    }
    async refreshLive() {
      try {
        const response = await fetch('/api/live');
        const payload = await response.json();
        this.state.live = payload;
        this.state.goal = payload.goal;
        this.renderLive();
      } catch (error) {
        if (this.refs.syncPill) {
          this.refs.syncPill.textContent = 'Live signal unavailable';
          this.refs.syncPill.className = 'status-pill warn';
        }
      }
    }
    async refreshHistory() {
      try {
        const response = await fetch('/api/history');
        const payload = await response.json();
        this.state.history = payload.history || [];
        this.state.dailyAverages = payload.dailyAverages || [];
        this.state.goal = payload.goal || this.state.goal;
        this.state.latestStep = this.state.history.length > 0 ? this.state.history[0] : null;
        this.renderHistory();
        this.renderLiveQuality();
      } catch (error) {}
    }
    async refreshVariables() {
      try {
        const response = await fetch('/api/variables');
        const payload = await response.json();
        this.state.variables = payload;
        this.renderVariables();
        if (this.refs.variablesStatus) {
          this.refs.variablesStatus.textContent = 'Variables loaded';
          this.refs.variablesStatus.className = 'status-pill';
        }
      } catch (error) {
        if (this.refs.variablesStatus) {
          this.refs.variablesStatus.textContent = 'Could not load variables';
          this.refs.variablesStatus.className = 'status-pill warn';
        }
      }
    }
    async saveVariables() {
      const form = new URLSearchParams();
      this.refs.variableInputs.forEach((input) => {
        if (!input.disabled) { form.append(input.name, input.value); }
      });
      try {
        const response = await fetch('/api/variables', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: form.toString()
        });
        if (!response.ok) { throw new Error('save failed'); }
        const payload = await response.json();
        this.state.variables = payload;
        this.renderVariables();
        if (this.refs.variablesStatus) {
          this.refs.variablesStatus.textContent = 'Variables applied live';
          this.refs.variablesStatus.className = 'status-pill';
        }
        await this.refreshLive();
      } catch (error) {
        if (this.refs.variablesStatus) {
          this.refs.variablesStatus.textContent = 'Save failed. Check variable values.';
          this.refs.variablesStatus.className = 'status-pill warn';
        }
      }
    }
    renderVariables() {
      Object.entries(this.state.variables).forEach(([key, value]) => {
        const input = document.getElementById(key);
        if (input) { input.value = value; }
      });
    }
    renderLive() {
      const live = this.state.live;
      if (this.refs.reading) { this.refs.reading.textContent = String(live.reading); }
      if (this.refs.percentStraight) { this.refs.percentStraight.textContent = live.percentStraight.toFixed(1) + '%'; }
      if (this.refs.lastScore) { this.refs.lastScore.textContent = live.lastScore.toFixed(1); }
      if (this.refs.stepCount) { this.refs.stepCount.textContent = String(live.stepCount); }
      if (this.refs.todayAverage) { this.refs.todayAverage.textContent = live.todayAverage.toFixed(1); }
      if (this.refs.goalInline) { this.refs.goalInline.textContent = String(this.state.goal); }
      if (this.refs.speed) { this.refs.speed.textContent = live.speed.toFixed(1); }
      if (this.refs.stepStatus) {
        this.refs.stepStatus.textContent = live.inStep ? 'Capturing active movement cycle' : 'Waiting for a full movement cycle';
        this.refs.stepStatus.className = live.inStep ? 'status-pill' : 'status-pill warn';
      }
      if (this.refs.syncPill) {
        this.refs.syncPill.textContent = live.timeSynced ? 'Phone time synced for daily tracking' : 'Waiting for phone time sync';
        this.refs.syncPill.className = live.timeSynced ? 'status-pill' : 'status-pill warn';
      }
      this.updateKnee(live.percentStraight);
      this.renderLiveQuality();
    }
    renderLiveQuality() {
      const latest = this.state.latestStep;
      if (!latest) {
        if (this.refs.liveShaky) { this.refs.liveShaky.textContent = '0.0'; }
        if (this.refs.liveDescent) { this.refs.liveDescent.textContent = '0.0'; }
        if (this.refs.liveCompensation) { this.refs.liveCompensation.textContent = '0.0'; }
        return;
      }
      if (this.refs.liveShaky) { this.refs.liveShaky.textContent = latest.shakiness.toFixed(1); }
      if (this.refs.liveDescent) { this.refs.liveDescent.textContent = latest.uncontrolledDescent.toFixed(1); }
      if (this.refs.liveCompensation) { this.refs.liveCompensation.textContent = latest.compensation.toFixed(1); }
    }
    renderHistory() {
      const rows = this.state.history;
      const daily = this.state.dailyAverages;
      if (this.refs.historyBody) {
        if (!rows.length) {
          this.refs.historyBody.innerHTML = '<tr><td class="empty" colspan="10">No steps recorded yet.</td></tr>';
        } else {
          this.refs.historyBody.innerHTML = rows.map((step) => `
            <tr>
              <td>${this.formatTimestamp(step.timestampMs)}</td>
              <td><span class="${this.scoreChipClass(step.score)}">${step.score.toFixed(1)}</span></td>
              <td>${step.shakiness.toFixed(1)}</td>
              <td>${step.uncontrolledDescent.toFixed(1)}</td>
              <td>${step.compensation.toFixed(1)}</td>
              <td>${Math.round(step.durationMs)} ms</td>
              <td>${step.range.toFixed(1)}%</td>
              <td>${step.descentAvgSpeed.toFixed(1)}</td>
              <td>${step.ascentAvgSpeed.toFixed(1)}</td>
              <td>${step.oscillations}</td>
            </tr>`).join('');
        }
      }
      if (this.refs.dailyAverageBody) {
        if (!daily.length) {
          this.refs.dailyAverageBody.innerHTML = '<tr><td class="empty" colspan="4">No synced history yet.</td></tr>';
        } else {
          this.refs.dailyAverageBody.innerHTML = daily.map((day) => `
            <tr>
              <td>${this.formatTimestamp(day.dayStartMs).split(',')[0]}</td>
              <td><span class="${this.scoreChipClass(day.averageScore)}">${day.averageScore.toFixed(1)}</span></td>
              <td>${day.count}</td>
              <td>${this.state.goal}</td>
            </tr>`).join('');
        }
      }
    }
  }
  const app = new RehabTrackerApp();
  app.init();
}());

)JS_ASSET";

