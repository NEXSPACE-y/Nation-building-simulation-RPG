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
$Output = Join-Path $RepositoryRoot 'out\stage-g2b-guardm'
$TestOutput = Join-Path $Output 'test-output'
$Reports = Join-Path $ProjectRoot 'SourceArt\StageG2B\GUARD_M\Meshy\v0.1\Working\Reports'
$Screenshots = Join-Path $ProjectRoot 'SourceArt\StageG2B\GUARD_M\Meshy\v0.1\Screenshots'
$SavedEvidence = Join-Path $ProjectRoot 'Saved\StageG2B'
$SavedScreenshots = Join-Path $SavedEvidence 'Screenshots'

$Protected = [ordered]@{
    (Join-Path $ProjectRoot 'Content\Maps\StageG2A_CameraRedesignPoC.umap') = '23D273BB49538EA1365DBDBC67074701B83CC5CBC8C632BB73E27B241196C6E6'
    (Join-Path $ProjectRoot 'Content\Maps\StageG1B_OriginalPlayerPoC.umap') = '623AF018D2682CA9CB573CA891CD6DF1B118584F260623282E2F874582FD062E'
    (Join-Path $ProjectRoot 'Content\StageG1B\Characters\PLAYER_M\MeshyV01\Mesh\SK_PLAYER_M_Meshy_v0_1.uasset') = '7A3BB33EAE59D193BC05FE2F2F8A4554E8B19330DC0E7F88B697DE506D2DB9AB'
    (Join-Path $ProjectRoot 'Content\StageG1B\Characters\PLAYER_M\MeshyV01\Skeleton\SKEL_PLAYER_M_Meshy_v0_1.uasset') = 'C81A688AEBCE5C23DED677DA81EC743AD6AE013BD443BFE57583CD91EF0FE5A0'
    (Join-Path $ProjectRoot 'Source\NationSimulationStageC\Public\StageG2A\StageG2ACameraModeAdapter.h') = '5D49D19F69CE878075A231978ABBBEDB4FACE442C4B77B7EF76778AF2833B487'
    (Join-Path $ProjectRoot 'Source\NationSimulationStageC\Private\StageG2A\StageG2ACameraModeAdapter.cpp') = '82D4C383BD8980123F5FDB15B407F06F0934A3055DF2055155880DF162A9900D'
    (Join-Path $ProjectRoot 'Source\NationSimulationStageC\Public\StageG1B\StageG1BPlayerVisualAdapter.h') = '6FCFE04292FF3CB6459C1BFF52DD416D9127F3F4B1FA2BBCED4330185E97CA54'
    (Join-Path $ProjectRoot 'Source\NationSimulationStageC\Private\StageG1B\StageG1BPlayerVisualAdapter.cpp') = '1C7E008D4D42B83447FC6E157FE33D8B3951510CE696C7E10F2079AB1367288C'
    (Join-Path $RepositoryRoot 'data\stage_a_fixture.json') = '62F9B1BAB98FB0E32BB5DB2C7FAAA7F31E9B40DDCF85C94A8ADC46DFACA98A2B'
    (Join-Path $RepositoryRoot 'data\stage_parity_contract.json') = 'B8D4B5D356731F96EFB0779095E725337E84252F15A829E2A1B922E1DF99DED3'
    (Join-Path $ProjectRoot 'Data\StageE\stage_e_state_definitions.json') = '9913D473DF3A1BE8B71400150E2D76A7F6B46A66DADC0F218027346F0978B833'
    (Join-Path $ProjectRoot 'Data\StageF\stage_f_save_schema.json') = '596576DC37D71B7D31E2B28E219309E27E4BAF1C03E49DF6C375A1A9B0B92DB5'
}

New-Item -ItemType Directory -Force -Path $TestOutput,$Reports,$Screenshots,$SavedEvidence | Out-Null
foreach ($Item in $Protected.GetEnumerator()) {
    if (-not (Test-Path -LiteralPath $Item.Key) -or
        (Get-FileHash -LiteralPath $Item.Key -Algorithm SHA256).Hash -ne $Item.Value) {
        throw "Protected Stage G-2B baseline mismatch: $($Item.Key)"
    }
}

$BuildLog = Join-Path $TestOutput 'editor_build.log'
& $Build NationSimulationStageCEditor Win64 Development "-Project=$Project" `
    -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=1 *> $BuildLog
if ($LASTEXITCODE -ne 0) { throw "Stage G-2B Editor build failed with exit code $LASTEXITCODE" }

$AutomationLog = Join-Path $TestOutput 'stage_g2b_guardm_automation.log'
& $Editor $Project -unattended -nop4 -nosplash -nullrhi -NoSound `
    '-ExecCmds=Automation RunTests NationSimulation.StageG2B.GuardM;Quit' `
    '-TestExit=Automation Test Queue Empty' "-abslog=$AutomationLog"
if ($LASTEXITCODE -ne 0) { throw "Stage G-2B automation failed with exit code $LASTEXITCODE" }
$AutomationResults = Join-Path $TestOutput 'stage_g2b_guardm_automation_results.txt'
if (-not (Test-Path -LiteralPath $AutomationResults) -or
    (Get-Content -LiteralPath $AutomationResults | Select-Object -Last 1) -ne 'SUMMARY | 18/18 tests passed') {
    throw 'Stage G-2B automation did not pass 18/18.'
}

$RuntimeLog = Join-Path $TestOutput 'stage_g2b_guardm_runtime.log'
& $Editor $Project '/Game/Maps/StageG2B_GuardM_PoC' -game -unattended -nop4 -nosplash -nullrhi -NoSound `
    -StageG2BEvidence -StageG2BEvidenceExit "-abslog=$RuntimeLog"
if ($LASTEXITCODE -ne 0) { throw "Stage G-2B runtime evidence failed with exit code $LASTEXITCODE" }
$RuntimeText = Get-Content -Raw -LiteralPath $RuntimeLog
if ($RuntimeText -notmatch
    'STAGE_G2B_GUARD_M_EVIDENCE_WRITTEN files=5 guard=1 bones=24 root=Hips material=1 standard=1 tactical=1 return=1 player_fallback=0 skeleton_shared=0 triangles=178245') {
    throw 'Stage G-2B runtime evidence PASS marker is missing.'
}
$EvidenceNames = @('guard_runtime_evidence.json','guard_animation_evidence.json',
    'guard_camera_evidence.json','guard_lod_evidence.json','guard_isolation_evidence.json')
foreach ($Name in $EvidenceNames) {
    $Source = Join-Path $SavedEvidence $Name
    if (-not (Test-Path -LiteralPath $Source)) { throw "Stage G-2B runtime evidence missing: $Name" }
    Copy-Item -LiteralPath $Source -Destination (Join-Path $Reports $Name) -Force
}

if (-not $SkipScreenshots) {
    $ScreenshotNames = @('01_guard_standard_front.png','02_guard_standard_close.png',
        '03_guard_standard_side.png','04_guard_standard_back.png','05_guard_player_scale_compare.png',
        '06_guard_tactical_initial.png','07_guard_tactical_zoom_max.png','08_guard_material_close.png',
        '09_guard_collision_near_wall.png','10_return_to_standard_with_guard.png')
    foreach ($Name in $ScreenshotNames) {
        $Path = Join-Path $SavedScreenshots $Name
        if (Test-Path -LiteralPath $Path) { Remove-Item -LiteralPath $Path -Force }
    }
    $ScreenshotLog = Join-Path $TestOutput 'stage_g2b_guardm_screenshots.log'
    & $Editor $Project '/Game/Maps/StageG2B_GuardM_PoC' -game -unattended -nop4 -nosplash -NoSound `
        -RenderOffscreen -ResX=1280 -ResY=720 -StageG2BScreenshots -StageG2BScreenshotsExit `
        "-abslog=$ScreenshotLog"
    if ($LASTEXITCODE -ne 0) { throw "Stage G-2B screenshot capture failed with exit code $LASTEXITCODE" }
    if ((Get-Content -Raw -LiteralPath $ScreenshotLog) -notmatch
        'STAGE_G2B_GUARD_M_SCREENSHOTS_REQUESTED count=10') {
        throw 'Stage G-2B screenshot capture marker is missing.'
    }
    $Manifest = @()
    foreach ($Name in $ScreenshotNames) {
        $Source = Join-Path $SavedScreenshots $Name
        if (-not (Test-Path -LiteralPath $Source)) { throw "Stage G-2B screenshot missing: $Name" }
        $Bytes = [System.IO.File]::ReadAllBytes($Source)
        if ($Bytes.Length -lt 24) { throw "Stage G-2B screenshot is truncated: $Name" }
        $Width = [System.Net.IPAddress]::NetworkToHostOrder([BitConverter]::ToInt32($Bytes,16))
        $Height = [System.Net.IPAddress]::NetworkToHostOrder([BitConverter]::ToInt32($Bytes,20))
        if ($Width -ne 1280 -or $Height -ne 720) { throw "Stage G-2B screenshot dimensions invalid: $Name $Width x $Height" }
        $Destination = Join-Path $Screenshots $Name
        Copy-Item -LiteralPath $Source -Destination $Destination -Force
        $Manifest += [ordered]@{ file=$Name; width=$Width; height=$Height; bytes=$Bytes.Length;
            sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $Destination).Hash }
    }
    $Manifest | ConvertTo-Json -Depth 4 |
        Set-Content -LiteralPath (Join-Path $Reports 'screenshot_manifest.json') -Encoding utf8NoBOM
}

if (-not $SkipRegression) {
    $RegressionLog = Join-Path $Output 'regression\stage_c_through_g2b_unreal.log'
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $RegressionLog) | Out-Null
    & $Editor $Project -unattended -nop4 -nosplash -nullrhi -NoSound `
        '-ExecCmds=Automation RunTests NationSimulation.Stage;Quit' `
        '-TestExit=Automation Test Queue Empty' "-abslog=$RegressionLog"
    if ($LASTEXITCODE -ne 0) { throw 'Stage C through G-2B Unreal regression failed.' }

    $Contracts = [ordered]@{
        (Join-Path $RepositoryRoot 'out\stage-c\test-output\acceptance_results.txt') = 'SUMMARY | 11/11 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-d\test-output\stage_d_acceptance_results.txt') = 'SUMMARY | 9/9 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-d\fix-output\stage_d_fix_results.txt') = 'SUMMARY | 8/8 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-f\test-output\stage_f_unreal_results.txt') = 'SUMMARY | 5/5 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-g0\test-output\stage_g0_automation_results.txt') = 'SUMMARY | 33/33 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-g1a\test-output\stage_g1a_automation_results.txt') = 'SUMMARY | 22/22 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-g1b-meshy\test-output\stage_g1b_meshy_automation_results.txt') = 'SUMMARY | 30/30 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-g2a-redesign\test-output\stage_g2a_redesign_automation_results.txt') = 'SUMMARY | 18/18 tests passed'
        $AutomationResults = 'SUMMARY | 18/18 tests passed'
    }
    foreach ($Contract in $Contracts.GetEnumerator()) {
        if (-not (Test-Path -LiteralPath $Contract.Key) -or
            (Get-Content -LiteralPath $Contract.Key | Select-Object -Last 1) -ne $Contract.Value) {
            throw "Stage G-2B regression contract failed: $($Contract.Key)"
        }
    }
}

foreach ($Item in $Protected.GetEnumerator()) {
    if ((Get-FileHash -LiteralPath $Item.Key -Algorithm SHA256).Hash -ne $Item.Value) {
        throw "Protected baseline changed during Stage G-2B acceptance: $($Item.Key)"
    }
}

if (-not $SkipPackage) {
    & (Join-Path $PSScriptRoot 'PackageStageG2BGuardM.ps1') -EngineRoot $EngineRoot
    if ($LASTEXITCODE -ne 0) { throw 'Stage G-2B Package verification failed.' }
}

@(
    'PASS | Stage G-2B GUARD_M Automation 18/18'
    'PASS | GUARD_M dedicated Skeleton, source Idle_11, single placement'
    'PASS | Standard/Tactical camera and PLAYER_M isolation runtime evidence'
    $(if ($SkipScreenshots) { 'SKIP | 10 evaluation screenshots' } else { 'PASS | 10 evaluation screenshots 1280x720' })
    $(if ($SkipRegression) { 'SKIP | Stage C through G-2A regression' } else { 'PASS | Stage C through G-2A regression' })
    $(if ($SkipPackage) { 'SKIP | Win64 Development Package' } else { 'PASS | Win64 Development Package and G2B/G2A/G1B launch checks' })
    'PENDING | User real-machine GUARD_M visual confirmation'
    'SUMMARY | Stage G-2B technically verified; user acceptance pending'
) | Set-Content -LiteralPath (Join-Path $Output 'stage_g2b_guardm_acceptance_results.txt') -Encoding utf8NoBOM

Write-Host 'STAGE_G2B_GUARD_M_AUTOMATION 18/18 PASS'
Write-Host $(if ($SkipRegression) { 'STAGE_G2B_REGRESSION SKIP' } else { 'STAGE_C_THROUGH_G2B_REGRESSION PASS' })
Write-Host $(if ($SkipPackage) { 'STAGE_G2B_PACKAGE SKIP' } else { 'STAGE_G2B_PACKAGE PASS' })
