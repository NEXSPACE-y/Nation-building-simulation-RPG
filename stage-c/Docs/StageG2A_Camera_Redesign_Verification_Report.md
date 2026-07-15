# Stage G-2A カメラ再設計 検証報告

検証日：2026-07-15 JST
判定：技術検証PASS／ユーザー実機確認待ち

## 1. 基準点と旧成果物処置

- branch：`main`
- HEAD／origin/main：`985155fa86586295b804601bbee33d01bf0b115f`
- tag points-at HEAD：`stage-g1b-meshy-original-player-v0.1.0`
- 旧G2Aアーカイブ：`C:\Users\rinpa\Desktop\TITLE_StageG2A_Old_Rejected_Archive_20260715`
- 保全：5,370ファイル／1,993,620,739 bytes
- SHA-256：5,370／5,370一致、FAIL 0
- 旧G2A untracked／ignored：撤去後0
- 現行再設計指示書PDF：旧成果物ではないため未変更

## 2. 実装結果

- 初期モード：StandardCharacterCamera
- 標準視点：520uu、Pitch -15度、FOV 75、非俯瞰
- 戦略俯瞰：F6限定、1200uu、Pitch -55度、FOV 60
- F6再押下：Standardへ復帰
- 右ドラッグ：両モードでraw MouseX／MouseYを使用
- ホイール：両モードでSpringArm距離を変更
- UI入力分離：実装済み
- Player追従：既存Player SpringArm接続
- Manny fallback：false

## 3. Runtime証拠

| 項目 | 結果 |
|---|---|
| 初期Standard／非俯瞰 | PASS |
| F6 Tactical切替 | PASS |
| F6 Standard復帰 | PASS |
| 目的地維持 | PASS |
| NavPath維持 | PASS |
| WALK／RUN／DASH状態維持 | PASS |
| 選択対象維持 | PASS |
| Camera Collision | PASS |
| Collision短縮 | 537.1uu → 244.3uu |
| 障害撤去後復帰 | 244.3uu → 537.1uu |
| PLAYER_M | PASS、175uu、fallback=false |

証拠JSON：`stage-c/SourceArt/StageG2A/CameraRedesign/v0.1/Reports/`

## 4. Automation

- Stage G-2A再設計：18/18 PASS
- Stage C：11/11 PASS
- Stage D：9/9 PASS
- Stage D Fix：8/8 PASS
- Stage F：5/5 PASS
- Stage G-0：33/33 PASS
- Stage G-1A：22/22 PASS
- Stage G-1B：30/30 PASS
- shared fixture／acceptance contract／Stage E／Stage F／save schema：変更なし

G0の固定カメラ契約を維持するため、G2Aの右クリック／F6 mappingはWin64 platform設定へ分離した。G0テストの変更は行っていない。

## 5. 評価画像

1280×720、12／12生成PASS。

1. `01_standard_initial.png`
2. `02_standard_min_zoom.png`
3. `03_standard_max_zoom.png`
4. `04_standard_pitch_min.png`
5. `05_standard_pitch_max.png`
6. `06_tactical_initial.png`
7. `07_tactical_min_zoom.png`
8. `08_tactical_max_zoom.png`
9. `09_tactical_yaw_90.png`
10. `10_tactical_yaw_180.png`
11. `11_collision_near_wall.png`
12. `12_return_to_standard.png`

保存先：`stage-c/SourceArt/StageG2A/CameraRedesign/v0.1/Screenshots/`

## 6. Package

- Win64 Development Package：PASS
- 実行ファイル：`out/stage-g2a-redesign/package/Windows/NationSimulationStageC.exe`
- SHA-256：`7931C90FA1B8C11845D8A6782B02B03BEF463DE800FFBB7DA258F37D1E2BCC64`
- 引数なし起動map：`/Game/Maps/StageG2A_CameraRedesignPoC` PASS
- 初期StandardCharacterCamera：PASS
- F6 Tactical切替／Standard復帰：PASS
- Camera Collision：PASS
- PLAYER_M表示／Manny fallback=false：PASS
- 同一PackageのG1B明示map回帰：PASS

## 7. 未完了

ユーザー実機で以下の操作感確認が必要。

- 標準視点が非俯瞰でPLAYER_Mを見やすいこと
- 右ドラッグ左右／上下
- 両モードのZoom
- TacticalのYaw感度
- F6往復
- 移動中切替
- 実際の壁際Camera Collision

ユーザー承認前のcommit、tag、pushは実施していない。
