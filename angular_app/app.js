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
