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
  $css = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/styles.css"
  $js = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/app.js"
  $state1 = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/state"
  $value1 = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/value"
  $live = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/api/live"
  $history = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/api/history"
  $sync = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/api/time-sync"
  $post = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/reading" `
    -Method Post `
    -ContentType 'application/json' `
    -Body '{"reading":3600}'
  $state2 = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/state"

  [pscustomobject]@{
    root_ok = ($root.StatusCode -eq 200 -and $root.Content -match '<script src="/app.js"></script>')
    css_ok = ($css.StatusCode -eq 200 -and $css.Content -match 'background')
    js_ok = ($js.StatusCode -eq 200 -and $js.Content -match "fetch\('/state'\)")
    state1 = $state1.Content
    value1 = $value1.Content.Trim()
    live = $live.Content
    history = $history.Content
    sync = $sync.Content
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
