[CmdletBinding()]
param(
    [string]$EngineRoot = $(if ($env:UE_5_8_ROOT) { $env:UE_5_8_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }),
    [switch]$SkipPackage
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$RepositoryRoot = (Resolve-Path (Join-Path $ProjectRoot '..')).Path
$Project = Join-Path $ProjectRoot 'NationSimulationStageC.uproject'
$Build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Output = Join-Path $RepositoryRoot 'out\stage-e'
$CoreBuild = Join-Path $Output 'core-build'
New-Item -ItemType Directory -Force -Path $Output | Out-Null

foreach ($RequiredPath in @($Project, $Build, $Editor)) {
    if (-not (Test-Path -LiteralPath $RequiredPath)) { throw "Required Stage E path does not exist: $RequiredPath" }
}

cmake -S $RepositoryRoot -B $CoreBuild
if ($LASTEXITCODE -ne 0) { throw "Stage E CMake configure failed with exit code $LASTEXITCODE" }
cmake --build $CoreBuild --config Debug --target nation_tests stage_e_state_validator
if ($LASTEXITCODE -ne 0) { throw "Stage E core build failed with exit code $LASTEXITCODE" }

$CoreResult = & (Join-Path $CoreBuild 'Debug\nation_tests.exe') 2>&1
$CoreResult | Set-Content -LiteralPath (Join-Path $Output 'core_acceptance.txt') -Encoding utf8
if ($LASTEXITCODE -ne 0 -or ($CoreResult -join "`n") -notmatch '10/10 tests passed') {
    throw 'Existing C++ causal-core acceptance did not remain 10/10.'
}

$ValidationDirectory = Join-Path $Output 'validation'
New-Item -ItemType Directory -Force -Path $ValidationDirectory | Out-Null
$ValidationJsonl = Join-Path $ValidationDirectory 'stage_e_validation.jsonl'
$ValidationSummary = & (Join-Path $CoreBuild 'Debug\stage_e_state_validator.exe') `
    (Join-Path $ProjectRoot 'Data\StageE\stage_e_state_definitions.json') `
    (Join-Path $RepositoryRoot 'data\stage_a_fixture.json') $ValidationJsonl 2>&1
$ValidationSummary | Set-Content -LiteralPath (Join-Path $ValidationDirectory 'stage_e_validation_summary.txt') -Encoding utf8
if ($LASTEXITCODE -ne 0 -or ($ValidationSummary -join "`n") -notmatch 'errors=0') {
    throw 'Stage E normal state definition validation emitted ERROR.'
}
foreach ($Line in Get-Content -LiteralPath $ValidationJsonl) {
    $Row = $Line | ConvertFrom-Json
    foreach ($Field in @('severity','npc_id','state_id','rule_id','error_code','message','source_file')) {
        if ($Row.PSObject.Properties.Name -notcontains $Field) { throw "Stage E JSONL field missing: $Field" }
    }
    if ($Row.severity -eq 'ERROR') { throw "Stage E validation ERROR: $($Row.error_code)" }
}

& $Build NationSimulationStageCEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw "UnrealBuildTool failed with exit code $LASTEXITCODE" }
& $Editor $Project -unattended -nop4 -NullRHI -nosplash -NoSound `
    '-ExecCmds=Automation RunTests NationSimulation.Stage;Quit' `
    '-TestExit=Automation Test Queue Empty' -log
if ($LASTEXITCODE -ne 0) { throw "Stage C/D/E Automation failed with exit code $LASTEXITCODE" }

$StageC = Join-Path $RepositoryRoot 'out\stage-c\test-output\acceptance_results.txt'
$StageD = Join-Path $RepositoryRoot 'out\stage-d\test-output\stage_d_acceptance_results.txt'
$StageDFix = Join-Path $RepositoryRoot 'out\stage-d\fix-output\stage_d_fix_results.txt'
if ((Get-Content -LiteralPath $StageC | Select-Object -Last 1) -ne 'SUMMARY | 11/11 tests passed') { throw 'Stage C 11/11 regression failed.' }
if ((Get-Content -LiteralPath $StageD | Select-Object -Last 1) -ne 'SUMMARY | 9/9 tests passed') { throw 'Stage D 9/9 regression failed.' }
if ((Get-Content -LiteralPath $StageDFix | Select-Object -Last 1) -ne 'SUMMARY | 8/8 tests passed') { throw 'Stage D fixes 8/8 regression failed.' }

$Behavior = Get-Content -Raw -LiteralPath (Join-Path $Output 'test-output\stage_e_scenario_evidence.json') | ConvertFrom-Json
if ($Behavior.summary.passed -ne 8 -or $Behavior.summary.failed -ne 0) { throw 'Stage E behavioral scenarios failed.' }
$Migration = Get-Content -Raw -LiteralPath (Join-Path $Output 'migration-output\stage_e_migration_evidence.json') | ConvertFrom-Json
if ($Migration.summary.passed -ne 4 -or $Migration.summary.failed -ne 0) { throw 'Stage E save migration acceptance failed.' }
$Validator = Get-Content -Raw -LiteralPath (Join-Path $Output 'validation-negative\stage_e_validator_negative_evidence.json') | ConvertFrom-Json
$ValidatorSummary = $Validator[-1].summary
if ($ValidatorSummary.passed -ne 22 -or $ValidatorSummary.failed -ne 0) { throw 'Stage E validator negative acceptance failed.' }
foreach ($LogName in @('stage_e_audit.jsonl','stage_e_causal.jsonl')) {
    $LogPath = Join-Path $Output "test-output\$LogName"
    if (-not (Test-Path -LiteralPath $LogPath) -or (Get-Item -LiteralPath $LogPath).Length -eq 0) {
        throw "Stage E evidence log is empty: $LogPath"
    }
}

if (-not $SkipPackage) {
    & (Join-Path $PSScriptRoot 'PackageStageE.ps1') -EngineRoot $EngineRoot
    if ($LASTEXITCODE -ne 0) { throw "Stage E packaging failed with exit code $LASTEXITCODE" }
}

$Results = Join-Path $Output 'stage_e_acceptance_results.txt'
@(
    'PASS | Existing C++ causal core 10/10'
    'PASS | Normal Stage E state definition ERROR=0'
    'PASS | Stage C regression 11/11'
    'PASS | Stage D vertical slice regression 9/9'
    'PASS | Stage D corrective regression 8/8'
    'PASS | Stage E behavioral scenarios 8/8'
    'PASS | Stage D to Stage E save migration 4/4'
    'PASS | Stage E validator baseline and negative cases 22/22'
    'PASS | Stage E audit and causal evidence logs are non-empty'
    $(if ($SkipPackage) { 'SKIP | Development package' } else { 'PASS | Development package and launch smoke' })
    'SUMMARY | Stage E acceptance passed'
) | Set-Content -LiteralPath $Results -Encoding utf8

Write-Host 'STAGE_E_ACCEPTANCE | PASS'
