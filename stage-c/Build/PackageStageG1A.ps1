[CmdletBinding()]
param(
    [string]$EngineRoot = $(if ($env:UE_5_8_ROOT) { $env:UE_5_8_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }),
    [string]$ArchiveDirectory = ''
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$RepositoryRoot = (Resolve-Path (Join-Path $ProjectRoot '..')).Path
$Project = Join-Path $ProjectRoot 'NationSimulationStageC.uproject'
$Build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$RunUat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$Output = Join-Path $RepositoryRoot 'out\stage-g1a'
$TestOutput = Join-Path $Output 'test-output'
if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) { $ArchiveDirectory = Join-Path $Output 'package' }
New-Item -ItemType Directory -Force -Path $TestOutput | Out-Null

foreach ($RequiredPath in @($Project, $Build, $RunUat, (Join-Path $ProjectRoot 'Content\Maps\StageG1_3DCharacterPoC.umap'))) {
    if (-not (Test-Path -LiteralPath $RequiredPath)) { throw "Required Stage G-1A path does not exist: $RequiredPath" }
}

Get-Process -Name 'NationSimulationStageC' -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -and $_.Path.StartsWith($ArchiveDirectory, [System.StringComparison]::OrdinalIgnoreCase) } |
    Stop-Process -Force

# -map is a cook inclusion list only. No map is injected through -cmdline, and
# the packaged executable is launched without a map argument below.
& $Build NationSimulationStageC Win64 Development "-Project=$Project" `
    -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=1
if ($LASTEXITCODE -ne 0) { throw "Stage G-1A Win64 Development build failed with exit code $LASTEXITCODE" }

& $RunUat BuildCookRun "-project=$Project" -noP4 -platform=Win64 -clientconfig=Development `
    -cook -stage -pak -package -archive "-archivedirectory=$ArchiveDirectory" `
    '-map=/Game/Maps/StageG1_3DCharacterPoC' -utf8output
if ($LASTEXITCODE -ne 0) { throw "Stage G-1A Development packaging failed with exit code $LASTEXITCODE" }

$Executable = Get-ChildItem -LiteralPath $ArchiveDirectory -Recurse -Filter 'NationSimulationStageC.exe' |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $Executable) { throw "Packaged Stage G-1A executable was not found under $ArchiveDirectory" }

$RunId = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$UserDirectory = Join-Path $Output "package-user\$RunId"
New-Item -ItemType Directory -Force -Path $UserDirectory | Out-Null
$Arguments = @('-nullrhi','-unattended','-nosound','-log','-StageG1AEvidence','-StageG1AEvidenceExit',"-UserDir=$UserDirectory")
$Process = Start-Process -FilePath $Executable -ArgumentList $Arguments `
    -WorkingDirectory (Split-Path -Parent $Executable) -WindowStyle Hidden -PassThru
if (-not $Process.WaitForExit(90000)) {
    Stop-Process -Id $Process.Id -Force
    throw 'Packaged Stage G-1A no-argument launch timed out.'
}
if ($Process.ExitCode -ne 0) { throw "Packaged Stage G-1A exited with code $($Process.ExitCode)" }

$RuntimeLog = Get-ChildItem -LiteralPath $UserDirectory -Recurse -Filter 'NationSimulationStageC.log' -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $RuntimeLog) { throw 'Packaged Stage G-1A did not create a runtime log.' }
$RuntimeText = Get-Content -Raw -LiteralPath $RuntimeLog.FullName
$RequiredPatterns = @(
    'STAGE_G1A_READY map=.*StageG1_3DCharacterPoC.*standard_3d_player=1 standard_3d_npc=1 paper2d=0 fang_rat=0',
    'STAGE_G1A_PLAYER_READY mesh=/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.*anim=/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed',
    'STAGE_G1A_ENVIRONMENT .*door_clearance_uu=240.*slope=1 stairs=4.*boundaries=4',
    'STAGE_G1A_LIGHTING .*sky_atmosphere=1 dynamic_shadow=1 contact_shadow=1 blob_shadow=0',
    'STAGE_G1A_EVIDENCE_WRITTEN files=10'
)
foreach ($Pattern in $RequiredPatterns) {
    if ($RuntimeText -notmatch $Pattern) { throw "Packaged Stage G-1A runtime marker missing: $Pattern" }
}
if ($RuntimeText -match 'Fatal error|Failed to load package .*StageG1_3DCharacterPoC') {
    throw 'Packaged Stage G-1A runtime log contains a fatal/map load error.'
}

$EvidenceNames = @(
    'standard_asset_evidence.json',
    'world_height_evidence.json',
    'movement_animation_sync_evidence.json',
    'click_move_evidence.json',
    'grounding_shadow_evidence.json',
    'walk_run_mode_evidence.json',
    'walk_animation_evidence.json',
    'run_animation_evidence.json',
    'movement_mode_toggle_evidence.json',
    'ui_input_isolation_evidence.json'
)
foreach ($Name in $EvidenceNames) {
    $Evidence = Get-ChildItem -LiteralPath $UserDirectory -Recurse -Filter $Name -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $Evidence) { throw "Packaged Stage G-1A evidence was not written: $Name" }
    Copy-Item -LiteralPath $Evidence.FullName -Destination (Join-Path $TestOutput $Name) -Force
}

$ExecutableSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $Executable).Hash.ToLowerInvariant()
@(
    'configuration=Development'
    'configured_default_map=/Game/Maps/StageG1_3DCharacterPoC'
    'launch_map_argument=NONE'
    "executable=$Executable"
    "executable_sha256=$ExecutableSha256"
    "runtime_log=$($RuntimeLog.FullName)"
    'no_argument_default_map=PASS'
    'standard_3d_player=PASS'
    'standard_3d_npc=PASS'
    'paper2d_in_g1a=0'
    'fang_rat_in_g1a=0'
    'launch_smoke=PASS'
) | Set-Content -LiteralPath (Join-Path $Output 'package_launch.txt') -Encoding utf8

Write-Host "STAGE_G1A_PACKAGE_READY $Executable"
Write-Host 'STAGE_G1A_NO_ARGUMENT_LAUNCH PASS'
