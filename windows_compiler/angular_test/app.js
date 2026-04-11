(function () {
  const tabs = Array.prototype.slice.call(document.querySelectorAll('#tabs button'));
  const viewIds = ['live', 'history', 'variables'];
  function setText(id, value) {
    const el = document.getElementById(id);
    if (el) { el.textContent = String(value); }
  }
  function setValue(id, value) {
    const el = document.getElementById(id);
    if (el) { el.value = value; }
  }
  function showView(name) {
    viewIds.forEach(function (viewId) {
      const section = document.getElementById(viewId + '-view');
      const button = document.querySelector('#tabs button[data-view="' + viewId + '"]');
      const active = viewId === name;
      if (section) { section.classList.toggle('hidden', !active); }
      if (button) { button.classList.toggle('active', active); }
    });
  }
  function scoreChip(score) {
    if (score >= 90) { return 'score-chip'; }
    if (score >= 75) { return 'score-chip warn'; }
    return 'score-chip bad';
  }
  function formatTime(timestampMs) {
    return new Date(timestampMs).toLocaleString();
  }
  function applyState(state) {
    const lowerLegEl = document.getElementById('lower-leg');
    const ankleEl = document.getElementById('ankle');
    setText('reading', state.reading);
    setText('percent-straight', String(state.percentStraight) + '%');
    setText('goal-inline', state.goal);
    setText('today-average', Number(state.todayAverage || 0).toFixed(1));
    setText('last-score', Number(state.lastScore || 0).toFixed(1));
    setText('step-count', state.stepCount);
    setText('speed', Number(state.speed || 0).toFixed(1));
    setText('live-shaky', Number(state.shaky || 0).toFixed(1));
    setText('live-descent', Number(state.uncontrolledDescent || 0).toFixed(1));
    setText('live-compensation', Number(state.compensation || 0).toFixed(1));
    setText('step-status', state.inStep ? 'Movement detected' : 'Waiting for a full movement cycle');
    const syncPill = document.getElementById('sync-pill');
    const statusPill = document.getElementById('step-status');
    if (syncPill) {
      syncPill.textContent = state.timeSynced ? 'Phone time synced for daily tracking' : 'Time sync pending';
      syncPill.classList.toggle('warn', !state.timeSynced);
    }
    if (statusPill) {
      statusPill.classList.toggle('warn', !state.inStep);
    }
    if (lowerLegEl) {
      lowerLegEl.setAttribute('x2', state.lowerLegX2);
      lowerLegEl.setAttribute('y2', state.lowerLegY2);
    }
    if (ankleEl) {
      ankleEl.setAttribute('cx', state.ankleX);
      ankleEl.setAttribute('cy', state.ankleY);
    }
  }
  function applyHistory(history) {
    const historyBody = document.getElementById('history-body');
    const dailyBody = document.getElementById('daily-average-body');
    if (dailyBody) {
      dailyBody.innerHTML = (history.dailyAverages || []).map(function (item) {
        return '<tr><td>' + formatTime(item.dayStartMs).split(',')[0] + '</td><td><span class="' + scoreChip(item.averageScore) + '">' + Number(item.averageScore).toFixed(1) + '</span></td><td>' + item.count + '</td><td>' + history.goal + '</td></tr>';
      }).join('') || '<tr><td class="empty" colspan="4">No synced history yet.</td></tr>';
    }
    if (historyBody) {
      historyBody.innerHTML = (history.history || []).map(function (item) {
        return '<tr><td>' + formatTime(item.timestampMs) + '</td><td><span class="' + scoreChip(item.score) + '">' + Number(item.score).toFixed(1) + '</span></td><td>' + Number(item.shakiness).toFixed(1) + '</td><td>' + Number(item.uncontrolledDescent).toFixed(1) + '</td><td>' + Number(item.compensation).toFixed(1) + '</td><td>' + Number(item.durationMs).toFixed(0) + ' ms</td><td>' + Number(item.range).toFixed(0) + '%</td><td>' + Number(item.descentAvgSpeed).toFixed(1) + '</td><td>' + Number(item.ascentAvgSpeed).toFixed(1) + '</td><td>' + item.oscillations + '</td></tr>';
      }).join('') || '<tr><td class="empty" colspan="10">No steps recorded yet.</td></tr>';
    }
  }
  function applyVariables(payload) {
    ['minReading','maxReading','bentAngle','straightAngle','sampleIntervalMs','filterAlpha','motionThreshold','stepRangeThreshold','startReadyThreshold','returnMargin','maxStepDurationMs','sampleHistoryEnabled'].forEach(function (key) {
      if (Object.prototype.hasOwnProperty.call(payload, key)) {
        setValue(key, payload[key]);
      }
    });
    setText('variables-status', payload.status || 'Variables loaded');
  }
  function fetchJson(url, options) {
    return fetch(url, options).then(function (response) {
      if (!response.ok) { throw new Error('Request failed for ' + url); }
      return response.json();
    });
  }
  function refreshAll() {
    return Promise.all([
      fetchJson('/state').then(applyState),
      fetchJson('/api/history').then(applyHistory),
      fetchJson('/api/variables').then(applyVariables)
    ]).catch(function (error) {
      console.error('[app] refresh failed', error);
      setText('variables-status', 'Refresh failed');
    });
  }
  tabs.forEach(function (button) {
    button.addEventListener('click', function () {
      showView(button.getAttribute('data-view'));
    });
  });
  document.getElementById('save-variables').addEventListener('click', function () {
    const payload = {
      minReading: document.getElementById('minReading').value,
      maxReading: document.getElementById('maxReading').value,
      bentAngle: document.getElementById('bentAngle').value,
      straightAngle: document.getElementById('straightAngle').value,
      sampleIntervalMs: document.getElementById('sampleIntervalMs').value,
      filterAlpha: document.getElementById('filterAlpha').value,
      motionThreshold: document.getElementById('motionThreshold').value,
      stepRangeThreshold: document.getElementById('stepRangeThreshold').value,
      startReadyThreshold: document.getElementById('startReadyThreshold').value,
      returnMargin: document.getElementById('returnMargin').value,
      maxStepDurationMs: document.getElementById('maxStepDurationMs').value,
      sampleHistoryEnabled: document.getElementById('sampleHistoryEnabled').value
    };
    fetchJson('/api/variables', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    }).then(function (result) {
      setText('variables-status', result.status || 'Variables saved');
    }).catch(function () {
      setText('variables-status', 'Save failed');
    });
  });
  document.getElementById('reload-variables').addEventListener('click', function () {
    fetchJson('/api/variables').then(applyVariables);
  });
  showView('live');
  refreshAll();
  window.setInterval(function () { fetchJson('/state').then(applyState); }, 1000);
}());
