# Stage D Playable Vertical Slice Design

## Scope

Stage D connects the accepted Stage C causal core to a playable Unreal Engine 5.8 map. All Stage D implementation is contained under `stage-c/`. Stage A, Stage B, the shared fixture, the parity contract, and the engine-independent C++23 causal core are unchanged.

## Runtime ownership

`UNationSimulationGameInstanceSubsystem` is the sole runtime owner of `nation_sim::Simulation`.

- It loads the shared JSON fixture in editor builds and the staged JSON fixture in packaged builds.
- It accepts player commands through `enqueue_player_action`.
- It advances a causal chain one core event at a time through `process_pending(1)` so the chain is visible and can be saved while pending work remains.
- It saves and reloads the core JSON without converting it to an Unreal save schema.
- It defers startup offline progress until restored pending events have resumed, then calls the core `advance_offline` implementation.
- It projects immutable world/NPC views for Actors and UMG.

No presentation Actor includes `simulation.hpp` or calls a `nation_sim` decision type.

## Presentation classes

- `AStageDGameMode`: builds the placeholder capital block, spawns 40 NPC presentation Actors, creates UMG, and pumps the persistent subsystem.
- `AStageDPlayerCharacter`: Third Person-style `ACharacter`, spring-arm camera, movement, target selection, and command submission.
- `AStageDNpcActor`: displays `npc_id`, AI/NON AI classification, role, and the action selected by the core. REPORT, WARN, REFUSE_TRADE, and FLEE are presentation mappings only.
- `AStageDLocationVolume`: reports entry into the exact fixture location ID.
- `UStageDHudWidget`: displays target, actions, conversation, location, country state, recent event, save/load status, and debug fields.

## Map

`Content/Maps/StageD_Capital.umap` contains the Stage D PlayerStart and uses `AStageDGameMode`. Runtime placeholder geometry creates five connected zones:

- `capital`
- `market`
- `tavern`
- `residential`
- `gate`

All meshes and labels use Unreal Engine basic shapes and text rendering. They are validation assets, not production content.

## Controls

- `W/A/S/D`: move
- Mouse: rotate camera
- `Tab`: cycle target NPC
- `1`: TALK
- `2`: HELP
- `3`: HARM
- `4`: TRADE
- `5`: STEAL
- `6`: WAIT
- `7`: move to the next fixture location
- `F5`: mid-chain save
- `F9`: reload save
- `F1`: toggle causal debug fields

The same actions are available as UMG buttons.

## Save and offline sequence

1. A player action is enqueued in the core.
2. Unreal projects one processed event every 0.30 seconds.
3. F5 persists both processed history and the remaining pending queue atomically through the core save implementation.
4. On a later launch, the subsystem restores that queue and resumes it one event at a time.
5. When the restored queue is empty, wall-clock elapsed time is passed to the core offline implementation, including its seven-day cap.

The timestamp sidecar uses schema `stage_d_save_metadata_v2` and binds `saved_at_utc_epoch` to the exact save JSON SHA-1. A failed write, missing/legacy sidecar, or hash mismatch makes the save/offline operation fail audibly; stale time is never applied.

## Interaction and MOVE boundary

- `interaction_range` is externalized as `InteractionRange=350.0` in `DefaultGame.ini`.
- Target actions require a core NPC, a live NPC Actor, the same fixture `location_id`, and a distance at or below the configured range.
- A rejected action never reaches `enqueue_player_action`; the reason is shown in normal UMG and emitted as `STAGE_D_ACTION_REJECTED`.
- Targets are cleared on location change, range/location invalidation, Actor destruction, or one second without a valid view target.
- MOVE checks the core pending queue for the same player and destination before enqueueing.
- A destination can be revisited after the previous MOVE is processed, the player has left it, and no MOVE back to it is already pending.

## Build tooling

- `Build/GenerateStageDMap.ps1`: regenerates the validation map.
- `Build/RunStageDAcceptance.ps1`: runs unchanged Stage C acceptance and Stage D acceptance.
- `Build/PackageStageD.ps1`: builds, cooks, packages, launches, and verifies a Win64 Development build.
