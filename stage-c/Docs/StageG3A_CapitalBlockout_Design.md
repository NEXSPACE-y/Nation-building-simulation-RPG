# Stage G-3A 王都背景Blockout 設計書

作成日：2026-07-15 JST
是正日：2026-07-16 JST
状態：初回ユーザー判定FAILを受け、クリック移動と外形を再設計済み。ユーザー再確認待ち

## 1. 基準点

- branch：`main`
- Stage G-2B commit：`e413ce6deae04ce7dfe295dc0004b4b33d4b9d10`
- annotated tag：`stage-g2b-guardm-poc-v0.1.0`
- G-2B Mapを複製し、G-1B PLAYER_M、G-2Aカメラ、G-2B GUARD_Mを参照のみで保持する。

## 2. 参照資料

- 正本画像：`C:\Users\rinpa\Desktop\TITLE\Docs\ChatGPT Image 2026年7月11日 21_50_21.png`
- 正本指示書PDF：`C:\Users\rinpa\Desktop\TITLE\Docs\Stage G-3A Capital Blockout 設計・実装指示書.pdf`
- 参照範囲：王都全景、王城、城門、街路、中央広場、市場、住宅、貴族、下町・工房、郊外・農地・森、限定的魔法文明の配置関係。
- 除外範囲：2.5D人物描画、8方向人物、牙鼠描画、完成背景美術、細密意匠。

## 3. 実装方式

- Map：`/Game/Maps/StageG3A_CapitalBlockout_PoC`
- Source Map：`/Game/Maps/StageG2B_GuardM_PoC`
- 背景はEngine標準のCube、Cylinder、Cone、Sphereだけで構成する。
- 色付きの巨大箱と区画名TextRenderに依存しない。回転した薄いCubeの二面で勾配屋根を構成し、木組み、窓、戸、店棚、煙突、扶壁、狭間、門塔、尖塔で中世王都の外形を読ませる。
- 色分けは`/Game/StageG3A/CapitalBlockout/Materials`の軽量Master Material 1件と用途別Material Instanceで行い、暖色の土・漆喰、テラコッタ屋根、スレート屋根、暗色の木材、暖灰色の石を使う。
- G-3A専用`AStageG3ACapitalGameMode`は受入済み`AStageG1APlayerCharacter`をDefault Pawnとして使用し、動的NavMeshと照明だけを起動する。
- 旧G-1A技術広場とQuinnはG-3A Mapでは生成しない。受入済みG-1A/G-2A/G-2B Map、PLAYER_M／GUARD_M資産は変更しない。
- `AStageG3ACapitalEvidenceActor`はruntime証拠と評価画像だけを担当し、AI、因果、保存、移動命令を所有しない。
- `AStageG3AStandardCameraAutoFollowActor`はG-3A Mapに1体だけ配置する。G-3A専用Yaw PivotをPlayer rootと既存SpringArmの間へ挿入し、SpringArmのCamera socket計算前に実Camera world Yawを構成する。G-2A Adapter、設定、Mapはbyte同一を維持し、G-2A/G-2B MapにはG-3A Actorを配置しない。

## 4. 寸法基準

- 王都Blockout基準範囲：約6000uu × 6000uu
- 内郭基準範囲：約5200uu × 4800uu
- 単位：1uu = 1cm相当
- PLAYER_M／GUARD_M基準身長：175uu
- 外周壁：高さ700uu、厚さ200uu
- 南門：開口幅700uu、通過高さ約550uu
- メイン街路：幅700uu
- 中央広場：1200uu × 1200uu
- 王城本体：幅1600uu、奥行900uu、高さ1100uu
- 王城中央塔：高さ1700uu

## 5. 区画構成

南から北へ、郊外・農地・森、門前街道、南門、下町・工房区、メイン街路、住宅区画、市場区画、中央広場、貴族区画、王城前、王城・王宮を配置する。

- 南門：左右外周壁、左右門塔、門上部、門番位置。
- メイン街路：南門から中央広場、王城前まで連続する700uu幅の主導線。
- 中央広場：中心Marker、魔法灯Marker 2基、東西横道。
- 市場：区画床、露店6基、商業建物3棟、通路。
- 住宅：住宅6棟、横道、行き止まり壁1か所。
- 貴族：大きめの邸宅Block 4棟、王城前接続路。
- 下町・工房：工房・倉庫3棟、煙突Marker 2基。
- 王城：本体、中央塔、左右塔、門Marker、王城前広場。
- 郊外：農地区画3面、区画線、Tree Marker 5基、街道延長。
- 限定的魔法文明：中央Markerと魔法灯Markerだけ。機能、Particle、UI、因果連携は持たない。

## 6. PLAYER_M／GUARD_M

- PLAYER開始地点：`(0, -3000, 96)`、北向き。
- GUARD_M：`(-260, -2440, 88)`、南門内側に1体だけ配置。
- GUARD_M Skeleton：G-2B専用Skeletonを維持。
- PLAYER_M Skeleton共有：なし。
- PLAYER_M／GUARD_MのMesh、Skeleton、Animation、Material、Textureは変更しない。

## 7. NavMesh導線

Dynamic Recast NavMeshと約7000uu四方のNavMeshBoundsを使用する。runtimeで次の10導線を投影・経路探索する。

1. PLAYER開始地点 → 南門
2. 南門 → メイン街路
3. メイン街路 → 中央広場
4. 中央広場 → 市場
5. 中央広場 → 住宅
6. 中央広場 → 王城前
7. 王城前 → 貴族区画
8. 南門 → 下町・工房区
9. 建物角の回り込み
10. 外周壁際移動

全経路で開始・終了地点のNavMesh投影、完全Path、2点以上のPath Pointを要求する。

### 7.1 初回FAILと是正契約

- 初回PackageではNavMesh座標間経路10/10だけを検証し、`ClickMoveSurface`が既定Ignoreであることを考慮せず、G-3A地面への`ECC_GameTraceChannel1=Block`を設定していなかった。
- 是正後は`WalkableSurface`タグを持つ物理地面1面だけが`ECC_GameTraceChannel1`をBlockする。建物・城壁・装飾はIgnoreとし、CameraとCharacterの衝突は維持する。
- runtime受入では座標間経路に加え、実際のクリック用Trace 6地点、`AStageG1APlayerCharacter::IssueMoveToLocation`の発行、Player実移動100uu以上をPASS必須とする。

## 8. Camera検証

- StandardCharacterCamera：南門、メイン街路、中央広場、王城前、壁際を確認する。
- TacticalOverlookCamera：区画構造、市場、住宅、貴族、工房、王城、農地を確認する。
- F6往復、Player追従、目的地・NavPath非干渉はG-2A仕様をそのまま使用する。
- 22枚の評価画像のうち全景、王城俯瞰、郊外は、G-3A証拠撮影専用ActorがTactical modeのまま広域フレームへ一時的に拡張する。G-2A設定値・ユーザー操作範囲は変更しない。

### 8.1 StandardCharacterCamera自動Yaw追従

- 適用対象：StandardCharacterCameraだけ。TacticalOverlookCameraでは自動Yaw追従を行わない。
- 目標Yaw：水平Velocityが10uu/s以上ならVelocity方向。低速時にMove Command、NavPathまたはDestinationが有効ならDestination方向。両方無効なら現在Yawを維持する。
- 補間：最短角で`YawInterpSpeed=5.0`。0／360度境界で逆回りや跳ねを発生させない。
- 手動優先：右ドラッグ中は自動追従を停止する。解放後`1.25秒`は手動Yawを維持し、その後に自動追従へ戻る。
- UI：UI上で実際にマウスボタンを入力している間だけ自動追従Targetを更新しない。UI上にカーソルがあるだけのhover状態では追従を止めない。
- 到着後：Velocity、Destination、NavPathが無効になった時点のYawを維持する。
- F6：Tactical中はPlayer位置追従だけを維持し、Standardへ戻った後に上記条件で自動追従を再開する。
- 既存操作との同期：右ドラッグ中はG-2Aと同じraw MouseX、同じStandardYawSensitivityを使用してG-3A管理Yawを同期する。Pitch、Zoom、F6、CollisionはG-2Aが引き続き所有する。
- 設定の正本：`UStageG3ACameraAutoFollowSettings`と`DefaultGame.ini`。別倍率、DeltaSecondsの二重乗算、OS cursor座標依存の計算は持たない。

## 9. Collision

- 外周壁、南門、王城、住宅、商業建物、邸宅、工房、露店、中心Markerに`BlockAll`を設定する。
- 道路・広場床は視覚表示のみで、下層のG-3A地面を唯一の歩行・クリック面とする。
- runtime証拠ではG-3A Collision tagを持つ全ActorがCamera channelをBlockすることを確認する。

## 10. 大容量Asset管理

- 新規背景Assetは50MB未満を必須とする。
- Engine Primitiveを参照し、外部Mesh、Texture、Foliage、Marketplace、Meshyを使用しない。
- SourceArtは評価画像と証拠JSONだけを格納し、Package、Intermediate、Saved、outは正本成果物へ含めない。
- Git LFSは導入しない。

## 11. G-3Aで作らないもの

完成背景、美術用建物、内装、住民NPC、GUARD複数配置、AI、会話、国家UI、経済UI、魔法機能、戦闘、因果コア、Save schema変更、NPC状態遷移、国家作用は実装しない。

## 12. 未解決事項

- Blockoutは寸法・導線・外形評価用である。勾配屋根、簡易窓、扉、木組み、煙突、狭間は持つが、石積みTexture、屋根瓦、完成植生、建物内部は持たない。
- Tactical視点での全域把握は区画ごとの確認を前提とする。全景評価画像だけは証拠用広域フレームを使用する。
- 住宅・貴族区画の細街路幅はユーザー実機で操作感を確認する。
- 背景美術化の前に建物Kit、Occlusion、HLOD、Nanite、LOD、描画距離の別裁定が必要。
