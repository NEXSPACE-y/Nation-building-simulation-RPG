# Nation-building Simulation RPG — Stage A / Stage B / Stage C

正本の「国家運営シミュレーションRPG 最小MVP 設計・実装指示書」に基づく、Stage A、Stage B、Stage Cのロジック検証環境です。

このリポジトリに含まれる国家名、NPC名、会話、人物設定、初期値は、すべて自動テスト用フィクスチャです。本番コンテンツではありません。Stage C通過後の継続開発はUnreal Engine＋C++構成で行います。

## 技術構成

- C++23
- CMake
- CLI
- JSONデータ駆動（`nlohmann/json` v3.12.0を固定取得）
- CTestから実行する外部テストフレームワーク非依存の受入テスト
- 外部LLM・ゲームエンジン・データベース不使用

## ビルドとテスト

Windows／Visual Studio 2026の例です。

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Debug --target stage_a_fixture_generator
./build/Debug/stage_a_fixture_generator.exe ./data/stage_a_fixture.json
cmake --build build --config Debug --target nation_cli nation_tests
ctest --test-dir build -C Debug --output-on-failure
```

Linux等の単一構成ジェネレーターでは、`--config Debug`と実行ファイルの`Debug/`部分を省略できます。

## CLI

```powershell
./build/Debug/nation_cli.exe
```

CLIでは、プレイヤー行動、移動、時間経過、噂伝達、セーブ／ロード、NPC・国家・イベント・因果経路の確認、監査ログ出力を操作できます。起動後に`help`で一覧を表示します。

簡単な因果連鎖の例です。

```text
move market
action HARM non_ai_npc_002
events 30
trace <COUNTRY_STATE_CHANGEDのevent_id>
export-logs build/manual-evidence
```

## フィクスチャ

- 実行時データ：[data/stage_a_fixture.json](data/stage_a_fixture.json)
- 再生成ツール：[tools/generate_stage_a_fixture.cpp](tools/generate_stage_a_fixture.cpp)

各AI NPCは個別の255状態をJSON内に持ちます。状態1〜5はStage Aの検証に使用し、状態6〜255は各NPCごとに明示的な`UNDEFINED`です。`UNDEFINED`への遷移またはロードはエラーになります。

## 証拠

テスト実行時、`build/test-output/`へ以下を生成します。

- `acceptance_results.txt`
- `stage_a_audit.jsonl`
- `stage_a_causal.jsonl`
- `scenario_d_causal_path.txt`
- `stage_a_reproducible_snapshot.json`
- `mid_chain_pending_save.json`（未処理イベントを保持した中間保存）
- `mid_chain_resumed_save.json`（再開完了後の保存）
- `scenario_g_evidence.json`（連続実行／再開実行のSHA-256一致証拠）
- `scenario_g_continuous_snapshot.json`
- `scenario_g_resumed_snapshot.json`

設計と正本の対応は[Stage A設計](Docs/StageA_Design.md)、実行済みの結果は[Stage A検証報告](Docs/StageA_Verification_Report.md)を参照してください。

## Stage B：Godot＋C#

Stage BはStage AのC++コードを流用せず、同じJSONフィクスチャと[data/stage_parity_contract.json](data/stage_parity_contract.json)を使ってGodot 4.7 .NET＋C#で因果システムを再検証します。

```powershell
dotnet build ./stage-b/NationSimulation.StageB.csproj -c Debug
$godot = "<Godot 4.7 .NET console executable>"
& $godot --headless --editor --path ./stage-b --quit
& $godot --headless --path ./stage-b -- --acceptance
```

Stage Bの証拠は`out/stage-b/test-output/`へ生成されます。詳細は[Stage B設計](Docs/StageB_Design.md)と[Stage B検証報告](Docs/StageB_Verification_Report.md)を参照してください。

## Stage C：Unreal Engine＋C++

Stage CはUnreal Engine 5.8 Runtime ModuleへC++23因果コアを組み込み、Unreal Automation Testとして同じScenario A〜Gを実行します。

```powershell
./stage-c/Build/RunStageCAcceptance.ps1
```

結果は`out/stage-c/test-output/`へ生成されます。詳細は[Stage C設計](Docs/StageC_Design.md)と[Stage C検証報告](Docs/StageC_Verification_Report.md)を参照してください。

Stage C通過後はStage Cを正規の本番継続構成とし、Stage A/Bは回帰基準として保持します。
