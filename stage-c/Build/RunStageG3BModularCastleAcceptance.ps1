[CmdletBinding()]
param(
    [string]$EngineRoot = $(if ($env:UE_5_8_ROOT) { $env:UE_5_8_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }),
    [switch]$SkipRegression,
    [switch]$SkipPackage,
    [switch]$SkipScreenshots
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$RepositoryRoot = (Resolve-Path (Join-Path $ProjectRoot '..')).Path
$Project = Join-Path $ProjectRoot 'NationSimulationStageC.uproject'
$Build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Output = Join-Path $RepositoryRoot 'out\stage-g3br-modular-castle'
$TestOutput = Join-Path $Output 'test-output'
$Reports = Join-Path $ProjectRoot 'SourceArt\StageG3B\ModularCastle\v0.1\Reports'
$Screenshots = Join-Path $ProjectRoot 'SourceArt\StageG3B\ModularCastle\v0.1\Screenshots'
$SavedEvidence = Join-Path $ProjectRoot 'Saved\StageG3BR'
$SavedScreenshots = Join-Path $SavedEvidence 'Screenshots'
$Archive = 'C:\Users\rinpa\Desktop\TITLE_StageG3B_Rejected_PrimitiveCastle_Archive_20260717_095537'
New-Item -ItemType Directory -Force -Path $TestOutput,$Reports,$Screenshots,$SavedEvidence,$SavedScreenshots | Out-Null

$Protected = [ordered]@{
    (Join-Path $ProjectRoot 'Content\Maps\StageG3A_CapitalBlockout_PoC.umap') = '0F4485410D0E3D74B8EA386F1BBBD57D1B66E9BC80056E5EF51F42E8CEFEC190'
    (Join-Path $ProjectRoot 'Content\Maps\StageG2B_GuardM_PoC.umap') = '71FE747D80E83C990D328FF879FA3AF837D2F01724D35F66C548EBD8AC069D59'
    (Join-Path $ProjectRoot 'Content\Maps\StageG2A_CameraRedesignPoC.umap') = '23D273BB49538EA1365DBDBC67074701B83CC5CBC8C632BB73E27B241196C6E6'
    (Join-Path $ProjectRoot 'Content\Maps\StageG1B_OriginalPlayerPoC.umap') = '623AF018D2682CA9CB573CA891CD6DF1B118584F260623282E2F874582FD062E'
    (Join-Path $ProjectRoot 'Content\StageG1B\Characters\PLAYER_M\MeshyV01\Mesh\SK_PLAYER_M_Meshy_v0_1.uasset') = '7A3BB33EAE59D193BC05FE2F2F8A4554E8B19330DC0E7F88B697DE506D2DB9AB'
    (Join-Path $ProjectRoot 'Content\StageG1B\Characters\PLAYER_M\MeshyV01\Skeleton\SKEL_PLAYER_M_Meshy_v0_1.uasset') = 'C81A688AEBCE5C23DED677DA81EC743AD6AE013BD443BFE57583CD91EF0FE5A0'
    (Join-Path $ProjectRoot 'Content\StageG2B\Characters\GUARD_M\MeshyV01\Mesh\SK_GUARD_M_Meshy_v0_1.uasset') = '7E8465ACA1672BDD69229ACB11073DC9681A21B813B45D0CE578E161E350C36D'
    (Join-Path $ProjectRoot 'Content\StageG2B\Characters\GUARD_M\MeshyV01\Skeleton\SKEL_GUARD_M_Meshy_v0_1.uasset') = '130AEE389FB34E46B828AD3EB698891CCD9DA67E8DA9517703780C7F95FB3316'
    (Join-Path $ProjectRoot 'Source\NationSimulationStageC\Public\StageG2A\StageG2ACameraModeAdapter.h') = '5D49D19F69CE878075A231978ABBBEDB4FACE442C4B77B7EF76778AF2833B487'
    (Join-Path $ProjectRoot 'Source\NationSimulationStageC\Private\StageG2A\StageG2ACameraModeAdapter.cpp') = '82D4C383BD8980123F5FDB15B407F06F0934A3055DF2055155880DF162A9900D'
    (Join-Path $ProjectRoot 'Source\NationSimulationStageC\Private\StageD\NationSimulationGameInstanceSubsystem.cpp') = 'ABBC6D31ED932F249575748EAC4EE3CC35B86D105BA2AC08EE08ECE110DA7ACD'
    (Join-Path $RepositoryRoot 'data\stage_a_fixture.json') = '62F9B1BAB98FB0E32BB5DB2C7FAAA7F31E9B40DDCF85C94A8ADC46DFACA98A2B'
    (Join-Path $RepositoryRoot 'data\stage_parity_contract.json') = 'B8D4B5D356731F96EFB0779095E725337E84252F15A829E2A1B922E1DF99DED3'
    (Join-Path $ProjectRoot 'Data\StageE\stage_e_state_definitions.json') = '9913D473DF3A1BE8B71400150E2D76A7F6B46A66DADC0F218027346F0978B833'
    (Join-Path $ProjectRoot 'Data\StageF\stage_f_save_schema.json') = '596576DC37D71B7D31E2B28E219309E27E4BAF1C03E49DF6C375A1A9B0B92DB5'
}
foreach ($Item in $Protected.GetEnumerator()) {
    if (-not (Test-Path -LiteralPath $Item.Key) -or
        (Get-FileHash -LiteralPath $Item.Key -Algorithm SHA256).Hash -ne $Item.Value) {
        throw "Protected baseline mismatch: $($Item.Key)"
    }
}

$ArchiveVerification = Get-Content -Raw -LiteralPath (Join-Path $Archive 'archive_verification.json') | ConvertFrom-Json
$ArchiveManifest = Get-Content -Raw -LiteralPath (Join-Path $Archive 'archive_sha256_manifest.json') | ConvertFrom-Json
if ($ArchiveVerification.status -ne 'PASS' -or -not $ArchiveManifest.all_sha_match -or
    $ArchiveVerification.file_count -ne 5284) {
    throw 'Rejected G-3B archive SHA contract failed.'
}

$Large = @(
    Get-ChildItem -LiteralPath (Join-Path $ProjectRoot 'Content\StageG3B') -File -Recurse
    Get-ChildItem -LiteralPath (Join-Path $ProjectRoot 'SourceArt\StageG3B') -File -Recurse
) | Where-Object Length -gt 50MB
if (@($Large).Count -ne 0) { throw 'G-3B-R contains a file over 50MB.' }

$BuildLog = Join-Path $TestOutput 'editor_build.log'
& $Build NationSimulationStageCEditor Win64 Development "-Project=$Project" `
    -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=1 *> $BuildLog
if ($LASTEXITCODE -ne 0) { throw "G-3B-R Editor build failed with exit code $LASTEXITCODE" }

Get-ChildItem -LiteralPath $SavedEvidence -Filter '*.json' -ErrorAction SilentlyContinue |
    ForEach-Object { [IO.File]::Delete($_.FullName) }
$RuntimeLog = Join-Path $TestOutput 'runtime_evidence.log'
& $Editor $Project '/Game/Maps/StageG3B_ModularCastle_PoC' -game -unattended -nop4 -nosplash -nullrhi -NoSound `
    -StageG3BREvidence -StageG3BREvidenceExit "-abslog=$RuntimeLog" *> (Join-Path $TestOutput 'runtime_console.log')
if ($LASTEXITCODE -ne 0) { throw "G-3B-R runtime evidence failed: $LASTEXITCODE" }
$RuntimeText = Get-Content -Raw -LiteralPath $RuntimeLog
if ($RuntimeText -notmatch
    'STAGE_G3BR_MODULAR_EVIDENCE_WRITTEN files=5 modules=22 walls=11 gate=1 tunnel=2 towers=2/1 spires=4 roofs=13 buttresses=14 windows=20 crenels=11 banners=5 magic=3 guard=1 routes=5/5 move=1 moved=[1-9][0-9]{3,}[^ ]* fallback=0 skeleton_shared=0 collision=16/16') {
    throw 'G-3B-R runtime evidence PASS marker is missing.'
}
$EvidenceNames = @('modular_castle_runtime_evidence.json','modular_castle_navmesh_evidence.json',
    'modular_castle_camera_evidence.json','modular_castle_visual_contract_evidence.json',
    'modular_castle_asset_isolation_evidence.json')
foreach ($Name in $EvidenceNames) {
    $Source = Join-Path $SavedEvidence $Name
    if (-not (Test-Path -LiteralPath $Source)) { throw "Runtime evidence missing: $Name" }
    Copy-Item -LiteralPath $Source -Destination (Join-Path $Reports $Name) -Force
}

if (-not $SkipScreenshots) {
    $Names = @(
        '01_modular_castle_tactical_overview.png','02_modular_castle_standard_approach.png',
        '03_modular_castle_gate_standard.png','04_modular_castle_plaza_standard.png',
        '05_modular_castle_towers_front.png','06_modular_castle_gate_depth.png',
        '07_modular_castle_wall_detail.png','08_modular_castle_side_silhouette.png',
        '09_modular_castle_magic_markers.png','10_modular_castle_collision_gate.png',
        '11_modular_castle_collision_wall.png','12_modular_castle_nav_route_plaza_to_gate.png',
        '13_modular_castle_return_tactical.png','14_modular_castle_return_standard_autofollow.png',
        '15_modular_castle_player_scale.png','16_modular_castle_gate_from_city_entrance.png')
    Get-ChildItem -LiteralPath $SavedScreenshots -Filter '*.png' -ErrorAction SilentlyContinue |
        ForEach-Object { [IO.File]::Delete($_.FullName) }
    $ScreenshotLog = Join-Path $TestOutput 'screenshots.log'
    & $Editor $Project '/Game/Maps/StageG3B_ModularCastle_PoC' -game -unattended -nop4 -nosplash -NoSound `
        -RenderOffscreen -ResX=1280 -ResY=720 -StageG3BRScreenshots -StageG3BRScreenshotsExit `
        "-abslog=$ScreenshotLog" *> (Join-Path $TestOutput 'screenshots_console.log')
    if ((Get-Content -Raw -LiteralPath $ScreenshotLog) -notmatch
        'STAGE_G3BR_MODULAR_SCREENSHOTS_REQUESTED count=16') {
        throw 'G-3B-R screenshot marker is missing.'
    }
    $Manifest = @()
    foreach ($Name in $Names) {
        $Source = Join-Path $SavedScreenshots $Name
        if (-not (Test-Path -LiteralPath $Source)) { throw "Screenshot missing: $Name" }
        $Bytes = [IO.File]::ReadAllBytes($Source)
        $Width = [Net.IPAddress]::NetworkToHostOrder([BitConverter]::ToInt32($Bytes,16))
        $Height = [Net.IPAddress]::NetworkToHostOrder([BitConverter]::ToInt32($Bytes,20))
        if ($Width -ne 1280 -or $Height -ne 720) { throw "Screenshot size invalid: $Name" }
        $Destination = Join-Path $Screenshots $Name
        Copy-Item -LiteralPath $Source -Destination $Destination -Force
        $Manifest += [ordered]@{file=$Name;width=$Width;height=$Height;bytes=(Get-Item $Destination).Length;
            sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $Destination).Hash}
    }
    [ordered]@{created_at=(Get-Date).ToString('o');image_count=16;images=$Manifest} |
        ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $Reports 'screenshot_manifest.json') -Encoding utf8NoBOM
}

$AutomationLog = Join-Path $TestOutput 'automation.log'
& $Editor $Project -unattended -nop4 -nosplash -nullrhi -NoSound `
    '-ExecCmds=Automation RunTests NationSimulation.StageG3B.ModularCastle;Quit' `
    '-TestExit=Automation Test Queue Empty' "-abslog=$AutomationLog"
if ($LASTEXITCODE -ne 0) { throw "G-3B-R Automation failed: $LASTEXITCODE" }
$AutomationResults = Join-Path $TestOutput 'stage_g3br_modular_automation_results.txt'
if (-not (Test-Path -LiteralPath $AutomationResults) -or
    (Get-Content -LiteralPath $AutomationResults | Select-Object -Last 1) -ne 'SUMMARY | 33/33 tests passed') {
    throw 'G-3B-R Automation did not pass 33/33.'
}

if (-not $SkipRegression) {
    $RegressionLog = Join-Path $Output 'regression\stage_c_through_g3br_unreal.log'
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $RegressionLog) | Out-Null
    & $Editor $Project -unattended -nop4 -nosplash -nullrhi -NoSound `
        '-ExecCmds=Automation RunTests NationSimulation.Stage;Quit' `
        '-TestExit=Automation Test Queue Empty' "-abslog=$RegressionLog"
    if ($LASTEXITCODE -ne 0) { throw 'Stage C through G-3B-R Unreal regression failed.' }
    $Contracts = [ordered]@{
        (Join-Path $RepositoryRoot 'out\stage-c\test-output\acceptance_results.txt') = 'SUMMARY | 11/11 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-d\test-output\stage_d_acceptance_results.txt') = 'SUMMARY | 9/9 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-d\fix-output\stage_d_fix_results.txt') = 'SUMMARY | 8/8 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-f\test-output\stage_f_unreal_results.txt') = 'SUMMARY | 5/5 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-g0\test-output\stage_g0_automation_results.txt') = 'SUMMARY | 33/33 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-g1b-meshy\test-output\stage_g1b_meshy_automation_results.txt') = 'SUMMARY | 30/30 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-g2a-redesign\test-output\stage_g2a_redesign_automation_results.txt') = 'SUMMARY | 18/18 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-g2b-guardm\test-output\stage_g2b_guardm_automation_results.txt') = 'SUMMARY | 18/18 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-g3a-capital-blockout\test-output\stage_g3a_capital_automation_results.txt') = 'SUMMARY | 29/29 tests passed'
        $AutomationResults = 'SUMMARY | 33/33 tests passed'
    }
    foreach ($Contract in $Contracts.GetEnumerator()) {
        if (-not (Test-Path -LiteralPath $Contract.Key) -or
            (Get-Content -LiteralPath $Contract.Key | Select-Object -Last 1) -ne $Contract.Value) {
            throw "Regression contract failed: $($Contract.Key)"
        }
    }
}

foreach ($Item in $Protected.GetEnumerator()) {
    if ((Get-FileHash -LiteralPath $Item.Key -Algorithm SHA256).Hash -ne $Item.Value) {
        throw "Protected baseline changed during acceptance: $($Item.Key)"
    }
}

if (-not $SkipPackage) {
    & (Join-Path $PSScriptRoot 'PackageStageG3BModularCastle.ps1') -EngineRoot $EngineRoot
    if ($LASTEXITCODE -ne 0) { throw 'G-3B-R Package verification failed.' }
}

@(
    'PASS | Rejected G-3B archive 5,284/5,284 SHA-256'
    'PASS | G-3B-R Modular Castle Automation 33/33'
    'PASS | NavMesh routes 5/5 and actual PLAYER_M movement'
    'PASS | Standard/Tactical camera, auto-follow, and collision isolation'
    $(if ($SkipScreenshots) { 'SKIP | 16 evaluation screenshots' } else { 'PASS | 16 evaluation screenshots 1280x720' })
    $(if ($SkipRegression) { 'SKIP | Stage C through G-3A regression' } else { 'PASS | Stage C through G-3A regression' })
    $(if ($SkipPackage) { 'SKIP | Win64 Development Package' } else { 'PASS | Win64 Development Package and G3BR/G3A/G2B/G2A/G1B launch checks' })
    'PENDING | 豆虎Gate 王城外観実機確認'
    'SUMMARY | Stage G-3B-R technically verified; visual acceptance pending'
) | Set-Content -LiteralPath (Join-Path $Output 'stage_g3br_modular_acceptance_results.txt') -Encoding utf8NoBOM

Write-Host 'STAGE_G3BR_MODULAR_AUTOMATION 33/33 PASS'
Write-Host $(if ($SkipRegression) { 'STAGE_G3BR_REGRESSION SKIP' } else { 'STAGE_C_THROUGH_G3BR_REGRESSION PASS' })
Write-Host $(if ($SkipPackage) { 'STAGE_G3BR_PACKAGE SKIP' } else { 'STAGE_G3BR_PACKAGE PASS' })
