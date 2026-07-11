# Stage E 保存・移行設計

## 1. 適用範囲

本書はStage DからStage Eへのランタイムsave移行だけを定義する。実装コード、Automation Test、NPC行動、状態定義、UI、次工程は本phaseの対象外とする。

ランタイム保存先は既存の`stage_d_save.json`を継続使用する。timestamp metadataも既存の`stage_d_save.json.utc`を継続使用する。Stage E専用の別ランタイムsaveは作成しない。

## 2. Stage E save schema

- `schema_version`: `stage_e_save_schema_v1`
- `simulation_version`: `stage-e-0.1.0`
- `fixture_id`: `stage_a_fixture_v1`
- `fixture_schema_version`: `1.0.0`
- `stage_e_overlay.overlay_schema_version`: `stage_e_state_overlay_v1`

```json
{
  "fixture_id": "stage_a_fixture_v1",
  "schema_version": "stage_e_save_schema_v1",
  "fixture_schema_version": "1.0.0",
  "simulation_version": "stage-e-0.1.0",
  "stage_e_overlay": {
    "overlay_id": "stage_e_behavioral_depth_v1",
    "overlay_schema_version": "stage_e_state_overlay_v1",
    "base_fixture_id": "stage_a_fixture_v1",
    "definition_sha256": "<正規化した状態定義JSONのSHA-256>"
  },
  "migration": {
    "migration_id": "stage_d_to_stage_e_v1",
    "source_schema_version": "1.0.0",
    "source_simulation_version": "stage-a-0.1.0",
    "source_save_sha256": "<移行前stage_d_save.jsonのSHA-256>",
    "source_metadata_sha256": "<移行前stage_d_save.json.utcのSHA-256>",
    "source_backup_path": "stage_d_save.json.stage-d-<source_save_sha256>.bak",
    "metadata_backup_path": "stage_d_save.json.utc.stage-d-<source_save_sha256>.bak"
  },
  "world": "<Stage D値を保持>",
  "country": "<Stage D値を保持>",
  "player": "<Stage D値を保持>",
  "ai_runtime": "<Stage D値を保持しStage E項目を補完>",
  "non_ai_runtime": "<Stage D値を保持>",
  "event_history": "<Stage D値を保持>",
  "audit_log": "<Stage D値を保持>",
  "pending_events": "<Stage D値を保持>",
  "root_processed_counts": "<Stage D値を保持>"
}
```

`simulation_version`はStage D値のまま保持せず、移行後に`stage-e-0.1.0`へ更新する。移行前の値は`migration.source_simulation_version`へ保存する。

## 3. Stage E追加フィールドの補完

Stage D saveに存在しない値は、移行時に以下の明示的な既定値を一度だけ補完する。既存値は上書きしない。

| 追加対象 | 既定値 |
| --- | --- |
| `stage_e_overlay` | 実際に読み込むoverlay ID、schema、base fixture ID、正規化JSONのSHA-256。 |
| `state_entered_at` | `world.current_world_time_minutes`。 |
| `timed_transition_at` | `null`。 |
| `legacy_state_pending_stage_e_entry` | 旧state 1〜5の場合は`true`。それ以外は`false`。 |
| `evidence_evaluation.source_event_id` | 空文字列。 |
| `evidence_evaluation.credibility` | `0.0`。 |
| `evidence_evaluation.evidence_level` | `0.0`。 |
| `evidence_evaluation.perception` | `NONE`。 |
| `last_transition_reason` | `MIGRATED_FROM_STAGE_D`。 |
| `last_rule_evaluations` | 空配列。 |
| GUARDの不足関係値 | 対playerの`trust=50`、`fear=0`。対CAPTAINの`loyalty=50`。 |
| BROKERの不足関係値 | 対playerの`trust=50`、`fear=0`、`commercial_interest=50`。対GUARD／CAPTAINの`trust=50`。 |
| CAPTAINの不足関係値 | 対playerの`trust=50`、`fear=0`、`utility=0`。対GUARD／BROKERの`trust=50`。 |

既存の`current_state_id`、`current_goal`、`player_evaluation`、relationships、known events、used rules、cooldown、once-only消費、active action、pending eventsはそのまま引き継ぐ。旧state 1〜5は移行時に別stateへ置換せず、明示的なlegacy entry ruleが成立した時だけStage E state 6〜25へ遷移する。

## 4. マイグレーション計画

1. `stage_d_save.json`と`stage_d_save.json.utc`をバイト列として読み取る。
2. Stage Dの`schema_version=1.0.0`、`simulation_version=stage-a-0.1.0`、fixture ID、metadata schema、save SHAを検証する。
3. 移行元JSONとmetadataのSHA-256を計算する。
4. Stage E追加フィールドを既定値で補完し、`schema_version`と`simulation_version`を更新する。
5. 新JSONと対応metadataを一時ファイルへ書き、両方を読戻して検証する。
6. SHA付き不変名のバックアップを確立する。
7. 一時JSONと一時metadataを既存の`stage_d_save.json`／`.utc`へ昇格する。
8. Stage E schemaとして再読込みし、schema、simulation version、overlay SHA、runtime項目、pending eventsを検証する。
9. 失敗時はSHA検証済みバックアップから元JSONとmetadataを復元し、一時ファイルを削除する。

移行完了後も保存パスは変えない。`migration`情報を保存するため、同じsaveに対してStage D→E移行を繰り返さない。

## 5. バックアップ方針

- JSON: `stage_d_save.json.stage-d-<source_save_sha256>.bak`
- metadata: `stage_d_save.json.utc.stage-d-<source_save_sha256>.bak`
- バックアップ名には移行前JSONのSHA-256を含める。固定名を使用しない。
- バックアップは不変とし、既存ファイルを上書きしない。
- 同名バックアップが既に存在する場合、内容と記録済みSHAが移行元に一致すれば再利用する。
- 同名バックアップの内容が一致しない場合、移行を失敗させて元saveを変更しない。
- JSONとmetadataの両バックアップが確立するまで、一時ファイルをランタイム保存先へ昇格しない。
- 昇格または読戻し検証に失敗した場合、バックアップから元の2ファイルを復元する。

## 6. 失敗時契約

次のいずれかが発生した場合、移行は失敗とし、元の`stage_d_save.json`と`.utc`のSHA-256が移行前と同一であることを保証する。

- Stage D schemaまたはsimulation version不一致
- metadata欠落・schema不一致・save SHA不一致
- Stage E定義SHAを確定できない
- 一時ファイルの書込み・読戻し失敗
- バックアップ作成失敗
- 既存バックアップの内容不一致
- JSONとmetadataの片方だけの昇格失敗
- Stage E再読込み検証失敗

エラーは握り潰さず、失敗工程、source SHA、backup path、復元結果を監査可能な形式で記録する。

## 7. 後続phaseで必要となる試験契約

本phaseでは試験コードを実装しない。後続の`E-save-tests`で最低限、次を自動検証する。

- Stage D save読込と全既存runtime値の継承
- Stage E追加項目の既定値補完
- `simulation_version=stage-e-0.1.0`への更新とsource version保存
- Stage E再保存・再読込み結果の一致
- SHA付きバックアップの作成と非上書き
- 既存バックアップ内容不一致時の移行拒否
- 移行失敗時の元JSON／metadata SHA不変
- legacy state 1〜5の保持と明示的入口規則によるStage E stateへの遷移

## 8. Phase gate

- `save_design_has_schema_version`: 本書にStage E schemaとsimulation versionが明記されている。
- `save_design_has_migration_plan`: Stage D saveから同一パスのStage E saveへ移行する手順、既定値、失敗時復元が明記されている。
- `save_design_has_backup_policy`: SHA付き不変バックアップ、非上書き、内容検証、復元方針が明記されている。
