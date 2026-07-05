# Package the Windows VST3: build Release, validate with pluginval, produce a versioned zip,
# and (if Inno Setup is installed) compile the installer.
#   pwsh packaging\package-windows.ps1 -Version 0.1.0
param(
  [string]$Version = "0.0.1",
  [int]$Strictness = 10
)
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root

Write-Host "==> Building Release (VST3)..." -ForegroundColor Cyan
cmake --build --preset default --target DetailForge_VST3

$vst3 = "build\DetailForge_artefacts\Release\VST3\Detail Forge.vst3"
if (-not (Test-Path $vst3)) { throw "VST3 not found at $vst3" }

$pluginval = "tools\pluginval.exe"
if (Test-Path $pluginval) {
  Write-Host "==> pluginval (strictness $Strictness)..." -ForegroundColor Cyan
  & $pluginval --strictness-level $Strictness --validate $vst3
  if ($LASTEXITCODE -ne 0) { throw "pluginval FAILED" }
} else {
  Write-Warning "pluginval not found at $pluginval — skipping validation."
}

New-Item -ItemType Directory -Force "packaging\output" | Out-Null
$zip = "packaging\output\DetailForge-$Version-Windows-x64-VST3.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Write-Host "==> Zipping -> $zip" -ForegroundColor Cyan
Compress-Archive -Path $vst3 -DestinationPath $zip -CompressionLevel Optimal

# Inno Setup installer (optional — only if ISCC is available)
$iscc = (Get-Command iscc -ErrorAction SilentlyContinue).Source
if (-not $iscc) {
  $iscc = @("C:\Program Files (x86)\Inno Setup 6\ISCC.exe","C:\Program Files\Inno Setup 6\ISCC.exe") |
          Where-Object { Test-Path $_ } | Select-Object -First 1
}
if ($iscc) {
  Write-Host "==> Compiling installer with Inno Setup..." -ForegroundColor Cyan
  & $iscc "/DMyAppVersion=$Version" "packaging\DetailForge.iss"
  if ($LASTEXITCODE -ne 0) { throw "ISCC FAILED" }
} else {
  Write-Warning "Inno Setup (ISCC.exe) not found — skipped installer. Install: winget install JRSoftware.InnoSetup"
}

Write-Host "==> Done. Artifacts in packaging\output\" -ForegroundColor Green
Get-ChildItem "packaging\output" | Select-Object Name, Length
