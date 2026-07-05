# Detail Forge — download pluginval (if needed) and validate the built VST3.
# Usage:  powershell -ExecutionPolicy Bypass -File scripts/validate.ps1 [-Strictness 5]
param([int]$Strictness = 5)

$ErrorActionPreference = "Stop"
$root  = Split-Path $PSScriptRoot -Parent
$tools = Join-Path $root "tools"
$exe   = Join-Path $tools "pluginval.exe"

if (-not (Test-Path $exe)) {
    New-Item -ItemType Directory -Force -Path $tools | Out-Null
    $zip = Join-Path $tools "pluginval.zip"
    $url = "https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_Windows.zip"
    Write-Host "Downloading pluginval from $url"
    Invoke-WebRequest -Uri $url -OutFile $zip
    Expand-Archive -Path $zip -DestinationPath $tools -Force
    Remove-Item $zip
}

$vst3 = Join-Path $root "build/DetailForge_artefacts/Release/VST3/Detail Forge.vst3"
if (-not (Test-Path $vst3)) { throw "VST3 not found at $vst3 — build first." }

Write-Host "Validating $vst3 (strictness $Strictness)"
& $exe --strictness-level $Strictness --validate-in-process --validate "$vst3"
exit $LASTEXITCODE
