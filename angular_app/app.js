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

function readSearchParam(name) {
  var params = new URLSearchParams(window.location.search || '');
  return params.get(name) || '';
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

function renderBridgeOperations(items) {
  var rows = Array.isArray(items) ? items : [];
  if (!rows.length) {
    setHtml('bridge-operations-list', '<div class="bridge-empty">No rehabilitation sessions queued right now.</div>');
    return;
  }
  setHtml('bridge-operations-list', rows.map(function (item) {
    var tone = item.status === 'Watch' ? 'warn' : '';
    return '<div class="bridge-list-item"><div><strong>' + item.patient + '</strong><span>' + item.phase + ' • ' + item.session + ' • ' + item.nextStep + '</span></div><span class="bridge-status ' + tone + '">' + item.status + '</span></div>';
  }).join(''));
}

function applyBridgeOverview(payload) {
  setText('bridge-clinic-name', payload.clinicName || 'Rehabilitation snapshot');
  setText('bridge-clinic-copy', payload.summary || 'Shared rehab operations feed loaded.');
  setText('bridge-active-patients', String(payload.activePatients || 0));
  setText('bridge-sessions-today', String(payload.sessionsToday || 0));
  setText('bridge-adherence', String(payload.adherenceAverage || 0) + '%');
  renderBridgeOperations(payload.operations);
}

function loadBridgeOverview() {
  fetch('/server/api/overview')
    .then(function (response) { return response.json(); })
    .then(function (payload) { applyBridgeOverview(payload || {}); })
    .catch(function () {
      setText('bridge-clinic-name', 'Rehabilitation feed unavailable');
      setText('bridge-clinic-copy', 'The Angular shell could not load the shared server overview endpoint.');
      setHtml('bridge-operations-list', '<div class="bridge-empty">Unable to load operations feed.</div>');
    });
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
  var bentLabel = document.getElementById('bent-label');
  var straightLabel = document.getElementById('straight-label');
  var bentActive = normalized <= 0.3;
  var straightActive = normalized >= 0.7;
  if (bentLabel) {
    bentLabel.classList.toggle('is-active', bentActive);
    bentLabel.classList.toggle('is-bent', bentActive);
  }
  if (straightLabel) {
    straightLabel.classList.toggle('is-active', straightActive);
    straightLabel.classList.toggle('is-straight', straightActive);
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

function applyMotionPayload(payload, app) {
  var nextPercent = Number(payload.percentStraight || 0);
  console.log('[app] applyMotionPayload reading=' + payload.reading + ' percent=' + payload.percentStraight);
  if (app && typeof app.setValue === 'function') {
    app.setValue('goal', payload.goal || 0);
  }
  setText('reading', String(payload.reading));
  setText('percent-straight', Number(payload.percentStraight || 0).toFixed(1) + '%');
  setText('speed', Number(payload.speed || 0).toFixed(1));
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

function applyLivePayload(payload, app) {
  console.log('[app] applyLivePayload lastScore=' + payload.lastScore + ' steps=' + payload.stepCount);
  if (app && typeof app.setValue === 'function') {
    app.setValue('goal', payload.goal || 0);
  }
  setText('last-score', Number(payload.lastScore || 0).toFixed(1));
  setText('step-count', String(payload.stepCount || 0));
  setText('today-average', Number(payload.todayAverage || 0).toFixed(1));
  setText('goal-inline', String(payload.goal || 0));
  setText('live-shaky', Number(payload.shaky || 0).toFixed(1));
  setText('live-descent', Number(payload.uncontrolledDescent || 0).toFixed(1));
  setText('live-compensation', Number(payload.compensation || 0).toFixed(1));
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

  app.setObservable('selectedView$', window.location.pathname === '/variables' ? 'variables' : (readSearchParam('view') || 'live'));
  setDemoCopy('Waiting for live knee movement');
  loadBridgeOverview();
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
window.applyMotionPayload = applyMotionPayload;
window.applyLivePayload = applyLivePayload;
window.applyVariablesPayload = applyVariablesPayload;
window.applyVariablesSaveResponse = applyVariablesSaveResponse;
window.collectVariablesPayload = collectVariablesPayload;
window.applyBridgeOverview = applyBridgeOverview;
window.loadBridgeOverview = loadBridgeOverview;
window.ngInitializeApp = ngInitializeApp;
