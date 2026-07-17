# Stage V-0 Asset候補比較表

## 1. 調査条件

- 調査日時：2026-07-17 JST
- 調査範囲：王都入口→主要街路→市場付近→王城前広場→王城正門
- 調査順序：無料候補→不足分析→不足カテゴリだけ有料候補
- 情報源：Fab正式listing／seller page、制作者公式配布page、Epic公式document、license原文
- Asset購入：0
- Assetダウンロード：0
- Unreal import：0
- 正式採用：未裁定

価格は2026-07-17調査時の通常価格、USD、税別を記録する。セール表示が確認できた場合も通常価格と混同しない。listingが価格、license種別、提供tierを本文へ描画しなかった項目は、それぞれを独立して`UNKNOWN`とした。

## 2. 共通ライセンス・UE互換性判定

- [Fab Standard License原文](https://www.fab.com/eula?lang=en)：Fab Standard Licenseはlicense種別であり、商用Projectへの組込み・商用配布・改変を許可し、Asset単体の再配布を禁止する。Personal／Professionalは同一権利範囲の価格tierであり、Reference-Onlyはsource形式を提供しない。候補表ではlicense種別と提供tierを分離し、表示されない項目だけを個別に`UNKNOWN`とする。
- [Epic UE 5.8 Asset Versioning](https://dev.epicgames.com/documentation/en-us/unreal-engine/versioning-of-assets-and-packages-in-unreal-engine)：古いUE versionのAssetは同じか新しいversionで読める。Plugin、Blueprint、shaderの完全動作はimport前には確定しない。
- [Epic UE 5.8 FBX/汎用形式](https://dev.epicgames.com/documentation/en-us/unreal-engine/migrating-assets-from-unity-to-unreal-engine)：Static MeshはFBX/OBJ、TextureはPNG/TGA等をimportできる。FBX 2020.2が推奨されるため、配布FBX version不明はriskとする。
- [Poly Haven license](https://polyhaven.com/license)：CC0、商用利用可、credit不要、Asset自体の改変・再配布可。
- [Quaternius Medieval Village MegaKit](https://quaternius.com/packs/medievalvillagemegakit.html)：配布pageでCC0を明記。
- [ambientCG Plaster 002](https://ambientcg.com/view?id=Plaster002)：配布pageで全AssetのCC0・商用利用可を明記。

## 3. 無料候補一覧

### 3.1 基本情報

| ID | Asset名 | カテゴリ | 配布元／制作者 | URL | 価格 | License種別 | 提供tier／商用利用 | UE対応／UE5.8 |
|---|---|---|---|---|---:|---|---|---|
| F1 | ElderBoom Hollow Massive Medieval Village Environment | 建築／植生／統合 | Fab／karaman | [一次情報](https://www.fab.com/listings/e12de9d5-be28-40df-a387-42ae6f84e05c) | 無料 | UNKNOWN（listing本文に表示なし） | UNKNOWN（取得画面で要確認）。Personal／Professionalは同一権利範囲、Reference-Onlyはsource形式なし | UE 5.3+明記。5.8は明記上対応、実import未確認 |
| F2 | Medieval Village MegaKit | 建築 | Quaternius公式／Quaternius | [一次情報](https://quaternius.com/packs/medievalvillagemegakit.html) | 無料 | CC0 | tier非該当、商用可 | Unreal source version収録。対象UE versionはUNKNOWN、5.8は推定 |
| F3 | Medieval Village Megascans Sample | 建築／街路／統合sample | Fab／Quixel Megascans | [一次情報](https://www.fab.com/listings/2e11a225-a6ea-4781-a3e1-fe975b461894) | 無料 | UNKNOWN（listing本文に表示なし） | UNKNOWN（取得画面で要確認）。Personal／Professionalは同一権利範囲、Reference-Onlyはsource形式なし | Unreal Engine形式、対象version UNKNOWN、5.8確認不能 |
| F4 | Modular Fantasy Castle | 王城・城壁 | Fab／Forge of Fantasy | [一次情報](https://www.fab.com/listings/6c8fd8d8-9295-4844-8466-0a9096e8872c) | 無料 | UNKNOWN（listing本文に表示なし） | UNKNOWN（取得画面で要確認）。Personal／Professionalは同一権利範囲、Reference-Onlyはsource形式なし | Unreal Engine形式、対象version UNKNOWN、5.8確認不能 |
| F5 | Paving Stones 149 | 街路・広場Material | ambientCG／ambientCG | [一次情報](https://ambientcg.com/view?id=PavingStones149) | 無料 | CC0 | tier非該当、商用可 | PBR画像形式。UE5.8 importは合理的に推定、未import |
| F6 | Medieval Market / Tent Prop_01 | 市場・生活小物 | Fab／nete.cakmak | [一次情報](https://www.fab.com/listings/1251b2d1-2eb8-4329-ac69-1c5bb73c12c7) | 無料 | UNKNOWN（listing本文に表示なし） | UNKNOWN（取得画面で要確認）。Personal／Professionalは同一権利範囲、Reference-Onlyはsource形式なし | FBX。FBX version UNKNOWN、UE5.8は推定 |
| F7 | Low Poly Market Pack | 市場・生活小物 | Fab／atomdev | [一次情報](https://www.fab.com/listings/db13cfd8-6e4f-4ebb-8b16-4680b5a72c24) | 無料 | UNKNOWN（listing本文に表示なし） | UNKNOWN（取得画面で要確認）。Personal／Professionalは同一権利範囲、Reference-Onlyはsource形式なし | Unreal Engine形式、対象version UNKNOWN |
| F8 | Free Shrubs Pack (Ultra Realistic Wind) | 植生 | Fab／Greenleaf Vision | [一次情報](https://www.fab.com/listings/7ca465ab-fb9c-4d6b-bddb-82c20f604657) | 無料 | UNKNOWN（listing本文に表示なし） | UNKNOWN（取得画面で要確認）。Personal／Professionalは同一権利範囲、Reference-Onlyはsource形式なし | UE5.4情報あり。5.8はversioning上推定、未import |
| F9 | Free Nanite Tree - Acer buergerianum | 植生 | Fab／3DGardenPlants | [一次情報](https://www.fab.com/listings/a3e5ca74-9923-41ef-a109-9016b127390c) | 無料 | UNKNOWN（listing本文に表示なし） | UNKNOWN（取得画面で要確認）。Personal／Professionalは同一権利範囲、Reference-Onlyはsource形式なし | Unreal Engine／Nanite。対象version UNKNOWN |
| F10 | Roofing Tiles 011 A | Material補助／屋根瓦 | ambientCG／ambientCG | [一次情報](https://ambientcg.com/view?id=RoofingTiles011A) | 無料 | CC0 | tier非該当、商用可 | PBR画像形式。UE5.8 importは合理的に推定、未import |
| F11 | Plaster 002 | Material補助／漆喰 | ambientCG／ambientCG | [一次情報](https://ambientcg.com/view?id=Plaster002) | 無料 | CC0 | tier非該当、商用可 | PBR画像形式。UE5.8 importは合理的に推定、未import |

### 3.2 技術情報

| ID | Modular | LOD | Nanite | Collision | Texture／Material | 容量 | 依存Asset／demo依存 | SourceArt |
|---|---|---|---|---|---|---|---|---|
| F1 | modular-friendly | 必要箇所に収録と記載、段数UNKNOWN | UE5.2+対応記載 | custom＋auto | 2K～4K PBR、Material数はlisting placeholderのためUNKNOWN | 展開約12GB | 完成demo scene、依存詳細UNKNOWN | UE project、GLB、FBX、OBJ |
| F2 | 304 model以上、grid snap | UNKNOWN | UNKNOWN | UNKNOWN | custom shader。Texture解像度、Material数、PBR状態UNKNOWN | UNKNOWN | Unreal source versionあり | FBX、OBJ、Blend、glTF |
| F3 | UNKNOWN | UNKNOWN | UNKNOWN | UNKNOWN | photorealistic Megascans。Texture解像度、Material数、PBR内訳UNKNOWN | UNKNOWN | sample project依存度UNKNOWN。商品画像の一部要素は未収録 | Unreal Engine project |
| F4 | 壁、屋根、ruin、窓、扉をgrid用に構成 | UNKNOWN | UNKNOWN | UNKNOWN | PBR tagあり。Texture解像度、Material数UNKNOWN | UNKNOWN | nature environmentは未収録 | Unreal Engine |
| F5 | Tileable Material、geometryなし | 非該当 | 非該当 | 非該当 | 1K～8K JPG/PNG PBR。UE Material数はimport前のため非該当 | 2K JPG 16MB、4K JPG 59MB、8K JPG 215MB | なし | JPG／PNG PBR maps |
| F6 | 個別配置可、module規格UNKNOWN | UNKNOWN | UNKNOWN | UNKNOWN | Texture解像度、Material数、PBR状態UNKNOWN | UNKNOWN | demo Map・依存Asset UNKNOWN | FBX |
| F7 | 市場scene用pack | UNKNOWN | UNKNOWN | UNKNOWN | 1 Material＋color atlas。Texture解像度、PBR状態UNKNOWN | UNKNOWN | Unreal project、demo依存UNKNOWN | Unreal Engine |
| F8 | 個別配置 | 明示なし | opaque full-geometry Nanite | UNKNOWN | 季節・葉密度・PP2 wind。Texture解像度、Material数UNKNOWN | UE5.4必要disk約1.3GB | shrubsのみ、草・樹木なし。demo Map依存UNKNOWN | Unreal Engine |
| F9 | 2 variation個別配置 | UNKNOWN | full-geometry Nanite ready | UNKNOWN | Texture解像度、Material数、PBR状態UNKNOWN | UNKNOWN | demo Map・依存Asset UNKNOWN | Unreal Engine |
| F10 | Tileable Material、geometry／decal meshなし | 非該当 | 非該当 | 非該当 | 1K～8K JPG/PNG PBR。UE Material数はimport前のため非該当 | 2K JPG 12MB、4K JPG 43MB、8K JPG 196MB | なし | JPG／PNG PBR maps |
| F11 | Tileable Material、geometry／decal meshなし | 非該当 | 非該当 | 非該当 | 1K～8K JPG/PNG PBR。UE Material数はimport前のため非該当 | 2K JPG 25MB、4K JPG 95MB、8K JPG 423MB | なし | JPG／PNG PBR maps |

### 3.3 適合・不足・リスク

| ID | 適用予定箇所 | 必須要件適合項目 | 不足要素 | 技術リスク | 視覚リスク | 根拠状態 |
|---|---|---|---|---|---|---|
| F1 | 入口～市場の住宅、植生 | 住宅、教会、樹木、草花、PBR、Collision、LOD | 城壁・王城正門・市場小物の明細不足 | 12GB、mesh/material数がlisting内placeholder、subset抽出可否UNKNOWN | 「Stylized or Realistic」と幅があり、基準画像との統一確認が必要 | 一部確認 |
| F2 | 建築module補助 | 壁、床、階段、屋根、扉、窓、grid snap | 現実寄り材質、近距離PBR、明確な城・広場 | UE5.8 source version、Collision、LOD UNKNOWN | Stylized low-poly傾向が基準画像と衝突 | 確認済み＋技術一部UNKNOWN |
| F3 | 無料統合sample比較 | photorealistic medieval environment | 個別収録物、再配置性、城・市場・街路部品明細 | sample依存、容量、version、license種別、提供tier UNKNOWN | artistic interpretation画像に未収録要素あり | 一部確認 |
| F4 | 城壁、正門、塔 | modular wall、roof、window、door、gate、lantern | 扶壁、尖頭窓、尖塔、胸壁の具体数、現実寄り尺度 | UE version、LOD、Nanite、Collision、容量UNKNOWN | Fantasy寄り度と基準画像適合は目視裁定必要 | 一部確認 |
| F5 | 主要街路、広場舗装 | 現実寄り石畳、PBR、2K以上 | 階段、縁石、排水溝、段差、壁際geometry | 4K以上は単zip 50MB超。geometry/COLは別途必要 | 反復感、尺度、色調の調整必要 | 確認済み |
| F6 | 市場中心 | 天幕、箱、食料、籠、台、bench、stool、rug、pot | 荷車、樽、麻袋、看板、街灯、松明、旗、ropeの全充足 | FBX version、LOD、Collision、容量UNKNOWN | 他packとのPBR・色調統一UNKNOWN | 一部確認 |
| F7 | 無料市場layout比較 | 市場scene、単一material、雪・風system | 現実寄り近距離品質、品目明細 | UE version、容量、Collision、LOD UNKNOWN | Low-poly／Stylizedで正式基準に不適合の可能性が高い | 一部確認 |
| F8 | 王城前低木、壁際植生 | 11 shrubs、季節、wind、Nanite | 樹木、草、鉢植え | 1.3GB、high-poly opaque Naniteの多重配置負荷 | 写実度が建築候補より高すぎる可能性 | 確認済み＋license種別／提供tier UNKNOWN |
| F9 | 王城前樹木 | 2 tree variation、Nanite | 低木、草、鉢植え、LOD/風仕様 | full geometryの負荷、容量UNKNOWN | 樹種・樹形が王都景観に合うか裁定必要 | 一部確認 |
| F10 | 王城・住宅の屋根surface | 瓦、PBR、2K以上、CC0 | 石材、木材、汚れ、苔、雨染み、摩耗、車輪跡、専用decal | 8Kは50MB超。UE Materialは自作必要 | 他surfaceとの色調・scale統一が必要 | 確認済み |
| F11 | 王城・住宅の壁surface | 漆喰、PBR、2K以上、CC0 | 石材、木材、汚れ、苔、雨染み、摩耗、車輪跡、専用decal | 4K/8Kは50MB超。UE Materialは自作必要 | cleanな漆喰へ経年表現を加える必要 | 確認済み |

## 4. 無料候補の充足判定

| カテゴリ | 判定 | 無料で確認できた要素 | 不足または確認不能 |
|---|---|---|---|
| 王都建築コア | 一部充足 | F1の19住宅・教会・PBR、F2の304+ modular部品、F3の写実sample | F1は12GBかつ明細placeholder、F2はstylized、F3は個別部品と容量UNKNOWN |
| 王城・城壁コア | 一部充足 | F4の壁・屋根・窓・扉・gate・lantern | 現実寄り王城の扶壁・尖頭窓・尖塔・胸壁・尺度・UE5.8技術明細 |
| 街路・広場 | 一部充足 | F5の写実PBR石畳 | 階段、縁石、排水溝、交差、曲がり、段差、壁際geometry |
| 市場・生活小物 | 一部充足 | F6の天幕・食料・箱・籠・bench等、F7の市場scene | 一貫した写実品質での荷車・麻袋・看板・照明・旗・ropeまでの一式 |
| 植生・景観補助 | 一部充足 | F8 shrubs、F9 tree、F1のtrees/flowers/grass | 鉢植え、LOD/Collision統一、必要subset容量、群衆併用時性能 |
| Material・Decal補助 | 一部充足 | F5の石畳、F10の屋根瓦、F11の漆喰をCC0 PBRで確認 | 木材、汚れ、苔、雨染み、摩耗、車輪跡、経年劣化decalは未確認 |

無料調査集計：6カテゴリ、候補11件、充足0、一部充足6、不足0、確認不能0。

## 5. 不足箇所に限定した有料候補

### 5.1 基本情報

| ID | Asset名 | 不足対象 | 配布元／制作者 | URL | 通常価格 | License種別 | 提供tier／商用利用 | UE対応／UE5.8 |
|---|---|---|---|---|---:|---|---|---|
| P1 | Townsmith: Modular Medieval Town | 建築コア | Fab／Hivemind | [一次情報](https://www.fab.com/listings/7cdd67b0-ff96-403b-afbf-54542f562357) | US$79.99 | UNKNOWN（listing本文に表示なし） | UNKNOWN（取得画面で要確認）。Personal／Professionalは同一権利範囲、Reference-Onlyはsource形式なし | bundle一次情報で5.6+。5.8は明記上対応、未import |
| P2 | Medieval Village Pack | 建築コア | Fab／FreshCan | [一次情報](https://www.fab.com/listings/1aa74b49-f837-49c2-bcd2-01076c19fc1d) | US$29.99 | UNKNOWN（listing本文に表示なし） | UNKNOWN（取得画面で要確認）。Personal／Professionalは同一権利範囲、Reference-Onlyはsource形式なし | Unreal Engine／Unity。対象version UNKNOWN |
| P3 | Castle Forge: Modular Medieval Castle | 王城・城壁 | Fab／Hivemind | [一次情報](https://www.fab.com/listings/8b55ff8e-9d1c-43d8-9078-6aa03a4ffcd7) | US$89.99 | UNKNOWN（listing本文に表示なし） | UNKNOWN（取得画面で要確認）。Personal／Professionalは同一権利範囲、Reference-Onlyはsource形式なし | bundle一次情報で5.6+、Lumen 5.0+、Nanite。5.8明記上対応 |
| P4 | Medieval Castle | 王城・城壁 | Fab／Sidearm Studios | [一次情報](https://www.fab.com/listings/31235bb2-9576-4ee1-abdb-168d1eebaa1e) | US$79.99 | UNKNOWN（listing本文に表示なし） | UNKNOWN（取得画面で要確認）。Personal／Professionalは同一権利範囲、Reference-Onlyはsource形式なし | Unreal Engine。対象version UNKNOWN |
| P5 | Medieval Street Pack | 街路・広場 | Fab／FreshCan | [一次情報](https://www.fab.com/listings/1ac27c48-88db-4f11-a231-7041b37f88f8) | US$19.99 | UNKNOWN（listing本文に表示なし） | UNKNOWN（取得画面で要確認）。Personal／Professionalは同一権利範囲、Reference-Onlyはsource形式なし | Unreal Engine／UEFN。対象version UNKNOWN |
| P6 | Medieval Market & Shop Kit | 市場・生活小物 | Fab／Hivemind | [一次情報](https://www.fab.com/listings/2f726e37-8d89-4ef5-a205-48c37d943323) | US$19.99 | UNKNOWN（listing本文に表示なし） | UNKNOWN（取得画面で要確認）。Personal／Professionalは同一権利範囲、Reference-Onlyはsource形式なし | Unreal Engine、Nanite。UE5.8は推定 |
| P7 | Medieval Market Prop Pack | 市場・生活小物／Decal | Fab／LDN127 | [一次情報](https://www.fab.com/listings/74655d75-bf7e-482b-a960-c863c6c1c54e) | UNKNOWN | UNKNOWN（listing本文に表示なし） | UNKNOWN（取得画面で要確認）。Personal／Professionalは同一権利範囲、Reference-Onlyはsource形式なし | Unreal Engine。対象version UNKNOWN |
| P8 | Trees and Shrubs - Vol.3 | 植生不足 | Fab／Greenleaf Vision | [一次情報](https://www.fab.com/listings/380a46e9-55fc-4ad8-8b93-7bf6d16f7256) | US$19.99 | UNKNOWN（listing本文に表示なし） | UNKNOWN（取得画面で要確認）。Personal／Professionalは同一権利範囲、Reference-Onlyはsource形式なし | Unreal Engine、Nanite更新あり。5.8は推定 |
| C1 | Medieval Kingdom | 統合pack比較 | Fab／Hivemind | [一次情報](https://www.fab.com/listings/42d4a792-2b66-423d-9b20-84d6b2c578d8) | US$139.99 | UNKNOWN（listing本文に表示なし） | UNKNOWN（取得画面で要確認）。Personal／Professionalは同一権利範囲、Reference-Onlyはsource形式なし | Unreal Engine。対象version UNKNOWN |

P1とP3の通常価格・5.6+表記は[制作者が公開するMedieval Mega Bundle内訳](https://www.fab.com/listings/fec333db-0a84-4d47-a70d-5abf326961c7)で確認した。P2/P5の通常価格は[FreshCan公式seller page](https://www.fab.com/sellers/FreshCan?lang=en)で確認した。P4は[Fab公式検索](https://www.fab.com/search?tags=inn)、P6は[Fab公式marketplace検索](https://www.fab.com/search?tags=marketplace)、P8は[Fab公式shrub検索](https://www.fab.com/search?tags=shrub)、C1は[Fab公式market検索](https://www.fab.com/search?tags=market)で通常価格を確認した。セール価格は比較価格へ採用していない。

### 5.2 技術・適合情報

| ID | Modular／内容 | LOD・Nanite・Collision | Texture／Material | 容量／依存／demo | SourceArt | 適用予定・必須要件適合項目 | 不足・risk | 根拠状態 |
|---|---|---|---|---|---|---|---|---|
| P1 | 壁・屋根・梁・床、full interior、階段、shop/tavern、barrel/crate/canopy等 | Nanite、Lumen。LOD/Collision UNKNOWN | Texture解像度・Material数・PBR内訳UNKNOWN | 容量UNKNOWN。Water/Buoyancy plugin、demo Map依存UNKNOWN | Unreal Engine | 主要街路～市場住宅：modular建築、full interior、市場小物、Nanite、Lumen | industrial clutter説明混入があり商品説明精度を再確認。城coreは別途必要 | 一部確認 |
| P2 | 建物、props、bush/grass/tree、market tent | LOD/Nanite/Collision UNKNOWN | trimsheet、tileable roof、個別prop texture。解像度・Material数UNKNOWN | 容量・依存・demo Map依存UNKNOWN | Unreal Engine／Unity | 入口～市場：建物、props、market tent、植生、tileable roof | 2階建・石造/木骨の具体数、UE5.8、近距離品質確認が必要 | 一部確認 |
| P3 | 150+、wall/tower/gate/arch、内外装、snap-friendly | Nanite-ready。LOD/Collision UNKNOWN | Material分離、consistent UV/texel density。Texture解像度・Material数UNKNOWN | 容量・依存・demo Map依存UNKNOWN | Unreal Engine | 王城正門・城壁・塔：wall、tower、gate、arch、内外装、Nanite | 扶壁・尖頭窓・尖塔の具体数、175uu尺度、容量はUNKNOWN | 一部確認 |
| P4 | 133 mesh、modular castle、village、interior、market stall、flag、fountain | optimized表記。LOD/Nanite/Collision UNKNOWN | Texture解像度・Material数UNKNOWN、damage decalあり | Water/Landmass plugin。容量・demo Map依存UNKNOWN | Unreal Engine | 王城＋接続街区：castle、village、interior、market stall、flag、damage decal | 城と村を含みscope過大、subset抽出と色調統一が必要 | 一部確認 |
| P5 | small medieval town、clock tower、alley/square | PBR表記。LOD/Nanite/Collision UNKNOWN | Texture解像度・Material数・PBR内訳UNKNOWN | 容量・依存・demo Map依存UNKNOWN | Unreal Engine／UEFN referenced asset | 主要街路・広場接続：street、alley、square、clock tower、PBR | 石畳・縁石・排水溝・階段の個別明細が不足 | 一部確認 |
| P6 | 33 Nanite mesh、stall/shop/food/cart等 | Nanite。LOD/Collision UNKNOWN | 4K、ORM。Material数UNKNOWN | 容量・依存・demo Map依存UNKNOWN | Unreal Engine | 市場中心：stall、shop、food、cart、4K、Nanite | 4KによるGit/VRAM負荷、街灯・松明・旗・ropeの明細UNKNOWN | 確認済み＋一部UNKNOWN |
| P7 | 8 stall、5 table、containers、food、rope/plank、11 decal、BP variation | LOD/Nanite/Collision UNKNOWN | PBR。Texture解像度、Material数UNKNOWN | 容量UNKNOWN。5 master BP、2 demo level | Unreal Engine | 市場＋地面汚れ：stall、container、food、rope、decal、variation | 価格、UE5.8、容量UNKNOWN | 一部確認 |
| P8 | 19 tree、12 shrub、16 plant、grass/ivy/moss、season/wind | Nanite更新、簡易treeも含む。LOD段数/Collision UNKNOWN | 4K/8K leaves。Material数UNKNOWN | 必要disk 6GB、推奨32GB RAM/RTX3060、demo Map収録 | Unreal Engine | 王城前植生：tree、shrub、plant、grass、ivy、moss、season、wind | 対象導線には過大。masked/Nanite/群衆同時負荷 | 一部確認 |
| C1 | 580 mesh、fully modular town/castle、full interior、wall spline、foliage、VFX | LOD/Collision明細UNKNOWN | Texture解像度・Material数・PBR内訳UNKNOWN | 容量、plugin、依存、demo Map依存UNKNOWN | Unreal Engine | 導線全体：town、castle、interior、wall spline、foliage、VFX | 単一pack依存、scope/容量過大、必要部品だけ抽出可能かUNKNOWN | 一部確認 |

## 6. 機械評価

配点：世界観20／導線適用20／Modular15／近距離10／俯瞰10／UE5.8 10／性能5／license明確性5／容量・Git 5。`未`は0点ではなく未評価で、合計分母から除外した。

| ID | 世界 | 導線 | Mod | 近 | 俯瞰 | UE | 性能 | License | 容量 | 評価可能分合計 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| F1 | 15 | 16 | 12 | 9 | 8 | 10 | 3 | 2 | 1 | 76/100 |
| F2 | 5 | 12 | 15 | 4 | 8 | 4 | 5 | 5 | 未 | 58/95 |
| F3 | 17 | 14 | 未 | 9 | 9 | 未 | 2 | 2 | 未 | 53/70 |
| F4 | 12 | 16 | 15 | 7 | 8 | 未 | 3 | 2 | 未 | 63/85 |
| F5 | 17 | 8 | 3 | 9 | 8 | 8 | 4 | 5 | 5 | 67/100 |
| F6 | 15 | 15 | 8 | 8 | 7 | 6 | 3 | 2 | 未 | 64/95 |
| F7 | 4 | 14 | 10 | 4 | 7 | 未 | 5 | 2 | 未 | 46/85 |
| F8 | 17 | 8 | 5 | 10 | 6 | 8 | 4 | 2 | 1 | 61/100 |
| F9 | 16 | 7 | 5 | 9 | 5 | 未 | 4 | 2 | 未 | 48/85 |
| F10 | 16 | 10 | 5 | 9 | 7 | 8 | 4 | 5 | 4 | 68/100 |
| F11 | 16 | 10 | 5 | 9 | 7 | 8 | 4 | 5 | 4 | 68/100 |
| P1 | 18 | 19 | 15 | 9 | 9 | 10 | 4 | 2 | 未 | 86/95 |
| P2 | 17 | 16 | 12 | 8 | 8 | 未 | 3 | 2 | 未 | 66/85 |
| P3 | 18 | 19 | 15 | 10 | 9 | 10 | 5 | 2 | 未 | 88/95 |
| P4 | 18 | 18 | 13 | 9 | 8 | 未 | 3 | 2 | 未 | 71/85 |
| P5 | 12 | 15 | 12 | 7 | 8 | 未 | 3 | 2 | 未 | 59/85 |
| P6 | 17 | 18 | 10 | 10 | 8 | 8 | 5 | 2 | 未 | 78/95 |
| P7 | 18 | 19 | 10 | 10 | 8 | 未 | 3 | 2 | 未 | 70/85 |
| P8 | 17 | 8 | 5 | 9 | 7 | 7 | 3 | 2 | 1 | 59/100 |
| C1 | 18 | 20 | 15 | 9 | 10 | 未 | 4 | 2 | 未 | 78/85 |

点数は比較のための機械評価であり、採用順位ではない。特にlisting画像と基準画像の外観一致、175uu尺度、近距離品質は豆虎Gateの目視と未実施importで再確認する。

## 7. 構成A・B・C比較

| 構成 | 内容 | 通常価格合計 | 充足見込み | 主なrisk |
|---|---|---:|---|---|
| A：無料のみ | F1＋F4＋F5＋F6＋F8＋F9＋F10＋F11を必要subsetとして組合せる | US$0 | 建築、城、石畳、市場、植生、屋根瓦、漆喰を形の上では配置可能 | F1約12GB、art style不統一、F4/F6のUE・容量・license種別／提供tier UNKNOWN、街路geometry不足 |
| B：無料中心＋不足だけ有料 | 無料F5＋F8＋F9＋F10＋F11を維持し、P1建築＋P3城＋P5街路＋P6市場で不足を補う | US$209.96 | 対象導線の各カテゴリを専用packで比較可能 | 4pack間の色調・尺度統一、総容量UNKNOWN、license種別／提供tier未確認、4K/Nanite同時負荷 |
| C：統合Asset Pack | C1 Medieval Kingdom 1packで建築・城・市場・植生を比較 | US$139.99 | 580 mesh、town/castle/interior/wall spline/foliageを一つのart directionで比較可能 | 容量・UE5.8・依存・subset抽出がUNKNOWN、scope過大、単一vendor依存 |

どの構成も正式採用案ではない。Aは無料での成立可能性、Bは不足個所ごとの費用、Cは統一感と容量を比較するための資料である。

## 8. License確認結果

- QuaterniusとambientCG：CC0、商用利用可を一次情報で確認済み。
- Fab Standard License：license種別であり、商用Projectへの組込み可、Asset単体再配布不可を原文で確認済み。
- Fabの提供tier：Personal／Professionalは同一権利範囲で、Reference-Onlyはsource形式なし。license種別とは別項目として扱う。
- Fab個別候補：listing本文で確認できないlicense種別を`UNKNOWN`、取得画面で未確認の提供tierを別途`UNKNOWN`とした。両者は全件取得前に独立して再確認する。
- Fab候補のSourceArtをpublic repositoryへ置くことは避け、licenseに適合するprivateなProject共有範囲を豆虎Gate裁定後に定義する。

## 9. UE5.8互換性確認結果

- 明記上対応：F1（5.3+）、P1/P3（5.6+）。実importは未実施。
- 合理的推定：F2、F5、F6、F8、F10、F11、P6、P8。汎用形式または古いUE5 Assetだが、Plugin・shader・FBX version確認が残る。
- 確認不能：F3、F4、F7、F9、P2、P4、P5、P7、C1。listingに対象UE versionがない。

## 10. 容量・Git LFS risk

- F1：約12GB。repository直格納は不適。外部原本保管と必要subset抽出の裁定が必要。
- F8：約1.3GB。樹木を含まないshrubs単体としても大きい。
- P8：約6GB。今回の小規模植生要件には過大。
- F5：4K JPG zip 59MB、8K JPG zip 215MB。2K JPG zip 16MBなら単体50MB未満。
- F10：2K JPG zip 12MB、4K JPG zip 43MB、8K JPG zip 196MB。展開後Textureと`.uasset`容量はUNKNOWN。
- F11：2K JPG zip 25MB、4K JPG zip 95MB、8K JPG zip 423MB。展開後Textureと`.uasset`容量はUNKNOWN。
- その他Fab pack：Download/Installed sizeと最大単体fileがlisting本文にないため全件UNKNOWN。
- 50MB超のTexture、100MB超の`.umap`またはSourceArtが発生する可能性がある。Git LFS必要性は高いが、Stage V-0では導入しない。

## 11. UNKNOWN一覧

1. 多くのFab listingのlicense種別と提供tier、Download size、Installed size、最大単体file。
2. F3/F4/F7/F9/P2/P4/P5/P7/C1の対応UE versionとUE5.8実動作。
3. F1の正確なmesh/material数。listing自体にplaceholderが残る。
4. 各候補の175uu人物に対する扉・階段・手すり・露店・馬車の実測scale。
5. F4/P3/P4の扶壁・尖頭窓・尖塔・胸壁の具体部品数。
6. 多くの候補のLOD段数、Collision方式、Material数、Texture枚数。
7. P1/P3/C1等の必要subsetだけをlicenseと参照を壊さず分離できるか。
8. StandardCharacterCamera近距離とTacticalOverlookでの実画面品質・performance。

## 12. 豆虎Gate裁定待ち

1. 無料F1の約12GBと説明placeholderを許容して無料構成Aの検証へ進むか。
2. 無料F4を王城候補として外観比較対象に残すか、P3/P4の有料比較を優先するか。
3. 構成A/B/Cのうち、購入・DLを伴わない次の詳細確認対象をどれにするか。
4. Fab個別license種別と提供tier、容量、UE5.8 supportを販売者確認してから裁定するか。
5. 50MB超Assetの外部保管方針とGit LFS検討を別Stageへ切り出すか。
6. 背景art style確定前に、未裁定の正式キャラクター方式との並置試験をどの段階で行うか。
