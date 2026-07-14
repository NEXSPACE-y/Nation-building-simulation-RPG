[CmdletBinding()]
param(
    [string]$EngineRoot = $(if ($env:UE_5_8_ROOT) { $env:UE_5_8_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }),
    [switch]$SkipRegression,
    [switch]$SkipPackage
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$RepositoryRoot = (Resolve-Path (Join-Path $ProjectRoot '..')).Path
$Project = Join-Path $ProjectRoot 'NationSimulationStageC.uproject'
$Build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Output = Join-Path $RepositoryRoot 'out\stage-g1a'
$TestOutput = Join-Path $Output 'test-output'
New-Item -ItemType Directory -Force -Path $TestOutput | Out-Null

& $Build NationSimulationStageCEditor Win64 Development "-Project=$Project" `
    -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=1
if ($LASTEXITCODE -ne 0) { throw "Stage G-1A UnrealBuildTool failed with exit code $LASTEXITCODE" }

$AutomationLog = Join-Path $TestOutput 'stage_g1a_automation.log'
& $Editor $Project -unattended -nop4 -nosplash -nullrhi -NoSound `
    '-ExecCmds=Automation RunTests NationSimulation.StageG1A;Quit' "-abslog=$AutomationLog"
if ($LASTEXITCODE -ne 0) { throw "Stage G-1A automation process failed with exit code $LASTEXITCODE" }
$AutomationResults = Join-Path $TestOutput 'stage_g1a_automation_results.txt'
if (-not (Test-Path -LiteralPath $AutomationResults) -or
    (Get-Content -LiteralPath $AutomationResults | Select-Object -Last 1) -ne 'SUMMARY | 22/22 tests passed') {
    throw 'Stage G-1A automation did not pass 22/22.'
}

$RuntimeLog = Join-Path $TestOutput 'stage_g1a_runtime_evidence.log'
& $Editor $Project -game -unattended -nop4 -nosplash -nullrhi -NoSound `
    -StageG1AEvidence -StageG1AEvidenceExit "-abslog=$RuntimeLog"
if ($LASTEXITCODE -ne 0) { throw "Stage G-1A runtime evidence process failed with exit code $LASTEXITCODE" }
if ((Get-Content -Raw -LiteralPath $RuntimeLog) -notmatch 'STAGE_G1A_EVIDENCE_WRITTEN files=10') {
    throw 'Stage G-1A runtime evidence marker is missing.'
}
foreach ($Name in @('standard_asset_evidence.json','world_height_evidence.json',
    'movement_animation_sync_evidence.json','click_move_evidence.json','grounding_shadow_evidence.json',
    'walk_run_mode_evidence.json','walk_animation_evidence.json','run_animation_evidence.json',
    'movement_mode_toggle_evidence.json','ui_input_isolation_evidence.json')) {
    $Source = Join-Path $ProjectRoot "Saved\StageG1A\$Name"
    if (-not (Test-Path -LiteralPath $Source)) { throw "Stage G-1A runtime evidence missing: $Name" }
    Copy-Item -LiteralPath $Source -Destination (Join-Path $TestOutput $Name) -Force
}

$Height = Get-Content -Raw -LiteralPath (Join-Path $TestOutput 'world_height_evidence.json') | ConvertFrom-Json
if (-not $Height.height_in_170_180_band) { throw "Stage G-1A measured character height is out of band: $($Height.character_mesh_height_uu)" }
$Click = Get-Content -Raw -LiteralPath (Join-Path $TestOutput 'click_move_evidence.json') | ConvertFrom-Json
if (-not $Click.move_command_observed -or $Click.wasd_bound_in_g1a_player) { throw 'Stage G-1A click/no-WASD evidence failed.' }
$Ground = Get-Content -Raw -LiteralPath (Join-Path $TestOutput 'grounding_shadow_evidence.json') | ConvertFrom-Json
if (-not $Ground.ground_trace_hit -or $Ground.blob_shadow_used) { throw 'Stage G-1A grounding/shadow evidence failed.' }
$Modes = Get-Content -Raw -LiteralPath (Join-Path $TestOutput 'walk_run_mode_evidence.json') | ConvertFrom-Json
if ($Modes.default_mode -ne 'WALK' -or $Modes.observed_walk_max_speed_uu_s -ne 250 -or
    $Modes.observed_run_max_speed_uu_s -ne 500 -or $Modes.save_schema_changed) {
    throw 'Stage G-1A WALK/RUN mode evidence failed.'
}
$WalkAnimation = Get-Content -Raw -LiteralPath (Join-Path $TestOutput 'walk_animation_evidence.json') | ConvertFrom-Json
if ($WalkAnimation.animation_state -ne 'WALK' -or -not $WalkAnimation.speed_within_walk_range -or
    -not $WalkAnimation.animation_time_progresses) { throw 'Stage G-1A WALK animation evidence failed.' }
$RunAnimation = Get-Content -Raw -LiteralPath (Join-Path $TestOutput 'run_animation_evidence.json') | ConvertFrom-Json
if ($RunAnimation.animation_state -ne 'RUN' -or -not $RunAnimation.speed_increased_from_walk -or
    -not $RunAnimation.animation_time_progresses) { throw 'Stage G-1A RUN animation evidence failed.' }
$Toggle = Get-Content -Raw -LiteralPath (Join-Path $TestOutput 'movement_mode_toggle_evidence.json') | ConvertFrom-Json
if (-not $Toggle.run_destination_maintained -or -not $Toggle.run_nav_path_maintained -or
    -not $Toggle.run_move_active -or -not $Toggle.walk_destination_maintained -or
    -not $Toggle.walk_nav_path_maintained -or -not $Toggle.walk_move_active) {
    throw 'Stage G-1A movement-mode toggle evidence failed.'
}
$UiIsolation = Get-Content -Raw -LiteralPath (Join-Path $TestOutput 'ui_input_isolation_evidence.json') | ConvertFrom-Json
if (-not $UiIsolation.normal_ui_button_present -or -not $UiIsolation.destination_update_count_unchanged -or
    -not $UiIsolation.selected_target_unchanged -or $UiIsolation.world_move_command_generated -or
    $UiIsolation.causal_event_generated -or -not $UiIsolation.ui_pass_through_guard) {
    throw 'Stage G-1A UI isolation evidence failed.'
}

if (-not $SkipRegression) {
    # Re-run the complete Unreal suite without invoking legacy package scripts.
    # The Stage F package script assumes the old no-argument Stage D default,
    # while G-1A intentionally changes the default map. Its retained package is
    # therefore verified separately with an explicit Stage D map argument.
    $RegressionLog = Join-Path $Output 'regression\stage_c_through_g1a_unreal.log'
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $RegressionLog) | Out-Null
    & $Editor $Project -unattended -nop4 -nosplash -nullrhi -NoSound `
        '-ExecCmds=Automation RunTests NationSimulation.Stage;Quit' `
        '-TestExit=Automation Test Queue Empty' "-abslog=$RegressionLog"
    if ($LASTEXITCODE -ne 0) { throw 'Stage C through G-1A Unreal regression process failed.' }

    $RegressionContracts = @{
        (Join-Path $RepositoryRoot 'out\stage-c\test-output\acceptance_results.txt') = 'SUMMARY | 11/11 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-d\test-output\stage_d_acceptance_results.txt') = 'SUMMARY | 9/9 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-d\fix-output\stage_d_fix_results.txt') = 'SUMMARY | 8/8 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-f\test-output\stage_f_unreal_results.txt') = 'SUMMARY | 5/5 tests passed'
        (Join-Path $RepositoryRoot 'out\stage-g0\test-output\stage_g0_automation_results.txt') = 'SUMMARY | 33/33 tests passed'
        $AutomationResults = 'SUMMARY | 22/22 tests passed'
    }
    foreach ($Contract in $RegressionContracts.GetEnumerator()) {
        if (-not (Test-Path -LiteralPath $Contract.Key) -or
            (Get-Content -LiteralPath $Contract.Key | Select-Object -Last 1) -ne $Contract.Value) {
            throw "Unreal regression contract failed: $($Contract.Key)"
        }
    }

    $CoreAcceptance = Join-Path $RepositoryRoot 'out\stage-g0\stage-f-regression\core_acceptance.txt'
    $StageFScenarios = Join-Path $RepositoryRoot 'out\stage-g0\stage-f-regression\stage_f_core_scenarios.txt'
    $StageFExplicitMap = Join-Path $Output 'regression\stage_f_package_explicit_map.txt'
    if ((Get-Content -Raw -LiteralPath $CoreAcceptance) -notmatch '10/10 tests passed') {
        throw 'Retained Stage A through E causal-core regression is not 10/10.'
    }
    if ((Get-Content -Raw -LiteralPath $StageFScenarios) -notmatch '9/9 passed') {
        throw 'Retained Stage F core scenario regression is not 9/9.'
    }
    if ((Get-Content -Raw -LiteralPath $StageFExplicitMap) -notmatch 'launch_smoke=PASS') {
        throw 'Stage F package explicit-map regression evidence is missing.'
    }
}
if (-not $SkipPackage) {
    & (Join-Path $PSScriptRoot 'PackageStageG1A.ps1') -EngineRoot $EngineRoot
    if ($LASTEXITCODE -ne 0) { throw 'Stage G-1A Development package/no-argument smoke failed.' }
}

@(
    'HISTORY | Initial user real-machine confirmation: WALK only unconfirmable because no WALK/RUN toggle existed; all other items PASS'
    'PASS | Stage G-1A automation G1A-1 through G1A-22 22/22'
    'PASS | Runtime evidence JSON 10/10'
    $(if ($SkipRegression) { 'SKIP | Stage A through G-0 regression' } else { 'PASS | Stage A through G-0 regression' })
    $(if ($SkipPackage) { 'SKIP | Win64 Development package and no-argument launch' } else { 'PASS | Win64 Development package and no-argument launch' })
    'PENDING | Corrected WALK/RUN Computer Use confirmation'
    'PENDING | Corrected user real-machine recheck 10 items'
    'SUMMARY | Stage G-1A correction technically verified; user WALK/RUN acceptance pending'
) | Set-Content -LiteralPath (Join-Path $Output 'stage_g1a_acceptance_results.txt') -Encoding utf8

Write-Host 'STAGE_G1A_TECHNICAL_ACCEPTANCE | PASS | USER_VISUAL_APPROVAL=PENDING'
