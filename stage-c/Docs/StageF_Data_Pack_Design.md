# Stage F Data Pack 設計

## 1. 生成物

追跡対象は生成器、validator、schema、generation config、hash contractである。約規模データは`stage-c/Data/StageF/Generated/`へ再生成し、Git追跡しない。

```text
world_manifest.json
countries/country_001.json ... country_005.json
ai_manifests/ai_npc_001.json ... ai_npc_2500.json
state_shards/ai_npc_001.json ... ai_npc_2500.json
state_index.json
definition_hash_manifest.json
population_spaces.json
expected_count_hash_contract.json
```

## 2. 規模契約

| 項目 | 値 |
|---|---:|
| 国家 | 5 |
| AI / 国家 | 500 |
| AI合計 | 2,500 |
| 状態 / AI | 255 |
| 状態枠合計 | 637,500 |
| 定住NON AI枠 | 250,000,000 |
| 流動NON AI枠 | 100,000,000 |

AI IDは連番を500件ずつcountry_001～005へ割り当てる。各manifestとstate shardはSHA-256で参照し、world manifestもSHA-256で証拠化する。

## 3. 既存定義保護

ai_npc_001～020のstate 1～5は`data/stage_a_fixture.json`のobjectを変更せず使用する。ai_npc_001、002、012のstate 6～25はStage E overlayのobjectを変更せず使用する。旧状態からの入口は`external_transition_rules`に保持し、Stage E対象3 NPCはoverlayの15入口規則をそのまま利用する。

残りは構造・負荷検証専用fixtureである。state name、説明、台詞、goal、rule IDへnpc_idとstate_idを含め、別NPCへ同一255状態表をコピーしない。

## 4. 生成決定論

generatorは固定seedと昇順IDを使用し、object key順をnlohmann JSONのordered map規則、indent 2、LF終端で固定する。同じconfig、fixture、overlayからbyte-identicalなshardと同じworld SHAを生成する。

## 5. validator

正常packでは全2,500 state shard、637,500状態を走査し`ERROR=0`だけを受け入れる。必須21 ERROR codeは専用負例で実発火する。JSONLは`severity / country_id / npc_id / state_id / rule_id / error_code / message / source_file`を出力する。

検査は件数、ID範囲・重複、国家配分、SHA、索引、Stage A/E定義保持、同一状態表、未定義event/action/relationship、到達不能、即時loop、priority衝突、schema境界を含む。

## 6. コマンド

```powershell
cmake --build out/stage-f/core-build --config Release --target stage_f_scale_data_generator stage_f_validator
out/stage-f/core-build/Release/stage_f_scale_data_generator.exe stage-c/Data/StageF/stage_f_generation_config.json data/stage_a_fixture.json stage-c/Data/StageE/stage_e_state_definitions.json stage-c/Data/StageF/Generated
out/stage-f/core-build/Release/stage_f_validator.exe stage-c/Data/StageF/Generated data/stage_a_fixture.json stage-c/Data/StageE/stage_e_state_definitions.json out/stage-f/validation/stage_f_validation.jsonl
```
