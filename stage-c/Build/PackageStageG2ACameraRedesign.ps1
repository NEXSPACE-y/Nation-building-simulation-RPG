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
$Output = Join-Path $RepositoryRoot 'out\stage-g2a-redesign'
$Reports = Join-Path $ProjectRoot 'SourceArt\StageG2A\CameraRedesign\v0.1\Reports'
if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) { $ArchiveDirectory = Join-Path $Output 'package' }
New-Item -ItemType Directory -Force -Path $Output,$Reports | Out-Null

$Required = @(
    $Project,$Build,$RunUat,
    (Join-Path $ProjectRoot 'Content\Maps\StageG2A_CameraRedesignPoC.umap'),
    (Join-Path $ProjectRoot 'Content\Maps\StageG1B_OriginalPlayerPoC.umap'),
    (Join-Path $ProjectRoot 'Content\Maps\StageG1_3DCharacterPoC.umap')
)
foreach ($Path in $Required) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "Required Stage G-2A package input is missing: $Path" }
}

& $Build NationSimulationStageC Win64 Development "-Project=$Project" `
    -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=1
if ($LASTEXITCODE -ne 0) { throw "Stage G-2A Win64 Development build failed with exit code $LASTEXITCODE" }

& $RunUat BuildCookRun "-project=$Project" -noP4 -platform=Win64 -clientconfig=Development `
    -cook -stage -pak -package -archive "-archivedirectory=$ArchiveDirectory" `
    '-map=/Game/Maps/StageG2A_CameraRedesignPoC+/Game/Maps/StageG1B_OriginalPlayerPoC+/Game/Maps/StageG1_3DCharacterPoC' -utf8output
if ($LASTEXITCODE -ne 0) { throw "Stage G-2A Development packaging failed with exit code $LASTEXITCODE" }

$Executable = Get-ChildItem -LiteralPath $ArchiveDirectory -Recurse -Filter 'NationSimulationStageC.exe' |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $Executable) { throw "Packaged executable was not found under $ArchiveDirectory" }

$RunId = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$UserDirectory = Join-Path $Output "package-user\$RunId"
New-Item -ItemType Directory -Force -Path $UserDirectory | Out-Null
$Arguments = @('-nullrhi','-unattended','-nosound','-log','-StageG2AEvidence','-StageG2AEvidenceExit',"-UserDir=$UserDirectory")
$Process = Start-Process -FilePath $Executable -ArgumentList $Arguments `
    -WorkingDirectory (Split-Path -Parent $Executable) -WindowStyle Hidden -PassThru
if (-not $Process.WaitForExit(90000)) {
    Stop-Process -Id $Process.Id -Force
    throw 'Packaged Stage G-2A no-argument launch timed out.'
}
if ($Process.ExitCode -ne 0) { throw "Packaged Stage G-2A exited with code $($Process.ExitCode)" }

$RuntimeLog = Get-ChildItem -LiteralPath $UserDirectory -Recurse -Filter 'NationSimulationStageC.log' -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $RuntimeLog) { throw 'Packaged Stage G-2A did not create a runtime log.' }
$RuntimeText = Get-Content -Raw -LiteralPath $RuntimeLog.FullName
$RequiredPatterns = @(
    'STAGE_G1A_READY map=.*StageG2A_CameraRedesignPoC',
    'STAGE_G1B_PLAYER_M_READY .*height=175.00 scale=1.0 fallback=false provisional=false',
    'STAGE_G2A_REDESIGN_READY initial_mode=StandardCharacterCamera standard_distance=520.0 standard_pitch=-15.0 standard_fov=75.0 tactical_distance=1200.0 tactical_pitch=-55.0 tactical_fov=60.0 toggle=F6 collision=1 player_follow=1',
    'STAGE_G2A_REDESIGN_EVIDENCE_WRITTEN files=4 mode=standard tactical=1 return=1 destination=1 path=1 movement_mode=1 collision=1 recovery=1'
)
foreach ($Pattern in $RequiredPatterns) {
    if ($RuntimeText -notmatch $Pattern) { throw "Packaged Stage G-2A marker is missing: $Pattern" }
}
if ($RuntimeText -match 'Fatal error|STAGE_G2A_REDESIGN_FAIL|STAGE_G1B_PLAYER_M_FAIL') {
    throw 'Packaged Stage G-2A runtime contains a fatal camera or PLAYER_M failure.'
}

$EvidenceNames = @('camera_modes_evidence.json','camera_preservation_evidence.json',
    'camera_collision_evidence.json','camera_input_evidence.json')
foreach ($Name in $EvidenceNames) {
    $Evidence = Get-ChildItem -LiteralPath $UserDirectory -Recurse -Filter $Name -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $Evidence) { throw "Packaged Stage G-2A evidence was not written: $Name" }
    Copy-Item -LiteralPath $Evidence.FullName -Destination (Join-Path $Reports $Name) -Force
}

$Modes = Get-Content -Raw -LiteralPath (Join-Path $Reports 'camera_modes_evidence.json') | ConvertFrom-Json
$Preservation = Get-Content -Raw -LiteralPath (Join-Path $Reports 'camera_preservation_evidence.json') | ConvertFrom-Json
$Collision = Get-Content -Raw -LiteralPath (Join-Path $Reports 'camera_collision_evidence.json') | ConvertFrom-Json
$Input = Get-Content -Raw -LiteralPath (Join-Path $Reports 'camera_input_evidence.json') | ConvertFrom-Json
if ($Modes.initial_mode -ne 'StandardCharacterCamera' -or -not $Modes.standard_is_non_overhead -or
    -not $Modes.tactical_reached_by_toggle -or -not $Modes.returned_to_standard -or $Modes.parameter_leakage) {
    throw 'Packaged Stage G-2A camera-mode evidence failed.'
}
if (-not $Preservation.destination_maintained -or -not $Preservation.nav_path_maintained -or
    -not $Preservation.movement_mode_maintained -or -not $Preservation.selected_target_maintained -or
    $Preservation.camera_changes_actor_yaw) { throw 'Packaged Stage G-2A preservation evidence failed.' }
if (-not $Collision.do_collision_test -or -not $Collision.collision_shortened -or
    -not $Collision.distance_recovered_after_clear -or -not $Collision.camera_channel_trace_hit_fixture -or
    -not $Collision.camera_channel_trace_cleared_after_remove) { throw 'Packaged Stage G-2A collision evidence failed.' }
if ([Math]::Abs($Input.tactical_yaw_vs_old_observed - 2.5) -gt 0.001 -or
    [Math]::Abs($Input.tactical_pitch_vs_old_observed - 1.2) -gt 0.001 -or
    -not $Input.right_drag_both_modes -or -not $Input.wheel_zoom_both_modes -or
    $Input.movement_or_causal_side_effect) { throw 'Packaged Stage G-2A input evidence failed.' }

# Explicit accepted G1B map regression from the same package.
$G1BUserDirectory = Join-Path $Output "package-user-g1b\$RunId"
New-Item -ItemType Directory -Force -Path $G1BUserDirectory | Out-Null
$G1BArguments = @('/Game/Maps/StageG1B_OriginalPlayerPoC','-nullrhi','-unattended','-nosound','-log',
    '-StageG1BEvidence','-StageG1BEvidenceExit',"-UserDir=$G1BUserDirectory")
$G1BProcess = Start-Process -FilePath $Executable -ArgumentList $G1BArguments `
    -WorkingDirectory (Split-Path -Parent $Executable) -WindowStyle Hidden -PassThru
if (-not $G1BProcess.WaitForExit(90000)) {
    Stop-Process -Id $G1BProcess.Id -Force
    throw 'Packaged explicit Stage G-1B regression timed out.'
}
if ($G1BProcess.ExitCode -ne 0) { throw "Packaged explicit Stage G-1B exited with code $($G1BProcess.ExitCode)" }
$G1BLog = Get-ChildItem -LiteralPath $G1BUserDirectory -Recurse -Filter 'NationSimulationStageC.log' -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $G1BLog -or (Get-Content -Raw -LiteralPath $G1BLog.FullName) -notmatch
    'STAGE_G1B_EVIDENCE_WRITTEN files=9.*walk=250.0 run=500.0 dash=750.0') {
    throw 'Packaged explicit Stage G-1B regression failed.'
}

$PackageEvidence = [ordered]@{
    configuration = 'Development'
    configured_default_map = '/Game/Maps/StageG2A_CameraRedesignPoC'
    launch_map_argument = 'NONE'
    executable = $Executable
    executable_sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $Executable).Hash
    no_argument_default_map = 'PASS'
    initial_standard_non_overhead = 'PASS'
    f6_tactical_toggle = 'PASS'
    return_to_standard = 'PASS'
    camera_collision = 'PASS'
    player_m_meshy = 'PASS'
    manny_fallback = $false
    explicit_stage_g1b_package_regression = 'PASS'
    commit_tag_push = 0
}
$PackageEvidence | ConvertTo-Json -Depth 4 |
    Set-Content -LiteralPath (Join-Path $Reports 'package_evidence.json') -Encoding utf8NoBOM

Write-Host "STAGE_G2A_REDESIGN_PACKAGE_READY $Executable"
Write-Host 'STAGE_G2A_REDESIGN_NO_ARGUMENT_LAUNCH PASS'
Write-Host 'STAGE_G1B_EXPLICIT_PACKAGE_REGRESSION PASS'
