# Stage V-1 無料Asset取得・実機適合検証PoC 実装設計書

## 1. 文書状態

- 作業種別：Stage V-1実装前の設計・検証計画
- 正式採用：未裁定
- Asset取得：未実施
- Unreal Engine import：未実施
- 豆虎Gate：実装開始前裁定待ち

本書は無料Assetを取得・importする許可ではない。取得画面の確認、License種別と提供tierの確認、無料取得操作、外観の最終判定は豆虎Gateが行う。

## 2. 目的

Stage V-0で定義した正式ビジュアル方針を維持し、王都入口から王城正門までに用いる無料Asset候補を、一候補ずつ隔離して実機検証する手順を固定する。

固定対象は次のとおり。

- 候補の取得順序と保留条件
- Asset取得前Gate
- SourceArt原本のリポジトリ外保管
- Unreal Engine 5.8への隔離import先
- 命名規則とVendor原名の追跡
- 175uu PLAYER_Mによる尺度検証
- StandardCharacterCamera／TacticalOverlookCameraによる表示検証
- 外観比較用スクリーンショット条件
- 性能、Collision、NavMesh、Package、既存Stage回帰の検証条件
- 不合格Assetの撤去と証拠保全
- Git、50MB超・100MB超ファイル、Git LFS未導入時の扱い

## 3. 正式基準点

- branch：`main`
- HEAD：`3ff9394d2fe8e57c2fb425ae84c52fdd90b311c6`
- origin/main：`3ff9394d2fe8e57c2fb425ae84c52fdd90b311c6`
- annotated tag：`stage-v0-visual-direction-v0.1.0`
- 開始時git status：clean

正本資料：

- `Docs/StageV0_Visual_Asset_Requirements.md`
- `Docs/StageV0_Asset_Candidate_Matrix.md`
- `Docs/ChatGPT Image 2026年7月11日 21_50_21.png`
- `Docs/Gemini_Generated_Image_7lbjkp7lbjkp7lbj.png`

基準画像確認結果：PASS。現実寄り中世、石造と木骨造の混在、市場から王城前広場へ変化する密度、門洞・左右塔・中央高層部・屋根で読める王城、人間スケール、夕方の局所照明を比較軸とする。完全再現は要求しない。

## 4. 最上位拘束

- Stage V-0の裁定を変更・再解釈しない。
- `UNKNOWN`を推測で補完しない。
- 機械評価点を採用順位として扱わない。
- 取得、import、Automation、Packageの成功を外観承認の代替にしない。
- 正式外観の合否は豆虎Gateだけが裁定する。
- 現行Stage G-1B～G-3B-Rは操作、Camera、尺度、NavMeshの比較対象として維持するが、正式ビジュアルとは扱わない。
- 現行PoCのContent、Map、Blueprint、C++、Configを上書きしない。
- 複数候補を同時importしない。
- 有料候補は、無料候補の不足が記録され豆虎Gateが個別承認するまで取得候補にしない。

## 5. 対象導線と対象外

検証対象導線：

1. 王都入口
2. 主要街路
3. 市場付近
4. 王城前広場
5. 王城正門

対象外：王城内装、住宅区全域、下町全域、郊外、戦闘、NPC追加、キャラクター方式の裁定、正式王都Mapへの置換。

## 6. 無料候補の分類

| 区分 | 候補 | Stage V-1での扱い |
|---|---|---|
| 第一検証群 | F5 Paving Stones 149 | 街路Materialの近距離品質、反復、尺度を検証 |
| 第一検証群 | F10 Roofing Tiles 011 A | 屋根Materialの勾配面表示、反復、色調を検証 |
| 第一検証群 | F11 Plaster 002 | 壁面Materialの近距離品質、経年表現の不足を検証 |
| 第二検証群 | F4 Modular Fantasy Castle | 城壁、城門、塔、王城moduleの形状と組立性を検証 |
| 第二検証群 | F6 Medieval Market / Tent Prop_01 | 市場小物、175uu尺度、配置Variationを検証 |
| 比較検証群 | F2 Medieval Village MegaKit | module構造、尺度、配置作業性の比較用。正式外観適合を前提にしない |
| 植生検証群 | F8 Free Shrubs Pack | F4/F6/F2の構造試験後にNanite・容量・性能を検証 |
| 植生検証群 | F9 Free Nanite Tree | F8と分離し、単独で形状・性能を検証 |
| 特別保留 | F1 ElderBoom Hollow Massive Medieval Village Environment | 約12GB。昇格条件を全て満たすまで取得しない |
| 初回対象外 | F3 Medieval Village Megascans Sample | `UNKNOWN`が多いため初回検証から除外。不採用裁定ではない |
| 初回対象外 | F7 Low Poly Market Pack | Stylized傾向との衝突riskにより初回検証から除外。不採用裁定ではない |

無料6カテゴリはいずれも一部充足であり、単独採用可能と確定したカテゴリはない。

## 7. 検証の原子単位

一つの候補について、次を一つの連続した検証単位とする。

1. Gate A：無料ライブラリ取得前確認
2. 豆虎Gateによる無料ライブラリ取得。実ファイルはまだダウンロードしない
3. Gate B：ライブラリ取得後、ダウンロード／import前確認
4. 豆虎Gateが承認した実ファイルのダウンロード
5. Gate C：ダウンロード後、import前の実ファイル確認
6. 原本保全とSHA-256記録
7. 候補専用Folderへの単独import
8. 専用Mapでの技術・尺度・Camera・性能検証
9. 固定6 Shotの作成
10. 豆虎Gateによる合格／条件付き合格／不合格／確認不能の裁定
11. 合格候補の保全、または不合格候補のProject内撤去

候補Aの裁定と保全／撤去が完了するまで候補Bをimportしない。

## 8. Phase順序

### Phase 0：Gate A → 無料ライブラリ取得 → Gate B → ダウンロード → Gate C

開始条件：Stage V-0基準点と作業ツリーcleanを確認済み。

終了条件：Gate A、Gate B、Gate Cが順番どおりPASSし、原本保全とSHA-256確認を完了した。無料ライブラリ取得と実ファイルダウンロードを同一操作として扱わない。

停止条件：各Gateの必須項目に`UNKNOWN`または`FAIL`が残る、手順を飛ばす必要がある、ダウンロード後に100MB以上の単体fileを検出する。

### Phase 1：F5 → F10 → F11

三候補を同時に扱わず、F5、F10、F11の順で候補単位の全サイクルを完了する。

開始条件：各候補のPhase 0 PASS、外部保管先の空き容量確認。F5、F10、F11のTexture解像度は2Kで固定する。

終了条件：同一基準Mesh上でMaterial compile、近距離、Standard、Tactical、反復、Texture streaming、容量を記録し、豆虎Gate裁定が完了した。

停止条件：原Texture加工が必要、色やscaleを隠すための破壊的加工が必要、Normal方向を安全に判断できない、単体100MB超ファイルが発生、License条件と保管方法が衝突する。

### Phase 2：F4

開始条件：F4のPhase 0 PASS、王城・城壁比較に使用するPhase 1 Materialの裁定完了、Plugin・demo依存の確認完了。

終了条件：城壁、通過可能な門洞、塔、屋根、窓、扉の実在をinventory化し、175uu尺度、Collision、NavMesh、Standard／Tactical表示、組立性、Packageを記録して豆虎Gate裁定が完了した。

停止条件：門が貼り付け表現のみ、必要部品をdemo Mapから安全に分離できない、UE5.8非互換、未承認Pluginが必須、既存Stageの変更が必要、巨大平面・直方体反復だけでしか王城を構成できない。

### Phase 3：F6

開始条件：F6のPhase 0 PASS、F4の保全／撤去完了。

終了条件：天幕、箱、食料、籠、台、bench、stool、rug、pot等をinventory化し、175uu人物との尺度、Collision、NavMesh、配置Variation、近距離品質を記録して豆虎Gate裁定が完了した。

停止条件：FBX versionを安全に読み込めない、尺度補正が複数箇所に分散する、主要小物にCollisionを安全に付与できない、Source形式やLicense条件が事前記録と一致しない。

### Phase 4：F2

開始条件：F2のPhase 0 PASS、F6の保全／撤去完了。F2を正式外観候補ではなく構造比較対象として扱うことを再確認する。

終了条件：grid snap、壁・床・階段・屋根・扉・窓のmodule構造、組立時間、175uu尺度を記録し、外観は独立して豆虎Gateが裁定した。

停止条件：CC0原本と配布物が一致しない、UE source versionを5.8で安全に開けない、比較目的を超えて正式背景へ流用する必要が生じる。

### Phase 5：F8 → F9

F8とF9を別候補として順番に検証する。

開始条件：王城前広場の構造候補が決まり、Nanite植生を置く位置と現実的な最大数が定義済み。F8は約1.3GBの保存・import容量を豆虎Gateが個別承認済み。

終了条件：1／10／50／100個または事前に理由を記録した現実的上限で、Standard／Tactical、Nanite、wind、VRAM、GPU、CPU、Collisionを記録して豆虎Gate裁定が完了した。

停止条件：GPU設定変更が必要、群衆・Dynamic Lightとの比較条件を維持できない、100MB以上の単体fileを検出する、王城前広場の構造が未確定。F8はPhase 5開始条件を満たすまでライブラリ取得を含め保留する。

### Phase 6：F1再裁定

F1は次を全て満たした場合だけ取得候補へ昇格する。

1. License種別確認
2. 提供tier確認
3. Gate BでDownload size、必要空き容量、UE version、配布形式、依存関係を確認
4. Gate CでInstalled size実測、最大単体file、50MB／100MB境界、file inventoryを確認
5. リポジトリ外SourceArt保管容量確認
6. 必要subset抽出可否確認
7. listingのplaceholder記載解消
8. F2/F4/F6等の既存無料候補では建築品質が不足したという記録
9. 豆虎GateのF1個別承認

一項目でも未達なら保留を維持する。

## 9. Asset取得・ダウンロード・import前Gate

### 9.1 Gate A：無料ライブラリ取得前

次を確認し、`UNKNOWN`または`FAIL`が一つでも残れば無料ライブラリへ追加しない。

- Asset名、候補ID、配布元、制作者、一次URLによるAsset同一性
- 無料表示
- License種別
- Fabの場合の提供tier
- 商用ゲーム利用可否
- 改変、ローカル保全、private repository／共同作業者共有、credit条件
- Source形式が提供されること。Reference-Onlyはsource形式なしとして停止
- 豆虎Gateによる無料ライブラリ取得の明示承認

Gate A PASS後に豆虎Gateが行うのは「無料ライブラリ取得」である。この時点では実ファイルをダウンロードしない。

### 9.2 Gate B：ライブラリ取得後、ダウンロード／import前

ライブラリまたはLauncherの表示から次を確認する。

- 表示されるDownload size
- 対応UE version
- 配布形式
- 依存Plugin、依存Asset
- Download sizeと展開余裕を含む必要空き容量

Gate Bに`UNKNOWN`または`FAIL`が残れば実ファイルをダウンロードしない。Gate B PASS後、豆虎Gateがダウンロードを明示承認してから実ファイルを取得する。無料ライブラリ取得済みであることを、ダウンロード済みまたはimport可能と解釈しない。

### 9.3 Gate C：ダウンロード後、import前

ダウンロード済み実ファイルを読み取り専用で検査し、次を確認する。

- archiveと展開物の実容量
- Installed size実測
- 最大単体file
- 50MB以上100MB未満の単体file一覧
- 100MB以上の単体file一覧
- file inventory、SHA-256、Source形式version
- Mesh、LOD、Nanite、Collision、Material、Texture、demo Map依存

50MB以上100MB未満はlocal検証を許可するが、commit／push前に件数、容量、pathを豆虎Gateへ報告する。100MB以上を一件でも検出した場合はimportせず停止する。最大単体fileとInstalled size実測はGate Aの取得前必須項目ではない。

### 9.4 import後の実機検証項目

Gate C PASS後に限り、次を実機で確認する。これはGate Cの代替ではない。

- 175uu PLAYER_Mとの尺度
- StandardCharacterCamera近距離品質
- TacticalOverlookCameraでのsilhouetteと区画可読性
- Camera Collision、NavMesh、歩行可能性
- Texture streaming、VRAM、GPU、CPU、frame time
- 反復配置時の単調さと色調統一
- 基準画像および他候補との外観整合
- Packageと既存Stage回帰

## 10. 人間操作とCodex作業の分離

### 豆虎Gateが行う

- Fab取得画面の表示確認
- License種別と提供tierの確認
- Gate A後の無料ライブラリ取得ボタンの操作
- Gate B後の実ファイルダウンロード承認と操作
- 有料購入判断
- 取得対象、解像度、容量例外の個別承認
- 外観・操作感の最終判定
- 合格／条件付き合格／不合格／確認不能の裁定

### Codexが行う

- Gate Cでユーザーがダウンロード済みの原本についてSHA-256、容量、最大単体file、構成を検査
- 原本・展開物・証拠のリポジトリ外保全
- 承認された候補一件の隔離import
- Folder整理、Vendor対応表、命名、Collision、NavMesh
- 尺度、Camera、性能、Automation、Package、回帰
- 固定スクリーンショットと記録作成
- 候補単位のGit差分監査

CodexはFabの契約、License選択、取得、購入操作を代行しない。

## 11. SourceArt保管

推奨方式はリポジトリ外保管とする。

```text
C:\Users\rinpa\Desktop\TITLE_ExternalAssets\
└─ StageV1\
   └─ <CandidateID>_<ShortName>\
      ├─ Original\
      ├─ Expanded\
      ├─ Evidence\
      │  ├─ License\
      │  └─ Listing\
      ├─ Manifest\
      └─ Reports\
```

候補例：

- `F4_ModularFantasyCastle`
- `F5_PavingStones149`
- `F6_MedievalMarketTent`
- `F10_RoofingTiles011A`
- `F11_Plaster002`

保全規則：

1. Originalは取得原名のまま読み取り専用原本として保持する。
2. Expandedは展開物を保持し、Originalを上書きしない。
3. OriginalとExpandedのSHA-256、file size、相対pathをmanifestへ記録する。
4. 取得URL、取得日時JST、制作者、License種別、提供tierを記録する。
5. 豆虎Gateが保全したLicense画面・HTML・PDFのSHA-256を記録する。
6. Unreal取込先とVendor原名の対応表をManifestへ記録する。
7. SourceArt原本をGitへ追加しない。

`SourceArt/StageV1/`案は、License、容量、Git LFSの裁定がないためStage V-1では採用しない。

## 12. Unreal Engine隔離Folder

```text
Content/StageV1/
├─ External/
│  ├─ F2_MedievalVillageMegaKit/
│  ├─ F4_ModularFantasyCastle/
│  ├─ F5_PavingStones149/
│  ├─ F6_MedievalMarketTent/
│  ├─ F8_FreeShrubs/
│  ├─ F9_FreeNaniteTree/
│  ├─ F10_RoofingTiles011A/
│  └─ F11_Plaster002/
├─ Validation/
└─ Maps/
   └─ StageV1_AssetValidation_PoC
```

既存の`Content/Characters/`、`Content/Maps/`、`Content/StageG1B/`、`Content/StageG2A/`、`Content/StageG2B/`、`Content/StageG3A/`、`Content/StageG3B/`へ直接importしない。

Vendor同梱Folderを一時的に維持する必要がある場合も、候補別rootより外へ参照を作らない。候補間の共通Material化は各候補の豆虎Gate裁定完了後の別工程とする。

## 13. 命名規則

検証用に作成するAssetは`<CandidateID>_<Prefix>_<DescriptiveName>`とする。

| Prefix | 用途 | 例 |
|---|---|---|
| `SM_` | Static Mesh | `F4_SM_CastleGate_01` |
| `M_` | Master Material | `F5_M_PavingStones149_Master` |
| `MI_` | Material Instance | `F10_MI_RoofingTiles011A_Default` |
| `T_` | Texture | `F11_T_Plaster002_BaseColor_2K` |
| `BP_` | Blueprint | `F6_BP_MarketTent_Validation` |
| `MAP_` | Map | `MAP_StageV1_AssetValidation_PoC` |
| `DA_` | Data Asset | `F4_DA_VendorAssetMap` |

Vendor原名は改名対応表へ必ず残す。Vendor Asset本体の一括改名で参照が壊れる場合は原名を維持し、検証用wrapperだけに候補IDを付ける。

## 14. 専用検証Map

Map名：`StageV1_AssetValidation_PoC`

このMapは正式王都Mapではなく、正式MapへAssetを移植するための承認も意味しない。

### 14.1 基準エリア

- 身長175uu、Component Scale 1.0のPLAYER_M
- 100uu grid
- 高さ200uuの扉基準frame
- 17／20uuの階段蹴上げ基準
- 高さ90／110uuの手すり基準
- 既存G-3B-R城moduleを「操作・寸法比較用PoC」と明示して並置

### 14.2 建築エリア

- 壁、屋根、扉、窓、塔、城門をカテゴリ別に単品配置
- 同じ部品で建物一棟または城門一組を組み立てる作業性試験
- snap単位、pivot、rotation、mirror可否、module間gapを記録

### 14.3 Materialエリア

同一の検証用平面、垂直壁、45度勾配屋根へ次を適用する。

- F5石畳
- F10屋根瓦
- F11漆喰
- 既存G-3B-R Material（PoC比較用）

Mesh、UV密度、照明、exposureを固定し、候補ごとに変更しない。

### 14.4 市場小物エリア

- 天幕、露店、樽、木箱、食料、bench等を単品配置
- 175uu PLAYER_Mを各groupの横へ配置
- 同一Assetの反復とVariationを分けて配置

### 14.5 Camera試験導線

- StandardCharacterCameraの近距離、通常距離、最大Zoom Out
- TacticalOverlookCameraの俯瞰
- 右ドラッグ、Zoom、F6切替、Player追従
- 壁・屋根・塔・天幕・樹木によるCamera Collision
- 左クリック移動、左クリック保持、NavPathとの共存

Stage G-2Aで受入済みのCamera設定値は変更しない。

### 14.6 性能試験エリア

候補を1、10、50、100個の順で配置する。市場の大型天幕や樹木など100個が用途上不合理な候補は、import前に現実的上限と理由を記録し、豆虎Gate承認後に上限を変更する。結果が悪いことを理由に測定後の上限を下げない。

## 15. 尺度検証

初回importはvendor原尺度、Import Uniform Scale 1.0、Component Scale 1.0で行う。

測定対象：

| 対象 | 基準／記録値 |
|---|---|
| PLAYER_M | 175uu |
| 扉開口高 | 200uu基準。実測値と比率を記録 |
| 階段蹴上げ | 17～20uu |
| 手すり高 | 90～110uu |
| 市場台・counter | PLAYER_Mの腰～胸との位置関係と実測値 |
| bench | 座面高とPLAYER_M膝位置、実測値 |
| 城門 | PLAYER_M、荷車想定幅、通過可能Collisionを記録 |

単位不一致を検出した場合、補正はImport Uniform Scaleまたは再import処理の一箇所に集約し、Component Scaleは最終的に1.0とする。撮影用Actor Scale、Map Scale、Camera歪曲による場当たり補正は禁止する。

## 16. Camera検証

### StandardCharacterCamera

- PLAYER_M横の近距離でTexture、mesh edge、法線、継ぎ目を確認
- 通常距離で建物・小物・PLAYER_Mの関係を確認
- 最大Zoom Outで反復、roof silhouette、街路接続を確認
- Camera Collisionで壁・屋根・塔の内側へ侵入しないことを確認
- 右ドラッグ中の手動操作、移動中の自動Yaw追従、到着後Yaw維持を回帰確認

### TacticalOverlookCamera

- F6で切り替え、Player位置追従を確認
- 自動Yaw追従が入らないことを確認
- 入口、主要街路、市場、広場、正門の階層が読めることを確認
- 大型Assetが通路やmarkerを隠さないことを確認

候補の見栄えを良くするためにFOV、Pitch、Zoom範囲、Camera方式を変更しない。

## 17. 外観評価と固定Shot

各候補について同一解像度、同一Camera設定、同一PLAYER_M、同一基準Mesh、同一lighting presetを使用する。

1. `shot_01_player_close`：PLAYER_M横、近距離
2. `shot_02_standard_default`：StandardCamera通常距離
3. `shot_03_standard_max_zoom_out`：StandardCamera最大Zoom Out
4. `shot_04_tactical_overlook`：TacticalOverlook
5. `shot_05_g3br_comparison`：既存G-3B-Rとの並置。G-3B-Rは正式品質ではなく比較対象と表示
6. `shot_06_evening_reference`：基準画像に近い夕方照明

Shot 1～5は固定した昼間lightingを使用する。Shot 6のDirectional Light角度とExposureは一つの共通値を候補群の最初に固定し、候補ごとに最適化しない。

変更可能：Camera位置、記録済みのDirectional Light角度、共通Exposure、Asset間隔。

変更禁止：Asset形状、Texture内容、尺度を隠すCamera歪曲、過剰な被写界深度、基準画像との差を隠す加工。

外観判定は「合格／条件付き合格／不合格／確認不能」で記録する。点数だけで判定しない。

判定項目：

- 現実寄り中世として見える
- LEGO／Minecraft／Primitive／豆腐建築に見えない
- 175uu人物と尺度が合う
- Standard近距離に耐える
- Tacticalで形状と導線が読める
- 基準画像の世界観と衝突しない
- 他候補と組み合わせられる
- Material Instance等で非破壊の色調調整が可能
- 反復配置しても単調になりにくい
- 王都入口～王城正門の使用箇所が明確

## 18. 技術・性能検証

技術項目：Asset Registry、shader compile、missing reference、Redirector、Collision、NavMesh、Camera Collision、LOD、Nanite、Material compile、Texture streaming、Package、既存Stage回帰。

性能条件：

- 解像度：1920×1080。実行開始時に実値を記録し、不一致なら停止
- 実描画Adapter：性能測定前にUE5.8が実際に使用しているRHI Adapter名とDedicated VRAMをlogまたはRHI表示で証拠化する
- GPU基準：実描画Adapterが`NVIDIA GeForce RTX 3060`かつDedicated VRAMが12GBと確認できた場合だけRTX 3060基準で続行する
- 停止対象：`GTX 1660 SUPER`、その他Adapter、Adapter名またはVRAMを証拠化できない状態
- GPU／CUDA構成：確認後の現行構成を維持
- Scalability、VSync、frame cap：候補群開始時に実値を記録し固定
- Camera：StandardとTacticalの両方
- Lighting：Stage V-1専用固定preset
- warm-up：shader compileとTexture streamingが安定するまで測定しない
- 計測時間：各配置数、各Cameraで初期表示後60秒以上
- 配置数：1、10、50、100、または事前承認済みの現実的上限

記録値：平均／最小FPS、平均／最大frame time、GPU使用率、VRAM使用量、CPU使用率、Texture streaming warning、visible triangle数またはNanite統計、shader compile残数。

UE5.8実描画Adapter名とDedicated VRAMの証拠を性能記録へ添付するまで、配置数1の測定も開始しない。GTX 1660 SUPERまたはRTX 3060 12GB以外を検出した場合は性能測定前に停止する。GPU設定、電力制限、driver、解像度、Scalabilityを変更する必要が生じた場合も比較条件が変わるため停止する。

## 19. 不合格Assetの撤去

1. 専用検証MapとValidation Assetから当該候補への参照を除去する。
2. Reference Viewerで候補外からの参照が0であることを確認する。
3. 当該候補の専用Folderだけを削除する。
4. 候補Folderに由来するRedirectorだけを整理する。
5. Asset Registry、missing reference、Content差分を再確認する。
6. 専用Mapと既存StageのPackage／回帰を確認する。
7. Git差分から当該候補以外の変更がないことを確認する。
8. SourceArt原本、SHA、取得元、License証拠、不合格理由はリポジトリ外で保全する。
9. 検証記録へ撤去日時、撤去Asset数、残存参照0、Project内残存0を記録する。

`git reset --hard`、`git clean`、`git restore .`、リポジトリ全体を対象とする削除は禁止する。

## 20. Git・容量管理

- 実装開始基準はStage V-0 commit `3ff9394d2fe8e57c2fb425ae84c52fdd90b311c6`とする。
- 一候補を一作業単位とし、候補間で未裁定差分を混在させない。
- 実装時は候補別local branch `stage-v1/validate-<candidate-id>`方式を採用する。各branchは直前の正式基準点または承認済み候補commitから作成し、別候補の未裁定差分を含めない。
- 豆虎Gate裁定前はcommit、tag、pushしない。
- 合格または条件付き合格後にcommitが承認された場合、一候補を一commitに限定する。
- 不合格候補は候補専用参照とFolderだけを明示撤去し、全体restoreを使わない。
- SourceArt原本、License画面、配布archiveをGitへ追加しない。
- import前後に50MB以上と100MB以上の単体fileを候補Folder単位で列挙する。
- 50MB以上100MB未満はlocal検証を許可するが、commit／push前に件数、容量、pathを豆虎Gateへ報告する。
- 100MB以上を一件でも検出した場合は、import、commit、pushへ進まず停止する。
- Git LFSは未導入のまま維持し、必要性を検出してもStage V-1内で勝手に導入しない。
- demo Mapは必要部品の依存検証に不可欠で、容量・License・Package影響が許容された場合だけ残す。

実装時の容量検出は、候補rootを明示した読取専用の次の方式で行う。

```powershell
Get-ChildItem -LiteralPath '<CandidateRoot>' -Recurse -File |
  Where-Object { $_.Length -ge 50MB } |
  Select-Object FullName, Length

Get-ChildItem -LiteralPath '<CandidateRoot>' -Recurse -File |
  Where-Object { $_.Length -ge 100MB } |
  Select-Object FullName, Length
```

Git対象は`git status --short`、`git diff --name-only`、`git diff --cached --name-only`で候補単位に監査する。候補が未commitなら、Reference Viewer確認後に候補専用参照とFolderだけを明示撤去する。豆虎Gate承認済みの候補単位commitを後日取り消す場合は、履歴を消すresetではなく、ユーザー承認を得て当該commitだけを`git revert`する。

## 21. 実装時の停止条件

- Gate A、Gate B、Gate Cの各必須項目に`UNKNOWN`または`FAIL`が残る
- 無料ライブラリ取得と実ファイルダウンロードを分離できない
- Fab取得画面をCodexが操作する必要がある
- 外部Asset、別候補、有料候補への勝手な切替が必要
- 未承認Plugin、Git LFS、Config変更が必要
- Stage G-1B～G-3B-Rの受入済みAsset、Map、C++を変更する必要がある
- 因果コア、GameInstanceSubsystem、save schemaを変更する必要がある
- ダウンロード後に100MB以上の単体fileを検出する
- SourceArtのLicenseと外部保管方針が衝突する
- UE5.8で安全に読み込めない
- 尺度補正を一箇所へ集約できない
- 固定Camera／lighting／性能条件を維持できない
- UE5.8実描画AdapterがRTX 3060 12GBではない、またはAdapter名とDedicated VRAMを証拠化できない
- Automation、Package、既存Stage回帰を通せない
- 正式ビジュアル適合をCodexだけで裁定する必要がある

停止時は、候補ID、原因、影響範囲、確認済み事実、選択肢だけを報告し、推測で継続しない。

## 22. 裁定反映と残裁定

反映済み：

- Phase 1のF5／F10／F11は2K Textureで固定する。
- 候補別local branch方式を採用する。
- 50MB以上100MB未満はlocal検証可とし、commit／push前に報告する。
- 100MB以上を一件でも検出した場合は停止する。
- F8はPhase 5まで、無料ライブラリ取得を含め保留する。

残裁定：

1. F4/F6のGate AとGate B確認後、どちらの無料ライブラリ取得・ダウンロードを先に個別承認するか。
2. Phase 5開始時にF8約1.3GBのライブラリ取得・ダウンロード・保管を個別承認するか。

## 23. 設計時点のUNKNOWN

1. Gate Aで確認するF4/F6/F8/F9のLicense種別と提供tier。
2. Gate Bで確認するDownload size、UE version、配布形式、依存関係と、Gate Cで実測するInstalled size、最大単体file、実file構成、LOD、Collision、Material、Texture、demo依存。
3. 各候補の175uu尺度、Standard／Tactical実画面品質、UE5.8実描画Adapter、VRAM・GPU・CPU負荷。
4. F1のplaceholder解消、必要subset抽出可否、約12GB保管の個別承認。

これらは該当するGateまたは候補別実機検証で解消し、推測で補完しない。
