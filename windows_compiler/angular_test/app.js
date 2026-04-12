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


  var ngApp = ngCreateApp();
  window.ngApp = ngApp;
  ngApp.registerObservable({ name: 'selectedView$', alias: 'selectedView', kind: 'state', path: '', seed: 'live', intervalMs: 0, steps: [{ kind: 'tap', argument: 'showView', args: ['showView'] }, { kind: 'effect-class', argument: 'is-transitioning', args: ['#{{value}}-view', 'is-transitioning', '460'] }] });
  ngApp.registerObservable({ name: 'motionState$', alias: 'motionState', kind: 'poll', path: '/api/motion', seed: null, intervalMs: 33, steps: [{ kind: 'tap', argument: 'applyMotionPayload', args: ['applyMotionPayload'] }, { kind: 'effect-class-at', argument: '0', args: ['[data-telemetry-card]', '0', 'is-live', '1000', 'changed', 'reading'] }, { kind: 'effect-class-at', argument: '1', args: ['[data-telemetry-card]', '1', 'is-live', '1000', 'changed', 'percentStraight'] }, { kind: 'effect-class', argument: 'is-energized', args: ['.meter-card', 'is-energized', '360', 'changed', 'percentStraight'] }, { kind: 'effect-style-var', argument: '--surface-shift', args: [':root', '--surface-shift', 'ternary-bool', 'inStep', '-2px', '0px'] }] });
  ngApp.registerObservable({ name: 'liveState$', alias: 'liveState', kind: 'poll', path: '/api/live', seed: null, intervalMs: 250, steps: [{ kind: 'tap', argument: 'applyLivePayload', args: ['applyLivePayload'] }, { kind: 'effect-class-at', argument: '2', args: ['[data-telemetry-card]', '2', 'is-live', '1000', 'changed', 'lastScore'] }, { kind: 'effect-class-at', argument: '3', args: ['[data-telemetry-card]', '3', 'is-live', '1000', 'changed', 'stepCount'] }] });
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
