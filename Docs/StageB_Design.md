# Stage B 設計固定

## 1. 適用範囲

Stage Bは、Stage Aで成立した因果システムをGodot 4.7 .NET＋C#で独立実装し、同じ入力データ、シード、受入契約で再検証する段階である。Stage CおよびUnreal Engineは対象外とする。

## 2. 技術構成

- Godot 4.7 stable .NET版
- C#／.NET 8
- Godotヘッドレス実行
- `System.Text.Json`
- 外部テストフレームワーク不使用のGodot内受入ランナー
- Stage Aと共有するJSONフィクスチャおよび受入契約

Godotプロジェクトは`stage-b/`に隔離し、Stage AのC++コードを参照しない。コードの偶然の共有ではなく、データと観測可能な結果の一致で検証する。

## 3. 共有境界

```text
data/stage_a_fixture.json
data/stage_parity_contract.json
          ↓
    ┌─────┴─────┐
    ↓           ↓
Stage A C++   Stage B C# / Godot
    ↓           ↓
同一Scenario A〜G・同一因果経路・同一国家差分
```

共有受入契約は、国家・NPC件数、状態数、Scenario A〜Eの期待状態・行動・国家差分、Scenario Dの完全なイベント種別列を持つ。

## 4. Godot統合

`Main.tscn`のNodeへC#スクリプトを接続する。通常のヘッドレス起動ではフィクスチャをロードし、`-- --acceptance`指定時はGodotプロセス内で全受入テストを実行して終了コードを返す。

```text
Godot SceneTree
  └─ Main.cs
       ├─ Simulation.cs
       └─ StageBAcceptance.cs
```

本番UIは作成しない。Godotとのビルド・SceneTree・C#アセンブリ接続を実証する最小シーンだけを持つ。

## 5. 因果・決定論

イベント採番、処理順、目撃スコア、伝聞信用度、ルール優先度、FNV-1a決定値はStage Aと同じ定義を使用する。Scenario Dの因果経路はイベントIDを含めStage Aと一致させる。

## 6. 監査主体

監査ログでは次を分離する。

- `event_actor_id`：入力イベントの主体または報告者
- `decision_npc_id`：ルール評価、状態遷移、会話・行動選択を実行したAI NPC
- `target_id`：知覚イベントの対象

AI NPC 001の報告をAI NPC 002が判断する場合は、順に`ai_npc_001`、`ai_npc_002`、`ai_npc_002`となる。

## 7. Scenario G証拠

中間保存と再開完了保存は別ファイルにする。中間保存を完了状態で上書きしない。

- `mid_chain_pending_save.json`：未処理イベントを保持
- `mid_chain_resumed_save.json`：再開完了後
- `scenario_g_continuous_snapshot.json`：無停止実行結果
- `scenario_g_resumed_snapshot.json`：保存・ロード・再開結果
- `scenario_g_evidence.json`：未処理イベント一覧と両SHA-256

両スナップショットのSHA-256一致をテストの合格条件とする。

## 8. Stage B通過条件

- Godot 4.7 .NET版でC#プロジェクトをビルドできる
- Godotヘッドレスプロセス内でScenario A〜Gが通る
- Stage A/B共有契約を満たす
- Scenario Dの因果イベント列がStage Aと完全一致する
- Scenario Gの中間保存に未処理イベントが存在する
- 連続／再開結果のSHA-256が一致する
- 監査主体3項目が曖昧なく出力される

