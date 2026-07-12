[CmdletBinding()]
param(
    [string]$EngineRoot = $(if ($env:UE_5_8_ROOT) { $env:UE_5_8_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }),
    [switch]$SkipPackage
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$RepositoryRoot = (Resolve-Path (Join-Path $ProjectRoot '..')).Path
$Output = Join-Path $RepositoryRoot 'out\stage-g0'
$CoreBuild = Join-Path $Output 'core-build'
New-Item -ItemType Directory -Force -Path $Output | Out-Null

# Re-run the Stage F core using a Stage G-0-owned performance path. The Stage F
# script's fixed standard-runtime directory is intentionally not reused because a
# second run would continue at generation 101 instead of starting at generation 1.
cmake -S $RepositoryRoot -B $CoreBuild
if ($LASTEXITCODE -ne 0) { throw 'Stage G-0 CMake configure failed.' }
cmake --build $CoreBuild --config Release --target `
    nation_tests stage_f_scale_data_generator stage_f_validator stage_f_acceptance stage_f_validator_negative stage_f_performance
if ($LASTEXITCODE -ne 0) { throw 'Stage F core regression build failed.' }

$Generated = Join-Path $ProjectRoot 'Data\StageF\Generated'
& (Join-Path $CoreBuild 'Release\stage_f_scale_data_generator.exe') `
    (Join-Path $ProjectRoot 'Data\StageF\stage_f_generation_config.json') `
    (Join-Path $RepositoryRoot 'data\stage_a_fixture.json') `
    (Join-Path $ProjectRoot 'Data\StageE\stage_e_state_definitions.json') $Generated
if ($LASTEXITCODE -ne 0) { throw 'Stage F deterministic scale generation failed.' }

$Regression = Join-Path $Output 'stage-f-regression'
New-Item -ItemType Directory -Force -Path $Regression | Out-Null
$ValidationOutput = & (Join-Path $CoreBuild 'Release\stage_f_validator.exe') $Generated `
    (Join-Path $RepositoryRoot 'data\stage_a_fixture.json') `
    (Join-Path $ProjectRoot 'Data\StageE\stage_e_state_definitions.json') `
    (Join-Path $Regression 'stage_f_validation.jsonl') 2>&1
$ValidationOutput | Set-Content -LiteralPath (Join-Path $Regression 'stage_f_validator_summary.txt') -Encoding utf8
if ($LASTEXITCODE -ne 0 -or ($ValidationOutput -join "`n") -notmatch 'errors=0') { throw 'Stage F validator regression failed.' }

$CoreOutput = & (Join-Path $CoreBuild 'Release\nation_tests.exe') 2>&1
$CoreOutput | Set-Content -LiteralPath (Join-Path $Regression 'core_acceptance.txt') -Encoding utf8
if ($LASTEXITCODE -ne 0 -or ($CoreOutput -join "`n") -notmatch '10/10 tests passed') { throw 'Existing causal core regression failed.' }
$ScenarioOutput = & (Join-Path $CoreBuild 'Release\stage_f_acceptance.exe') 2>&1
$ScenarioOutput | Set-Content -LiteralPath (Join-Path $Regression 'stage_f_core_scenarios.txt') -Encoding utf8
if ($LASTEXITCODE -ne 0 -or ($ScenarioOutput -join "`n") -notmatch '9/9 passed') { throw 'Stage F core scenarios failed.' }
$NegativeOutput = & (Join-Path $CoreBuild 'Release\stage_f_validator_negative.exe') 2>&1
$NegativeOutput | Set-Content -LiteralPath (Join-Path $Regression 'stage_f_negative_validator_results.txt') -Encoding utf8
if ($LASTEXITCODE -ne 0 -or ($NegativeOutput -join "`n") -notmatch '21/21 required errors detected') { throw 'Stage F negative validator failed.' }

$RunId = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$PerformanceDirectory = Join-Path $Regression "performance-$RunId"
New-Item -ItemType Directory -Force -Path $PerformanceDirectory | Out-Null
& (Join-Path $CoreBuild 'Release\stage_f_performance.exe') standard `
    (Join-Path $PerformanceDirectory 'stage_f_performance.json')
if ($LASTEXITCODE -ne 0) { throw 'Stage F standard performance/recovery regression failed.' }

& (Join-Path $PSScriptRoot 'RunStageEAcceptance.ps1') -EngineRoot $EngineRoot -SkipPackage
if ($LASTEXITCODE -ne 0) { throw 'Stage A-E and Unreal regression acceptance failed.' }
$StageFUnreal = Join-Path $RepositoryRoot 'out\stage-f\test-output\stage_f_unreal_results.txt'
if (-not (Test-Path -LiteralPath $StageFUnreal) -or
    (Get-Content -LiteralPath $StageFUnreal | Select-Object -Last 1) -ne 'SUMMARY | 5/5 tests passed') {
    throw 'Stage F Unreal production boundary regression failed.'
}

& (Join-Path $PSScriptRoot 'PackageStageF.ps1') -EngineRoot $EngineRoot
if ($LASTEXITCODE -ne 0) { throw 'Stage F Development package/smoke regression failed.' }

$SoakPath = Join-Path $RepositoryRoot 'out\stage-f\performance\stage_f_soak.json'
if (-not (Test-Path -LiteralPath $SoakPath)) { throw 'Accepted Stage F 30-minute soak evidence is missing.' }
$Soak = Get-Content -Raw -LiteralPath $SoakPath | ConvertFrom-Json
if (-not $Soak.pass -or $Soak.duration_seconds -ne 1800 -or $Soak.crash -or $Soak.deadlock) {
    throw 'Accepted Stage F 30-minute soak evidence is invalid.'
}

$StageG0Automation = Join-Path $Output 'test-output\stage_g0_automation_results.txt'
if (-not (Test-Path -LiteralPath $StageG0Automation) -or
    (Get-Content -LiteralPath $StageG0Automation | Select-Object -Last 1) -ne 'SUMMARY | 33/33 tests passed') {
    throw 'Stage G-0 automation did not pass 33/33.'
}

if (-not $SkipPackage) {
    & (Join-Path $PSScriptRoot 'PackageStageG0.ps1') -EngineRoot $EngineRoot
    if ($LASTEXITCODE -ne 0) { throw 'Stage G-0 Development package/smoke failed.' }
}

$Results = Join-Path $Output 'stage_g0_acceptance_results.txt'
@(
    'PASS | Stage G-0 automation G0-1 through G0-19 and G0-M1..M8/G0-T1..T6 33/33'
    'PASS | Existing C++ causal core 10/10'
    'PASS | Stage C regression 11/11'
    'PASS | Stage D regression 9/9'
    'PASS | Stage D corrective regression 8/8'
    'PASS | Stage E behavioral scenarios 8/8'
    'PASS | Stage E save migration 4/4'
    'PASS | Stage E validator 22/22'
    'PASS | Stage F core scenarios 9/9'
    'PASS | Stage F validator ERROR=0 and negative cases 21/21'
    'PASS | Stage F Unreal boundary 5/5'
    'PASS | Stage F standard performance and recovery'
    'PASS | Accepted Stage F 1800-second soak evidence retained'
    'PASS | Stage F Development package and launch smoke'
    $(if ($SkipPackage) { 'SKIP | Stage G-0 Development package and launch smoke' } else { 'PASS | Stage G-0 Development package and launch smoke' })
    'PENDING | Stage G-0 user real-machine visual approval'
    'SUMMARY | Stage G-0 automated technical acceptance passed'
) | Set-Content -LiteralPath $Results -Encoding utf8

Write-Host 'STAGE_G0_AUTOMATED_ACCEPTANCE | PASS'
