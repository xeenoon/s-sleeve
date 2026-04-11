$ErrorActionPreference = 'Stop'

$serverOutLog = (Join-Path $PSScriptRoot '..\angular_test\server_stdout.log')
$serverErrLog = (Join-Path $PSScriptRoot '..\angular_test\server_stderr.log')
$serverExe = (Resolve-Path (Join-Path $PSScriptRoot '..\angular_test\bin\generated_demo.exe')).Path
$serverWd = (Resolve-Path (Join-Path $PSScriptRoot '..\angular_test')).Path
$generatedJsPath = (Resolve-Path (Join-Path $PSScriptRoot '..\angular_test\app.js')).Path
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
  $state1b = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/state"
  $value1 = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/value"
  $live = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/api/live"
  $history = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/api/history"
  $variables1 = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/api/variables"
  $variablesPost = Invoke-WebRequest -UseBasicParsing "http://127.0.0.1:$port/api/variables" `
    -Method Post `
    -ContentType 'application/json' `
    -Body '{"goal":85}'
  $historyJson = $history.Content | ConvertFrom-Json
  $generatedJsBytes = (Get-Item $generatedJsPath).Length
  $servedJsBytes = [System.Text.Encoding]::UTF8.GetByteCount($js.Content)
  $rootHasRawTemplate = $root.Content -match '\{\{' -or $root.Content -match '\[attr\.' -or $root.Content -match '\[class\.' -or $root.Content -match '\(click\)'
  $rootHasExpectedTabs = $root.Content -match 'id="tabs"' -and $root.Content -match 'data-view="live"' -and $root.Content -match 'data-view="history"' -and $root.Content -match 'data-view="variables"'
  $rootHasExpectedViews = $root.Content -match 'id="live-view"' -and $root.Content -match 'id="history-view"' -and $root.Content -match 'id="variables-view"'
  $rootHasAssets = $root.Content -match '<link rel="stylesheet" href="/styles\.css">' -and $root.Content -match '<script src="/app\.js"></script>'
  $rootHasDiagnostics = $root.Content -match 'id="best-step-score"' -and $root.Content -match 'id="worst-step-score"' -and $root.Content -match 'id="best-day-score"' -and $root.Content -match 'id="average-step-duration"'
  $jsHasCompiledObservableRuntime = $js.Content -match 'ngApplyPipeChain' -and $js.Content -match 'registerObservable' -and $js.Content -match "runObservable" -and $js.Content -match "refreshObservable"
  $jsHasPipeHooks = $js.Content -match "formatHistoryRow" -and $js.Content -match "renderHistoryRows" -and $js.Content -match "applyLivePayload"
  $jsHasReducePipeline = $js.Content -match "step.kind === 'reduce'" -and $js.Content -match "summarizeHistory" -and $js.Content -match "renderHistoryDiagnostics"
  $jsHasRawObservableSyntax = $js.Content -match 'rx\.poll\(' -or $js.Content -match 'rx\.post\(' -or $js.Content -match 'rx\.state\(' -or $js.Content -match '\.pipe\('

  [pscustomobject]@{
    root_ok = ($root.StatusCode -eq 200 -and -not $rootHasRawTemplate -and $rootHasExpectedTabs -and $rootHasExpectedViews -and $rootHasAssets -and $rootHasDiagnostics)
    variables_page_ok = ($variablesPage.StatusCode -eq 200 -and $variablesPage.Content -match 'data-view="variables"')
    css_ok = ($css.StatusCode -eq 200 -and $css.Content -match 'background')
    js_ok = ($js.StatusCode -eq 200 -and $jsHasCompiledObservableRuntime -and $jsHasPipeHooks -and $jsHasReducePipeline -and -not $jsHasRawObservableSyntax -and $js.Content -match "window\.history\.replaceState" -and $servedJsBytes -eq $generatedJsBytes)
    history_ok = (@($historyJson.history).Count -eq 50 -and @($historyJson.dailyAverages).Count -gt 0)
    root_has_raw_template = $rootHasRawTemplate
    root_has_expected_tabs = $rootHasExpectedTabs
    root_has_expected_views = $rootHasExpectedViews
    root_has_assets = $rootHasAssets
    root_has_diagnostics = $rootHasDiagnostics
    js_has_compiled_observable_runtime = $jsHasCompiledObservableRuntime
    js_has_pipe_hooks = $jsHasPipeHooks
    js_has_reduce_pipeline = $jsHasReducePipeline
    js_has_raw_observable_syntax = $jsHasRawObservableSyntax
    generated_js_bytes = $generatedJsBytes
    served_js_bytes = $servedJsBytes
    history_count = @($historyJson.history).Count
    daily_average_count = @($historyJson.dailyAverages).Count
    state1 = $state1.Content
    state1b = $state1b.Content
    value1 = $value1.Content.Trim()
    live = $live.Content
    history = $history.Content
    variables1 = $variables1.Content
    variablesPost = $variablesPost.Content
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
