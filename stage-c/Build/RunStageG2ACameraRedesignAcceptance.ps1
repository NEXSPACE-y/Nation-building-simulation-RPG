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
$Output = Join-Path $RepositoryRoot 'out\stage-g2a-redesign'
$TestOutput = Join-Path $Output 'test-output'
$Reports = Join-Path $ProjectRoot 'SourceArt\StageG2A\CameraRedesign\v0.1\Reports'
$Screenshots = Join-Path $ProjectRoot 'SourceArt\StageG2A\CameraRedesign\v0.1\Screenshots'
$SavedEvidence = Join-Path $ProjectRoot 'Saved\StageG2A'
$SavedScreenshots = Join-Path $SavedEvidence 'Screenshots'

$Protected = [ordered]@{
    (Join-Path $ProjectRoot 'Content\Maps\StageG1B_OriginalPlayerPoC.umap') = '623AF018D2682CA9CB573CA891CD6DF1B118584F260623282E2F874582FD062E'
    (Join-Path $ProjectRoot 'Source\NationSimulationStageC\Public\StageG1A\StageG1APlayerCharacter.h') = '74116027B736F39FD385A2D73C53492461967A597CDDAB487568BF2467787DDD'
    (Join-Path $ProjectRoot 'Source\NationSimulationStageC\Private\StageG1A\StageG1APlayerCharacter.cpp') = '492762F42A2ED7C24A5B670106868F4C98C6E7F6D2618FB71DE088642FEE3E3E'
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
        throw "Protected Stage G-2A redesign baseline mismatch: $($Item.Key)"
    }
}

& $Build NationSimulationStageCEditor Win64 Development "-Project=$Project" `
    -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=1
if ($LASTEXITCODE -ne 0) { throw "Stage G-2A redesign Editor build failed with exit code $LASTEXITCODE" }

$AutomationLog = Join-Path $TestOutput 'stage_g2a_redesign_automation.log'
& $Editor $Project -unattended -nop4 -nosplash -nullrhi -NoSound `
    '-ExecCmds=Automation RunTests NationSimulation.StageG2A.CameraRedesign;Quit' `
    '-TestExit=Automation Test Queue Empty' "-abslog=$AutomationLog"
if ($LASTEXITCODE -ne 0) { throw "Stage G-2A redesign automation failed with exit code $LASTEXITCODE" }
$AutomationResults = Join-Path $TestOutput 'stage_g2a_redesign_automation_results.txt'
if (-not (Test-Path -LiteralPath $AutomationResults) -or
    (Get-Content -LiteralPath $AutomationResults | Select-Object -Last 1) -ne 'SUMMARY | 18/18 tests passed') {
    throw 'Stage G-2A redesign automation did not pass 18/18.'
}

$RuntimeLog = Join-Path $TestOutput 'stage_g2a_redesign_runtime.log'
& $Editor $Project '/Game/Maps/StageG2A_CameraRedesignPoC' -game -unattended -nop4 -nosplash -nullrhi -NoSound `
    -StageG2AEvidence -StageG2AEvidenceExit "-abslog=$RuntimeLog"
if ($LASTEXITCODE -ne 0) { throw "Stage G-2A redesign runtime evidence failed with exit code $LASTEXITCODE" }
$RuntimeText = Get-Content -Raw -LiteralPath $RuntimeLog
if ($RuntimeText -notmatch
    'STAGE_G2A_REDESIGN_EVIDENCE_WRITTEN files=4 mode=standard tactical=1 return=1 destination=1 path=1 movement_mode=1 collision=1 recovery=1') {
    throw 'Stage G-2A redesign runtime evidence PASS marker is missing.'
}
$EvidenceNames = @('camera_modes_evidence.json','camera_preservation_evidence.json',
    'camera_collision_evidence.json','camera_input_evidence.json')
foreach ($Name in $EvidenceNames) {
    $Source = Join-Path $SavedEvidence $Name
    if (-not (Test-Path -LiteralPath $Source)) { throw "Stage G-2A runtime evidence missing: $Name" }
    Copy-Item -LiteralPath $Source -Destination (Join-Path $TestOutput $Name) -Force
    Copy-Item -LiteralPath $Source -Destination (Join-Path $Reports $Name) -Force
}

if (-not $SkipScreenshots) {
    $ScreenshotNames = @('01_standard_initial.png','02_standard_min_zoom.png','03_standard_max_zoom.png',
        '04_standard_pitch_min.png','05_standard_pitch_max.png','06_tactical_initial.png',
        '07_tactical_min_zoom.png','08_tactical_max_zoom.png','09_tactical_yaw_90.png',
        '10_tactical_yaw_180.png','11_collision_near_wall.png','12_return_to_standard.png')
    foreach ($Name in $ScreenshotNames) {
        $Path = Join-Path $SavedScreenshots $Name
        if (Test-Path -LiteralPath $Path) { Remove-Item -LiteralPath $Path -Force }
    }
    $ScreenshotLog = Join-Path $TestOutput 'stage_g2a_redesign_screenshots.log'
    & $Editor $Project '/Game/Maps/StageG2A_CameraRedesignPoC' -game -unattended -nop4 -nosplash -NoSound `
        -RenderOffscreen -ResX=1280 -ResY=720 -StageG2AScreenshots -StageG2AScreenshotsExit `
        "-abslog=$ScreenshotLog"
    if ($LASTEXITCODE -ne 0) { throw "Stage G-2A screenshot capture failed with exit code $LASTEXITCODE" }
    if ((Get-Content -Raw -LiteralPath $ScreenshotLog) -notmatch
        'STAGE_G2A_REDESIGN_SCREENSHOTS_REQUESTED count=12') {
        throw 'Stage G-2A screenshot capture marker is missing.'
    }
    $Manifest = @()
    foreach ($Name in $ScreenshotNames) {
        $Source = Join-Path $SavedScreenshots $Name
        if (-not (Test-Path -LiteralPath $Source)) { throw "Stage G-2A screenshot missing: $Name" }
        $Bytes = [System.IO.File]::ReadAllBytes($Source)
        if ($Bytes.Length -lt 24) { throw "Stage G-2A screenshot is truncated: $Name" }
        $Width = [System.Net.IPAddress]::NetworkToHostOrder([BitConverter]::ToInt32($Bytes,16))
        $Height = [System.Net.IPAddress]::NetworkToHostOrder([BitConverter]::ToInt32($Bytes,20))
        if ($Width -ne 1280 -or $Height -ne 720) { throw "Stage G-2A screenshot dimensions invalid: $Name $Width x $Height" }
        $Destination = Join-Path $Screenshots $Name
        Copy-Item -LiteralPath $Source -Destination $Destination -Force
        $Manifest += [ordered]@{ file=$Name; width=$Width; height=$Height; bytes=$Bytes.Length;
            sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $Destination).Hash }
    }
    $Manifest | ConvertTo-Json -Depth 4 |
        Set-Content -LiteralPath (Join-Path $Reports 'screenshot_manifest.json') -Encoding utf8NoBOM
}

if (-not $SkipRegression) {
    $RegressionLog = Join-Path $Output 'regression\stage_c_through_g2a_redesign_unreal.log'
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $RegressionLog) | Out-Null
    & $Editor $Project -unattended -nop4 -nosplash -nullrhi -NoSound `
        '-ExecCmds=Automation RunTests NationSimulation.Stage;Quit' `
        '-TestExit=Automation Test Queue Empty' "-abslog=$RegressionLog"
    if ($LASTEXITCODE -ne 0) { throw 'Stage C through G-2A redesign Unreal regression failed.' }

    $Contracts = [ordered]@{
        (Join-Path $RepositoryRoot 'out\stage-c\test-output\acceptance_results.txt') = 'SUMMARY | 11/11 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-d\test-output\stage_d_acceptance_results.txt') = 'SUMMARY | 9/9 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-d\fix-output\stage_d_fix_results.txt') = 'SUMMARY | 8/8 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-f\test-output\stage_f_unreal_results.txt') = 'SUMMARY | 5/5 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-g0\test-output\stage_g0_automation_results.txt') = 'SUMMARY | 33/33 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-g1a\test-output\stage_g1a_automation_results.txt') = 'SUMMARY | 22/22 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-g1b-meshy\test-output\stage_g1b_meshy_automation_results.txt') = 'SUMMARY | 30/30 tests passed'
        $AutomationResults = 'SUMMARY | 18/18 tests passed'
    }
    foreach ($Contract in $Contracts.GetEnumerator()) {
        if (-not (Test-Path -LiteralPath $Contract.Key) -or
            (Get-Content -LiteralPath $Contract.Key | Select-Object -Last 1) -ne $Contract.Value) {
            throw "Stage G-2A redesign regression contract failed: $($Contract.Key)"
        }
    }
}

foreach ($Item in $Protected.GetEnumerator()) {
    if ((Get-FileHash -LiteralPath $Item.Key -Algorithm SHA256).Hash -ne $Item.Value) {
        throw "Protected baseline changed during Stage G-2A redesign acceptance: $($Item.Key)"
    }
}

if (-not $SkipPackage) {
    & (Join-Path $PSScriptRoot 'PackageStageG2ACameraRedesign.ps1') -EngineRoot $EngineRoot
    if ($LASTEXITCODE -ne 0) { throw 'Stage G-2A redesign Package verification failed.' }
}

@(
    'PASS | Stage G-2A camera redesign Automation 18/18'
    'PASS | StandardCharacterCamera initial non-overhead runtime evidence'
    'PASS | F6 TacticalOverlookCamera toggle and Standard return'
    'PASS | Destination, NavPath, movement mode, target preservation'
    'PASS | Camera collision shortening and recovery'
    $(if ($SkipScreenshots) { 'SKIP | 12 evaluation screenshots' } else { 'PASS | 12 evaluation screenshots 1280x720' })
    $(if ($SkipRegression) { 'SKIP | Stage C through G-1B regression' } else { 'PASS | Stage C through G-1B regression' })
    $(if ($SkipPackage) { 'SKIP | Win64 Development Package' } else { 'PASS | Win64 Development Package and no-argument launch' })
    'PENDING | User real-machine camera-operation confirmation'
    'SUMMARY | Stage G-2A redesign technically verified; user acceptance pending'
) | Set-Content -LiteralPath (Join-Path $Output 'stage_g2a_redesign_acceptance_results.txt') -Encoding utf8NoBOM

Write-Host 'STAGE_G2A_REDESIGN_AUTOMATION 18/18 PASS'
Write-Host $(if ($SkipRegression) { 'STAGE_G2A_REDESIGN_REGRESSION SKIP' } else { 'STAGE_C_THROUGH_G2A_REDESIGN_REGRESSION PASS' })
Write-Host $(if ($SkipPackage) { 'STAGE_G2A_REDESIGN_PACKAGE SKIP' } else { 'STAGE_G2A_REDESIGN_PACKAGE PASS' })
