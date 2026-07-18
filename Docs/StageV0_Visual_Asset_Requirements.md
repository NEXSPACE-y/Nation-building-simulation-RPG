# Stage V-0 正式ビジュアル方針再設計 Asset要件定義書

## 1. 作業目的

Stage G-3B-Rを操作・寸法・Camera・NavMeshの比較基準として維持しつつ、王都入口から王城正門までを正式なゲーム画面として成立させるための背景Asset要件を固定する。

本書はAssetの要件定義であり、Assetの採用、購入、ダウンロード、Unreal Engineへのimportを承認するものではない。正式採用は豆虎Gateが個別に裁定する。

## 2. 作業基準点

- branch：`main`
- HEAD：`80cf7d555aa1970fd2979bd1c681558a47605b32`
- annotated tag：`stage-g3b-modular-castle-poc-v0.1.0`
- 開始時git status：clean
- 現行PoC：破棄せず比較基準として維持

現行PoCを正式製品キャラクター、正式背景、正式画面品質として扱わない。

## 3. 基準資料

### 基準資料A：上位ビジュアル仕様

- ファイル：`Docs/ChatGPT Image 2026年7月11日 21_50_21.png`
- 確認：PASS
- 用途：王都の階層構造、王城中心の構図、区画差、現実寄り中世、限定的な魔法文明、生活感、色調

### 基準資料B：最初の正式画面参考

- ファイル：`Docs/Gemini_Generated_Image_7lbjkp7lbjkp7lbj.png`
- 確認：PASS
- 用途：入口から王城前広場までの密度、王城の存在感、市場・街路・広場の接続、人間スケール、Standard／Tactical両Cameraでの可読性

完全再現は要求しない。

## 4. 対象導線

1. 王都入口
2. 主要街路
3. 市場付近
4. 王城前広場
5. 王城正門

王都全域、住宅区全域、下町全域、郊外、王城内装は対象外とする。

## 5. 基準画像からの観察事項

以下は既存必須要件を変更するものではなく、候補比較時の観察事項である。

- 王都入口では城壁・門・街路の先に王城が見え、導線の終点が早い段階から分かる。
- 木骨造、石造、漆喰、瓦屋根が混在し、同じ直方体建物の反復には見えない。
- 市場の高密度な小物群から王城前広場の余白へ、画面密度が段階的に変化する。
- 建物の高さ、屋根勾配、張り出し、塔、旗、煙突が遠景silhouetteを形成する。
- 石畳、縁石、階段、擁壁、排水境界が街路と広場の接続を視覚化する。
- 露店、馬車、樽、木箱、天幕、看板、照明、植栽が175uu人物の基準尺度を補強する。
- 王城は正面壁だけでなく、門洞、左右塔、中央高層部、屋根、扶壁、窓、胸壁の前後差で読める。
- 魔法文明は街全体を置換せず、照明、紋章、局所marker等へ限定して追加できる余地が必要である。

## 6. ビジュアル必須要件

- スクリーンショット一枚でゲーム画面に見える。
- 現実寄り中世として読める。
- 限定的な魔法文明を局所的に追加できる。
- LEGO、Minecraft、Primitive、豆腐建築に見えない。
- 屋根勾配、窓、梁、壁面変化、奥行きがある。
- 王都入口から王城正門まで連続した導線を構築できる。
- 175uuのPLAYER_Mと極端なスケール矛盾がない。
- StandardCharacterCameraの近距離観察に耐える。
- TacticalOverlookCameraで区画構造とランドマークが読める。
- 石材、木材、漆喰、屋根材を組み合わせられる。
- 王都の生活感を小物、照明、植生で構成できる。

## 7. Assetカテゴリ別要件

### 7.1 王都建築コア

必須：木骨造住宅、石造住宅、商店、宿屋または公共建築、2階以上、勾配屋根、煙突、窓、扉、庇、壁面凹凸。

評価時には、壁・屋根・梁・床の再配置可否、外観だけに依存しない入口構造、同一建物反復を避けるVariationを確認する。

### 7.2 王城・城壁コア

必須：城壁、城門、塔、胸壁、扶壁、王城正門、尖頭窓、尖塔または屋根塔、城壁と王城を接続できる部品。

正門は貼り付け模様ではなく通過可能な門洞として構築できること。巨大な平面壁だけで王城を構成しない。

### 7.3 街路・広場

必須：石畳、階段、縁石、広場舗装、排水溝、段差、壁際処理、道路と建物の接続部品。

Tileable Materialだけでなく、曲がり、交差、段差、縁部を処理できるgeometryまたはdecalが必要である。

### 7.4 市場・生活小物

必須：露店、天幕、荷車、樽、木箱、麻袋、食料品または雑貨、看板、ベンチ、街灯、松明、旗、ロープ。

一種類の小物を大量複製するのではなく、色・形・内容物のVariationが必要である。

### 7.5 植生・景観補助

必須：王城前の樹木、低木、草、鉢植え、石垣周辺の植生。

大規模森林Mapは対象外。単体配置または小規模clusterとして再利用できることを優先する。

### 7.6 Material・Decal補助

必須：石材、漆喰、木材、屋根瓦、汚れ、苔、雨染み、地面摩耗、車輪跡、壁面経年劣化。

色調変更はMaterial Instance等の非破壊方式を優先し、元Textureの再配布条件を確認する。

## 8. Camera・尺度要件

- Unreal単位は`1uu = 1cm`とする。
- 基準人物身長は175uuとする。
- 扉、階段、手すり、窓、露店台、馬車、ベンチを175uu人物との比較対象にする。
- StandardCharacterCameraでは扉・梁・窓・石畳・小物の近距離破綻を確認する。
- TacticalOverlookCameraでは入口、主要街路、市場、広場、王城正門の階層と通行可能空間を確認する。
- Camera Collision用の単純Collisionを確保できること。
- 見た目の地面とNavMesh用Collisionの段差を分離して調整できること。

## 9. 技術要件

### 必須

- Unreal Engine 5.8で利用可能、または一次情報とEpic公式versioning仕様から合理的に互換性を確認できる。
- Static MeshまたはUE project形式で再利用可能である。
- 依存Plugin、依存Asset、demo Map依存を列挙できる。
- Collision、LOD、Naniteの状態を確認できる。未記載は`UNKNOWN`とする。
- PBR用Base Color、Normal、Roughness等の状態を確認できる。
- 個別部品を再配置できる。統合demo Mapだけの場合は技術リスクとして記録する。

### 加点

- Modular構造
- LOD収録
- Nanite対応
- Collision収録
- PBR Material
- 2K以上のTexture
- Material Instance
- 内部入口構造
- Decal・汚れVariation
- demo Mapから独立した部品利用

Epic公式では、古いEngine versionで保存されたAssetは同じか新しいversionで読める一方、新しいversionのAssetを古いversionで開けない。したがって「UE5.0+」「UE5.3+」「UE5.6+」の明記はUE5.8互換の合理的根拠になるが、Plugin、Blueprint、shader、deprecated APIを含む完全動作はimport前には確定しない。

## 10. ライセンス要件

- 商用ゲーム利用可否を一次情報で確認する。
- standalone Assetとしての再配布禁止条件を記録する。
- private repositoryで共同作業者へ共有できるか確認する。
- 改変可否、credit要否、SourceArt保管条件を確認する。
- Fabのlicense種別と提供tierは別項目として確認する。Fab Standard Licenseはlicense種別であり、Personal／Professionalは同一権利範囲の価格tier、Reference-Onlyはsource形式なしの提供tierである。
- Fab Standard Licenseは商用Projectへの組込みと商用配布を許可するが、Asset単体の再配布を禁止する。
- listing本文でlicense種別を確認できない場合はlicense種別を`UNKNOWN`とし、取得画面で提供tierを確認できない場合は提供tierを別途`UNKNOWN`とする。両者を混同しない。
- Fab候補は取得画面で実際に提供されるtierを購入・取得前に再確認する。
- CC0候補は商用利用可能だが、原作者詐称やサイトのロゴ・商品画像の再利用はAsset自体のlicenseと分離して扱う。
- license種別が画面上で確認できない場合、商用利用可否を推測せず`UNKNOWN`とする。

## 11. 容量・Git管理要件

- 50MB超の単体ファイル有無を取得前に確認する。
- 100MB超の単体ファイル可能性を確認する。
- Download size、Installed size、SourceArt原本容量を区別する。
- `.uasset`、`.umap`、高解像度Texture、demo Mapを直接Git管理する場合の増分を評価する。
- 4K・8K Textureを無条件に原寸で格納しない。解像度方針は別裁定とする。
- 大規模packは導線に必要なsubset抽出可否を確認する。
- Git LFSの必要性と影響だけを報告し、Stage V-0では導入しない。
- 容量が一次情報にない場合は`UNKNOWN`とし、購入・DL前の販売者確認項目へ送る。

## 12. キャラクター方式の扱い

正式キャラクター方式は未裁定である。3D継続、Meshy再制作、外部3D Character、Blender調整、2.5D回帰、Stylized low-polyのいずれの方式を採用するかはStage V-0では裁定しない。

背景側は次だけを拘束する。

- 175uu人物と建物・小物の尺度比較ができる。
- 近距離Cameraでキャラクターと背景のTexture密度差が過大にならない。
- 群衆配置を想定し、Nanite、masked foliage、translucency、dynamic lightの同時負荷を記録する。
- 背景のart styleが特定の未裁定キャラクター方式だけを前提にしない。

## 13. 採用裁定

候補Assetの点数、Automation、Package、UE import成功は外観承認の代替ではない。

正式採用、購入、ダウンロード、import、Git LFS導入、SourceArt保管方式はすべて豆虎Gateの個別裁定待ちとする。

## 14. 一次情報

- [Fab Standard License](https://www.fab.com/eula?lang=en)
- [Epic: Versioning of Assets and Packages in Unreal Engine 5.8](https://dev.epicgames.com/documentation/en-us/unreal-engine/versioning-of-assets-and-packages-in-unreal-engine)
- [Epic: Updating Projects to Newer Versions of Unreal Engine 5.8](https://dev.epicgames.com/documentation/unreal-engine/updating-projects-to-newer-versions-of-unreal-engine?lang=en-US)
- [Epic: Migrating Assets in Unreal Engine 5.8](https://dev.epicgames.com/documentation/unreal-engine/migrating-assets-in-unreal-engine?lang=en-US)
- [Poly Haven Asset License](https://polyhaven.com/license)
- [ambientCG Plaster 002（ページ内にCC0説明を含む）](https://ambientcg.com/view?id=Plaster002)

## 15. Stage V-1実績反映と実装順規則

### 15.1 Stage V-1／F5正式基準点

- F5 `Paving Stones 149`は、2026-07-18 JSTの豆虎Gateで正式合格し、commit `f08a16c489bdfdaed2e118f99c989ae88728c026`、annotated tag `stage-v1-f5-paving-stones149-v0.1.0`として基準点固定済みである。
- 合格範囲は、単一2K Texture set、地面用Master Material、Material Instance 1件、Tiling 1.0、Base Color、NormalDX、Roughness、Ambient Occlusion、175uu人物との尺度感、Standard近距離／通常距離／最大Zoom Out、Tactical可読性、同一模様反復時の外観である。
- F5は再取得、再import、再検証しない。既存の合格済みMaterial／Material Instanceを後続PoCで参照利用する。
- F5合格は街路完成を意味しない。街路geometry、縁石、排水溝、段差、建物際処理、曲がり、交差・分岐、Height／Displacement、色違い、回転Variation、ランダム化、複数舗装パターン、正式王都Map採用、他Assetとの統合品質は合格範囲外である。

### 15.2 候補検証順と実装順

- F1～F11はAsset候補IDであり、採用、取得許可、実装順を意味しない。
- 候補検証順と実装順を分離する。実装順は、対象導線と対象構造物を入口側から完成させる順序で決定する。
- Material候補は、対象建築、対象部位、対象Mesh、勾配または面角度、UV方式、UV密度、Material Slotが確定した後に適合検証する。
- F10は対象建築、屋根Mesh、勾配、UV、Material Slotが確定するまで停止する。
- F11は対象建築、壁Mesh、壁構成、UV、Material Slotが確定するまで停止する。
- F4 `Modular Fantasy Castle`はLEGO／Minecraft／直方体中心の外観riskが正式ビジュアル要件と衝突するため、外観Pre-Gate不合格としてActive候補から除外する。取得、import、使用は行わない。Stage V-0時点の調査記録は調査履歴として保持する。
- F6、F2、F8、F9は、新しい実装順が確定するまで停止する。
- 次工程は`Stage V-2 王都主要街路Geometry PoC`とし、王都南門の王都内側接続部から主要街路を経て市場入口手前までを対象にする。後続Assetの検索・取得は別裁定とする。

### 15.3 用語定義

- Gate A／B／C：Asset検査工程。物理的な門ではない。
- 王都南門：王都入口側にある街の物理門。
- 王都城壁：王都外周側の城壁。
- 王城正門：王城前広場から王城へ入る物理門。
- 王城正門門楼：王城正門の門洞上部と周辺を構成する建築。
- 王城正門左右塔：王城正門の左右に配置する塔。
- 王城外郭城壁：王城敷地を囲う城壁。
- 王城中央棟：王城正面奥の主要高層建築。

実装指示では「門」「城門」「屋根」「壁」だけの曖昧な表現を使用せず、対象導線、対象建築または対象部位を必ず付記する。本節はStage V-0作成時点の要件、観察事項、技術条件、ライセンス条件および調査履歴を削除・再解釈せず、後続の正式裁定を追補するものである。
