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
