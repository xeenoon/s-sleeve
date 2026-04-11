$ErrorActionPreference = 'Stop'

$exe = Join-Path $PSScriptRoot 'bin\main.exe'
$inputDir = Join-Path $PSScriptRoot '..\angular_app'

if (-not (Test-Path $exe)) {
  throw "Missing compiler executable: $exe"
}

if (-not (Test-Path $inputDir)) {
  throw "Missing test folder: $inputDir"
}

Push-Location $PSScriptRoot
try {
  & $exe $inputDir
} finally {
  Pop-Location
}
