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
$Output = Join-Path $RepositoryRoot 'out\stage-g3a-capital-blockout'
$Reports = Join-Path $ProjectRoot 'SourceArt\StageG3A\CapitalBlockout\v0.1\Reports'
if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) { $ArchiveDirectory = Join-Path $Output 'package' }
New-Item -ItemType Directory -Force -Path $Output,$Reports,$ArchiveDirectory | Out-Null

$Required = @(
    $Project,$Build,$RunUat,
    (Join-Path $ProjectRoot 'Content\Maps\StageG3A_CapitalBlockout_PoC.umap'),
    (Join-Path $ProjectRoot 'Content\Maps\StageG2B_GuardM_PoC.umap'),
    (Join-Path $ProjectRoot 'Content\Maps\StageG2A_CameraRedesignPoC.umap'),
    (Join-Path $ProjectRoot 'Content\Maps\StageG1B_OriginalPlayerPoC.umap')
)
foreach ($Path in $Required) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "Required Stage G-3A package input is missing: $Path" }
}

$BuildLog = Join-Path $Output 'package_build.log'
& $Build NationSimulationStageC Win64 Development "-Project=$Project" `
    -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=1 *> $BuildLog
if ($LASTEXITCODE -ne 0) { throw "Stage G-3A Win64 Development build failed with exit code $LASTEXITCODE" }

$UatLog = Join-Path $Output 'package_uat.log'
& $RunUat BuildCookRun "-project=$Project" -noP4 -platform=Win64 -clientconfig=Development `
    -cook -stage -pak -package -archive "-archivedirectory=$ArchiveDirectory" `
    '-map=/Game/Maps/StageG3A_CapitalBlockout_PoC+/Game/Maps/StageG2B_GuardM_PoC+/Game/Maps/StageG2A_CameraRedesignPoC+/Game/Maps/StageG1B_OriginalPlayerPoC' `
    -utf8output *> $UatLog
if ($LASTEXITCODE -ne 0) { throw "Stage G-3A Development packaging failed with exit code $LASTEXITCODE" }

$Executable = Get-ChildItem -LiteralPath $ArchiveDirectory -Recurse -Filter 'NationSimulationStageC.exe' |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $Executable) { throw "Packaged executable was not found under $ArchiveDirectory" }
$RunId = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')

function Invoke-PackagedCheck {
    param(
        [string]$Name,
        [string[]]$Arguments,
        [string[]]$RequiredPatterns,
        [int]$TimeoutMilliseconds = 120000
    )
    $UserDirectory = Join-Path $Output "package-user-$Name\$RunId"
    New-Item -ItemType Directory -Force -Path $UserDirectory | Out-Null
    $AllArguments = @($Arguments) + @('-nullrhi','-unattended','-nosound','-log',"-UserDir=$UserDirectory")
    $Process = Start-Process -FilePath $Executable -ArgumentList $AllArguments `
        -WorkingDirectory (Split-Path -Parent $Executable) -WindowStyle Hidden -PassThru
    if (-not $Process.WaitForExit($TimeoutMilliseconds)) {
        Stop-Process -Id $Process.Id -Force
        throw "Packaged Stage G-3A check timed out: $Name"
    }
    if ($Process.ExitCode -ne 0) { throw "Packaged Stage G-3A check exited with code $($Process.ExitCode): $Name" }
    $RuntimeLog = Get-ChildItem -LiteralPath $UserDirectory -Recurse -Filter 'NationSimulationStageC.log' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $RuntimeLog) { throw "Packaged Stage G-3A check did not create a runtime log: $Name" }
    $RuntimeText = Get-Content -Raw -LiteralPath $RuntimeLog.FullName
    foreach ($Pattern in $RequiredPatterns) {
        if ($RuntimeText -notmatch $Pattern) { throw "Packaged Stage G-3A marker is missing ($Name): $Pattern" }
    }
    if ($RuntimeText -match 'Fatal error|STAGE_G3A_CAPITAL_FAIL|STAGE_G2B_GUARD_M_FAIL|STAGE_G2A_REDESIGN_FAIL|STAGE_G1B_PLAYER_M_FAIL') {
        throw "Packaged Stage G-3A runtime contains a fatal accepted-system failure: $Name"
    }
    return [ordered]@{ name=$Name; user_directory=$UserDirectory; runtime_log=$RuntimeLog.FullName; exit_code=$Process.ExitCode }
}

$G3A = Invoke-PackagedCheck -Name 'g3a' `
    -Arguments @('-StageG3AEvidence','-StageG3AEvidenceExit') `
    -RequiredPatterns @(
        'STAGE_G3A_CAPITAL_GAMEMODE_READY player=StageG1APlayerCharacter navmesh=dynamic lighting=1 walkable_surfaces=1 click_trace_block=1 legacy_plaza=0 quinn=0',
        'STAGE_G1B_PLAYER_M_READY .*height=175.00 scale=1.0 fallback=false provisional=false',
        'STAGE_G2A_REDESIGN_READY initial_mode=StandardCharacterCamera .*toggle=F6 collision=1 player_follow=1',
        'STAGE_G2B_GUARD_M_READY .*height=175.00 scale=1.0 material_slots=1 player_skeleton_shared=false placement=1',
        'STAGE_G3A_CAPITAL_READY map=StageG3A_CapitalBlockout_PoC blockout=6000x6000 guard=1 standard=1 tactical=1 navmesh=dynamic external_assets=0',
        'STAGE_G3A_CAPITAL_EVIDENCE_WRITTEN files=5 zones=9 guard=1 nav_routes=10/10 click_traces=6/6 move_command=1 moved=[3-9][0-9]{3,}[^ ]* move_pass=1 traversal=1 .*autofollow=1 manual=1 resume=1 tactical_no_follow=1 return_follow=1 fallback=0 skeleton_shared=0 oversized=0'
    )

$EvidenceNames = @('capital_runtime_evidence.json','navmesh_route_evidence.json',
    'camera_collision_evidence.json','asset_isolation_evidence.json','camera_autofollow_evidence.json')
foreach ($Name in $EvidenceNames) {
    $Evidence = Get-ChildItem -LiteralPath $G3A.user_directory -Recurse -Filter $Name -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $Evidence) { throw "Packaged Stage G-3A evidence was not written: $Name" }
    Copy-Item -LiteralPath $Evidence.FullName -Destination (Join-Path $Reports $Name) -Force
}

$Runtime = Get-Content -Raw -LiteralPath (Join-Path $Reports 'capital_runtime_evidence.json') | ConvertFrom-Json
$Nav = Get-Content -Raw -LiteralPath (Join-Path $Reports 'navmesh_route_evidence.json') | ConvertFrom-Json
$Camera = Get-Content -Raw -LiteralPath (Join-Path $Reports 'camera_collision_evidence.json') | ConvertFrom-Json
$Isolation = Get-Content -Raw -LiteralPath (Join-Path $Reports 'asset_isolation_evidence.json') | ConvertFrom-Json
$AutoFollow = Get-Content -Raw -LiteralPath (Join-Path $Reports 'camera_autofollow_evidence.json') | ConvertFrom-Json
if ($Runtime.guard_actor_count -ne 1 -or $Runtime.player_fallback -or $Runtime.guard_player_skeleton_shared -or
    $Runtime.external_assets_used -or $Runtime.causal_core_connected -or $Runtime.save_schema_connected) {
    throw 'Packaged G3A capital runtime evidence failed.'
}
if ($Nav.route_count -ne 10 -or $Nav.passed_route_count -ne 10 -or -not $Nav.all_routes_passed -or
    -not $Nav.click_move_surface_passed -or -not $Nav.actual_player_movement_passed) {
    throw 'Packaged G3A NavMesh route evidence failed.'
}
if (-not $Camera.standard_initial -or -not $Camera.tactical_reached -or -not $Camera.returned_to_standard -or
    -not $Camera.camera_collision_contract) { throw 'Packaged G3A camera evidence failed.' }
if ($Isolation.player_assets_modified -or $Isolation.guard_assets_modified -or
    $Isolation.game_instance_subsystem_modified -or $Isolation.causal_core_modified -or
    $Isolation.save_schema_modified -or $Isolation.new_background_asset_over_50mb) {
    throw 'Packaged G3A isolation evidence failed.'
}
if (-not $AutoFollow.standard_autofollow_observed -or -not $AutoFollow.manual_drag_priority_observed -or
    -not $AutoFollow.manual_release_hold_observed -or -not $AutoFollow.standard_autofollow_resumed_after_drag -or
    -not $AutoFollow.tactical_autofollow_disabled -or -not $AutoFollow.return_standard_autofollow_observed) {
    throw 'Packaged G3A Standard camera auto-follow evidence failed.'
}

$G2B = Invoke-PackagedCheck -Name 'g2b' `
    -Arguments @('/Game/Maps/StageG2B_GuardM_PoC','-StageG2BEvidence','-StageG2BEvidenceExit') `
    -RequiredPatterns @(
        'STAGE_G1A_READY map=.*StageG2B_GuardM_PoC',
        'STAGE_G2B_GUARD_M_READY .*height=175.00 scale=1.0 material_slots=1 player_skeleton_shared=false placement=1',
        'STAGE_G2B_GUARD_M_EVIDENCE_WRITTEN files=5 guard=1 bones=24 root=Hips material=1 standard=1 tactical=1 return=1 player_fallback=0 skeleton_shared=0 triangles=178245'
    )

$G2A = Invoke-PackagedCheck -Name 'g2a' `
    -Arguments @('/Game/Maps/StageG2A_CameraRedesignPoC','-StageG2AEvidence','-StageG2AEvidenceExit') `
    -RequiredPatterns @(
        'STAGE_G1A_READY map=.*StageG2A_CameraRedesignPoC',
        'STAGE_G1B_PLAYER_M_READY .*fallback=false',
        'STAGE_G2A_REDESIGN_EVIDENCE_WRITTEN files=4 mode=standard tactical=1 return=1 destination=1 path=1 movement_mode=1 collision=1 recovery=1'
    )

$G1B = Invoke-PackagedCheck -Name 'g1b' `
    -Arguments @('/Game/Maps/StageG1B_OriginalPlayerPoC','-StageG1BEvidence','-StageG1BEvidenceExit') `
    -RequiredPatterns @(
        'STAGE_G1A_READY map=.*StageG1B_OriginalPlayerPoC',
        'STAGE_G1B_PLAYER_M_READY .*idle=.*Idle11.*walk=.*Walking.*run=.*Running.*dash=.*Run02.*fallback=false provisional=false',
        'STAGE_G1B_EVIDENCE_WRITTEN files=9 idle_anim=1 walk=250.0 run=500.0 dash=750.0 walk_anim=1 run_anim=1 dash_anim=1 idle_return=1 fallback=false'
    )

$PackageEvidence = [ordered]@{
    created_at=(Get-Date).ToString('o')
    executable=$Executable
    executable_sha256=(Get-FileHash -LiteralPath $Executable -Algorithm SHA256).Hash
    development_package='PASS'
    no_argument_g3a_map='PASS'
    explicit_g2b_map='PASS'
    explicit_g2a_map='PASS'
    explicit_g1b_map='PASS'
    navmesh_routes='10/10 PASS'
    click_move_surface='6/6 PASS'
    actual_player_movement='PASS'
    guard_actor_count=1
    player_fallback=$false
    guard_player_skeleton_shared=$false
    new_background_asset_over_50mb=0
    external_communication=0
}
$PackageEvidence | ConvertTo-Json -Depth 5 |
    Set-Content -LiteralPath (Join-Path $Reports 'package_evidence.json') -Encoding utf8NoBOM

Write-Host "STAGE_G3A_CAPITAL_PACKAGE_PASS executable=$Executable sha256=$($PackageEvidence.executable_sha256)"
