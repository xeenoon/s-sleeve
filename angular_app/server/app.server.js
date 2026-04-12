server.get('/server', (req, res) => {
  const nav = {
    links: [
      { href: '/', label: 'Angular shell' },
      { href: '/server', label: 'Care hub' },
      { href: '/server/operations', label: 'Operations' },
      { href: '/server/reports', label: 'Reports' },
      { href: '/server/variables', label: 'Variables' }
    ]
  };
  const dashboard = {
    pageTitle: 'Hybrid Care Hub',
    eyebrow: 'Angular + EJS',
    subtitle: 'Server-rendered launch surface for clinicians, coordinators, and field teams.',
    audience: title(query('audience')),
    heroMarkup: '<span class="hero-badge">Compiled layout, partials, loops, and helpers</span>',
    nav: nav,
    stats: [
      { label: 'Live wards', value: 4, detail: 'Clinician stations synced' },
      { label: 'Queued launches', value: 3, detail: 'Next sleeve dispatch wave' },
      { label: 'Open reports', value: 6, detail: 'Review surface ready' }
    ],
    notices: [
      { tone: 'good', title: 'Shift stable', detail: 'Field staffing is green for the next block.' },
      { tone: 'warn', title: 'Battery watch', detail: 'Two mobile kits should be topped up before handoff.' }
    ],
    quickLinks: [
      { href: '/server/reports/daily-brief', label: 'Open daily brief' },
      { href: '/server/team/amelia-ross', label: 'View clinician card' }
    ]
  };
  res.render('pages/server-home', dashboard);
});

server.get('/server/operations', (req, res) => {
  const nav = {
    links: [
      { href: '/', label: 'Angular shell' },
      { href: '/server', label: 'Care hub' },
      { href: '/server/reports', label: 'Reports' }
    ]
  };
  const board = {
    pageTitle: 'Operations Board',
    eyebrow: 'Ops stream',
    subtitle: 'Shift handoff snapshot for deployments, stock, and coaching readiness.',
    nav: nav,
    columns: [
      { heading: 'Ready', cards: ['North ward kit staged', 'Fit coach on site', 'Telehealth slot reserved'] },
      { heading: 'Watch', cards: ['One tablet offline', 'Strap stock below threshold'] },
      { heading: 'Next', cards: ['Afternoon outreach batch', 'Print briefing export'] }
    ],
    summaryMarkup: '<strong>Queue ready:</strong> seven sleeve kits staged for dispatch.'
  };
  res.render('pages/operations', board);
});

server.get('/server/reports', (req, res) => {
  const nav = {
    links: [
      { href: '/', label: 'Angular shell' },
      { href: '/server', label: 'Care hub' },
      { href: '/server/operations', label: 'Operations' }
    ]
  };
  const reports = {
    pageTitle: 'Reporting Room',
    eyebrow: 'Review surface',
    subtitle: 'Daily review surface for outcomes, compliance, and trend notes.',
    nav: nav,
    reportRows: [
      { slug: 'daily-brief', title: 'Daily brief', owner: 'Shift desk' },
      { slug: 'mobility-trend', title: 'Mobility trend', owner: 'Clinical analytics' },
      { slug: 'fit-followup', title: 'Fit follow-up', owner: 'Field operations' }
    ],
    insightMarkup: '<em>Trend note:</em> step quality improved after the last fit adjustment block.'
  };
  res.render('pages/reports', reports);
});

server.get('/server/reports/:reportId', (req, res) => {
  const detail = {
    pageTitle: upper(param('reportId')),
    eyebrow: 'Route params',
    subtitle: 'Detail view driven by a route parameter and reused layout.',
    reportId: param('reportId'),
    sections: [
      { title: 'Summary', text: 'This report is being rendered from a param-driven route.' },
      { title: 'Audience', text: title(query('audience')) },
      { title: 'Source', text: header('User-Agent') }
    ]
  };
  res.render('pages/report-detail', detail);
});

server.get('/server/team/:clinician', (req, res) => {
  const card = {
    pageTitle: title(param('clinician')),
    eyebrow: 'Clinician card',
    subtitle: 'Simple profile surface backed by route params and helper functions.',
    clinician: title(param('clinician')),
    highlights: [
      'Onboarding complete',
      'Telemetry ready',
      'Print briefing attached'
    ]
  };
  res.render('pages/team-card', card);
});

server.get('/server/variables', (req, res) => {
  const profile = {
    pageTitle: 'Variables Console',
    eyebrow: 'Config mirror',
    subtitle: 'Use the SPA for live tuning. Use this page for an audit-friendly snapshot.',
    status: 'Latest thresholds mirrored from the device configuration feed.',
    summary: 'This page uses the shared server layout and partial navigation.',
    checklist: [
      'Pressure profile synced',
      'Session pacing confirmed',
      'Escalation contact stored'
    ]
  };
  res.render('pages/variables', profile);
});

server.get('/server/welcome.txt', (req, res) => {
  const greeting = title(query('name'));
  res.send(greeting);
});

server.post('/server/api/launch', (req, res) => {
  const payload = {
    ok: true,
    queued: 3,
    mode: body('mode'),
    priority: body('priority'),
    requestedBy: header('X-Operator')
  };
  res.status(202).json(payload);
});
