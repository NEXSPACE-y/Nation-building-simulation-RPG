# Stage G-1B Meshy PLAYER_M Verification Report

## Status

Formal Meshy Idle_11, formal single-FBX Run_02 DASH, WALK/RUN/DASH mode switching, Automation, full regression, Win64 Development Package, no-argument launch, packaged visual capture, and user real-machine confirmation are complete. The user accepted Stage G-1B on **2026-07-15 JST**, and Stage G-1B is formally approved as **PASS**. Computer Use native-pipe access remains BLOCKED as a known environment constraint.

## User real-machine acceptance

- Confirmation date: `2026-07-15 JST`
- User real-machine confirmation: PASS
- Stage G-1B formal approval: PASS
- Appearance and motion acceptance: complete
- IDLE: `Idle_11`, PASS
- WALK: `Walking`, 250uu/s, PASS
- RUN: `Running`, 500uu/s, PASS
- DASH: `Run_02`, 750uu/s, PASS
- WALK/RUN/DASH switching: PASS
- Destination/NavPath retention: PASS
- Foot sliding and cloak/arm/leg clipping: within PoC acceptance range
- Manny fallback: false

## Git baseline and isolation

- Branch: `main`
- HEAD/local main/origin main/tag target: `8bc0adf632eb5933eaeddefebcc0384c5ec7a919`
- Staged changes: 0
- Accepted Stage G-1A C++ tracked diff: 0
- Accepted Stage G-1A map tracked diff: 0
- Failed proxy archive remains outside the repository and was not reused.

## Formal additional inputs

IDLE ZIP:

- Path: `C:\Users\rinpa\Desktop\Meshy_AI_Brave_Little_Adventur_biped (1).zip`
- Size: 33,337,204 bytes
- SHA-256: `830D95AB15D11565DCFD8C7AE50A687D43598F80B73B339B3DFC06099153B6A6`
- CRC: PASS
- Expected take: `Idle_11`, present

DASH ZIP:

- Path: `C:\Users\rinpa\Desktop\Meshy_AI_Brave_Little_Adventur_biped (2).zip`
- Size: 33,308,537 bytes
- SHA-256: `2435688009EBC7D3E31655D609282389C8FCD3C3767DF5AB63C74B61D086523B`
- CRC: PASS
- Expected take: `Run_02`, present

Both ZIPs have 6/6 readable members. Their Character FBX and four textures match the formal PLAYER_M source by SHA-256, 10/10 comparisons PASS. Skeleton comparison is 24/24 bone names, ordered identically, with root `Hips`.

Preserved originals:

- `SourceArt/StageG1B/PLAYER_M/Meshy/v0.1/AdditionalAnimations/Original/PLAYER_M_Meshy_Rigged_IDLE_v0.1.zip`
- `SourceArt/StageG1B/PLAYER_M/Meshy/v0.1/AdditionalAnimations/Original/PLAYER_M_Meshy_Rigged_DASH_Run02_v0.1.zip`

Source/preserved SHA mismatches: 0. External communication: 0. Meshy credit consumption: 0.

## Animation import and selection

Formal IDLE:

- Asset: `/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Animations/IDLE/A_PLAYER_M_Meshy_v0_1_IDLE_Idle11`
- Skeleton: `SKEL_PLAYER_M_Meshy_v0_1`
- Bone count: 24
- Root: `Hips`
- Play length: 1.900000 seconds
- Frame rate: 60fps
- Inclusive sampled frames: 115
- Reference Pose provisional: false

DASH comparison:

- Existing merged Run_02 length: 1.466667 seconds
- Formal single-FBX Run_02 length: 0.733333 seconds
- Pose comparison: 17,280 sampled values; local-basis mean absolute difference `1.958e-6`; maximum `0.001636`
- Timing identical: false
- Decision: import formal single FBX as a distinct DASH asset
- Duplicate asset created without reason: false

Formal DASH:

- Asset: `/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Animations/DASH/A_PLAYER_M_Meshy_v0_1_DASH_Run02`
- Play length: 0.733333 seconds
- Frame rate: 60fps
- Inclusive sampled frames: 45

Runtime selection:

- IDLE: Meshy `Idle_11`
- WALK: `Walking`
- RUN: `Running`
- DASH: formal single-FBX `Run_02`
- Unused retained: Character output, Head Down Charge, merged Run_02, Run_03, Standard Forward Charge

## Existing model contract

- Skeletal Mesh: `SK_PLAYER_M_Meshy_v0_1`
- Skeleton: Meshy, 24 bones, root `Hips`
- LOD0 vertices: 75,249
- LOD0 triangles: 38,862
- LOD count: 1
- Material slots: 1
- Textures: Base Color, Normal, Metallic, Roughness; 2048 x 2048
- World height: 175uu
- Component Scale: 1.0
- Manny fallback: false
- Lit/Dynamic Shadow/Contact Shadow: PASS

## WALK/RUN/DASH runtime

- Default mode: WALK
- WALK configured/observed: 250/250uu/s; Walking observed; play rate 1.0
- RUN configured/observed: 500/500uu/s; Running observed; play rate 1.0
- DASH configured/observed: 750/750uu/s; formal Run_02 observed; play rate 1.0
- Cycle: WALK -> RUN -> DASH -> WALK
- Destination maintained: PASS
- NavPath maintained: PASS
- Move active after each transition: PASS
- Destination update count unchanged: PASS
- Selected target unchanged: PASS
- World move command from UI: 0
- Causal event from UI: 0
- UI click pass-through: false
- Arrival return to Idle_11: PASS

## IDLE loop and Root Motion

- Idle_11 sequence observed at rest: PASS
- Animation time progressed: PASS
- More than two play lengths elapsed while IDLE remained active: PASS
- Actor location/rotation during idle sample: stable
- IDLE/WALK/RUN/DASH Enable Root Motion: OFF
- IDLE/WALK/RUN/DASH Force Root Lock: ON
- AnimInstance mode: Ignore Root Motion
- Actor movement owner: CharacterMovement
- Double movement observed: false

## Automation and regression

- Stage G-1B: 30/30 PASS
- Stage C: 11/11 PASS
- Stage D acceptance: 9/9 PASS
- Stage D fix: 8/8 PASS
- Stage F: 5/5 PASS
- Stage G-0: 33/33 PASS
- Stage G-1A: 22/22 PASS
- Retained Stage A-E causal core: 10/10 PASS
- Retained Stage F scenarios: 9/9 PASS
- Shared fixture/acceptance contract, Stage E definitions, Stage F runtime, G-0 map, and G-1A map: unchanged

Full regression log: `out/stage-g1b-meshy/idle-dash/final/stage_a_through_g1b_unreal.log`

## Package

- Configuration: Win64 Development
- Executable: `out/stage-g1b-meshy/package/Windows/NationSimulationStageC.exe`
- Executable SHA-256: `7931c90fa1b8c11845d8a6782b02b03bef463de800ffbb7da258f37d1e2bcc64`
- Default map: `/Game/Maps/StageG1B_OriginalPlayerPoC`
- Launch map argument: NONE
- No-argument launch: PASS
- Formal Idle_11 and looping: PASS
- WALK 250 / RUN 500 / DASH 750: PASS
- Three-mode UI and destination/NavPath retention: PASS
- Formal Run_02 DASH: PASS
- Four-state root-motion isolation: PASS
- PLAYER_M/Lit/shadow/Quinn/Manny fallback false: PASS
- Same-package explicit Stage G-1A map regression: PASS

## Packaged visual capture

Five final images PASS after the G1B UI was made opaque and large enough to cover the unchanged G1A HUD without displaying a conflicting RUN label during DASH.

Directory: `SourceArt/StageG1B/PLAYER_M/Meshy/v0.1/AdditionalAnimations/Working/Reports/Screenshots`

- `PLAYER_M_IDLE.png`
- `PLAYER_M_WALK.png`
- `PLAYER_M_RUN.png`
- `PLAYER_M_DASH.png`
- `PLAYER_M_F1_IDLE_DASH.png`

F1 visibly reports Meshy 24 bones, DASH, Run_02, configured and horizontal speed 750, play rate 1.00, Root Motion Ignored, Manny Fallback false, and Idle Provisional false.

## Computer Use

Computer Use: **BLOCKED, not PASS**. The mandatory native connection was attempted twice and both attempts returned `failed to connect native pipe: 指定されたファイルが見つかりません (os error 2)`. No helper was independently started. This remains recorded as a known environment constraint. The user subsequently completed the real-machine appearance and motion review and accepted Stage G-1B on 2026-07-15 JST.

## Known issues and user judgement points

- Computer Use native-pipe access is unavailable in the current Windows session; this is a known environment constraint.
- The formal DASH Run_02 is short and fast at 0.733333 seconds; the user accepted its foot sliding and motion feel within the PoC range.
- The user accepted Idle/DASH cloak, arm, and leg clipping within the PoC range during real-machine confirmation.
- The source model remains LOD0-only.
- Existing source-FBX bind-pose rebuild and missing-smoothing-group warnings remain unchanged.
- The accepted fixed isometric camera limits close facial inspection.

## Not implemented by design

- Manny retargeting, Manny Skeleton assignment, or IK Retargeter
- Keyboard/WASD movement or camera redesign
- Combat/Charge/Head animation connections
- Female PLAYER, NPC, guard, captain, broker, resident, fang rat, capital production, combat, character creator, or equipment system

## Acceptance state

PASS. The user completed the no-argument Win64 real-machine review on 2026-07-15 JST and formally accepted Stage G-1B. Appearance and motion acceptance is complete.

## Final repository and process state

- Accepted Stage G-1A source/map tracked diff: 0
- Unreal/game/Blender/Computer Use helper processes: 0
- Final commit, annotated tag, push, and clean-state identifiers are recorded in the completion report after baseline fixation.
