# Stage V-2 王都主要街路Geometry PoC 実装設計書

## 1. 文書状態

- Stage名：`Stage V-2 王都主要街路Geometry PoC`
- 文書作成日：2026-07-18 JST
- 現在工程：既存PoC／Assetの読取監査と実装方式選定前設計
- Geometry作成：未実施
- Map変更：未実施
- Asset検索／取得／import：未実施
- 実装方式：豆虎Gate裁定待ち

本書は実装設計と読取監査の記録であり、Geometry作成、Map保存、Asset取得、Unreal Asset変更を許可しない。今回の作業範囲は本書の実装順1～4までとする。

## 2. 目的

王都南門の王都内側接続部から主要街路を経て市場入口手前までを、一続きの現実寄り中世街路として構築するため、必要geometry、既存PoC／Assetの再利用可能性、実装方式の選択肢、尺度、Material、Collision、NavMesh、Camera、外観、性能、証拠条件を固定する。

Stage V-1／F5の正式合格は街路表面Materialの合格であって、街路完成ではない。本StageはF5の合格範囲外だった街路geometryを対象にする。

## 3. 正式基準点

- Repository：`C:\Users\rinpa\Desktop\TITLE`
- branch：`main`
- HEAD／main／origin/main：`f08a16c489bdfdaed2e118f99c989ae88728c026`
- annotated tag：`stage-v1-f5-paving-stones149-v0.1.0`
- 開始時git status：clean
- 上位ビジュアル要件基準点 Stage V-0：commit `3ff9394d2fe8e57c2fb425ae84c52fdd90b311c6`、tag `stage-v0-visual-direction-v0.1.0`

## 4. 上位拘束

- Stage V-0の現実寄り中世、人間尺度、Standard近距離品質、Tactical区画可読性を維持する。
- Gate A／B／CはAsset検査工程であり、物理的な門と混同しない。
- F1～F11は候補IDであり、採用、取得許可、実装順を意味しない。
- F5 `Paving Stones 149`の既存合格済みMaster Material／Material Instanceを参照利用する。再取得、再import、Texture変更、Master Material再設計、再検証を行わない。
- F5 Height Textureは未接続のまま維持する。Height表現をgeometryで偽装しない。
- 既存Stage G-3A、G-3B-R、Stage V-1 Validation Mapは読取・参照だけとし、元Asset、元Material、Mapを変更しない。
- Stage G-2Aで受入済みのCamera、FOV、Pitch、Zoom、操作仕様を変更しない。
- PLAYER_M、GUARD_M、因果コア、GameInstanceSubsystem、Save schema、Config、Plugins、Build.cs、`.uproject`を変更しない。
- Asset検索、取得、ダウンロード、importは別裁定まで行わない。

## 5. 対象範囲

対象導線は次の連続範囲に限定する。

`王都南門の王都内側接続部 → 主要街路 → 市場入口手前`

- 王都南門：王都入口側にある街の物理門。
- 主要街路：王都南門から市場入口手前までの主導線。
- 市場入口：市場内部へ入る境界。今回の終端はその手前とする。

南門開口、街路中心線、幅、曲がり、分岐、路肩、縁石、排水、高低差、建物際境界を同一PoCの中で評価できる範囲とする。

## 6. 対象外

- 市場内部
- 王城前広場
- 王城正門
- 王都全域
- 住宅建築
- 屋根
- 壁Material
- F10
- F11
- F4
- 植生
- NPC追加
- Camera仕様変更
- 因果コア
- Save schema
- 正式王都Mapへの採用
- F5 Height／Displacement追加実装
- 後続Assetの検索・取得

## 7. 固定済み入力

| 入力 | 固定値／状態 | 根拠 |
|---|---|---|
| 単位 | `1uu = 1cm` | Stage G-3A設計 |
| 人物尺度 | PLAYER_M 175uu、Component Scale 1.0 | Stage G-3A／Stage V-1受入 |
| G-3A王都Blockout | 6000uu × 6000uu基準 | G-3A検証報告 |
| 王都南門開口 | 幅700uu、通過高さ約550uu | G-3A設計 |
| G-3A主要街路 | 幅700uu | G-3A設計 |
| F5 Material | `/Game/StageV1/External/F5_PavingStones149/Materials/F5_M_PavingStones149_Master` | Stage V-1正式基準点 |
| F5 Material Instance | `/Game/StageV1/External/F5_PavingStones149/Materials/F5_MI_PavingStones149_Default` | Stage V-1正式基準点 |
| F5 Tiling | 1.0 | 豆虎Gate合格範囲 |
| F5 UV密度基準 | 400cm四方でTexture 1 tile、UV0 0～1 | Stage V-1検証Map証拠 |
| F5接続 | Base Color、NormalDX、Roughness、Ambient Occlusion | Stage V-1検証記録 |
| Height | Texture保全、Material未接続 | Stage V-1正式合格条件 |
| Camera | Standard／Tacticalの受入済み設定を変更しない | Stage G-2A回帰 |

700uu幅のG-3A主要街路はPoC開始尺度であり、完成街路幅の最終美術裁定ではない。変更する場合は理由、175uu人物比較、南門接続、Camera、NavMeshへの影響を先に豆虎Gateへ提示する。

## 8. 必要Geometry

最低限、次を別機能として識別できるgeometry構成を設計する。

1. 王都南門内側の接続部
2. 主要街路直線部
3. 緩い曲がり
4. 分岐または交差検証部
5. 縁石
6. 排水溝
7. 路肩または建物際処理
8. 高低差接続
9. 段差処理
10. F5表面と周辺地面の境界

近代的な全面歩道を無条件に採用しない。175uu人物、荷車を想定した通行幅、雨水の流れ、建物際の不整形さを読める、現実寄り中世の路肩・縁石・排水構造として設計する。F5は表面Materialであり、これらのgeometryを含まない。

## 9. 既存PoC／Asset再利用監査

監査方法は、committed Content path、既存設計・検証報告、SourceArt証拠JSON、F5外部保管Reportの読取に限定した。Unreal Editorは起動せず、`.uasset`／`.umap`を開いて再保存していない。

| Asset名／Actor | path | 種別 | 寸法・Transform証拠 | Collision | UV | Material Slot | 再利用可能性 | 品質区分 | 再利用risk |
|---|---|---|---|---|---|---|---|---|---|
| Stage G-3A王都Blockout Map | `/Game/Maps/StageG3A_CapitalBlockout_PoC` | Map／中心線・尺度基準 | 6000×6000uu、主要街路幅700uu、南門開口700×約550uu | NavMesh 10/10、クリックTrace 6/6。歩行面は専用下地1面 | 道路ActorのUVは`UNKNOWN` | 道路ActorのSlot名は`UNKNOWN` | 中心線、南門接続位置、幅、Nav導線を参照可能。元Map変更不可 | 正式受入済みBlockout、完成美術ではない | Map直編集、Primitive外観の継承は禁止 |
| G3A主要街路Actor群 | Map内`G3A_Road_GateApproach`、`G3A_Road_MainStreet`、`G3A_Road_CrossStreet`、`G3A_MainStreet_Joint_00～21` | Engine PrimitiveのStaticMeshActor | 主要街路幅700uu。記録ScaleはMainStreet `(7.2,41.0,0.07)`、GateApproach `(7.2,14.4,0.07)`、CrossStreet `(46.0,4.4,0.07)` | 道路面は視覚表示。下層`G3A_Ground_Walkable_7000`が歩行・click面 | `UNKNOWN` | `UNKNOWN` | 位置・連続長・中心線の参照は可。正式street geometryとしての直接再利用は不可 | PoC限定 | 専用Meshではなく、曲がり・排水・建物際・正式UVがない |
| `G3A_Ground_Walkable_7000` | Stage G-3A Map内Actor | 歩行／click substrate | Location `(0,-300,-10)`、Scale `(70,70,0.2)` | `WalkableSurface`が`ECC_GameTraceChannel1`をBlock | 非表示下地のため正式表面UV対象外 | `UNKNOWN` | accepted click／Nav contractの方式を再利用候補にできる | 技術基準 | Actor自体の流用ではなく、範囲と高さをStage V-2へ適合させる必要 |
| G3A広場縁石Actor | Map内`G3A_Plaza_Curb_+590`、`G3A_Plaza_Curb_-590` | Engine Primitiveの縁石PoC | Scale `(0.22,12.0,0.18)` | Collision tagなし。実Collisionは`UNKNOWN` | `UNKNOWN` | `UNKNOWN` | 寸法比較用参照のみ。主要街路用moduleとしては再利用不可 | PoC限定 | 広場直線用2本だけで、曲がり、端部、排水との接続がない |
| G3A道路Material | `/Game/StageG3A/CapitalBlockout/Materials/MI_G3A_Road`、`MI_G3A_RoadDark` | Blockout Material Instance | N/A | N/A | Texture UV要件なし | N/A | F5との比較色基準に限り参照可能 | PoC限定 | 正式舗装Materialではない。F5を上書きしない |
| G-3B-R王城入口階段 | `/Game/StageG3B/ModularCastle/Meshes/SM_G3BR_EntranceStair` | Static Mesh | Actor Scale 1.0。Source FBX 32,892 bytes。正確な外形寸法は`UNKNOWN` | Actor tagからは確認不能、`UNKNOWN` | `UNKNOWN` | Source manifestのmaterial roleは`plaza`、Slot名は`UNKNOWN` | 高低差・蹴上げ比較の参照候補。街路への直接採用は未承認 | 正式受入済みG-3B-R内のPoC module | 王城専用意匠、寸法、Collision、UV、Slot未確定。元Asset変更不可 |
| Stage V-1 Validation Map | `/Game/StageV1/Maps/StageV1_AssetValidation_PoC` | Material検証Map | 水平／垂直400cm基準面、100面区画 | Nav用substrateはF5 geometryではない | UV0 0～1、400cmで1 tile | F5 MI適用 | UV密度、Tiling、尺度、照明の比較基準として再利用可能 | F5正式検証基準 | 正式街路Mapではなく、geometry、縁石、排水溝、段差を含まない |
| F5 Paving Stones 149 MI | `/Game/StageV1/External/F5_PavingStones149/Materials/F5_MI_PavingStones149_Default` | Material Instance | Tiling 1.0 | geometry／Collisionなし | 400cmで1 tileの受入基準 | 適用先Slot名はStage V-2側で`RoadSurface`を提案 | 街路表面へ参照利用可 | 正式合格済み | Height未接続を維持。Materialだけで段差や排水を表現しない |
| 専用道路／排水AssetのContent path監査 | `stage-c/Content`全体 | file inventory | 道路専用Static Mesh 0、排水溝／gutter／ditch専用Asset 0をpath名で確認。G3A道路MaterialとG3BR階段だけが該当 | N/A | N/A | N/A | 再利用可能な正式街路一式なし | 読取監査結果 | binary内部に別名部品がある可能性は`UNKNOWN`。証拠なしに存在扱いしない |

読取証拠：

- `stage-c/Docs/StageG3A_CapitalBlockout_Design.md`
- `stage-c/Docs/StageG3A_CapitalBlockout_Verification_Report.md`
- `stage-c/SourceArt/StageG3A/CapitalBlockout/v0.1/Reports/map_generation_evidence.json`
- `stage-c/SourceArt/StageG3A/CapitalBlockout/v0.1/Reports/capital_runtime_evidence.json`
- `stage-c/SourceArt/StageG3A/CapitalBlockout/v0.1/Reports/navmesh_route_evidence.json`
- `stage-c/Docs/StageG3B_ModularCastle_Verification_Report.md`
- `stage-c/SourceArt/StageG3B/ModularCastle/v0.1/Reports/module_source_manifest.json`
- `C:\Users\rinpa\Desktop\TITLE_ExternalAssets\StageV1\F5_PavingStones149\Reports\F5_Accepted_Reference_Inventory.json`
- `C:\Users\rinpa\Desktop\TITLE_ExternalAssets\StageV1\F5_PavingStones149\Reports\F5_FitValidation_Map_Evidence.json`
- `C:\Users\rinpa\Desktop\TITLE_ExternalAssets\StageV1\F5_PavingStones149\Reports\StageV1_F5_PavingStones149_Validation_Record.md`

### 9.1 読取監査で残ったUNKNOWN

1. G-3A道路Actorのcomponent mesh path、正確な外形寸法、Collision、UV、Material Slot。
2. G-3A広場縁石Actorの実Collision、UV、Material Slotと、主要街路向けmoduleとして必要な端部形状。
3. G-3B-R `SM_G3BR_EntranceStair`の正確な外形寸法、Collision、UV、Material Slot。
4. path名に道路／縁石／排水を含まないbinary Asset内部に、別名の再利用部品が存在するか。
5. Stage V-2の正式実装方式、曲がり半径、分岐角、縁石・排水溝・路肩・段差の最終寸法。
6. `Curb`、`Drain`、`EdgeGround`へ適用するMaterial。

これらはUnreal Assetを変更せず安全に確定できなかったため、推測でPASSにしない。実装方式裁定後の読取専用inventory工程で解消する。

## 10. 実装方式選定Gate

| 選択肢 | 内容 | 利点 | 主なrisk | 現在判定 |
|---|---|---|---|---|
| A：新規モジュラーGeometry | G-3A中心線・尺度だけを参照し、直線、曲がり、分岐、縁石、排水、路肩、高低差をStage V-2専用Static Mesh moduleとして作る | UV、Material Slot、Collision、snap、Variationの責任範囲が明確 | module数、継ぎ目、曲率、端部設計が必要 | 推奨案。豆虎Gate未裁定 |
| B：Spline＋断面module | 中心線Splineへ道路断面、縁石、排水を追従させる | 緩い曲がりと長さ変更へ対応しやすい | UV伸び、Spline tangent、Collision、Cook、編集事故の検証が必要 | 選択肢。`UNKNOWN`あり |
| C：G-3A Primitiveの一時複製＋不足module | G-3A道路面をStage V-2専用Mapへ複製し、縁石・排水だけ新規作成する | 最短で導線PoCを作れる | Primitive外観を温存し、正式geometryへの二重作業になりやすい | 一時PoC案。正式採用非推奨 |

豆虎Gateは、曲がり品質、UVの安定性、Collision／NavMesh、編集の局所性、将来の市場入口接続、Primitive外観riskを比較して方式を裁定する。裁定前にGeometryを作らない。現在の実装方式は`UNKNOWN`である。

## 11. 尺度・寸法基準

- 1uu = 1cm、PLAYER_M 175uu、Component Scale 1.0。
- PoC開始時の街路有効幅はG-3A基準の700uuとする。南門開口幅700uuとの接続を優先し、見栄えのために候補別変更しない。
- 直線module長の初期案は400uuとする。F5の受入UV密度と一致させ、1 module長あたりTexture 1 tileを基準にできる。
- 700uu × 400uuの表面でF5を伸張しないため、UVは幅方向1.75 tile、長さ方向1.0 tile相当とし、Material InstanceのTilingは1.0を維持する。
- 曲がり半径、分岐角、縁石幅・高さ、排水溝幅・深さ、路肩幅、最大高低差、段差蹴上げは`UNKNOWN`。方式選定Gateで175uu人物、荷車想定、Camera、NavMeshと併せて裁定する。
- 100uu gridと400uu module基準を記録し、Actor Scaleによる場当たり調整を禁止する。

## 12. Material適用

- `RoadSurface` Slotへ既存`F5_MI_PavingStones149_Default`を適用する。
- F5 Material InstanceのTilingは1.0、Textureは2K、Base Color／NormalDX／Roughness／AO接続を維持する。
- Height Textureは未接続のまま保全し、World Position Offset、Tessellation、Nanite Displacement、Virtual Heightfield Meshを追加しない。
- 新規geometry側のUVで400cmあたり1 tileを維持し、F5 MaterialまたはTextureを加工しない。
- `Curb`、`Drain`、`EdgeGround`のSlotは実装方式選定後のStage V-2専用geometry側に提案する。適用Materialは現時点`UNKNOWN`であり、新規Assetを検索しない。
- F5は道路表面だけを担当し、縁石、排水溝、段差、建物際境界をMaterialで偽装しない。

## 13. Collision・NavMesh

- G-3A受入方式を基準に、歩行・click用substrateだけが`ECC_GameTraceChannel1`をBlockし、装飾geometryがclick traceを奪わない構成を候補とする。
- 道路表面、縁石、排水溝、段差のPawn／Camera Collisionを機能別に記録する。
- NavMeshは王都南門内側から市場入口手前まで連続し、曲がり、分岐、高低差、縁石際で完全Pathを返すことを必須とする。
- 左クリック移動、左クリック保持移動、NavPath、Camera Collisionを既存仕様のまま回帰確認する。
- 見た目だけの穴へPlayerが落ちる、排水溝Collisionで主導線が分断される、縁石がCameraを不必要に押し上げる構成はSTOPとする。
- Collision方式、simple collision数、NavMesh agent設定変更要否は実装方式裁定前は`UNKNOWN`とする。Config変更が必要なら実装せず停止する。

## 14. Standard／Tactical Camera

Stage G-2Aで受入済みのCamera設定を変更せず、将来の実装時に次を確認する。

- Standard：南門接続、F5近距離、縁石・排水・建物際、緩い曲がり、分岐、段差の視認性。
- Standard：右ドラッグ、移動中の自動Yaw追従、到着後Yaw維持、Camera Collision。
- Tactical：F6切替、Player追従、自動Yaw非追従、南門から市場入口手前までの導線可読性。
- Tactical：曲がり、分岐、路肩、排水境界が過度に潰れないこと。
- Camera、FOV、Pitch、Zoom範囲、操作仕様を候補の見栄えのために変更しない。

## 15. 実装順

| 順序 | 工程 | 今回の状態 |
|---:|---|---|
| 1 | 既存G-3Aの王都南門～主要街路中心線と尺度を読取確認 | 完了 |
| 2 | 既存の街路・縁石・排水geometry再利用可否を監査 | 完了 |
| 3 | 再利用可能な候補と根拠を記録 | 完了。中心線、尺度、accepted collision方式、F5 MIを候補化 |
| 4 | 再利用不可の場合の新規モジュラーGeometry方式を提示 | 完了。A／B／Cを提示 |
| 5 | 実装方式を豆虎Gateが裁定 | 未実施。ここで停止 |
| 6 | 直線1区間 | 未実施 |
| 7 | 縁石・排水溝 | 未実施 |
| 8 | 南門接続 | 未実施 |
| 9 | 曲がり | 未実施 |
| 10 | 分岐 | 未実施 |
| 11 | 市場入口手前まで接続 | 未実施 |
| 12 | Collision／NavMesh | 未実施 |
| 13 | Standard／Tactical | 未実施 |
| 14 | 外観Gate | 未実施 |
| 15 | Automation／Package | 未実施 |
| 16 | 基準点固定 | 未実施 |

今回の作業では1～4の読取設計だけを完了し、5以降へ進まない。

## 16. 外観評価

豆虎Gateが、175uu人物、昼間固定照明、受入済みStandard／Tactical Cameraで次を裁定する。

- 王都南門内側から市場入口手前までが連続した主要街路として読める。
- 近代的な全面歩道ではなく、現実寄り中世の路肩、縁石、排水構造として読める。
- F5が表面Materialとして自然にgeometryへ載り、目地、反復、尺度が破綻しない。
- 曲がり、分岐、段差、建物際がPrimitive／豆腐建築に見えない。
- Standard近距離とTactical俯瞰の両方で導線と形状が読める。
- G-3A／G-3B-Rは比較PoCであり、正式品質の代替として表示しない。

Codexは外観合否を代行しない。

## 17. 性能・Automation・Package

Geometry実装と外観Gate後の別工程で実施する。今回の実測結果は`N/A`である。

将来の必須検査：Asset Registry、Material compile、missing reference 0、Redirector 0、Collision、NavMesh、左クリック移動、左クリック保持移動、Camera Collision、Standard／Tactical、F5 Height未接続、既存Stage全回帰、50MB／100MB境界、Win64 Development Package、引数なし起動、Fatal／Crashなし。

性能測定はUE5.8実描画AdapterがRTX 3060かつDedicated VRAM 12GBと証拠化できた場合だけ行い、解像度、Scalability、VSync、frame cap、照明、Camera、測定時間を固定する。GPU設定、driver、Configを変更しない。

## 18. Git・証拠

- 今回の変更対象はStage V-1既存5文書と本書の計6文書だけである。
- `.uasset`、`.umap`、Content、Source、Config、Plugins、Build.cs、`.uproject`を変更しない。
- 読取監査の証拠path、既存Actor名、寸法、Collision、UV、Material Slot、再利用可否、riskを本書に残す。
- `UNKNOWN`を推測で埋めない。
- commit、tag、push、Git LFS導入を行わない。
- 将来の実装は豆虎Gateが方式を裁定した後、専用branchと専用Map／Folderを別指示で固定してから開始する。

## 19. 停止条件

- 正式基準点またはclean状態が崩れている。
- F5 Material／Texture／Height接続の変更が必要になる。
- 既存G-3A／G-3B-R／Stage V-1 Mapまたは元Materialを変更する必要がある。
- Stage V-2対象範囲を市場内部、王城前広場、王城正門、王都全域へ広げる必要がある。
- 新規Asset検索、取得、import、Plugin、Config、Camera、C++、因果コア、Save schema変更が必要になる。
- 実装方式を豆虎Gateが未裁定である。
- 既存Assetの寸法、Collision、UV、Material Slotを安全に確認できず、推測が必要になる。
- 100MB以上の単体fileを検出する。
- 正式外観の合否をCodexだけで決める必要がある。

停止時は原因、対象、確認済み事実、影響、選択肢を報告し、推測で実装しない。

## 20. 完了条件

今回の文書・読取設計は、次を満たしたとき完了とする。

- 対象範囲が王都南門内側接続部から市場入口手前までに固定されている。
- F5の役割と合格範囲外が明確である。
- 必要Geometryが列挙されている。
- 既存Assetの再利用可否が証拠path、実測済み事実、`UNKNOWN`を分離して記録されている。
- 実装方式の選択肢が提示されている。
- Asset検索、取得、Unreal変更をしていない。
- 指定6文書以外を変更していない。
- commit、tag、pushをしていない。

Stage V-2実装自体の完了条件は、実装順6～16、外観Gate、Automation、Package、基準点固定まで別途全項目PASSすることであり、本書作成だけでは満たさない。

## 21. 次工程

豆虎Gateが実装方式A／B／Cまたは修正案を裁定する。裁定後は専用作業branch、Stage V-2専用Map、作成を許可するgeometry path、Material Slot、寸法、停止地点を明示した実装指示を受けて、直線1区間から開始する。

F10、F11、F4、F6、F2、F8、F9の取得・importへは進まない。
