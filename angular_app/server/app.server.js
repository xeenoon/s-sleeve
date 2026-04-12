server.get('/server', (req, res) => {
  const nav = {
    links: [
      { href: '/?view=live', label: 'Angular live' },
      { href: '/?view=history', label: 'Angular history' },
      { href: '/?view=variables', label: 'Angular variables' },
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
      { tone: 'good', title: 'Shift stable', detail: 'Field staffing is green for the next rehabilitation block.' },
      { tone: 'warn', title: 'Battery watch', detail: 'Two mobile gait kits should be topped up before afternoon sessions.' }
    ],
    quickLinks: [
      { href: '/server/reports/daily-brief?audience=clinical%20lead', label: 'Open daily brief' },
      { href: '/server/team/amelia-ross', label: 'View clinician card' },
      { href: '/?view=history', label: 'Jump to Angular history view' }
    ]
  };
  res.render('pages/server-home', dashboard);
});

server.get('/server/operations', (req, res) => {
  const nav = {
    links: [
      { href: '/?view=live', label: 'Angular live' },
      { href: '/?view=history', label: 'Angular history' },
      { href: '/server', label: 'Care hub' },
      { href: '/server/reports', label: 'Reports' }
    ]
  };
  const board = {
    pageTitle: 'Operations Board',
    eyebrow: 'Ops stream',
    subtitle: 'Shift handoff snapshot for rehabilitation sessions, gait review blocks, and equipment readiness.',
    nav: nav,
    summaryMarkup: '<strong>Queue ready:</strong> four mobility sessions, one telehealth review, and one brace refit handoff are staged.',
    operations: [
      { patient: 'Mia Chen', phase: 'Gait retraining', session: '10:30', status: 'Ready', nextStep: 'Review dorsiflexion range and stair confidence.' },
      { patient: 'Noah Patel', phase: 'Strength block', session: '11:15', status: 'Ready', nextStep: 'Repeat resisted extension set and compare symmetry.' },
      { patient: 'Jules Hart', phase: 'Brace tuning', session: '13:00', status: 'Watch', nextStep: 'Recheck strap pressure after 20 minutes of walking.' },
      { patient: 'Amelia Ross', phase: 'Telehealth review', session: '15:30', status: 'Ready', nextStep: 'Confirm home exercise adherence and progression plan.' }
    ],
    alerts: [
      'One ankle sleeve battery should be swapped before the afternoon round.',
      'Brace strap stock is down to the last two medium sets.',
      'History review requested for Jules Hart before the 13:00 refit.'
    ]
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

server.get('/server/api/overview', (req, res) => {
  res.json({
    clinicName: 'North Ward Mobility Lab',
    summary: 'Shared rehab feed used by the Angular shell and compiled EJS briefings.',
    activePatients: 12,
    sessionsToday: 18,
    adherenceAverage: 91,
    operations: [
      { patient: 'Mia Chen', phase: 'Gait retraining', session: '10:30', status: 'Ready', nextStep: 'Range review' },
      { patient: 'Noah Patel', phase: 'Strength block', session: '11:15', status: 'Ready', nextStep: 'Symmetry check' },
      { patient: 'Jules Hart', phase: 'Brace tuning', session: '13:00', status: 'Watch', nextStep: 'Pressure recheck' },
      { patient: 'Amelia Ross', phase: 'Telehealth review', session: '15:30', status: 'Ready', nextStep: 'Progression plan' }
    ]
  });
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
