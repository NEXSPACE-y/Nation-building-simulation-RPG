# Stage G-2B GUARD_M 検証報告

検証日：2026-07-15 JST

判定：技術検証PASS、ユーザー実機確認待ち

## 1. 基準点

- branch：`main`
- 開始HEAD：`a2dbf4ee87ceb81817fbb97426645ce16f9c6bff`
- origin/main：開始HEADと一致
- tag points-at HEAD：`stage-g2a-camera-redesign-v0.1.0`
- 開始時git status：clean
- 開始時関連プロセス：0

## 2. 素材検証

- 使用ZIP：`C:\Users\rinpa\Desktop\Meshy_AI_Lioncrest_Knight_biped.zip`
- SHA-256：`AC7DEB2B0238970E413B8784FF46941860F08C29C51F03F88A3FFC88BDDFD6A8`
- SHA一致：PASS
- 内部ファイル：6/6
- ZIP CRC：PASS
- FBX 7.4：2/2
- 2048×2048 Texture：4/4
- 作業コピーSHA一致：6/6
- Clean Pack使用：なし
- 外部通信：0
- クレジット消費：0

## 3. GUARD_M Import結果

- Mesh：`/Game/StageG2B/Characters/GUARD_M/MeshyV01/Mesh/SK_GUARD_M_Meshy_v0_1`
- Skeleton：`/Game/StageG2B/Characters/GUARD_M/MeshyV01/Skeleton/SKEL_GUARD_M_Meshy_v0_1`
- Physics Asset：`PHYS_GUARD_M_Meshy_v0_1`
- bone数：24
- root bone：`Hips`
- world height：175.0uu
- import scale：1.0
- component scale：1.0
- FBX原本vertices：89,279
- Unreal render vertices：297,881
- triangles：178,245
- Material slot：1
- LOD：LOD0のみ

## 4. Skeleton／Animation

- PLAYER_M Skeleton共有：false
- GUARD専用Skeleton：PASS
- Idle：GUARD素材内 `Idle_11`
- 再生方式：AnimationSingleNode loop
- Retarget：なし
- Reference Pose：なし
- PLAYER_M Animation変更：0
- PLAYER_M ABP変更：0
- 観測した破綻：なし

importされたAnimationは12本。今回runtimeへ接続したのはIdle_11のみで、残り11本は未使用在庫として保持した。

## 5. Material／Texture

- Base Color接続：PASS
- Normal接続：PASS
- Metallic接続：PASS
- Roughness接続：PASS
- 2K寸法：4/4 PASS
- sRGB／Compression：PASS
- Default Lit：PASS
- Dynamic Shadow：PASS
- Contact Shadow：PASS
- 黒潰れ：目視なし
- Normal反転：目視なし
- PLAYER_M Material変更：0

## 6. Map／配置

- Map：`/Game/Maps/StageG2B_GuardM_PoC`
- GUARD_M：1体
- PLAYER_M adapter：1
- G-2A Camera adapter：1
- PlayerStartからの距離：425uu
- 接地：PASS
- 上下反転：初回画像監査で検出し、G-2B map配置Yawの名前付き指定へ是正済み
- 最終正面／側面／背面：PASS

## 7. Camera／PLAYER回帰

- StandardCharacterCamera：PASS
- F6 TacticalOverlookCamera：PASS
- F6 Standard復帰：PASS
- 右ドラッグYaw／Pitch：既存G-2A回帰PASS
- ホイールZoom：既存G-2A回帰PASS
- Camera Collision：PASS
- 左クリック移動：PASS
- 左クリック保持移動：PASS
- WALK 250uu/s：PASS
- RUN 500uu/s：PASS
- DASH 750uu/s：PASS
- 目的地／NavPath維持：PASS
- PLAYER_M fallback：false

## 8. Automation

- Stage G-2B：18/18 PASS
- Stage G-2A：18/18 PASS
- Stage G-1B：30/30 PASS
- Stage G-1A：22/22 PASS
- Stage G-0：33/33 PASS
- Stage F：5/5 PASS
- Stage D：9/9 PASS
- Stage D Fix：8/8 PASS
- Stage C：11/11 PASS

全Stage一括実行の最終exit code：0。

## 9. Package

- Win64 Development Package：PASS
- 実行ファイル：`C:\Users\rinpa\Desktop\TITLE\out\stage-g2b-guardm\package\Windows\NationSimulationStageC.exe`
- SHA-256：`7931C90FA1B8C11845D8A6782B02B03BEF463DE800FFBB7DA258F37D1E2BCC64`
- 引数なしG-2B map起動：PASS
- GUARD_M 1体表示：PASS
- GUARD専用Skeleton：PASS
- Idle_11：PASS
- PLAYER_M fallback false：PASS
- G-2A明示map回帰：PASS
- G-1B明示map回帰：PASS

## 10. 評価画像

保存先：

`stage-c/SourceArt/StageG2B/GUARD_M/Meshy/v0.1/Screenshots/`

- `01_guard_standard_front.png`
- `02_guard_standard_close.png`
- `03_guard_standard_side.png`
- `04_guard_standard_back.png`
- `05_guard_player_scale_compare.png`
- `06_guard_tactical_initial.png`
- `07_guard_tactical_zoom_max.png`
- `08_guard_material_close.png`
- `09_guard_collision_near_wall.png`
- `10_return_to_standard_with_guard.png`

10/10、1280×720。正立、質感、接地、PLAYER比較、Standard／Tactical表示を目視確認した。

## 11. 保護境界

- PLAYER_M Skeleton変更：0
- PLAYER_M Mesh変更：0
- PLAYER_M Material／Texture変更：0
- PLAYER_M Animation／ABP変更：0
- Stage G-2A map変更：0
- Stage G-1B map変更：0
- G-2A Camera C++変更：0
- 因果コア変更：0
- Stage E状態定義変更：0
- Stage F runtime／save schema変更：0
- AI接続：0
- 外部通信：0

## 12. 既知の問題／未解決事項

- LOD0のみ。複数GUARD配置前にLOD1～LOD3が必須。
- 約178k trianglesで高負荷のため、G-2Bでは1体限定。
- FBX原本vertex 89,279に対し、Unreal render vertexは分割後297,881。
- GUARDはIdleのみ。移動、会話、AI、状態遷移は未接続。
- CapsuleはPoC用の最小設定で、量産NPC collision設計ではない。
- ユーザー実機での外観、操作、質感、体格判定は未実施。

## 13. 最終状態

- 技術成功条件：PASS
- ユーザー実機確認：待ち
- 豆虎Gate正式承認：待ち
- commit：未実施
- tag：未実施
- push：未実施
