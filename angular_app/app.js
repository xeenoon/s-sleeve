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

function withTemporaryClass(element, className, durationMs) {
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

function flashCollection(selector, className, staggerMs, durationMs) {
  Array.prototype.forEach.call(document.querySelectorAll(selector), function (element, index) {
    window.setTimeout(function () {
      withTemporaryClass(element, className, durationMs);
    }, index * (staggerMs || 40));
  });
}

var telemetryState = {
  reading: null,
  percentStraight: null,
  lastScore: null,
  stepCount: null
};
var demoState = {
  lastMotionAt: 0,
  timerId: null,
  phase: 0,
  active: false,
  lastPercent: null,
  lastLivePercent: null
};

function markTelemetryActive(card, active) {
  if (!card) {
    return;
  }
  card.classList.toggle('is-live', !!active);
}

function pulseTelemetryFor(card, durationMs) {
  markTelemetryActive(card, true);
  window.setTimeout(function () {
    markTelemetryActive(card, false);
  }, durationMs || 1000);
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

function simulateGaitFrame() {
  var secondsSinceMotion = (Date.now() - demoState.lastMotionAt) / 1000;
  var primary;
  var secondary;
  var tertiary;
  var percent;

  if (secondsSinceMotion < 1.0) {
    if (demoState.active) {
      demoState.active = false;
      demoState.lastPercent = null;
      setDemoCopy('Live reading active');
    }
    return;
  }

  demoState.active = true;
  demoState.phase += 0.22;
  primary = (Math.sin(demoState.phase) * 0.5) + 0.5;
  secondary = (Math.sin((demoState.phase * 2) - 0.8) * 0.5) + 0.5;
  tertiary = (Math.sin((demoState.phase * 3) + 0.6) * 0.5) + 0.5;
  percent = 14 + (Math.pow(primary, 0.86) * 50) + (secondary * 18) - (tertiary * 6);
  percent = Math.max(10, Math.min(92, percent));
  updateKnee(percent);
  setText('percent-straight', percent.toFixed(1) + '%');
  if (demoState.lastPercent === null || Math.abs(demoState.lastPercent - percent) > 0.4) {
    setDemoCopy('Demo gait running while live reading is static');
  }
  demoState.lastPercent = percent;
}

function ensureDemoGaitLoop() {
  if (demoState.timerId !== null) {
    return;
  }
  demoState.timerId = window.setInterval(simulateGaitFrame, 90);
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
  if (demoState.lastLivePercent === null || Math.abs(demoState.lastLivePercent - nextPercent) > 0.35) {
    demoState.lastMotionAt = Date.now();
  }
  demoState.lastLivePercent = nextPercent;
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
  setDemoCopy('Live reading active');
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

function animateViewTransition(view) {
  console.log('[app] animateViewTransition', view);
  var section = document.getElementById(view + '-view');
  if (section) {
    withTemporaryClass(section, 'is-transitioning', 460);
  }
}

function animateLiveSignal(payload) {
  console.log('[app] animateLiveSignal reading=' + payload.reading);
  var cards = document.querySelectorAll('[data-telemetry-card]');
  var readingChanged = telemetryState.reading !== payload.reading;
  var percentChanged = telemetryState.percentStraight !== payload.percentStraight;
  var scoreChanged = telemetryState.lastScore !== payload.lastScore;
  var stepsChanged = telemetryState.stepCount !== payload.stepCount;
  if (cards[0]) {
    markTelemetryActive(cards[0], false);
    if (readingChanged) {
      pulseTelemetryFor(cards[0], 1000);
    }
  }
  if (cards[1]) {
    markTelemetryActive(cards[1], false);
    if (percentChanged) {
      pulseTelemetryFor(cards[1], 1000);
    }
  }
  if (cards[2]) {
    markTelemetryActive(cards[2], false);
    if (scoreChanged) {
      pulseTelemetryFor(cards[2], 1000);
    }
  }
  if (cards[3]) {
    markTelemetryActive(cards[3], false);
    if (stepsChanged) {
      pulseTelemetryFor(cards[3], 1000);
    }
  }
  var meterCard = document.querySelector('.meter-card');
  if (meterCard) {
    withTemporaryClass(meterCard, 'is-energized', 360);
  }
  var root = document.documentElement;
  if (root) {
    root.style.setProperty('--surface-shift', (payload.inStep ? -2 : 0) + 'px');
  }
  telemetryState.reading = payload.reading;
  telemetryState.percentStraight = payload.percentStraight;
  telemetryState.lastScore = payload.lastScore;
  telemetryState.stepCount = payload.stepCount;
}

function animateHistoryRows(rows) {
  console.log('[app] animateHistoryRows count=' + ((rows && rows.length) || 0));
  flashCollection('#history-body tr', 'table-row-enter', 16, 420);
}

function animateDailyAverageRows(rows) {
  console.log('[app] animateDailyAverageRows count=' + ((rows && rows.length) || 0));
  flashCollection('#daily-average-body tr', 'table-row-enter', 24, 420);
}

function animateDiagnostics(summary) {
  console.log('[app] animateDiagnostics count=' + (summary && summary.count));
}

function animateVariableHydration(payload) {
  console.log('[app] animateVariableHydration keys=' + Object.keys(payload || {}).length);
}

function animateSaveSuccess(payload) {
  console.log('[app] animateSaveSuccess', payload && payload.status);
  var status = document.getElementById('variables-status');
  withTemporaryClass(status, 'warn', 520);
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
  demoState.lastMotionAt = 0;
  ensureDemoGaitLoop();
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
window.animateViewTransition = animateViewTransition;
window.animateLiveSignal = animateLiveSignal;
window.animateHistoryRows = animateHistoryRows;
window.animateDailyAverageRows = animateDailyAverageRows;
window.animateDiagnostics = animateDiagnostics;
window.animateVariableHydration = animateVariableHydration;
window.animateSaveSuccess = animateSaveSuccess;
window.collectVariablesPayload = collectVariablesPayload;
window.ngInitializeApp = ngInitializeApp;
