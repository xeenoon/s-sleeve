(function () {
  const readingEl = document.getElementById('reading');
  const angleTextEl = document.getElementById('angle-text');
  const lowerLegEl = document.getElementById('lower-leg');
  const ankleEl = document.getElementById('ankle');
  function applyState(state) {
    readingEl.textContent = String(state.reading);
    angleTextEl.textContent = state.hasSignal ? String(state.percentStraight) + '% straight' : 'Signal error';
    lowerLegEl.setAttribute('x2', state.lowerLegX2);
    lowerLegEl.setAttribute('y2', state.lowerLegY2);
    ankleEl.setAttribute('cx', state.ankleX);
    ankleEl.setAttribute('cy', state.ankleY);
  }
  function refresh() {
    fetch('/state')
      .then(function (response) { return response.json(); })
      .then(applyState)
      .catch(function () { angleTextEl.textContent = 'Signal error'; });
  }
  refresh();
  window.setInterval(refresh, 1000);
}());
