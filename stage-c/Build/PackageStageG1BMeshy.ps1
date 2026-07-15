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
$Output = Join-Path $RepositoryRoot 'out\stage-g1b-meshy'
$TestOutput = Join-Path $Output 'test-output'
$Reports = Join-Path $ProjectRoot 'SourceArt\StageG1B\PLAYER_M\Meshy\v0.1\Working\Reports'
$AdditionalReports = Join-Path $ProjectRoot 'SourceArt\StageG1B\PLAYER_M\Meshy\v0.1\AdditionalAnimations\Working\Reports'
if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) { $ArchiveDirectory = Join-Path $Output 'package' }
New-Item -ItemType Directory -Force -Path $TestOutput,$Reports,$AdditionalReports | Out-Null

$Required = @(
    $Project,
    $Build,
    $RunUat,
    (Join-Path $ProjectRoot 'Content\Maps\StageG1B_OriginalPlayerPoC.umap'),
    (Join-Path $ProjectRoot 'Content\Maps\StageG1_3DCharacterPoC.umap')
)
foreach ($Path in $Required) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "Required Stage G-1B package path is missing: $Path" }
}

Get-Process -Name 'NationSimulationStageC' -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -and $_.Path.StartsWith($ArchiveDirectory, [System.StringComparison]::OrdinalIgnoreCase) } |
    Stop-Process -Force

& $Build NationSimulationStageC Win64 Development "-Project=$Project" `
    -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=1
if ($LASTEXITCODE -ne 0) { throw "Stage G-1B Win64 Development build failed with exit code $LASTEXITCODE" }

# -map is only a cook inclusion list.  No map argument is injected into the
# packaged command line; the first smoke below exercises GameDefaultMap.
& $RunUat BuildCookRun "-project=$Project" -noP4 -platform=Win64 -clientconfig=Development `
    -cook -stage -pak -package -archive "-archivedirectory=$ArchiveDirectory" `
    '-map=/Game/Maps/StageG1B_OriginalPlayerPoC+/Game/Maps/StageG1_3DCharacterPoC' -utf8output
if ($LASTEXITCODE -ne 0) { throw "Stage G-1B Development packaging failed with exit code $LASTEXITCODE" }

$Executable = Get-ChildItem -LiteralPath $ArchiveDirectory -Recurse -Filter 'NationSimulationStageC.exe' |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $Executable) { throw "Packaged executable was not found under $ArchiveDirectory" }

$RunId = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$UserDirectory = Join-Path $Output "package-user\$RunId"
New-Item -ItemType Directory -Force -Path $UserDirectory | Out-Null
$Arguments = @('-nullrhi','-unattended','-nosound','-log','-StageG1BEvidence','-StageG1BEvidenceExit',"-UserDir=$UserDirectory")
$Process = Start-Process -FilePath $Executable -ArgumentList $Arguments `
    -WorkingDirectory (Split-Path -Parent $Executable) -WindowStyle Hidden -PassThru
if (-not $Process.WaitForExit(90000)) {
    Stop-Process -Id $Process.Id -Force
    throw 'Packaged Stage G-1B no-argument launch timed out.'
}
if ($Process.ExitCode -ne 0) { throw "Packaged Stage G-1B exited with code $($Process.ExitCode)" }

$RuntimeLog = Get-ChildItem -LiteralPath $UserDirectory -Recurse -Filter 'NationSimulationStageC.log' -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $RuntimeLog) { throw 'Packaged Stage G-1B did not create a runtime log.' }
$RuntimeText = Get-Content -Raw -LiteralPath $RuntimeLog.FullName
$RequiredPatterns = @(
    'STAGE_G1A_READY map=.*StageG1B_OriginalPlayerPoC',
    'STAGE_G1B_PLAYER_M_READY mesh=/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Mesh/SK_PLAYER_M_Meshy_v0_1.*idle=A_PLAYER_M_Meshy_v0_1_IDLE_Idle11.*dash=A_PLAYER_M_Meshy_v0_1_DASH_Run02.*height=175.00 scale=1.0 fallback=false provisional=false',
    'STAGE_G1B_EVIDENCE_WRITTEN files=9 idle_anim=1 walk=250.0 run=500.0 dash=750.0 walk_anim=1 run_anim=1 dash_anim=1 idle_return=1 fallback=false',
    'STAGE_G1A_ENVIRONMENT .*slope=1 stairs=4.*boundaries=4',
    'STAGE_G1A_LIGHTING .*dynamic_shadow=1 contact_shadow=1'
)
foreach ($Pattern in $RequiredPatterns) {
    if ($RuntimeText -notmatch $Pattern) { throw "Packaged Stage G-1B runtime marker is missing: $Pattern" }
}
if ($RuntimeText -match 'Fatal error|STAGE_G1B_PLAYER_M_FAIL|Failed to load package .*StageG1B') {
    throw 'Packaged Stage G-1B runtime contains a fatal, asset, or map load failure.'
}

$EvidenceNames = @(
    'runtime_player_evidence.json',
    'animation_selection_evidence.json',
    'root_motion_evidence.json',
    'walk_run_evidence.json',
    'grounding_shadow_evidence.json',
    'idle_loop_evidence.json',
    'walk_run_dash_evidence.json',
    'movement_mode_toggle_evidence.json',
    'root_motion_four_state_evidence.json'
)
foreach ($Name in $EvidenceNames) {
    $Evidence = Get-ChildItem -LiteralPath $UserDirectory -Recurse -Filter $Name -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $Evidence) { throw "Packaged Stage G-1B evidence was not written: $Name" }
    Copy-Item -LiteralPath $Evidence.FullName -Destination (Join-Path $TestOutput $Name) -Force
    Copy-Item -LiteralPath $Evidence.FullName -Destination (Join-Path $Reports $Name) -Force
    Copy-Item -LiteralPath $Evidence.FullName -Destination (Join-Path $AdditionalReports $Name) -Force
}

$RuntimeEvidence = Get-Content -Raw -LiteralPath (Join-Path $TestOutput 'runtime_player_evidence.json') | ConvertFrom-Json
$WalkRun = Get-Content -Raw -LiteralPath (Join-Path $TestOutput 'walk_run_dash_evidence.json') | ConvertFrom-Json
$Grounding = Get-Content -Raw -LiteralPath (Join-Path $TestOutput 'grounding_shadow_evidence.json') | ConvertFrom-Json
$IdleLoop = Get-Content -Raw -LiteralPath (Join-Path $TestOutput 'idle_loop_evidence.json') | ConvertFrom-Json
$Toggle = Get-Content -Raw -LiteralPath (Join-Path $TestOutput 'movement_mode_toggle_evidence.json') | ConvertFrom-Json
$RootMotionFourState = Get-Content -Raw -LiteralPath (Join-Path $TestOutput 'root_motion_four_state_evidence.json') | ConvertFrom-Json
if (-not $RuntimeEvidence.meshy_visual_active -or $RuntimeEvidence.manny_fallback -or
    $RuntimeEvidence.world_height_uu -lt 170 -or $RuntimeEvidence.world_height_uu -gt 180 -or
    $RuntimeEvidence.component_scale -ne 1) { throw 'Packaged Stage G-1B visual/runtime evidence failed.' }
if (-not $WalkRun.walk_sequence_observed -or -not $WalkRun.run_sequence_observed -or -not $WalkRun.dash_sequence_observed -or
    -not $WalkRun.destination_maintained -or -not $WalkRun.nav_path_maintained -or
    -not $WalkRun.idle_return_observed -or $WalkRun.observed_walk_speed_uu_s -ne 250 -or
    $WalkRun.observed_run_speed_uu_s -ne 500 -or $WalkRun.observed_dash_speed_uu_s -ne 750) {
    throw 'Packaged Stage G-1B WALK/RUN/DASH evidence failed.'
}
if (-not $Grounding.ground_trace_hit -or -not $Grounding.dynamic_shadow -or -not $Grounding.contact_shadow) {
    throw 'Packaged Stage G-1B grounding/shadow evidence failed.'
}
if (-not $IdleLoop.idle_sequence_observed -or -not $IdleLoop.animation_time_progressed -or
    -not $IdleLoop.loop_maintained -or -not $IdleLoop.actor_location_rotation_stable -or
    $IdleLoop.reference_pose_provisional) { throw 'Packaged Stage G-1B formal IDLE evidence failed.' }
if (-not $Toggle.destination_maintained -or -not $Toggle.nav_path_maintained -or
    -not $Toggle.destination_update_count_unchanged -or -not $Toggle.selected_target_unchanged -or
    $Toggle.world_move_command_generated -or $Toggle.causal_event_generated -or $Toggle.ui_click_pass_through) {
    throw 'Packaged Stage G-1B three-mode UI isolation evidence failed.'
}
if (-not $RootMotionFourState.idle_isolated -or -not $RootMotionFourState.walk_isolated -or
    -not $RootMotionFourState.run_isolated -or -not $RootMotionFourState.dash_isolated -or
    $RootMotionFourState.double_movement_observed) { throw 'Packaged Stage G-1B four-state root-motion evidence failed.' }

# The accepted Stage G-1A map was included in the same package; verify it with
# an explicit map without changing the no-argument G-1B contract above.
$G1AUserDirectory = Join-Path $Output "package-user-g1a\$RunId"
New-Item -ItemType Directory -Force -Path $G1AUserDirectory | Out-Null
$G1AArguments = @('/Game/Maps/StageG1_3DCharacterPoC','-nullrhi','-unattended','-nosound','-log',
    '-StageG1AEvidence','-StageG1AEvidenceExit',"-UserDir=$G1AUserDirectory")
$G1AProcess = Start-Process -FilePath $Executable -ArgumentList $G1AArguments `
    -WorkingDirectory (Split-Path -Parent $Executable) -WindowStyle Hidden -PassThru
if (-not $G1AProcess.WaitForExit(90000)) {
    Stop-Process -Id $G1AProcess.Id -Force
    throw 'Packaged explicit Stage G-1A regression timed out.'
}
if ($G1AProcess.ExitCode -ne 0) { throw "Packaged explicit Stage G-1A exited with code $($G1AProcess.ExitCode)" }
$G1ALog = Get-ChildItem -LiteralPath $G1AUserDirectory -Recurse -Filter 'NationSimulationStageC.log' -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $G1ALog -or (Get-Content -Raw -LiteralPath $G1ALog.FullName) -notmatch
    'STAGE_G1A_EVIDENCE_WRITTEN files=10') { throw 'Packaged explicit Stage G-1A regression failed.' }

$ExecutableSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $Executable).Hash.ToLowerInvariant()
$PackageEvidence = [ordered]@{
    configuration = 'Development'
    configured_default_map = '/Game/Maps/StageG1B_OriginalPlayerPoC'
    launch_map_argument = 'NONE'
    executable = $Executable
    executable_sha256 = $ExecutableSha256
    runtime_log = $RuntimeLog.FullName
    no_argument_default_map = 'PASS'
    player_m_meshy = 'PASS'
    manny_fallback = $false
    texture_lit_shadow = 'PASS'
    idle_mode = 'Meshy Idle_11 formal'
    walk = 'PASS'
    run = 'PASS'
    dash = 'PASS'
    walk_run_dash_toggle_path_maintained = 'PASS'
    explicit_stage_g1a_package_regression = 'PASS'
    external_communication = 0
    meshy_credit_consumption = 0
}
$PackageEvidence | ConvertTo-Json -Depth 4 |
    Set-Content -LiteralPath (Join-Path $Reports 'package_evidence.json') -Encoding utf8
$PackageIdleDashEvidence = [ordered]@{
    package = $Executable
    no_argument_default_map = 'PASS'
    idle_11 = 'PASS'
    idle_loop = 'PASS'
    walking_250 = 'PASS'
    running_500 = 'PASS'
    dash_run_02_750 = 'PASS'
    three_mode_toggle = 'PASS'
    destination_nav_path_maintained = 'PASS'
    root_motion_four_state_isolated = 'PASS'
    manny_fallback = $false
}
$PackageIdleDashEvidence | ConvertTo-Json -Depth 4 |
    Set-Content -LiteralPath (Join-Path $AdditionalReports 'package_idle_dash_evidence.json') -Encoding utf8
$PackageEvidence.GetEnumerator() | ForEach-Object { "{0}={1}" -f $_.Key,$_.Value } |
    Set-Content -LiteralPath (Join-Path $Output 'package_launch.txt') -Encoding utf8

Write-Host "STAGE_G1B_PACKAGE_READY $Executable"
Write-Host 'STAGE_G1B_NO_ARGUMENT_LAUNCH PASS'
Write-Host 'STAGE_G1A_EXPLICIT_PACKAGE_REGRESSION PASS'
