# Stage B 検証報告

## 1. 実施内容

Godot 4.7 stable .NET版とC#／.NET 8でStage Bを構築した。Stage Aと同じJSONフィクスチャおよび共有受入契約を読み込み、Godotヘッドレスプロセス内で因果システムを検証した。

## 2. 変更ファイル

- `stage-b/project.godot`
- `stage-b/Main.tscn`
- `stage-b/NationSimulation.StageB.csproj`
- `stage-b/src/Main.cs`
- `stage-b/src/Models.cs`
- `stage-b/src/Simulation.cs`
- `stage-b/tests/StageBAcceptance.cs`
- `data/stage_parity_contract.json`
- `.github/workflows/stage-b-ci.yml`
- `Docs/StageB_Design.md`
- `Docs/StageB_Verification_Report.md`

Stage Aには、共有契約の参照、Scenario G証拠分離、監査主体フィールド分離だけを反映した。正本は変更していない。

## 3. 本書の対応項目

- 20 AI NPC×個別255状態の共有JSON読込
- 目撃・伝聞・関与の分離
- AI NPC間報告と国家波及
- 決定論的再実行
- オフライン集約
- 連鎖途中の保存・ロード・再開
- 監査ログと因果経路ログ
- Godot実行環境内の自動テスト

## 4. 実装済み

Stage BのGodot＋C#因果システム、Godotヘッドレス受入ランナー、共有契約、CIを実装済み。Stage Cは未着手。

## 5. 未実装

- Stage C／Unreal Engine＋C++
- 本番コンテンツ
- 本番UI

## 6. テスト結果

```text
Fixture contract                                      PASS
Scenario A: NON AI善行の伝聞                         PASS
Scenario B: 暴力の目撃と衛兵行動                    PASS
Scenario C: 信用度の低い伝聞を疑い調査              PASS
Scenario D: AI NPC間報告から国家状態変更             PASS
Scenario E: オフライン時間と7日上限                 PASS
Scenario F: 決定論的再実行                           PASS
Scenario G: 未処理イベントを保持した保存・再開一致  PASS
CHAIN_LIMIT_REACHED                                  PASS
指定プレイヤー行動の配線                            PASS
監査主体フィールド分離                              PASS
SUMMARY                                               11/11 PASS
```

Godot実行環境：`4.7.stable.mono.official.5b4e0cb0f`

Scenario DのStage A/B因果経路はイベントID・イベント種別とも完全一致した。

Scenario GのStage B中間保存は`simulation_tick=4`、`next_event_sequence=12`、`pending_event_count=7`。連続実行と再開実行はいずれも27イベントで、SHA-256は一致した。

```text
edfc926cb100c8926796968b61e0a7c40e7860357fd6fd7e88d0bd4bb5dfcc58
```

## 7. 既知の問題

Stage B受入テストの既知の失敗はない。画面は本番UIではなく、GodotとC#の接続を検証する最小Nodeだけである。

## 8. 仕様確認が必要な点

Stage Bの検証用設定について確認待ちはない。Stage Cの仕様は本報告から推測しない。

## 9. 次に実施する内容

Stage Bを維持し、Stage Cはユーザーからの明示指示があるまで開始しない。

