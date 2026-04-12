server.get('/server', (req, res) => {
  res.render('server-home', {
    title: 'Hybrid Care Hub',
    subtitle: 'Server-rendered launch surface for clinicians, coordinators, and field teams.',
    status: 'Clinical sync stable',
    heroMarkup: '<span class="hero-badge">EJS route compiled into C</span>',
    showHighlights: true
  });
});

server.get('/server/operations', (req, res) => {
  res.render('operations', {
    title: 'Operations Board',
    subtitle: 'Shift handoff snapshot for deployments, stock, and coaching readiness.',
    panelState: 'Field team green across the current roster.',
    calloutMarkup: '<strong>Queue ready:</strong> seven sleeve kits staged for dispatch.',
    showBoard: true
  });
});

server.get('/server/reports', (req, res) => {
  res.render('reports', {
    title: 'Reporting Room',
    subtitle: 'Daily review surface for outcomes, compliance, and trend notes.',
    reportState: 'Morning digest compiled and ready for review.',
    insightMarkup: '<em>Trend note:</em> step quality improved after the last fit adjustment block.',
    showDigest: true
  });
});

server.get('/server/variables', (req, res) => {
  res.render('variables', {
    title: 'Variables Console',
    status: 'Latest thresholds mirrored from the device configuration feed.',
    summary: 'Use the SPA when you need live tuning. Use this server page when you need a printable briefing view.',
    hintMarkup: '<span class="inline-chip">Hybrid workflow active</span>',
    showChecklist: true
  });
});

server.get('/server/welcome.txt', (req, res) => {
  res.send('s-sleeve hybrid compiler demo is online');
});

server.post('/server/api/launch', (req, res) => {
  res.status(202).json({
    ok: true,
    queued: 3,
    message: 'Launch packet queued'
  });
});
