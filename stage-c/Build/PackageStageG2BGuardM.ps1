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
$Output = Join-Path $RepositoryRoot 'out\stage-g2b-guardm'
$Reports = Join-Path $ProjectRoot 'SourceArt\StageG2B\GUARD_M\Meshy\v0.1\Working\Reports'
if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) { $ArchiveDirectory = Join-Path $Output 'package' }
New-Item -ItemType Directory -Force -Path $Output,$Reports,$ArchiveDirectory | Out-Null

$Required = @(
    $Project,$Build,$RunUat,
    (Join-Path $ProjectRoot 'Content\Maps\StageG2B_GuardM_PoC.umap'),
    (Join-Path $ProjectRoot 'Content\Maps\StageG2A_CameraRedesignPoC.umap'),
    (Join-Path $ProjectRoot 'Content\Maps\StageG1B_OriginalPlayerPoC.umap')
)
foreach ($Path in $Required) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "Required Stage G-2B package input is missing: $Path" }
}

$BuildLog = Join-Path $Output 'package_build.log'
& $Build NationSimulationStageC Win64 Development "-Project=$Project" `
    -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=1 *> $BuildLog
if ($LASTEXITCODE -ne 0) { throw "Stage G-2B Win64 Development build failed with exit code $LASTEXITCODE" }

$UatLog = Join-Path $Output 'package_uat.log'
& $RunUat BuildCookRun "-project=$Project" -noP4 -platform=Win64 -clientconfig=Development `
    -cook -stage -pak -package -archive "-archivedirectory=$ArchiveDirectory" `
    '-map=/Game/Maps/StageG2B_GuardM_PoC+/Game/Maps/StageG2A_CameraRedesignPoC+/Game/Maps/StageG1B_OriginalPlayerPoC' `
    -utf8output *> $UatLog
if ($LASTEXITCODE -ne 0) { throw "Stage G-2B Development packaging failed with exit code $LASTEXITCODE" }

$Executable = Get-ChildItem -LiteralPath $ArchiveDirectory -Recurse -Filter 'NationSimulationStageC.exe' |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $Executable) { throw "Packaged executable was not found under $ArchiveDirectory" }
$RunId = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')

function Invoke-PackagedCheck {
    param(
        [string]$Name,
        [string[]]$Arguments,
        [string[]]$RequiredPatterns,
        [int]$TimeoutMilliseconds = 90000
    )
    $UserDirectory = Join-Path $Output "package-user-$Name\$RunId"
    New-Item -ItemType Directory -Force -Path $UserDirectory | Out-Null
    $AllArguments = @($Arguments) + @('-nullrhi','-unattended','-nosound','-log',"-UserDir=$UserDirectory")
    $Process = Start-Process -FilePath $Executable -ArgumentList $AllArguments `
        -WorkingDirectory (Split-Path -Parent $Executable) -WindowStyle Hidden -PassThru
    if (-not $Process.WaitForExit($TimeoutMilliseconds)) {
        Stop-Process -Id $Process.Id -Force
        throw "Packaged Stage G-2B check timed out: $Name"
    }
    if ($Process.ExitCode -ne 0) { throw "Packaged Stage G-2B check exited with code $($Process.ExitCode): $Name" }
    $RuntimeLog = Get-ChildItem -LiteralPath $UserDirectory -Recurse -Filter 'NationSimulationStageC.log' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $RuntimeLog) { throw "Packaged Stage G-2B check did not create a runtime log: $Name" }
    $RuntimeText = Get-Content -Raw -LiteralPath $RuntimeLog.FullName
    foreach ($Pattern in $RequiredPatterns) {
        if ($RuntimeText -notmatch $Pattern) { throw "Packaged Stage G-2B marker is missing ($Name): $Pattern" }
    }
    if ($RuntimeText -match 'Fatal error|STAGE_G2B_GUARD_M_FAIL|STAGE_G2A_REDESIGN_FAIL|STAGE_G1B_PLAYER_M_FAIL') {
        throw "Packaged Stage G-2B runtime contains a fatal accepted-system failure: $Name"
    }
    return [ordered]@{ name=$Name; user_directory=$UserDirectory; runtime_log=$RuntimeLog.FullName; exit_code=$Process.ExitCode }
}

$G2B = Invoke-PackagedCheck -Name 'g2b' `
    -Arguments @('-StageG2BEvidence','-StageG2BEvidenceExit') `
    -RequiredPatterns @(
        'STAGE_G1A_READY map=.*StageG2B_GuardM_PoC',
        'STAGE_G1B_PLAYER_M_READY .*height=175.00 scale=1.0 fallback=false provisional=false',
        'STAGE_G2A_REDESIGN_READY initial_mode=StandardCharacterCamera .*toggle=F6 collision=1 player_follow=1',
        'STAGE_G2B_GUARD_M_READY .*height=175.00 scale=1.0 material_slots=1 player_skeleton_shared=false placement=1',
        'STAGE_G2B_GUARD_M_EVIDENCE_WRITTEN files=5 guard=1 bones=24 root=Hips material=1 standard=1 tactical=1 return=1 player_fallback=0 skeleton_shared=0 triangles=178245'
    )

$EvidenceNames = @('guard_runtime_evidence.json','guard_animation_evidence.json',
    'guard_camera_evidence.json','guard_lod_evidence.json','guard_isolation_evidence.json')
foreach ($Name in $EvidenceNames) {
    $Evidence = Get-ChildItem -LiteralPath $G2B.user_directory -Recurse -Filter $Name -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $Evidence) { throw "Packaged Stage G-2B evidence was not written: $Name" }
    Copy-Item -LiteralPath $Evidence.FullName -Destination (Join-Path $Reports $Name) -Force
}

$Runtime = Get-Content -Raw -LiteralPath (Join-Path $Reports 'guard_runtime_evidence.json') | ConvertFrom-Json
$Animation = Get-Content -Raw -LiteralPath (Join-Path $Reports 'guard_animation_evidence.json') | ConvertFrom-Json
$Camera = Get-Content -Raw -LiteralPath (Join-Path $Reports 'guard_camera_evidence.json') | ConvertFrom-Json
$Isolation = Get-Content -Raw -LiteralPath (Join-Path $Reports 'guard_isolation_evidence.json') | ConvertFrom-Json
if ($Runtime.guard_actor_count -ne 1 -or $Runtime.bone_count -ne 24 -or $Runtime.root_bone -ne 'Hips' -or
    $Runtime.material_slot_count -ne 1 -or $Runtime.triangles_lod0 -ne 178245 -or
    $Runtime.player_fallback -or $Runtime.player_skeleton_shared) { throw 'Packaged G2B GUARD_M runtime evidence failed.' }
if (-not $Animation.guard_source_idle_11 -or -not $Animation.is_playing -or $Animation.reference_pose -or
    $Animation.retarget_used -or $Animation.player_animation_modified) { throw 'Packaged G2B GUARD_M animation evidence failed.' }
if (-not $Camera.standard_initial -or -not $Camera.tactical_reached -or -not $Camera.returned_to_standard -or
    -not $Camera.standard_visibility_distance) { throw 'Packaged G2B camera evidence failed.' }
if ($Isolation.player_skeleton_shared -or $Isolation.player_mesh_modified -or $Isolation.player_animation_modified -or
    $Isolation.causal_core_connected -or $Isolation.save_schema_connected) { throw 'Packaged G2B isolation evidence failed.' }

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
    ) `
    -TimeoutMilliseconds 120000

$PackageEvidence = [ordered]@{
    created_at=(Get-Date).ToString('o')
    executable=$Executable
    executable_sha256=(Get-FileHash -LiteralPath $Executable -Algorithm SHA256).Hash
    development_package='PASS'
    no_argument_g2b_map='PASS'
    explicit_g2a_map='PASS'
    explicit_g1b_map='PASS'
    guard_actor_count=1
    player_fallback=$false
    player_skeleton_shared=$false
    external_communication=0
}
$PackageEvidence | ConvertTo-Json -Depth 5 |
    Set-Content -LiteralPath (Join-Path $Reports 'package_evidence.json') -Encoding utf8NoBOM

Write-Host "STAGE_G2B_GUARD_M_PACKAGE_PASS executable=$Executable sha256=$($PackageEvidence.executable_sha256)"
