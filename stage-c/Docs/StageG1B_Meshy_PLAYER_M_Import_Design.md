# Stage G-1B Meshy PLAYER_M Import Design

## Scope and baseline

Stage G-1B replaces only the visible player presentation on the dedicated PoC map. The accepted Stage G-1A player owns capsule collision, click/hold NavMesh movement, destination and marker, target selection, camera, grounding, WALK 250uu/s, RUN 500uu/s, Quinn, and fall recovery. Stage G-1A C++, accepted maps, shared fixtures, causal core, Stage E state definitions, Stage F runtime, and save schema remain unchanged.

Baseline HEAD, local `main`, `origin/main`, and annotated tag target are all `8bc0adf632eb5933eaeddefebcc0384c5ec7a919`. Stage G-1B remains uncommitted until user acceptance.

## Formal model input

The original Meshy model ZIP is retained unchanged. It supplies the PLAYER_M mesh, 24-bone Meshy Skeleton rooted at `Hips`, four 2K textures, and the original animation inventory. The original proxy, Manny Skeleton assignment, model regeneration, retexturing, Blender remodeling, Meshy reconnection, and external assets are prohibited.

## Formal additional animation inputs

Formal IDLE input:

- Source: `C:\Users\rinpa\Desktop\Meshy_AI_Brave_Little_Adventur_biped (1).zip`
- SHA-256: `830D95AB15D11565DCFD8C7AE50A687D43598F80B73B339B3DFC06099153B6A6`
- Take: `Idle_11`
- Preserved name: `PLAYER_M_Meshy_Rigged_IDLE_v0.1.zip`
- Working FBX: `PLAYER_M_Meshy_v0.1_IDLE_Idle11.fbx`

Formal DASH input:

- Source: `C:\Users\rinpa\Desktop\Meshy_AI_Brave_Little_Adventur_biped (2).zip`
- SHA-256: `2435688009EBC7D3E31655D609282389C8FCD3C3767DF5AB63C74B61D086523B`
- Take: `Run_02`
- Preserved name: `PLAYER_M_Meshy_Rigged_DASH_Run02_v0.1.zip`
- Working FBX: `PLAYER_M_Meshy_v0.1_DASH_Run02.fbx`

Both ZIPs contain six readable members and pass CRC. Their Base Mesh and four textures match the formal PLAYER_M source by SHA-256, so they are not reimported. Blender structural inspection found the same 24 bone names, root `Hips`, and ordering as the existing PLAYER_M Skeleton.

## Source preservation and manifest

Original copies are stored below:

`SourceArt/StageG1B/PLAYER_M/Meshy/v0.1/AdditionalAnimations/Original`

Only the two animation FBXs are extracted to `AdditionalAnimations/Working/IDLE` and `Working/DASH`. The source, preserved copy, working FBX SHA-256, take, length, frame rate/count, Skeleton, root-motion, and deduplication results are recorded in:

`AdditionalAnimations/Manifest/PLAYER_M_Meshy_v0.1_AdditionalAnimations_Manifest.json`

External communication and Meshy credit use remain zero.

## Unreal animation import

IDLE is imported as:

`/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Animations/IDLE/A_PLAYER_M_Meshy_v0_1_IDLE_Idle11`

- Existing Skeleton: `SKEL_PLAYER_M_Meshy_v0_1`
- Play length: 1.900000 seconds
- 60fps inclusive sampled-frame count: 115
- Root Motion: disabled
- Force Root Lock: enabled
- AnimInstance Root Motion Mode: Ignore Root Motion

The DASH single FBX and the already imported merged `Run_02` have the same take name, Skeleton, 24-bone pose content, and frame-index pose within FBX export precision. Their timing differs: the merged asset is 1.466667 seconds, while the formal single FBX is 0.733333 seconds. The single FBX is therefore a distinct animation and is imported as:

`/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Animations/DASH/A_PLAYER_M_Meshy_v0_1_DASH_Run02`

- Play length: 0.733333 seconds
- 60fps inclusive sampled-frame count: 45
- Root Motion: disabled
- Force Root Lock: enabled

The merged `Run_02` remains retained and unconnected. Only one asset exists in the formal `Animations/DASH` directory.

## Four-state animation presentation

`ABP_PLAYER_M_Meshy_v0_1` continues to derive from `UStageG1BMeshyAnimInstance` and presents one looping sequence selected by state:

- IDLE: `Idle_11`
- WALK: `Walking`
- RUN: `Running`
- DASH: formal single-FBX `Run_02`

Reference Pose is no longer a runtime IDLE source and has no silent fallback. The current horizontal speed drives play rate using 250, 500, or 750uu/s as the nominal value, clamped by Stage G-1B settings. CharacterMovement remains the sole actor-translation owner.

## Three-stage movement mode

Stage G-1B introduces `EStageG1BMoveMode` with WALK, RUN, and DASH. Speeds are externalized in `DefaultGame.ini` through `UStageG1BSettings`:

- `WalkSpeed=250.0`
- `RunSpeed=500.0`
- `DashSpeed=750.0`

The dedicated visual adapter maps WALK/RUN to the accepted Stage G-1A movement modes. DASH keeps the accepted RUN navigation mode but overrides only the Stage G-1B player's `MaxWalkSpeed` to 750 and selects the formal DASH animation. Switching changes speed and animation only; it does not stop movement, issue a destination, rebuild the path, alter the selected target, or generate a causal event.

## Stage G-1B UI and F1

A G1B-only movement widget overlays the unchanged G1A HUD on the dedicated map. Its button cycles:

`歩行 -> 走行 -> ダッシュ -> 歩行`

The overlay consumes the button hit and does not pass it to world movement. No keyboard movement or new shortcut is added.

F1 reports model, 24-bone Meshy Skeleton, Japanese move mode, current state, sequence source, configured speed, measured horizontal speed, play rate, root-motion policy, Manny fallback false, and Idle provisional false.

## Evidence and fallback policy

Runtime evidence exercises multiple Idle_11 cycles, then WALK -> RUN -> DASH -> WALK while retaining destination and NavPath, and finally verifies arrival returns to Idle_11. Runtime fails visibly if any formal Meshy asset is missing. Manny and Reference Pose are never silent fallbacks.

The default packaged map remains `/Game/Maps/StageG1B_OriginalPlayerPoC`. G1A regression uses its explicit accepted map. Package `-map` is cook inclusion only and is not used as a launch argument.
