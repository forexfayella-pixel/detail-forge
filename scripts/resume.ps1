<#
    Detail Forge - resume dev session.
    Prints where we left off + handy commands, then launches Claude Code
    in the VST Creator directory (which preserves the project memory + brain).

    Usage:
        .\resume.ps1            # status, then launch Claude Code
        .\resume.ps1 -Run       # also launch the Standalone plugin
        .\resume.ps1 -Build     # build first (Release), then launch
        .\resume.ps1 -NoClaude  # just show status, don't launch Claude Code
#>
param(
    [switch]$Run,
    [switch]$Build,
    [switch]$NoClaude
)

$ErrorActionPreference = 'SilentlyContinue'

$root  = 'C:\Users\Fayella\Desktop\VST Creator'
$ws    = Join-Path $root 'brain-workspace'
$proj  = Join-Path $ws   'projects\detail-forge'
$exe   = Join-Path $proj 'build\DetailForge_artefacts\Release\Standalone\Detail Forge.exe'
$vst3  = Join-Path $proj 'build\DetailForge_artefacts\Release\VST3\Detail Forge.vst3'

function Line($c='DarkGray') { Write-Host ('  ' + ('-' * 58)) -ForegroundColor $c }

Clear-Host
Write-Host ''
Write-Host '   ......  DETAIL FORGE  ......' -ForegroundColor DarkYellow
Write-Host '   resume dev session'          -ForegroundColor Gray
Line

# --- Where we left off (from the brain) ---
Write-Host "`n  WHERE WE LEFT OFF" -ForegroundColor Cyan
$cp = Join-Path $proj 'brain\30-Roadmap\31-Current-Priorities.md'
if (Test-Path $cp) {
    Get-Content $cp -Encoding UTF8 |
        Where-Object { $_ -match '\S' -and $_ -notmatch '^(---|title:|type:|tags:|status:|updated:|# )' } |
        Select-Object -First 12 |
        ForEach-Object { '    ' + ($_ -replace '\*\*','' -replace '\[\[','' -replace '\]\]','') }
}

# --- Recent commits ---
Write-Host "`n  RECENT COMMITS" -ForegroundColor Cyan
git -C $ws log --oneline -5 | ForEach-Object { '    ' + $_ }

# --- Quick command reference ---
Write-Host "`n  QUICK COMMANDS  (run inside projects\detail-forge)" -ForegroundColor Cyan
Write-Host '    Build     : cmake --build --preset default'
Write-Host '    Standalone: & ".\build\DetailForge_artefacts\Release\Standalone\Detail Forge.exe"'
Write-Host '    Validate  : & ".\tools\pluginval.exe" --validate ".\build\DetailForge_artefacts\Release\VST3\Detail Forge.vst3"'
Write-Host '    Reinstall : copy the built .vst3 to "C:\Program Files\Common Files\VST3\"'
Line

# --- Optional: build ---
if ($Build) {
    Write-Host "`n  Building (Release)..." -ForegroundColor Yellow
    Push-Location $proj
    cmake --build --preset default
    Pop-Location
}

# --- Optional: run the Standalone ---
if ($Run) {
    if (Test-Path $exe) {
        Write-Host "`n  Launching Standalone..." -ForegroundColor Yellow
        Start-Process -FilePath $exe
    } else {
        Write-Host "`n  Standalone not built yet (run with -Build)." -ForegroundColor DarkYellow
    }
}

# --- Launch Claude Code in VST Creator (keeps memory + brain context) ---
Set-Location $root
if (-not $NoClaude) {
    if (Get-Command claude -ErrorAction SilentlyContinue) {
        Write-Host "`n  Launching Claude Code in $root ...`n" -ForegroundColor Green
        claude
    } else {
        Write-Host "`n  'claude' not found on PATH." -ForegroundColor DarkYellow
        Write-Host "  You are now in $root - start Claude Code however you normally do."
    }
} else {
    Write-Host "`n  Ready in $root. Type 'claude' to continue." -ForegroundColor Green
}
