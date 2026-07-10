# Stage A 検証報告

## 1. 実施内容

C++23、CMake、CLI、JSONデータ駆動でStage Aを構築した。イベントキュー、目撃、伝聞、AI NPC状態遷移、会話、行動、AI間報告、国家波及、現実時間変換、保存再開、監査・因果ログを自動テストへ接続した。

## 2. 変更ファイル

- `CMakeLists.txt`
- `.github/workflows/stage-a-ci.yml`
- `include/nation_sim/simulation.hpp`
- `src/simulation.cpp`
- `src/main.cpp`
- `tests/test_main.cpp`
- `tools/generate_stage_a_fixture.cpp`
- `data/stage_a_fixture.json`
- `README.md`
- `Docs/StageA_Design.md`
- `Docs/StageA_Verification_Report.md`

正本は変更していない。

## 3. 本書の対応項目

- 1国家、AI NPC 20人、各255状態、NON AI NPC 20人
- JSONによるNPC、状態、遷移、会話、行動、国家初期値、場所、設定の外部化
- 目撃と伝聞を別イベントとして処理
- AI NPCの判断を経由した国家状態変更
- 決定論的選択と再現ログ
- 現実時間1分＝ゲーム内1時間、7日上限
- 原子的セーブとイベント連鎖途中の再開
- CLIによる状態、イベント、関係、因果経路の確認
- CTestによる自動受入テスト

## 4. 実装済み

Stage Aの指定範囲および正本Scenario A〜Gに必要な経路を実装済み。Stage B／Cは未着手。

## 5. 未実装

- Stage BおよびStage C
- 本番国家・NPC・会話・人物設定
- 本番UI
- 正本でMVP対象外とされた大規模人口、完成版経済・政治等

これらをStage A完成扱いには含めない。

## 6. テスト結果

2026-07-10、MSVC 19.50／CMake 4.3.3でDebug構成および新規ディレクトリからのRelease構成を実行。

```text
Fixture contract                                      PASS
Scenario A: NON AI善行の伝聞                         PASS
Scenario B: 暴力の目撃と衛兵行動                    PASS
Scenario C: 信用度の低い伝聞を疑い調査              PASS
Scenario D: AI NPC間報告から国家状態変更             PASS
Scenario E: オフライン時間と7日上限                 PASS
Scenario F: 同一条件の完全再現                       PASS
Scenario G: イベント連鎖途中の保存／再開一致         PASS
CHAIN_LIMIT_REACHED監査                              PASS
指定プレイヤー行動の配線                            PASS
SUMMARY                                               10/10 PASS
```

`ctest --test-dir build -C Debug --output-on-failure`も`100% tests passed`。

クリーンRelease構成の`ctest --test-dir out/clean -C Release --output-on-failure`も`100% tests passed`。フィクスチャ再生成結果はコミット対象JSONとSHA-256が一致した。

```text
62F9B1BAB98FB0E32BB5DB2C7FAAA7F31E9B40DDCF85C94A8ADC46DFACA98A2B
```

実行証拠はテストごとに`build/test-output/`へ生成される。

Scenario Gでは、未処理イベント7件を保持した`mid_chain_pending_save.json`を完了状態で上書きせず保存する。`scenario_g_evidence.json`には連続実行と保存・ロード・再開実行のSHA-256を記録し、両者一致を合格条件としている。

監査ログは`actor`を廃止し、`event_actor_id`、`decision_npc_id`、`target_id`へ分離した。

## 7. 既知の問題

Stage A固有の既知の失敗はない。フィクスチャの状態6〜255は意図的に`UNDEFINED`であり、アクセス時はエラーとなる。本番コンテンツ不足ではなく、正本指定に基づく未使用状態の明示である。

## 8. 仕様確認が必要な点

Stage Aの動作検証用設定について確認待ちはない。本番コンテンツとStage B／Cの仕様は本報告の対象外であり、仮データから推測しない。

## 9. 次に実施する内容

Stage Aを維持し、Stage B／Cはユーザーからの明示指示があるまで開始しない。
