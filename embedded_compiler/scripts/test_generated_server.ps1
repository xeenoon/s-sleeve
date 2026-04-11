$ErrorActionPreference = 'Stop'

$serverOutLog = Join-Path $PSScriptRoot '..\angular_test\server_stdout.log'
$serverErrLog = Join-Path $PSScriptRoot '..\angular_test\server_stderr.log'
$serverExe = Join-Path $PSScriptRoot '..\angular_test\bin\generated_demo.exe'
$serverWd = Join-Path $PSScriptRoot '..\angular_test'
$port = 18091

if (Test-Path $serverOutLog) {
  Remove-Item $serverOutLog -Force
}

if (Test-Path $serverErrLog) {
  Remove-Item $serverErrLog -Force
}

$process = Start-Process -FilePath $serverExe `
  -ArgumentList @("$port", "0") `
  -WorkingDirectory $serverWd `
  -RedirectStandardOutput $serverOutLog `
  -RedirectStandardError $serverErrLog `
  -PassThru

Start-Sleep -Seconds 2

try {
  $root = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/"
  $variablesPage = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/variables"
  $css = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/styles.css"
  $js = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/app.js"
  $state1 = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/state"
  $value1 = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/value"
  $live = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/api/live"
  $history = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/api/history"
  $variables1 = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/api/variables"
  $variablesPost = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/api/variables" `
    -Method Post `
    -ContentType 'application/json' `
    -Body '{"goal":85}'
  $post = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/reading" `
    -Method Post `
    -ContentType 'application/json' `
    -Body '{"reading":3600}'
  $state2 = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/state"

  [pscustomobject]@{
    root_ok = ($root.StatusCode -eq 200 -and $root.Content -match '<button data-view="variables"')
    variables_page_ok = ($variablesPage.StatusCode -eq 200 -and $variablesPage.Content -match 'Calibration Variables')
    css_ok = ($css.StatusCode -eq 200 -and $css.Content -match 'background')
    js_ok = ($js.StatusCode -eq 200 -and $js.Content -match "fetchJson\('/api/variables'")
    state1 = $state1.Content
    value1 = $value1.Content.Trim()
    live = $live.Content
    history = $history.Content
    variables1 = $variables1.Content
    variablesPost = $variablesPost.Content
    post = $post.Content
    state2 = $state2.Content
  } | ConvertTo-Json -Depth 4

  '---SERVER STDOUT---'
  Get-Content $serverOutLog
  '---SERVER STDERR---'
  if (Test-Path $serverErrLog) {
    Get-Content $serverErrLog
  }
}
finally {
  if ($process -and -not $process.HasExited) {
    Stop-Process -Id $process.Id -Force
  }
}
