# Stage F Production Scale Runtime Foundation 検証報告

## 1. 実施内容

Stage E基準点から、5国家、2,500 AI NPC、637,500状態枠、350,000,000 NON AI人口枠を扱う本番規模ランタイム基盤を実装した。scale data生成・検査、activity tier、due scheduler、国家partition、人口仮想化、世代保存・復旧、Stage E移行、ログsegment、7日オフライン、Unreal接続、負荷試験を配線した。

## 2. Stage E基準点確認

作業開始時のbranchは`main`、HEADと`origin/main`と注釈付きtag `stage-e-behavioral-depth-v0.1.0`のtargetはすべて`8313d46a37370e0544c55100f21e81a86565cd4d`で一致した。Stage E受入済み状態から開始し、commit、tag、pushは行っていない。

## 3. 変更ファイル

- root core: `CMakeLists.txt`、`include/nation_sim/stage_f_runtime.hpp`、`src/stage_f_runtime.cpp`
- scale data: `stage-c/Data/StageF/*.json`
- generator/validator: `stage-c/Tools/StageF*`
- core tests: `tests/stage_f_acceptance.cpp`、`tests/stage_f_validator_negative.cpp`、`tests/stage_f_performance.cpp`
- Unreal: Build.cs、StageCCoreBridge、GameInstanceSubsystem、StageDTypes、StageDHudWidget、Stage F Automation test
- build: `stage-c/Build/RunStageFAcceptance.ps1`、`stage-c/Build/PackageStageF.ps1`
- design/report: `stage-c/Docs/StageF_*.md`
- ignore: `.gitignore`へ`stage-c/Data/StageF/Generated/`を追加

共有fixture、共有受入契約、Stage E状態定義は変更していない。`out/`と生成scale packはGit追跡対象にしていない。

## 4. アーキテクチャ概要

既存`nation_sim::Simulation`を画面表示範囲の因果正本として維持し、`StageFProductionRuntime`が本番規模partition、scheduler、人口空間、世代保存、ログarchiveを管理する。UnrealはGameInstanceSubsystemから両coreを所有し、ActorとHUDは入力・表示だけを行う。2,500 AI Actorや巨大NON AI Actorは生成しない。

## 5. 5国家・2,500 AI NPCの実装結果

country_001～005へ各500 AIを固定割当し、合計2,500永続個体をロードした。既存ai_npc_001～020を保持した。活動階層はACTIVE 20、BACKGROUND 496、DORMANT 1,984である。PASS。

## 6. 637,500状態の格納・検査結果

各AIに固有255状態枠、合計637,500をNPC単位shardへ分割した。生成物は5,010ファイル、879,817,053 bytes。state index、definition hash manifest、shard SHA、world SHAを検証し、cache上限32で遅延ロードする。Stage E対象3 NPCのstate 6～25と既存入口規則を保持した。PASS。

world SHA-256:

```text
d8fe2891b6faf63b703f417a97c79a93d4adbd13ce90287e0deae939984ca2b3
```

## 7. activity tierとscheduler結果

production pathはACTIVE set、BACKGROUND due queue、DORMANT skipを使い、2,500件の毎tick走査を行わない。同時刻順は`scheduled_at / target_country_id / priority / event_id / actor_id`で固定した。全ACTIVE参照とtier処理のcanonical SHAは一致した。PASS。

```text
5b271f7b40832d3e22a594600cc9a84f127231a86f106eec35ea7b843adecbd6
```

## 8. NON AI人口空間結果

定住250,000,000枠と流動100,000,000枠をseed/index空間で表現した。全件ID・runtime・saveは生成しない。同一seed/index/countryから同一個体を再生成し、別indexは別個体になる。MATERIALIZEDからPROMOTEDへ昇格した個体はsave/load後もIDと重要event記憶を保持した。PASS。

## 9. Stage E→F移行結果

既存`stage_d_save.json`論理slotを継続した。Stage E saveとmetadataをSHA付き不変名へバックアップし、visible core全文を保持した。player、country_001、既存20 AI、Stage E対象3 NPC、pending、history、audit、causal chainを維持し、残る4国家と2,480 AIをfixture既定値で追加した。移行失敗時のsource byte一致と二重移行抑止を確認した。PASS。

## 10. 保存世代・復旧結果

一時世代へshardを書き、全SHA読戻し後だけcurrent generationを切り替える。6障害注入点は6/6 PASS。破損世代`gen_000000000002`を拒否し、`gen_000000000001`へ復旧した。archive segmentからroot causal pathを再構築できる。PASS。

## 11. オフライン結果

604,800秒の上限を適用し、pending再開、due処理、country集約、population集約を区間処理した。連続実行とsave/load再開のSHAは一致した。PASS。

```text
1eeec575f3062ab752e72cdc3c2808bbc2a8bd9f43e185c430022335202c12ca
```

## 12. 決定論hash

- scale pack再生成: `d8fe2891...984ca2b3`で2回一致
- tier/reference: `5b271f7b...adecbd6`で一致
- 7日offline再開: `1eeec575...2c12ca`で一致
- 10,000 event: `7a222cd4...ed328`
- 30分soak最終: `17745071...4211d`

## 13. 負荷試験結果

- 完成規模load: PASS
- event 10,000件: PASS、pending 0
- 7日offline: PASS
- save/load 100回: PASS、hash不一致0
- 中断復旧6点: 6/6 PASS
- 30分soak: PASS、16,054 event、29 save/load

## 14. memory/時間実測

- 完成規模load: 409 ms、8,630,272 bytes
- 10,000 event: 228 ms
- save/load 100回: 77,372 ms
- 標準試験peak working set: 42,254,336 bytes
- soak warmup/end: 13,619,200 / 14,163,968 bytes
- soak増加率: 4.0%、pending 0、crash 0、deadlock 0

詳細は`stage-c/Docs/StageF_Performance_Report.md`と性能JSONを正本とする。

## 15. Stage A～E回帰結果

| 対象 | 結果 |
|---|---|
| Existing C++ causal core | 10/10 PASS |
| Stage C | 11/11 PASS |
| Stage D | 9/9 PASS |
| Stage D corrective | 8/8 PASS |
| Stage E behavioral | 8/8 PASS |
| Stage E migration | 4/4 PASS |
| Stage E validator | 22/22 PASS |

期待値、共有fixture、共有受入契約は変更していない。

## 16. Stage F Scenario結果

core Scenario F-1～F-9は9/9 PASS。Unreal表示境界に相当するF-10はStage F Unreal Automation 5/5とDevelopment launch smokeでPASSした。

## 17. Validator結果

正常packは`ERROR=0 / issues=0`。指示書所定の21 ERROR codeはすべて専用負例で実発火し、21/21 PASS。generatorの同一入力2回実行でworld SHAが一致した。

## 18. Package/smoke結果

Win64 Development packageを生成した。起動ごとに固有UserDirを作り、今回の新規runtime logだけを判定した。

- map: `StageD_Capital`
- Player Pawn: あり
- presentation NPC: 40
- Stage F country: 5
- Stage F AI: 2,500
- ACTIVE/BACKGROUND/DORMANT: 20 / 496 / 1,984
- `STAGE_F_CORE_READY`: PASS
- headless launch smoke: PASS

## 19. 既知の問題

Stage F受入範囲内の既知障害はない。保存直後に一時的なworking setピークがあるが、30分soak終了値はwarmup比4.0%であり、恒常増加ではない。生成scale packは約880 MBであるため、受入時に再生成しGit追跡しない。

## 20. 未実装

Stage F指示範囲の未完了項目はない。並列workerは導入していないためworker数差試験は適用外である。本番コンテンツ、本番ビジュアル、完成版戦闘、クエスト、政治・経済制度は指示どおり対象外である。

## 21. 仕様確認が必要な点

Stage F完了に必要な追加確認事項はない。Stage Gには着手していない。

## 22. 証拠パス

- `out/stage-f/stage_f_acceptance_results.txt`
- `out/stage-f/test-output/stage_f_scenario_evidence.json`
- `out/stage-f/test-output/stage_f_scale_contract.json`
- `out/stage-f/test-output/stage_f_determinism_evidence.json`
- `out/stage-f/test-output/stage_f_offline_evidence.json`
- `out/stage-f/test-output/stage_f_save_recovery_evidence.json`
- `out/stage-f/test-output/stage_f_non_ai_population_evidence.json`
- `out/stage-f/validation/stage_f_validation.jsonl`
- `out/stage-f/validation-negative/stage_f_validator_negative_evidence.json`
- `out/stage-f/performance/stage_f_performance.json`
- `out/stage-f/performance/stage_f_soak.json`
- `out/stage-f/package_launch.txt`

