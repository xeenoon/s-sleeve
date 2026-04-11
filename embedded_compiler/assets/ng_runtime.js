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
