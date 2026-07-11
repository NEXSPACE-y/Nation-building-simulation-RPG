[CmdletBinding()]
param(
    [string]$EngineRoot = $(if ($env:UE_5_8_ROOT) { $env:UE_5_8_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }),
    [string]$ArchiveDirectory = ''
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$RepositoryRoot = (Resolve-Path (Join-Path $ProjectRoot '..')).Path
$Project = Join-Path $ProjectRoot 'NationSimulationStageC.uproject'
$RunUat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) {
    $ArchiveDirectory = Join-Path $RepositoryRoot 'out\stage-f\package'
}

Get-Process -Name 'NationSimulationStageC' -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -and $_.Path.StartsWith($ArchiveDirectory, [System.StringComparison]::OrdinalIgnoreCase) } |
    Stop-Process -Force

& $RunUat BuildCookRun "-project=$Project" -noP4 -platform=Win64 -clientconfig=Development `
    -build -cook -stage -pak -package -archive "-archivedirectory=$ArchiveDirectory" `
    '-map=/Game/Maps/StageD_Capital' -utf8output
if ($LASTEXITCODE -ne 0) { throw "Stage F Development packaging failed with exit code $LASTEXITCODE" }

$Executable = Get-ChildItem -LiteralPath $ArchiveDirectory -Recurse -Filter 'NationSimulationStageC.exe' |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $Executable) { throw "Packaged Stage F executable was not found under $ArchiveDirectory" }
$StagedWorld = Get-ChildItem -LiteralPath $ArchiveDirectory -Recurse -Filter 'world_manifest.json' |
    Where-Object { $_.FullName -match '[\\/]Data[\\/]StageF[\\/]' } | Select-Object -First 1
if (-not $StagedWorld) { throw 'Packaged Stage F scale data world manifest was not staged.' }

$RunId = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$UserDirectory = Join-Path $RepositoryRoot "out\stage-f\package-user\$RunId"
New-Item -ItemType Directory -Force -Path $UserDirectory | Out-Null
$Process = Start-Process -FilePath $Executable `
    -ArgumentList '-nullrhi','-unattended','-nosound','-log',"-UserDir=$UserDirectory" `
    -WorkingDirectory (Split-Path -Parent $Executable) -WindowStyle Hidden -PassThru
$Stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
$RuntimeLog = $null
$RuntimeText = ''
while ($Stopwatch.Elapsed.TotalSeconds -lt 30) {
    $RuntimeLog = Get-ChildItem -LiteralPath $UserDirectory -Recurse -Filter 'NationSimulationStageC.log' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($RuntimeLog) {
        $RuntimeText = Get-Content -Raw -LiteralPath $RuntimeLog.FullName
        if ($RuntimeText -match 'STAGE_D_PLAYABLE_READY pawn=(?!NONE).*npc_count=40.*StageD_Capital' -and
            $RuntimeText -match 'STAGE_F_CORE_READY countries=5 ai_npcs=2500.*dataset_sha256=[0-9a-f]{64}') { break }
    }
    if ($Process.HasExited) { break }
    Start-Sleep -Seconds 1
}
if (-not $Process.HasExited) { Stop-Process -Id $Process.Id }
elseif ($Process.ExitCode -ne 0) { throw "Packaged Stage F exited with code $($Process.ExitCode)" }
if (-not $RuntimeLog) { throw 'Packaged Stage F did not create a runtime log.' }
$RuntimeText = Get-Content -Raw -LiteralPath $RuntimeLog.FullName
if ($RuntimeText -notmatch 'STAGE_D_PLAYABLE_READY pawn=(?!NONE).*npc_count=40.*StageD_Capital') {
    throw 'Packaged Stage F did not initialize StageD_Capital, a player pawn, and exactly 40 presentation NPCs.'
}
if ($RuntimeText -notmatch 'STAGE_F_CORE_READY countries=5 ai_npcs=2500.*dataset_sha256=[0-9a-f]{64}') {
    throw 'Packaged Stage F did not initialize the production-scale core.'
}
if ($RuntimeText -match 'Fatal error|LoadCore failed') { throw 'Packaged Stage F runtime log contains a fatal/core initialization error.' }

$ExecutableSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $Executable).Hash.ToLowerInvariant()
$DatasetSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $StagedWorld.FullName).Hash.ToLowerInvariant()
$LaunchEvidence = Join-Path $RepositoryRoot 'out\stage-f\package_launch.txt'
@(
    'configuration=Development'
    'map=/Game/Maps/StageD_Capital'
    "executable=$Executable"
    "executable_sha256=$ExecutableSha256"
    "stage_f_world_manifest=$($StagedWorld.FullName)"
    "stage_f_world_manifest_sha256=$DatasetSha256"
    "runtime_log=$($RuntimeLog.FullName)"
    'presentation_npc_count=40'
    'stage_f_country_count=5'
    'stage_f_ai_npc_count=2500'
    'stage_f_core_ready=PASS'
    'launch_smoke=PASS'
) | Set-Content -LiteralPath $LaunchEvidence -Encoding utf8

Write-Host "STAGE_F_PACKAGE_READY $Executable"
Write-Host 'STAGE_F_LAUNCH_SMOKE PASS'
