# Stage C 検証報告

## 1. 実施内容

Unreal Engine 5.8＋C++23のStage Cプロジェクトを構築した。因果コアをUnreal Runtime Moduleへ組み込み、Unreal Automation Testから共有フィクスチャと受入契約を実行した。

## 2. 変更ファイル

- `stage-c/NationSimulationStageC.uproject`
- `stage-c/Config/`
- `stage-c/Source/NationSimulationStageC.Target.cs`
- `stage-c/Source/NationSimulationStageCEditor.Target.cs`
- `stage-c/Source/NationSimulationStageC/`
- `stage-c/Build/RunStageCAcceptance.ps1`
- `.github/workflows/stage-c-ci.yml`
- `Docs/StageC_Design.md`
- `Docs/StageC_Verification_Report.md`

正本は変更していない。

## 3. 本書の対応項目

- JSONデータ駆動
- AI NPC 20人×個別255状態
- 目撃、伝聞、AI間連鎖、国家波及
- 決定論的再現
- オフライン集約
- 中間保存と再開
- 監査ログ、因果ログ
- Unreal Automation Test

## 4. 実装済み

Stage CのUnreal C++プロジェクト、Runtime Module、C++23因果コア接続、Automation Test、実行スクリプト、self-hosted CI定義を実装済み。

## 5. 未実装

- 本番国家・NPC・会話コンテンツ
- 本番UI
- Unrealアセット、マップ、演出

これらは因果システムのStage C通過条件には含めない。

## 6. テスト結果

```text
UnrealBuildTool Development Editor build                PASS
Unreal module load                                      PASS
Unreal Automation Test discovery                        PASS
Fixture contract                                        PASS
Scenario A                                              PASS
Scenario B                                              PASS
Scenario C                                              PASS
Scenario D                                              PASS
Scenario E                                              PASS
Scenario F                                              PASS
Scenario G                                              PASS
CHAIN_LIMIT_REACHED                                     PASS
Player action wiring                                    PASS
Audit identity fields                                   PASS
SUMMARY                                                  11/11 PASS
```

実行環境：

```text
Unreal Engine 5.8.0, changelist 55116800
MSVC 14.50.35728
Windows SDK 10.0.26100
C++23
Automation exit code 0
```

## 7. Stage間比較

- Scenario D：Stage A/B/CのイベントID・イベント種別8件が完全一致
- Scenario G：全Stageでtick 4、次採番12、未処理7件、完了27イベント
- Stage A/C正規化スナップショットSHA-256一致

```text
855d142c881e5f7218843793ef007fbe19cd171cd5b5a8949cb4a2ed43e00016
```

## 8. 既知の問題

ローカルUnreal Automation Testの既知の失敗はない。GitHub ActionsのStage CジョブはUE 5.8導入済みWindows self-hosted runnerを前提とし、まだGitHub上では実行していない。

## 9. 次に実施する内容

以後の本番開発はStage CのUnreal Engine＋C++構成で継続する。Stage A/Bは回帰基準として保持する。

