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
$Output = Join-Path $RepositoryRoot 'out\stage-g3a-capital-blockout'
$TestOutput = Join-Path $Output 'test-output'
$Reports = Join-Path $ProjectRoot 'SourceArt\StageG3A\CapitalBlockout\v0.1\Reports'
$Screenshots = Join-Path $ProjectRoot 'SourceArt\StageG3A\CapitalBlockout\v0.1\Screenshots'
$SavedEvidence = Join-Path $ProjectRoot 'Saved\StageG3A'
$SavedScreenshots = Join-Path $SavedEvidence 'Screenshots'

$Protected = [ordered]@{
    (Join-Path $ProjectRoot 'Content\Maps\StageG2B_GuardM_PoC.umap') = '71FE747D80E83C990D328FF879FA3AF837D2F01724D35F66C548EBD8AC069D59'
    (Join-Path $ProjectRoot 'Content\Maps\StageG2A_CameraRedesignPoC.umap') = '23D273BB49538EA1365DBDBC67074701B83CC5CBC8C632BB73E27B241196C6E6'
    (Join-Path $ProjectRoot 'Content\Maps\StageG1B_OriginalPlayerPoC.umap') = '623AF018D2682CA9CB573CA891CD6DF1B118584F260623282E2F874582FD062E'
    (Join-Path $ProjectRoot 'Content\StageG1B\Characters\PLAYER_M\MeshyV01\Mesh\SK_PLAYER_M_Meshy_v0_1.uasset') = '7A3BB33EAE59D193BC05FE2F2F8A4554E8B19330DC0E7F88B697DE506D2DB9AB'
    (Join-Path $ProjectRoot 'Content\StageG1B\Characters\PLAYER_M\MeshyV01\Skeleton\SKEL_PLAYER_M_Meshy_v0_1.uasset') = 'C81A688AEBCE5C23DED677DA81EC743AD6AE013BD443BFE57583CD91EF0FE5A0'
    (Join-Path $ProjectRoot 'Content\StageG2B\Characters\GUARD_M\MeshyV01\Mesh\SK_GUARD_M_Meshy_v0_1.uasset') = '7E8465ACA1672BDD69229ACB11073DC9681A21B813B45D0CE578E161E350C36D'
    (Join-Path $ProjectRoot 'Content\StageG2B\Characters\GUARD_M\MeshyV01\Skeleton\SKEL_GUARD_M_Meshy_v0_1.uasset') = '130AEE389FB34E46B828AD3EB698891CCD9DA67E8DA9517703780C7F95FB3316'
    (Join-Path $ProjectRoot 'Source\NationSimulationStageC\Public\StageG2A\StageG2ACameraModeAdapter.h') = '5D49D19F69CE878075A231978ABBBEDB4FACE442C4B77B7EF76778AF2833B487'
    (Join-Path $ProjectRoot 'Source\NationSimulationStageC\Private\StageG2A\StageG2ACameraModeAdapter.cpp') = '82D4C383BD8980123F5FDB15B407F06F0934A3055DF2055155880DF162A9900D'
    (Join-Path $ProjectRoot 'Source\NationSimulationStageC\Public\StageG2B\StageG2BGuardMActor.h') = 'EEFD290741500ED67AFC1885FA61969BDDC34ABF1700C78F8DAC803896B928F2'
    (Join-Path $ProjectRoot 'Source\NationSimulationStageC\Private\StageG2B\StageG2BGuardMActor.cpp') = 'BE5B544089AABB6EE8E3A272382F7FD9667F830B2782EEDF9B26A6F233FE8E91'
    (Join-Path $ProjectRoot 'Source\NationSimulationStageC\Private\StageD\NationSimulationGameInstanceSubsystem.cpp') = 'ABBC6D31ED932F249575748EAC4EE3CC35B86D105BA2AC08EE08ECE110DA7ACD'
    (Join-Path $RepositoryRoot 'data\stage_a_fixture.json') = '62F9B1BAB98FB0E32BB5DB2C7FAAA7F31E9B40DDCF85C94A8ADC46DFACA98A2B'
    (Join-Path $RepositoryRoot 'data\stage_parity_contract.json') = 'B8D4B5D356731F96EFB0779095E725337E84252F15A829E2A1B922E1DF99DED3'
    (Join-Path $ProjectRoot 'Data\StageE\stage_e_state_definitions.json') = '9913D473DF3A1BE8B71400150E2D76A7F6B46A66DADC0F218027346F0978B833'
    (Join-Path $ProjectRoot 'Data\StageF\stage_f_save_schema.json') = '596576DC37D71B7D31E2B28E219309E27E4BAF1C03E49DF6C375A1A9B0B92DB5'
}

New-Item -ItemType Directory -Force -Path $TestOutput,$Reports,$Screenshots,$SavedEvidence | Out-Null
foreach ($Item in $Protected.GetEnumerator()) {
    if (-not (Test-Path -LiteralPath $Item.Key) -or
        (Get-FileHash -LiteralPath $Item.Key -Algorithm SHA256).Hash -ne $Item.Value) {
        throw "Protected Stage G-3A baseline mismatch: $($Item.Key)"
    }
}

$Large = @(Get-ChildItem -LiteralPath (Join-Path $ProjectRoot 'Content\StageG3A') -File -Recurse |
    Where-Object Length -gt 50MB)
if ($Large.Count -ne 0) { throw 'Stage G-3A contains a new background Content asset over 50MB.' }

$BuildLog = Join-Path $TestOutput 'editor_build.log'
& $Build NationSimulationStageCEditor Win64 Development "-Project=$Project" `
    -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=1 *> $BuildLog
if ($LASTEXITCODE -ne 0) { throw "Stage G-3A Editor build failed with exit code $LASTEXITCODE" }

$RuntimeLog = Join-Path $TestOutput 'stage_g3a_capital_runtime.log'
& $Editor $Project '/Game/Maps/StageG3A_CapitalBlockout_PoC' -game -unattended -nop4 -nosplash -nullrhi -NoSound `
    -StageG3AEvidence -StageG3AEvidenceExit "-abslog=$RuntimeLog"
if ($LASTEXITCODE -ne 0) { throw "Stage G-3A runtime evidence failed with exit code $LASTEXITCODE" }
$RuntimeText = Get-Content -Raw -LiteralPath $RuntimeLog
if ($RuntimeText -notmatch
    'STAGE_G3A_CAPITAL_EVIDENCE_WRITTEN files=5 zones=9 guard=1 nav_routes=10/10 click_traces=6/6 move_command=1 moved=[3-9][0-9]{3,}[^ ]* move_pass=1 traversal=1 .*autofollow=1 manual=1 resume=1 tactical_no_follow=1 return_follow=1 fallback=0 skeleton_shared=0 oversized=0') {
    throw 'Stage G-3A runtime evidence PASS marker is missing.'
}
$EvidenceNames = @('capital_runtime_evidence.json','navmesh_route_evidence.json',
    'camera_collision_evidence.json','asset_isolation_evidence.json','camera_autofollow_evidence.json')
foreach ($Name in $EvidenceNames) {
    $Source = Join-Path $SavedEvidence $Name
    if (-not (Test-Path -LiteralPath $Source)) { throw "Stage G-3A runtime evidence missing: $Name" }
    Copy-Item -LiteralPath $Source -Destination (Join-Path $Reports $Name) -Force
}

$AutomationLog = Join-Path $TestOutput 'stage_g3a_capital_automation.log'
& $Editor $Project -unattended -nop4 -nosplash -nullrhi -NoSound `
    '-ExecCmds=Automation RunTests NationSimulation.StageG3A.Capital;Quit' `
    '-TestExit=Automation Test Queue Empty' "-abslog=$AutomationLog"
if ($LASTEXITCODE -ne 0) { throw "Stage G-3A automation failed with exit code $LASTEXITCODE" }
$AutomationResults = Join-Path $TestOutput 'stage_g3a_capital_automation_results.txt'
if (-not (Test-Path -LiteralPath $AutomationResults) -or
    (Get-Content -LiteralPath $AutomationResults | Select-Object -Last 1) -ne 'SUMMARY | 29/29 tests passed') {
    throw 'Stage G-3A automation did not pass 29/29.'
}

if (-not $SkipScreenshots) {
    $ScreenshotNames = @(
        '01_overview_tactical_full.png','02_south_gate_standard.png','03_south_gate_tactical.png',
        '04_main_street_standard.png','05_central_plaza_standard.png','06_central_plaza_tactical.png',
        '07_market_district.png','08_residential_district.png','09_noble_district.png',
        '10_workshop_district.png','11_castle_approach_standard.png','12_castle_overlook.png',
        '13_outskirts_farmland_forest.png','14_camera_collision_wall.png',
        '15_navmesh_route_gate_to_plaza.png','16_return_to_standard_after_tactical.png',
        'standard_autofollow_move_start.png','standard_autofollow_while_moving.png',
        'standard_manual_drag_override.png','standard_autofollow_resume_after_drag.png',
        'tactical_no_autofollow.png','return_standard_autofollow.png')
    New-Item -ItemType Directory -Force -Path $SavedScreenshots | Out-Null
    foreach ($Name in $ScreenshotNames) {
        $Path = Join-Path $SavedScreenshots $Name
        if (Test-Path -LiteralPath $Path) { Remove-Item -LiteralPath $Path -Force }
    }
    $ScreenshotLog = Join-Path $TestOutput 'stage_g3a_capital_screenshots.log'
    & $Editor $Project '/Game/Maps/StageG3A_CapitalBlockout_PoC' -game -unattended -nop4 -nosplash -NoSound `
        -RenderOffscreen -ResX=1280 -ResY=720 -StageG3AScreenshots -StageG3AScreenshotsExit `
        "-abslog=$ScreenshotLog"
    if ($LASTEXITCODE -ne 0) { throw "Stage G-3A screenshot capture failed with exit code $LASTEXITCODE" }
    if ((Get-Content -Raw -LiteralPath $ScreenshotLog) -notmatch
        'STAGE_G3A_CAPITAL_SCREENSHOTS_REQUESTED count=22') {
        throw 'Stage G-3A screenshot capture marker is missing.'
    }
    $Manifest = @()
    foreach ($Name in $ScreenshotNames) {
        $Source = Join-Path $SavedScreenshots $Name
        if (-not (Test-Path -LiteralPath $Source)) { throw "Stage G-3A screenshot missing: $Name" }
        $Bytes = [System.IO.File]::ReadAllBytes($Source)
        if ($Bytes.Length -lt 24) { throw "Stage G-3A screenshot is truncated: $Name" }
        $Width = [System.Net.IPAddress]::NetworkToHostOrder([BitConverter]::ToInt32($Bytes,16))
        $Height = [System.Net.IPAddress]::NetworkToHostOrder([BitConverter]::ToInt32($Bytes,20))
        if ($Width -ne 1280 -or $Height -ne 720) { throw "Stage G-3A screenshot dimensions invalid: $Name $Width x $Height" }
        $Destination = Join-Path $Screenshots $Name
        Copy-Item -LiteralPath $Source -Destination $Destination -Force
        $Manifest += [ordered]@{ file=$Name; width=$Width; height=$Height; bytes=$Bytes.Length;
            sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $Destination).Hash }
    }
    $Manifest | ConvertTo-Json -Depth 4 |
        Set-Content -LiteralPath (Join-Path $Reports 'screenshot_manifest.json') -Encoding utf8NoBOM
}

if (-not $SkipRegression) {
    $RegressionLog = Join-Path $Output 'regression\stage_c_through_g3a_unreal.log'
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $RegressionLog) | Out-Null
    & $Editor $Project -unattended -nop4 -nosplash -nullrhi -NoSound `
        '-ExecCmds=Automation RunTests NationSimulation.Stage;Quit' `
        '-TestExit=Automation Test Queue Empty' "-abslog=$RegressionLog"
    if ($LASTEXITCODE -ne 0) { throw 'Stage C through G-3A Unreal regression failed.' }

    $Contracts = [ordered]@{
        (Join-Path $RepositoryRoot 'out\stage-c\test-output\acceptance_results.txt') = 'SUMMARY | 11/11 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-d\test-output\stage_d_acceptance_results.txt') = 'SUMMARY | 9/9 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-d\fix-output\stage_d_fix_results.txt') = 'SUMMARY | 8/8 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-f\test-output\stage_f_unreal_results.txt') = 'SUMMARY | 5/5 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-g0\test-output\stage_g0_automation_results.txt') = 'SUMMARY | 33/33 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-g1a\test-output\stage_g1a_automation_results.txt') = 'SUMMARY | 22/22 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-g1b-meshy\test-output\stage_g1b_meshy_automation_results.txt') = 'SUMMARY | 30/30 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-g2a-redesign\test-output\stage_g2a_redesign_automation_results.txt') = 'SUMMARY | 18/18 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-g2b-guardm\test-output\stage_g2b_guardm_automation_results.txt') = 'SUMMARY | 18/18 tests passed'
        $AutomationResults = 'SUMMARY | 29/29 tests passed'
    }
    foreach ($Contract in $Contracts.GetEnumerator()) {
        if (-not (Test-Path -LiteralPath $Contract.Key) -or
            (Get-Content -LiteralPath $Contract.Key | Select-Object -Last 1) -ne $Contract.Value) {
            throw "Stage G-3A regression contract failed: $($Contract.Key)"
        }
    }
}

foreach ($Item in $Protected.GetEnumerator()) {
    if ((Get-FileHash -LiteralPath $Item.Key -Algorithm SHA256).Hash -ne $Item.Value) {
        throw "Protected baseline changed during Stage G-3A acceptance: $($Item.Key)"
    }
}

if (-not $SkipPackage) {
    & (Join-Path $PSScriptRoot 'PackageStageG3ACapitalBlockout.ps1') -EngineRoot $EngineRoot
    if ($LASTEXITCODE -ne 0) { throw 'Stage G-3A Package verification failed.' }
}

@(
    'PASS | Stage G-3A Capital Blockout Automation 29/29'
    'PASS | NavMesh routes 10/10 and single GUARD_M placement'
    'PASS | Standard/Tactical camera and protected asset isolation'
    $(if ($SkipScreenshots) { 'SKIP | 22 evaluation screenshots' } else { 'PASS | 22 evaluation screenshots 1280x720' })
    $(if ($SkipRegression) { 'SKIP | Stage C through G-2B regression' } else { 'PASS | Stage C through G-2B regression' })
    $(if ($SkipPackage) { 'SKIP | Win64 Development Package' } else { 'PASS | Win64 Development Package and G3A/G2B/G2A/G1B launch checks' })
    'PENDING | User real-machine capital Blockout confirmation'
    'SUMMARY | Stage G-3A technically verified; user acceptance pending'
) | Set-Content -LiteralPath (Join-Path $Output 'stage_g3a_capital_acceptance_results.txt') -Encoding utf8NoBOM

Write-Host 'STAGE_G3A_CAPITAL_AUTOMATION 29/29 PASS'
Write-Host $(if ($SkipRegression) { 'STAGE_G3A_REGRESSION SKIP' } else { 'STAGE_C_THROUGH_G3A_REGRESSION PASS' })
Write-Host $(if ($SkipPackage) { 'STAGE_G3A_PACKAGE SKIP' } else { 'STAGE_G3A_PACKAGE PASS' })
