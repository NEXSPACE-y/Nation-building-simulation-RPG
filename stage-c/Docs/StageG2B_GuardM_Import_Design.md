# Stage G-2B GUARD_M 取込設計書

作成日：2026-07-15 JST

状態：技術検証完了、ユーザー実機確認待ち

## 1. 目的

Meshy製GUARD_Mを正式3D NPC候補としてUnreal Engine 5.8へ取り込み、Stage G-2AのPLAYER_M・StandardCharacterCamera・TacticalOverlookCameraと同じ検証環境で、外観、身長、質感、表示負荷を確認する。

本StageではGUARD_Mを国家AI、因果コア、保存Schema、NPC判断、会話、国家作用へ接続しない。配置は1体に限定する。

## 2. 正式基準点

- branch：`main`
- commit：`a2dbf4ee87ceb81817fbb97426645ce16f9c6bff`
- annotated tag：`stage-g2a-camera-redesign-v0.1.0`
- 開始時HEAD == origin/main：PASS
- 開始時git status clean：PASS
- Unreal／ゲーム／Blender関連プロセス0：PASS

## 3. 使用素材

- 入力ZIP：`C:\Users\rinpa\Desktop\Meshy_AI_Lioncrest_Knight_biped.zip`
- ファイルサイズ：52,772,677 bytes
- SHA-256：`AC7DEB2B0238970E413B8784FF46941860F08C29C51F03F88A3FFC88BDDFD6A8`
- SHA一致：PASS
- ZIP CRC：PASS
- 内部ファイル：6件
- Clean Pack使用：なし
- 外部通信：0
- Meshyクレジット消費：0

原本保全先：

`stage-c/SourceArt/StageG2B/GUARD_M/Meshy/v0.1/Original/Meshy_AI_Lioncrest_Knight_biped.zip`

作業コピー：

- `Working/FBX/GUARD_M_Meshy_v0.1_Base_Rigged.fbx`
- `Working/FBX/GUARD_M_Meshy_v0.1_AllAnimations.fbx`
- `Working/Textures/T_GUARD_M_BaseColor_2K.png`
- `Working/Textures/T_GUARD_M_Normal_2K.png`
- `Working/Textures/T_GUARD_M_Metallic_2K.png`
- `Working/Textures/T_GUARD_M_Roughness_2K.png`

原本内部ファイルと作業コピーのSHAは6/6一致した。

## 4. Import方式

- 形式：Kaydara Binary FBX 7.4
- Skeletal Mesh：ON
- Import Materials：OFF
- Import Textures：OFF
- Normal：Import Normals and Tangents
- Convert Scene：ON
- Force Front XAxis：OFF
- Import Uniform Scale：1.0
- Component Scale：1.0
- Physics Asset：自動生成
- Per-poly collision：不使用

取込先：

`/Game/StageG2B/Characters/GUARD_M/MeshyV01/`

主要Asset：

- Mesh：`SK_GUARD_M_Meshy_v0_1`
- Skeleton：`SKEL_GUARD_M_Meshy_v0_1`
- Physics：`PHYS_GUARD_M_Meshy_v0_1`
- Material：`M_GUARD_M_Meshy_v0_1`
- Material Instance：`MI_GUARD_M_Meshy_v0_1`
- Actor：`BP_StageG2B_GUARD_M`

## 5. Mesh仕様

- ワールド身長：175.0uu
- FBX原本頂点：89,279
- Unreal LOD0 render vertex：297,881
- triangles：178,245
- Material slot：1
- bone数：24
- root bone：`Hips`

Unrealのrender vertexは、法線、UV、Material境界などの頂点分割を反映するためFBX原本頂点数より多い。三角面数は原本契約と一致する。

## 6. Skeleton方針

GUARD_M専用Skeletonを採用した。PLAYER_M Skeletonへの強制割当、共有、書換えは行っていない。

理由：

- 骨名・骨順が一致しても基準姿勢と骨長差があり得る
- PLAYER_M正式基準点を保護する
- 鎧体型と装備差による変形リスクを分離する
- 将来のRetarget検証をGUARD側へ閉じ込める

## 7. Animation方針

GUARD素材内の `Idle_11` をGUARD専用Skeletonへ直接importし、`AnimationSingleNode` でループ再生する。

- 使用Idle：`GUARD_M_Meshy_v0_1_AllAnimationsIdle_11_frame_rate_60_fbx`
- PLAYER_M Animation使用：なし
- Retarget：なし
- Reference Pose暫定表示：なし
- PLAYER_M ABP変更：なし
- Root MotionによるActor移動：なし

未使用Animation 11本は在庫として保持し、runtimeへ接続しない。

## 8. Material／Texture

- Base Color：2048×2048、sRGB ON、Default
- Normal：2048×2048、sRGB OFF、Normalmap
- Metallic：2048×2048、sRGB OFF、Masks
- Roughness：2048×2048、sRGB OFF、Masks
- Material Domain：Surface
- Blend Mode：Opaque
- Shading Model：Default Lit
- Two Sided：OFF
- Dynamic Shadow：ON
- Contact Shadow：ON

PLAYER_M Material／Textureは変更していない。

## 9. LOD方針

- G-2B：LOD0原本のみ
- LOD数：1
- 複数GUARD配置：禁止
- LOD0：178,245 triangles

1体の外観PoCではLOD0のみを許容する。複数配置へ進む前にLOD1～LOD3の作成と外観・負荷検証を必須とする。目標比率はLOD1約50%、LOD2約25%、LOD3約12.5%とするが、見た目の破綻が大きい自動LODは採用しない。

## 10. Actor／Collision

`AStageG2BGuardMActor` は表示専用Actorである。

- GUARD_M配置：1体
- Capsule：半径42uu、半高88uu
- Capsule collision：Query Only
- Pawn／Camera：Block
- SkeletalMesh collision：No Collision
- AI：なし
- Tick走査：なし
- 因果コア：未接続
- Save schema：未接続

## 11. Map／Camera

専用map：

`/Game/Maps/StageG2B_GuardM_PoC`

Stage G-2A mapを複製し、次を維持した。

- PLAYER_M adapter：1
- G-2A Camera adapter：1
- GUARD_M actor：1
- StandardCharacterCamera
- F6 TacticalOverlookCamera
- 右ドラッグYaw／Pitch
- ホイールZoom
- Camera Collision
- 左クリック／左保持NavMesh移動
- WALK 250／RUN 500／DASH 750
- 目的地／NavPath維持

GUARD_MはPlayerStart前方425uuへ配置した。

## 12. Package設定

- 引数なしdefault map：`/Game/Maps/StageG2B_GuardM_PoC`
- G-2A map：明示起動回帰対象
- G-1B map：明示起動回帰対象
- `/Game/StageG2B`：Always Cook

Windows設定にはG-2Aの履歴宣言を残し、その後段のG-2B宣言を最終overrideとしている。実際のPackage引数なし起動でG-2B mapを確認する。

## 13. 未実装／次工程境界

- GUARD AI
- 国家AI／因果コア接続
- Save schema接続
- NPC判断、会話、状態遷移
- GUARD複数配置
- GUARD移動Animation接続
- Retarget設定
- LOD1～LOD3
- 背景制作
- commit／tag／push

ユーザー実機確認と豆虎Gate承認前に次工程へ進まない。
