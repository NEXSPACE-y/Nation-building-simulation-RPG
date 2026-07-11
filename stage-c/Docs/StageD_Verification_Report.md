# Stage D Verification Report

## Baseline

- Accepted Stage C commit: `18be9721b184cf9529c62ec24405de6d7ee7a62b`
- Accepted Stage C tag: `stage-c-baseline-v0.1.0`
- Remote branch and annotated tag were verified after push.
- Stage D baseline implementation commit: `49ffdb638624dda3496b9382d66db779dcf31321`
- Stage D annotated tag: `stage-d-playable-v0.1.0`
- Tag target and remote `main` were verified as the same commit on `2026-07-11 +09:00`.

## Automated results

- Existing Stage C Automation: `11/11 tests passed`
- Stage D Playable Vertical Slice Automation: `9/9 tests passed`
- Stage D corrective Automation: `8/8 tests passed`
- Win64 Development BuildCookRun: `BUILD SUCCESSFUL`
- Packaged launch smoke: `PASS`
- Runtime ready marker: Pawn present, NPC count 40, map `StageD_Capital`

Stage D verifies:

1. Exact five location IDs and 20 AI plus 20 NON AI NPCs.
2. Market HARM witnesses are AI 001, 007, 012, and 017.
3. Core-selected actions are REPORT, WARN, REFUSE_TRADE, and FLEE.
4. AI 001 reports to AI 002 with distinct `event_actor_id`, `decision_npc_id`, and `target_id`.
5. Country security changes 50 to 48 and crime level changes 10 to 12.
6. A save with pending events reloads to the same final SHA-256 as uninterrupted execution.
7. Offline processing uses the core seven-day cap.
8. Player, NPC, UMG, persistent subsystem, and capital map presentation surfaces exist.
9. Presentation Actor/UI source files contain no duplicated causal-core decision types.

## Evidence

- `out/stage-d/test-output/stage_d_acceptance_results.txt`
- `out/stage-d/test-output/stage_d_scenario_evidence.json`
- `out/stage-d/test-output/mid_chain_pending_save.json`
- `out/stage-d/package_launch.txt`
- `out/stage-d/package/Windows/NationSimulationStageC/Saved/Logs/NationSimulationStageC.log`

The deterministic mid-chain evidence currently records 12 pending events and matching hashes:

`855d142c881e5f7218843793ef007fbe19cd171cd5b5a8949cb4a2ed43e00016`

## Development package

Executable:

`out/stage-d/package/Windows/NationSimulationStageC.exe`

The package script starts the executable headlessly, confirms the playable-ready log marker, rejects fatal/core-load errors, and terminates both the bootstrap and child game process.

## Corrective results

- Timestamp metadata write failure returns failure with an auditable error.
- Save JSON/metadata integrity mismatch prevents offline time application.
- Same-destination pending MOVE is suppressed; a different destination and later revisit remain valid.
- Missing, inactive, remote-location, and greater-than-350uu NPC targets are rejected before causal event creation.
- UI exposes the rejection reason in the normal display.
- Existing Scenario D still reaches security 48 and crime level 12.

Evidence:

- `out/stage-d/fix-output/stage_d_fix_results.txt`
- `out/stage-d/fix-output/stage_d_fix_evidence.json`

## Corrected Development package

- Build time: `2026-07-11 09:17:47 +09:00`
- Executable SHA-256: `7931C90FA1B8C11845D8A6782B02B03BEF463DE800FFBB7DA258F37D1E2BCC64`
- BuildCookRun: `BUILD SUCCESSFUL`
- Headless launch readiness: `PASS`

## Manual acceptance status

The authoritative 18-item manual checklist is complete: `18/18` passed.

- Items 1–16 and 18 were recorded through Codex Computer Use foreground operation.
- Item 17 was confirmed by the user through real-machine human operation. The Computer Use axis-input limitation is retained only as a traceability note and is not treated as a failed acceptance item.
- The detailed record is `stage-c/Docs/StageD_Manual_Verification_Checklist.md`.

Stage D therefore satisfies the manual-acceptance prerequisite for baseline commit, annotated tag creation, push, and the start of Phase E-Design.
