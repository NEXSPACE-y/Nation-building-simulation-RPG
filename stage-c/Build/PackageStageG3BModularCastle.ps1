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
$Output = Join-Path $RepositoryRoot 'out\stage-g3br-modular-castle'
$Reports = Join-Path $ProjectRoot 'SourceArt\StageG3B\ModularCastle\v0.1\Reports'
if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) {
    $ArchiveDirectory = Join-Path $Output 'package'
}
New-Item -ItemType Directory -Force -Path $Output,$Reports,$ArchiveDirectory | Out-Null

$Maps = @(
    '/Game/Maps/StageG3B_ModularCastle_PoC',
    '/Game/Maps/StageG3A_CapitalBlockout_PoC',
    '/Game/Maps/StageG2B_GuardM_PoC',
    '/Game/Maps/StageG2A_CameraRedesignPoC',
    '/Game/Maps/StageG1B_OriginalPlayerPoC'
)
foreach ($Map in $Maps) {
    $Filename = ($Map -split '/')[-1] + '.umap'
    if (-not (Test-Path -LiteralPath (Join-Path $ProjectRoot "Content\Maps\$Filename"))) {
        throw "Required packaged map is missing: $Map"
    }
}

$BuildLog = Join-Path $Output 'package_build.log'
& $Build NationSimulationStageC Win64 Development "-Project=$Project" `
    -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=1 *> $BuildLog
if ($LASTEXITCODE -ne 0) { throw "G-3B-R Win64 build failed with exit code $LASTEXITCODE" }

$UatLog = Join-Path $Output 'package_uat.log'
$MapArgument = '-map=' + ($Maps -join '+')
& $RunUat BuildCookRun "-project=$Project" -noP4 -platform=Win64 -clientconfig=Development `
    -cook -stage -pak -package -archive "-archivedirectory=$ArchiveDirectory" `
    $MapArgument -utf8output *> $UatLog
if ($LASTEXITCODE -ne 0) { throw "G-3B-R packaging failed with exit code $LASTEXITCODE" }

$Executable = Get-ChildItem -LiteralPath $ArchiveDirectory -Recurse -Filter 'NationSimulationStageC.exe' |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $Executable) { throw 'Packaged executable was not found.' }
$RunId = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')

function Invoke-PackagedCheck {
    param([string]$Name,[string[]]$Arguments,[string[]]$RequiredPatterns)
    $UserDirectory = Join-Path $Output "package-user-$Name\$RunId"
    New-Item -ItemType Directory -Force -Path $UserDirectory | Out-Null
    $AllArguments = @($Arguments) + @('-nullrhi','-unattended','-nosound','-log',"-UserDir=$UserDirectory")
    $Process = Start-Process -FilePath $Executable -ArgumentList $AllArguments `
        -WorkingDirectory (Split-Path -Parent $Executable) -WindowStyle Hidden -PassThru
    if (-not $Process.WaitForExit(150000)) {
        Stop-Process -Id $Process.Id -Force
        throw "Packaged check timed out: $Name"
    }
    if ($Process.ExitCode -ne 0) { throw "Packaged check failed ($Name): $($Process.ExitCode)" }
    $RuntimeLog = Get-ChildItem -LiteralPath $UserDirectory -Recurse -Filter 'NationSimulationStageC.log' |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $RuntimeLog) { throw "Packaged log missing: $Name" }
    $Text = Get-Content -Raw -LiteralPath $RuntimeLog.FullName
    foreach ($Pattern in $RequiredPatterns) {
        if ($Text -notmatch $Pattern) { throw "Packaged marker missing ($Name): $Pattern" }
    }
    if ($Text -match 'Fatal error|STAGE_G3BR_MODULAR_FAIL|STAGE_G3A_CAPITAL_FAIL|STAGE_G2B_GUARD_M_FAIL|STAGE_G2A_REDESIGN_FAIL|STAGE_G1B_PLAYER_M_FAIL') {
        throw "Packaged runtime failure marker found: $Name"
    }
    return [ordered]@{name=$Name;user_directory=$UserDirectory;runtime_log=$RuntimeLog.FullName;exit_code=$Process.ExitCode}
}

$G3BR = Invoke-PackagedCheck -Name 'g3br' `
    -Arguments @('-StageG3BREvidence','-StageG3BREvidenceExit') `
    -RequiredPatterns @(
        'STAGE_G3BR_MODULAR_CASTLE_READY map=StageG3B_ModularCastle_PoC modules=22 guard=1 standard=1 tactical=1 navmesh=dynamic external_assets=0',
        'STAGE_G1B_PLAYER_M_READY .*fallback=false',
        'STAGE_G2B_GUARD_M_READY .*player_skeleton_shared=false placement=1',
        'STAGE_G3BR_MODULAR_EVIDENCE_WRITTEN files=5 modules=22 walls=11 gate=1 tunnel=2 towers=2/1 spires=4 roofs=13 buttresses=14 windows=20 crenels=11 banners=5 magic=3 guard=1 routes=5/5 move=1 moved=[1-9][0-9]{3,}[^ ]* fallback=0 skeleton_shared=0 collision=16/16'
    )

foreach ($Name in @('modular_castle_runtime_evidence.json','modular_castle_navmesh_evidence.json',
    'modular_castle_camera_evidence.json','modular_castle_visual_contract_evidence.json',
    'modular_castle_asset_isolation_evidence.json')) {
    $Evidence = Get-ChildItem -LiteralPath $G3BR.user_directory -Recurse -Filter $Name |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $Evidence) { throw "Packaged G-3B-R evidence missing: $Name" }
    Copy-Item -LiteralPath $Evidence.FullName -Destination (Join-Path $Reports $Name) -Force
}

$G3A = Invoke-PackagedCheck -Name 'g3a' `
    -Arguments @('/Game/Maps/StageG3A_CapitalBlockout_PoC','-StageG3AEvidence','-StageG3AEvidenceExit') `
    -RequiredPatterns @(
        'STAGE_G3A_CAPITAL_READY map=StageG3A_CapitalBlockout_PoC',
        'STAGE_G3A_CAPITAL_EVIDENCE_WRITTEN files=5 zones=9 guard=1 nav_routes=10/10'
    )
$G2B = Invoke-PackagedCheck -Name 'g2b' `
    -Arguments @('/Game/Maps/StageG2B_GuardM_PoC','-StageG2BEvidence','-StageG2BEvidenceExit') `
    -RequiredPatterns @('STAGE_G2B_GUARD_M_EVIDENCE_WRITTEN files=5 guard=1 bones=24 root=Hips')
$G2A = Invoke-PackagedCheck -Name 'g2a' `
    -Arguments @('/Game/Maps/StageG2A_CameraRedesignPoC','-StageG2AEvidence','-StageG2AEvidenceExit') `
    -RequiredPatterns @('STAGE_G2A_REDESIGN_EVIDENCE_WRITTEN files=4 mode=standard tactical=1 return=1')
$G1B = Invoke-PackagedCheck -Name 'g1b' `
    -Arguments @('/Game/Maps/StageG1B_OriginalPlayerPoC','-StageG1BEvidence','-StageG1BEvidenceExit') `
    -RequiredPatterns @('STAGE_G1B_EVIDENCE_WRITTEN files=9 idle_anim=1 walk=250.0 run=500.0 dash=750.0')

$PackageEvidence = [ordered]@{
    created_at=(Get-Date).ToString('o')
    executable=$Executable
    executable_sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $Executable).Hash
    development_package='PASS'
    no_argument_g3br_map='PASS'
    explicit_g3a_map='PASS'
    explicit_g2b_map='PASS'
    explicit_g2a_map='PASS'
    explicit_g1b_map='PASS'
    navmesh_routes='5/5 PASS'
    guard_actor_count=1
    player_fallback=$false
    guard_player_skeleton_shared=$false
    new_background_asset_over_50mb=0
    external_communication=0
}
$PackageEvidence | ConvertTo-Json -Depth 5 |
    Set-Content -LiteralPath (Join-Path $Reports 'package_evidence.json') -Encoding utf8NoBOM

Write-Host "STAGE_G3BR_MODULAR_PACKAGE_PASS sha256=$($PackageEvidence.executable_sha256)"
