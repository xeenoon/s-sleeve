server.get('/variables-server', (req, res) => {
  res.render('variables', {
    title: 'Variables',
    status: 'Loaded'
  });
});
