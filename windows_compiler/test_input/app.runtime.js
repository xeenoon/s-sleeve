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
