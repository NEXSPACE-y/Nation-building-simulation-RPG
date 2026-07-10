# Stage C 設計固定

## 1. 適用範囲

Stage CはUnreal Engine 5.8＋C++23を本番継続構成として採用し、Stage A/Bで確定した因果システムをUnreal EditorモジュールとAutomation Testへ接続する段階である。

Stage C通過後の新規開発は`stage-c/`のUnreal構成で進める。Stage A/Bは回帰基準と証拠として保持するが、新機能の実装先にはしない。

## 2. 技術構成

- Unreal Engine 5.8.0
- UnrealBuildTool
- C++23
- Visual Studio 2026 MSVC 14.50.35728
- Windows SDK 10.0.26100
- Unreal Automation Test
- NullRHIによるヘッドレス検証
- `nlohmann/json` 3.12.0（MIT、固定ベンダー）

## 3. モジュール境界

```text
Unreal Engine 5.8
  └─ NationSimulationStageC Runtime Module
       ├─ Unrealライフサイクル／Automation Test
       ├─ Stage C bridge
       └─ engine-independent C++23 causal core
            ├─ JSON fixture
            ├─ deterministic event queue
            ├─ save/load
            └─ audit/causal logs
```

因果コアはUnreal型へ不要に依存させない。Unreal固有のScene、Subsystem、UI、アセット接続はモジュール境界の外側から追加する。これによりヘッドレス回帰試験を維持しながら本番機能を拡張する。

## 4. データと受入契約

Stage A/B/Cは次を共有する。

- `data/stage_a_fixture.json`
- `data/stage_parity_contract.json`

受入契約はNPC件数、255状態、期待状態、選択行動、国家差分、Scenario Dの完全イベント列、時間変換を固定する。Stage Cが契約を変更して通過扱いにすることは禁止する。

## 5. Unreal統合証拠

`NationSimulation.StageC.Acceptance`はUnreal Automation Testとして登録される。`UnrealEditor-Cmd.exe`をNullRHI／unattendedで起動し、Unrealのモジュールロード、テスト検出、実行、終了コード0までを検証する。

## 6. Stage間整合性

Scenario Dの因果経路はStage A/B/Cで次の8イベントが完全一致する。

```text
PLAYER_HARMED_NON_AI_NPC
AI_NPC_OBSERVED_EVENT
AI_NPC_STARTED_ACTION
AI_NPC_COMPLETED_ACTION
AI_NPC_HEARD_EVENT
AI_NPC_STARTED_ACTION
AI_NPC_COMPLETED_ACTION
COUNTRY_STATE_CHANGED
```

## 7. Scenario G

Stage Cも中間保存と再開完了保存を分離する。

- 中間保存：tick 4、次採番12、未処理7件
- 完了結果：27イベント
- 連続実行／保存再開のSHA-256一致を必須とする

Stage AとStage Cは同一C++因果コアのため、正規化スナップショットのSHA-256も一致する。Stage BはC#保存表現が異なるため、Stage内の連続／再開一致と共有受入契約の一致を検証する。

## 8. 監査主体

全Stageで次を固定する。

- `event_actor_id`
- `decision_npc_id`
- `target_id`

報告者と判断主体を同一フィールドへ戻してはならない。

## 9. 継続開発規則

- 本番コードはStage C Unrealモジュールへ追加する
- 因果コアの変更時はStage A/B/C受入試験をすべて再実行する
- UIやアセット都合で因果処理を直接変更しない
- データ駆動定義をコードへ戻さない
- Automation Test未通過を完成扱いにしない

