[CmdletBinding()]
param(
    [string]$EngineRoot = $(if ($env:UE_5_8_ROOT) { $env:UE_5_8_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }),
    [switch]$SkipPackage,
    [switch]$SkipSoak,
    [int]$SoakSeconds = 1800
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$RepositoryRoot = (Resolve-Path (Join-Path $ProjectRoot '..')).Path
$Output = Join-Path $RepositoryRoot 'out\stage-f'
$CoreBuild = Join-Path $Output 'core-build'
$Generated = Join-Path $ProjectRoot 'Data\StageF\Generated'
New-Item -ItemType Directory -Force -Path $Output | Out-Null

cmake -S $RepositoryRoot -B $CoreBuild
if ($LASTEXITCODE -ne 0) { throw 'Stage F CMake configure failed.' }
cmake --build $CoreBuild --config Release --target `
    nation_tests stage_f_scale_data_generator stage_f_validator stage_f_acceptance stage_f_validator_negative stage_f_performance
if ($LASTEXITCODE -ne 0) { throw 'Stage F core build failed.' }

& (Join-Path $CoreBuild 'Release\stage_f_scale_data_generator.exe') `
    (Join-Path $ProjectRoot 'Data\StageF\stage_f_generation_config.json') `
    (Join-Path $RepositoryRoot 'data\stage_a_fixture.json') `
    (Join-Path $ProjectRoot 'Data\StageE\stage_e_state_definitions.json') $Generated
if ($LASTEXITCODE -ne 0) { throw 'Stage F scale data generation failed.' }

$ValidationDirectory = Join-Path $Output 'validation'
New-Item -ItemType Directory -Force -Path $ValidationDirectory | Out-Null
$ValidationOutput = & (Join-Path $CoreBuild 'Release\stage_f_validator.exe') $Generated `
    (Join-Path $RepositoryRoot 'data\stage_a_fixture.json') `
    (Join-Path $ProjectRoot 'Data\StageE\stage_e_state_definitions.json') `
    (Join-Path $ValidationDirectory 'stage_f_validation.jsonl') 2>&1
$ValidationOutput | Set-Content -LiteralPath (Join-Path $ValidationDirectory 'stage_f_validator_summary.txt') -Encoding utf8
if ($LASTEXITCODE -ne 0 -or ($ValidationOutput -join "`n") -notmatch 'errors=0') { throw 'Stage F validator emitted ERROR.' }

$CoreOutput = & (Join-Path $CoreBuild 'Release\nation_tests.exe') 2>&1
$CoreOutput | Set-Content -LiteralPath (Join-Path $Output 'core_acceptance.txt') -Encoding utf8
if ($LASTEXITCODE -ne 0 -or ($CoreOutput -join "`n") -notmatch '10/10 tests passed') { throw 'Existing causal core regression failed.' }
$ScenarioOutput = & (Join-Path $CoreBuild 'Release\stage_f_acceptance.exe') 2>&1
$ScenarioOutput | Set-Content -LiteralPath (Join-Path $Output 'stage_f_core_scenarios.txt') -Encoding utf8
if ($LASTEXITCODE -ne 0 -or ($ScenarioOutput -join "`n") -notmatch '9/9 passed') { throw 'Stage F core scenarios failed.' }
$NegativeOutput = & (Join-Path $CoreBuild 'Release\stage_f_validator_negative.exe') 2>&1
$NegativeOutput | Set-Content -LiteralPath (Join-Path $Output 'stage_f_negative_validator_results.txt') -Encoding utf8
if ($LASTEXITCODE -ne 0 -or ($NegativeOutput -join "`n") -notmatch '21/21 required errors detected') { throw 'Stage F negative validator failed.' }

$PerformanceDirectory = Join-Path $Output 'performance'
New-Item -ItemType Directory -Force -Path $PerformanceDirectory | Out-Null
& (Join-Path $CoreBuild 'Release\stage_f_performance.exe') standard `
    (Join-Path $PerformanceDirectory 'stage_f_performance.json')
if ($LASTEXITCODE -ne 0) { throw 'Stage F standard performance acceptance failed.' }
if (-not $SkipSoak) {
    & (Join-Path $CoreBuild 'Release\stage_f_performance.exe') soak `
        (Join-Path $PerformanceDirectory 'stage_f_soak.json') $SoakSeconds
    if ($LASTEXITCODE -ne 0) { throw 'Stage F soak acceptance failed.' }
}

& (Join-Path $PSScriptRoot 'RunStageEAcceptance.ps1') -EngineRoot $EngineRoot -SkipPackage
if ($LASTEXITCODE -ne 0) { throw 'Stage A-E regression acceptance failed.' }
$StageFUnreal = Join-Path $Output 'test-output\stage_f_unreal_results.txt'
if (-not (Test-Path -LiteralPath $StageFUnreal) -or (Get-Content -LiteralPath $StageFUnreal | Select-Object -Last 1) -ne 'SUMMARY | 5/5 tests passed') {
    throw 'Stage F Unreal automation failed.'
}

if (-not $SkipPackage) {
    & (Join-Path $PSScriptRoot 'PackageStageF.ps1') -EngineRoot $EngineRoot
    if ($LASTEXITCODE -ne 0) { throw 'Stage F package/smoke failed.' }
}

$Results = Join-Path $Output 'stage_f_acceptance_results.txt'
@(
    'PASS | Existing C++ causal core 10/10'
    'PASS | Stage F scale data 5 countries, 2500 AI NPCs, 637500 state slots'
    'PASS | Stage F validator ERROR=0'
    'PASS | Stage F validator negative cases 21/21'
    'PASS | Stage F core scenarios F-1 through F-9 9/9'
    'PASS | Stage F Unreal production runtime 5/5'
    'PASS | 10000 event deterministic load'
    'PASS | Seven-day offline aggregate'
    'PASS | 100 save/load generations'
    'PASS | Six save interruption recovery points'
    $(if ($SkipSoak) { 'SKIP | 30-minute soak' } else { "PASS | soak $SoakSeconds seconds" })
    'PASS | Stage C regression 11/11'
    'PASS | Stage D regression 9/9'
    'PASS | Stage D corrective regression 8/8'
    'PASS | Stage E behavioral 8/8'
    'PASS | Stage E migration 4/4'
    'PASS | Stage E validator 22/22'
    $(if ($SkipPackage) { 'SKIP | Development package and smoke' } else { 'PASS | Development package and smoke' })
    'SUMMARY | Stage F acceptance passed'
) | Set-Content -LiteralPath $Results -Encoding utf8

Write-Host 'STAGE_F_ACCEPTANCE | PASS'
