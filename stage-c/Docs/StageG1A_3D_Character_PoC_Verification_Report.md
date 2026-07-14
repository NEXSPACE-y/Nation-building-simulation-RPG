# Stage G-1A 標準3Dキャラクター技術PoC 検証報告

## 現在の正式判定

- Stage G-1A: **正式受入・完了**
- 技術実装: 実装・Package済み
- Automation G1A-1～G1A-22: **22/22 PASS**
- Stage C～G-0 Unreal回帰: **PASS**
- Stage A～F core/validator回帰: **PASS**
- Win64 Development Package: **PASS**
- 初回Computer Use確認: **PASS**（既存G1A実装）
- WALK/RUN是正後Computer Use確認: **PASS**
- 初回ユーザー実機確認: **WALKのみ確認不能、その他PASS**
- 是正後ユーザー実機確認: **10/10 PASS**
- Stage G-1A総合ユーザー実機確認: **PASS**
- 既知の受入阻害問題: **なし**

## 基準点と非変更境界

- branch: `main`
- Stage G-0 commit: `823e96206165f558bceb7af10cf7f91953045591`
- annotated tag: `stage-g0-rendering-poc-v0.1.0`
- 旧Stage G-1 2.5D失敗アーカイブはリポジトリ外で保全し、再利用しない。
- commit / tag / pushは実施しない。

## 成果物

- map: `/Game/Maps/StageG1_3DCharacterPoC`
- Player: Manny standard skeletal mesh + `ABP_Unarmed`
- NPC: Quinn standard skeletal mesh + `ABP_Unarmed`
- evidence: `out/stage-g1a/test-output`のJSON 10種
- package: `out/stage-g1a/package`
- Computer Use画像: `out/stage-g1a/manual-evidence/StageG1A_Initial_Player_NPC.png`

## 初回ユーザー実機確認の履歴

```text
初回ユーザー実機確認：
WALK／RUN切替手段が存在しないため、
WALKのみ確認不能。
その他の項目はPASS。
```

既存のPASS結果は維持する。この是正はWALK/RUNの確認手段を追加するものであり、初回結果を上書きしない。

## 自動試験・実行時証拠

- G1A-1～G1A-15: 既存試験を維持してPASS
- G1A-16～G1A-22: Default Walk、Walk Animation、Run Toggle、Walk Toggle、Idle Return、UI Isolation、Movement Regressionを追加
- 総合: `SUMMARY | 22/22 tests passed`
- 標準asset: Manny / Quinn / `ABP_Unarmed`解決、Paper2D使用なし、牙鼠実装なし
- 実身長: 175.13uu、Capsule高192uu、扉開口高240uu、scale 0.97
- 接地: ground trace PASS、Capsule足元差2.15uu、dynamic/contact shadow有効、blob shadowなし
- WALK: 実測250uu/s、設定最大250uu/s、Animation=WALK、play-rate 1.00
- RUN: 実測500uu/s、設定最大500uu/s、Animation=RUN、play-rate 1.00
- RUN→WALK復帰: 即時250uu/s、最大250uu/s
- 移動中の両方向切替: 目的地、NavPath、移動継続、目的地更新数、選択対象を維持
- 入力: NavMesh完全経路 + CharacterMovement waypoint追従、WASD bindなし、UI貫通防止あり
- UI隔離: world移動命令なし、target変更なし、因果イベントなし

## 回帰

2026-07-14 11:26 JSTにWALK/RUN是正後コードで`NationSimulation.Stage`を一括実行した。

- Stage C: 11/11 PASS
- Stage D: 9/9 PASS
- Stage D corrective: 8/8 PASS
- Stage E: scenario/migration/validator既存受入PASS
- Stage F Unreal boundary: 5/5 PASS
- Stage G-0: 33/33 PASS
- Stage G-1A: 22/22 PASS
- shared fixture、Stage E定義、Stage F runtime、Stage G-0正本mapは変更していない。
- 新しい既定map要件と旧Stage Fの引数なし起動期待は両立しないため、Stage F package回帰は`/Game/Maps/StageD_Capital`明示起動でPASSを確認した。

## Package

- configuration: Win64 Development
- executable: `out/stage-g1a/package/Windows/NationSimulationStageC.exe`
- executable SHA-256: `7931c90fa1b8c11845d8a6782b02b03bef463de800ffbb7da258f37d1e2bcc64`
- configured default map: `/Game/Maps/StageG1_3DCharacterPoC`
- launch map argument: NONE
- no-argument default-map smoke: PASS
- standard 3D Player/NPC runtime marker: PASS
- Paper2D/Fang Rat混在: 0/0

## 初回Computer Use観察（是正前）

- 標準MannyのLit表示、dynamic/contact shadow、IDLEを確認した。
- 左クリックによるNavMesh移動が停止せず到着することを確認した。
- 移動中RUN、speed 500uu/s、play-rate 1.00、移動方向への回頭をF1表示で確認した。
- F1技術表示が通常UIと分離されることを確認した。
- 引数なしPackageの初期画面でManny PlayerとQuinn NPCが同時表示されることを確認した。
- F9画像を採取後にゲームを終了し、Computer Use停止後の関連プロセス数0を確認した。
- 画像: `out/stage-g1a/manual-evidence/StageG1A_Initial_Player_NPC.png`
- image SHA-256: `889c8ece939290f9397207c9900c9f9c46e079635ef0fad8a051e9cb946adf02`

## WALK/RUN是正後Computer Use観察

- 2026-07-14 11:36 JSTに、引数なしPackageをComputer Useから通常起動した。
- 起動直後は`移動：歩行 / 走行へ切替 / IDLE`だった。
- WALK移動中は最大速度250、水平速度250、Animation=WALK、再生倍率1.00をF1で確認した。
- 移動中にRUNへ切り替え、最大速度500、水平速度500、Animation=RUN、再生倍率1.00を確認した。
- RUN→WALKを移動中に連続操作し、同じ目的地`X=-626.50, Y=-965.50, Z=10.00`と目的地更新数4を維持したまま、最大速度/水平速度が250、Animation=WALKへ変化した。
- 到着後はIDLE、水平速度0、再生倍率1.00へ戻った。
- IDLE中のボタン押下では目的地更新数4、選択対象noneを維持し、world移動が発生しなかった。
- native比較画像3件を採取した。
  - `out/stage-g1a/manual-evidence/StageG1A_WALK_Moving_F1.png` SHA-256 `8a505e9b63a6020c7d115d681e3418921a330efdbf320a52f23024725fd4a876`
  - `out/stage-g1a/manual-evidence/StageG1A_RUN_Moving_F1.png` SHA-256 `ff139ec7743927e515b91b7642b89b95f84b2a81b17a45cabff0f74b652e9e42`
  - `out/stage-g1a/manual-evidence/StageG1A_WALK_Return_Moving_F1.png` SHA-256 `2908a199a2135fb23ad58cb924e9856c6d478ebb7e468b1754c40bbec8878b24`
- 確認後にPackageゲームを終了し、Computer Use接続を明示終了した。ゲーム、Unreal Editor/Cmd、Computer Use関連プロセスの残存0件を確認した。
- 判定: **PASS**（外観の最終受入はユーザー再確認待ち）

## 完了判定ルール

Automation 22/22、実行時証拠10/10、Stage A～G-0回帰、Win64 Development Package、引数なし通常起動、Computer Use、是正後ユーザー実機確認10/10がすべてPASSした。初回確認履歴を保持したまま、Stage G-1A標準3Dキャラクター技術PoCを正式受入・完了とする。
