[CmdletBinding()]
param(
    [string]$EngineRoot = $(if ($env:UE_5_8_ROOT) { $env:UE_5_8_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }),
    [string]$ArchiveDirectory = ''
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$RepositoryRoot = (Resolve-Path (Join-Path $ProjectRoot '..')).Path
$Project = Join-Path $ProjectRoot 'NationSimulationStageC.uproject'
$RunUat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) {
    $ArchiveDirectory = Join-Path $RepositoryRoot 'out\stage-e\package'
}

Get-Process -Name 'NationSimulationStageC' -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -and $_.Path.StartsWith($ArchiveDirectory, [System.StringComparison]::OrdinalIgnoreCase) } |
    Stop-Process -Force

& $RunUat BuildCookRun "-project=$Project" -noP4 -platform=Win64 -clientconfig=Development `
    -build -cook -stage -pak -package -archive "-archivedirectory=$ArchiveDirectory" `
    '-map=/Game/Maps/StageD_Capital' -utf8output
if ($LASTEXITCODE -ne 0) {
    throw "Stage E Development packaging failed with exit code $LASTEXITCODE"
}

$Executable = Get-ChildItem -LiteralPath $ArchiveDirectory -Recurse -Filter 'NationSimulationStageC.exe' |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $Executable) {
    throw "Packaged Stage E executable was not found under $ArchiveDirectory"
}

$Process = Start-Process -FilePath $Executable -ArgumentList '-nullrhi','-unattended','-nosound','-log' `
    -WorkingDirectory (Split-Path -Parent $Executable) -WindowStyle Hidden -PassThru
if (-not $Process.WaitForExit(8000)) {
    Stop-Process -Id $Process.Id
} elseif ($Process.ExitCode -ne 0) {
    throw "Packaged Stage E exited with code $($Process.ExitCode)"
}
Get-Process -Name 'NationSimulationStageC' -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -and $_.Path.StartsWith($ArchiveDirectory, [System.StringComparison]::OrdinalIgnoreCase) } |
    Stop-Process -Force

$RuntimeLog = Get-ChildItem -LiteralPath $ArchiveDirectory -Recurse -Filter 'NationSimulationStageC.log' |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $RuntimeLog) {
    throw 'Packaged Stage E did not create a runtime log.'
}
$RuntimeText = Get-Content -Raw -LiteralPath $RuntimeLog.FullName
if ($RuntimeText -notmatch 'STAGE_D_PLAYABLE_READY pawn=(?!NONE).*npc_count=40.*StageD_Capital') {
    throw 'Packaged Stage E did not initialize a playable pawn, 40 NPCs, and StageD_Capital.'
}
if ($RuntimeText -notmatch 'STAGE_E_CORE_READY overlay=stage_e_behavioral_depth_v1 definition_sha256=[0-9a-f]{64}') {
    throw 'Packaged Stage E did not initialize the accepted Stage E overlay and causal core.'
}
if ($RuntimeText -match 'Fatal error|LoadCore failed') {
    throw 'Packaged Stage E runtime log contains a fatal/core initialization error.'
}

$ExecutableSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $Executable).Hash.ToLowerInvariant()
$LaunchEvidence = Join-Path $RepositoryRoot 'out\stage-e\package_launch.txt'
$EvidenceDirectory = Split-Path -Parent $LaunchEvidence
New-Item -ItemType Directory -Force -Path $EvidenceDirectory | Out-Null
@(
    'configuration=Development'
    'map=/Game/Maps/StageD_Capital'
    "executable=$Executable"
    "executable_sha256=$ExecutableSha256"
    "runtime_log=$($RuntimeLog.FullName)"
    'playable_ready=PASS'
    'stage_e_core_ready=PASS'
    'launch_smoke=PASS'
) | Set-Content -LiteralPath $LaunchEvidence -Encoding utf8

Write-Host "STAGE_E_PACKAGE_READY $Executable"
Write-Host 'STAGE_E_LAUNCH_SMOKE PASS'
