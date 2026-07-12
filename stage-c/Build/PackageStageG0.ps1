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
$Output = Join-Path $RepositoryRoot 'out\stage-g0'
if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) {
    $ArchiveDirectory = Join-Path $Output 'package'
}

foreach ($RequiredPath in @($Project, $RunUat)) {
    if (-not (Test-Path -LiteralPath $RequiredPath)) { throw "Required Stage G-0 path does not exist: $RequiredPath" }
}

Get-Process -Name 'NationSimulationStageC' -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -and $_.Path.StartsWith($ArchiveDirectory, [System.StringComparison]::OrdinalIgnoreCase) } |
    Stop-Process -Force

& $RunUat BuildCookRun "-project=$Project" -noP4 -platform=Win64 -clientconfig=Development `
    -build -cook -stage -pak -package -archive "-archivedirectory=$ArchiveDirectory" `
    '-map=/Game/Maps/StageG0_VisualPoC' '-cmdline=/Game/Maps/StageG0_VisualPoC' -utf8output
if ($LASTEXITCODE -ne 0) { throw "Stage G-0 Development packaging failed with exit code $LASTEXITCODE" }

$Executable = Get-ChildItem -LiteralPath $ArchiveDirectory -Recurse -Filter 'NationSimulationStageC.exe' |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $Executable) { throw "Packaged Stage G-0 executable was not found under $ArchiveDirectory" }
$PackagedProjectDirectory = Join-Path $ArchiveDirectory 'Windows\NationSimulationStageC'
$PackageMarker = Join-Path $PackagedProjectDirectory 'StageG0Package.marker'
if (-not (Test-Path -LiteralPath $PackagedProjectDirectory)) {
    throw "Packaged Stage G-0 project directory was not found: $PackagedProjectDirectory"
}
@(
    'purpose=Stage G-0 manual visual verification'
    'default_map=/Game/Maps/StageG0_VisualPoC'
) | Set-Content -LiteralPath $PackageMarker -Encoding ascii
$StagedWorld = Get-ChildItem -LiteralPath $ArchiveDirectory -Recurse -Filter 'world_manifest.json' |
    Where-Object { $_.FullName -match '[\\/]Data[\\/]StageF[\\/]' } | Select-Object -First 1
if (-not $StagedWorld) { throw 'Packaged Stage F scale data world manifest was not staged with Stage G-0.' }

$RunId = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$UserDirectory = Join-Path $Output "package-user\$RunId"
New-Item -ItemType Directory -Force -Path $UserDirectory | Out-Null
$Process = Start-Process -FilePath $Executable `
    -ArgumentList '-nullrhi','-unattended','-nosound','-log',"-UserDir=$UserDirectory" `
    -WorkingDirectory (Split-Path -Parent $Executable) -WindowStyle Hidden -PassThru
$Stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
$RuntimeLog = $null
$RuntimeText = ''
while ($Stopwatch.Elapsed.TotalSeconds -lt 45) {
    $RuntimeLog = Get-ChildItem -LiteralPath $UserDirectory -Recurse -Filter 'NationSimulationStageC.log' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($RuntimeLog) {
        $RuntimeText = Get-Content -Raw -LiteralPath $RuntimeLog.FullName
        if ($RuntimeText -match 'STAGE_G0_PERFORMANCE .*characters=42 sprites=42 flipbooks=42') { break }
    }
    if ($Process.HasExited) { break }
    Start-Sleep -Seconds 1
}
if (-not $Process.HasExited) { Stop-Process -Id $Process.Id }
elseif ($Process.ExitCode -ne 0) { throw "Packaged Stage G-0 exited with code $($Process.ExitCode)" }
if (-not $RuntimeLog) { throw 'Packaged Stage G-0 did not create a runtime log.' }
$RuntimeText = Get-Content -Raw -LiteralPath $RuntimeLog.FullName

$RequiredPatterns = @(
    'STAGE_D_PLAYABLE_READY pawn=(?!NONE).*npc_count=40.*StageG0_VisualPoC',
    'STAGE_F_CORE_READY countries=5 ai_npcs=2500.*dataset_sha256=[0-9a-f]{64}',
    'STAGE_G0_ENVIRONMENT_READY geometry=VISUAL_PLACEHOLDER.*occluders=1 collision=3D test_points=5 blocking_volumes=4 slope=1',
    'STAGE_G0_NAVIGATION_BUILD requested=1 runtime_generation=Dynamic bounds=StageG0_NavMeshBounds',
    'STAGE_G0_NAVIGATION_READY ready=1 path_points=[2-9][0-9]* click_move_surface=GameTraceChannel1 click_targetable=GameTraceChannel2',
    'STAGE_G0_VERIFICATION_PANEL_READY language=ja targets=6 directions=8 human_actions=7 rat_actions=6 test_points=5 shadow_toggle=1 camera_follow=1 point_drag_move=1 causal_events=0',
    'STAGE_G0_VISUAL_READY map=StageG0_VisualPoC.*npc_count=40 fang_rat_count=1 paper2d=1 visual_placeholders=42 directional_placeholder=PROC_ARROW japanese_panel=1.*camera_yaw=45\.0 camera_pitch=-55\.0',
    'STAGE_G0_PERFORMANCE .*characters=42 sprites=42 flipbooks=42'
)
foreach ($Pattern in $RequiredPatterns) {
    if ($RuntimeText -notmatch $Pattern) { throw "Packaged Stage G-0 runtime marker missing: $Pattern" }
}
if ($RuntimeText -match 'Fatal error|LoadCore failed|Failed to load package .*StageG0_VisualPoC') {
    throw 'Packaged Stage G-0 runtime log contains a fatal/core/map initialization error.'
}

$RuntimePerformance = Get-ChildItem -LiteralPath $UserDirectory -Recurse -Filter 'stage_g0_performance.json' -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $RuntimePerformance) { throw 'Packaged Stage G-0 did not emit performance evidence.' }
$Performance = Get-Content -Raw -LiteralPath $RuntimePerformance.FullName | ConvertFrom-Json
if ($Performance.rendered_character_count -ne 42 -or $Performance.sprite_count -ne 42 -or $Performance.flipbook_count -ne 42) {
    throw 'Packaged Stage G-0 performance evidence has an incorrect visual actor boundary.'
}
if (-not $Performance.navigation_ready -or $Performance.navigation_probe_path_points -lt 2) {
    throw 'Packaged Stage G-0 navigation probe failed.'
}
$PerformanceDirectory = Join-Path $Output 'performance'
New-Item -ItemType Directory -Force -Path $PerformanceDirectory | Out-Null
Copy-Item -LiteralPath $RuntimePerformance.FullName -Destination (Join-Path $PerformanceDirectory 'stage_g0_performance.json') -Force

$ExecutableSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $Executable).Hash.ToLowerInvariant()
$DatasetSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $StagedWorld.FullName).Hash.ToLowerInvariant()
$LaunchEvidence = Join-Path $Output 'package_launch.txt'
@(
    'configuration=Development'
    'map=/Game/Maps/StageG0_VisualPoC'
    "executable=$Executable"
    "executable_sha256=$ExecutableSha256"
    "stage_f_world_manifest=$($StagedWorld.FullName)"
    "stage_f_world_manifest_sha256=$DatasetSha256"
    "runtime_log=$($RuntimeLog.FullName)"
    "package_marker=$PackageMarker"
    'default_launch_map_redirect=PASS'
    'presentation_npc_count=40'
    'fang_rat_count=1'
    'rendered_character_count=42'
    'paper2d=PASS'
    'directional_placeholder=PROC_ARROW'
    'japanese_verification_panel=PASS'
    'test_points=5'
    'blocking_volumes=4'
    'click_move=NavMesh_SimpleMoveToLocation'
    'click_target=IStageG0Targetable'
    'visual_placeholder=PASS'
    'stage_f_core_ready=PASS'
    'launch_smoke=PASS'
) | Set-Content -LiteralPath $LaunchEvidence -Encoding utf8

Write-Host "STAGE_G0_PACKAGE_READY $Executable"
Write-Host 'STAGE_G0_LAUNCH_SMOKE PASS'
