# Stage V-1 Asset取得前チェックリスト

## 0. Asset取得指示ヘッダー

候補ごとに、外観Pre-Gateより前に次を固定する。未確定項目は`UNKNOWN`とし、曖昧な「門」「城門」「屋根」「壁」だけで取得を指示しない。

| 項目 | 記録値 |
|---|---|
| 候補ID |  |
| 正式Asset名 |  |
| 公式直リンク |  |
| Listing ID／Asset ID |  |
| 配布元 |  |
| 制作者 |  |
| 検証目的 |  |
| 使用予定導線 |  |
| 対象建築 |  |
| 対象部位 |  |
| 対象Mesh |  |
| 想定勾配または面角度 |  |
| 想定UV／UV密度 |  |
| 想定Material Slot |  |
| 取得対象 |  |
| 形式 |  |
| version |  |
| 解像度 |  |
| 取得後の停止地点 |  |
| 公式画像Pre-Gate判定 |  |
| 豆虎Gate判定日時 |  |

## 0.1 外観Pre-Gate

Gate A／B／CはAsset検査工程であり、物理的な門ではない。外観Pre-GateはGate Aより前に行い、公式掲載情報だけで正式ビジュアル要件との明白な衝突を除外する。

| 確認項目 | 記録値 | 状態 | 証拠path／URL |
|---|---|---|---|
| 公式直リンク |  |  |  |
| Listing ID／Asset ID |  |  |  |
| 配布元 |  |  |  |
| 制作者 |  |  |  |
| 公式掲載画像 |  |  |  |
| 正式ビジュアル要件との明白な衝突 |  |  |  |
| LEGO／Minecraft／Primitive／豆腐建築risk |  |  |  |
| 対象導線 |  |  |  |
| 対象建築 |  |  |  |
| 対象部位 |  |  |  |
| 豆虎Gate目視判定 |  |  |  |

外観Pre-Gate判定：`GO / STOP`

判定日時（JST）：

外観Pre-Gateが`STOP`の場合は、License確認、無料ライブラリ取得、直接ダウンロード、Gate A以降へ進まない。

## 1. 使用方法

- 一候補につき本チェックリストを一部複製して記録する。
- `PASS`、`FAIL`、`UNKNOWN`、`N/A`のいずれかを記入する。
- 外観Pre-Gateが`STOP`の場合、Gate A以降へ進まない。
- Gate Aに`UNKNOWN`または`FAIL`が一つでもある場合、無料ライブラリ取得または直接ダウンロードへ進まない。
- Gate Bに`UNKNOWN`または`FAIL`が一つでもある場合、実ファイルをダウンロードしない。
- Gate Cに`UNKNOWN`または`FAIL`が一つでもある場合、importしない。
- 「無料ライブラリ取得」と「実ファイルダウンロード」を別工程として記録する。
- FabのLicense種別と提供tierを混同しない。
- Fab取得画面の確認と取得ボタン操作は豆虎Gateが行う。Codexは代行しない。
- 取得経路を`Fab／ライブラリ型`または`ambientCG等の直接ダウンロード型`から選ぶ。直接ダウンロード型の無料ライブラリ取得は`N/A`とする。
- F5は正式合格・基準点固定済みであり、本チェックリストを使って再取得、再import、再検証しない。
- 物理構造の用語は`Docs/StageV0_Visual_Asset_Requirements.md`の15.3を正本とし、「門」「城門」「屋根」「壁」だけでなく対象建築または対象部位を記録する。

## 2. 候補分類と現在状態

| 状態 | 候補 | 目的 | 現在の扱い |
|---:|---|---|---|
| 完了 | F5 Paving Stones 149 | 街路Material | 正式合格・基準点固定済み。再取得・再import・再検証禁止 |
| 停止 | F10 Roofing Tiles 011 A | 屋根Material | 対象建築、屋根Mesh、勾配、UV、Material Slot確定待ち |
| 停止 | F11 Plaster 002 | 壁Material | 対象建築、壁Mesh、壁構成、UV、Material Slot確定待ち |
| Inactive | F4 Modular Fantasy Castle | 調査履歴のみ | 外観Pre-Gate不合格。Active候補除外、取得・import・使用禁止 |
| 停止 | F6 Medieval Market / Tent Prop_01 | 市場・生活小物 | 実装順再裁定待ち |
| 停止 | F2 Medieval Village MegaKit | module・尺度・配置作業性比較 | 実装順再裁定待ち。正式外観を前提にしない |
| 停止 | F8 Free Shrubs Pack | 植生・Nanite・性能 | 実装順再裁定待ち |
| 停止 | F9 Free Nanite Tree | 植生・Nanite・性能 | 実装順再裁定待ち |
| 保留 | F1 ElderBoom Hollow Massive Medieval Village Environment | 建築統合候補 | 8条件と個別承認が必要 |
| 対象外 | F3 Medieval Village Megascans Sample | 統合sample | 初回対象外。不採用ではない |
| 対象外 | F7 Low Poly Market Pack | 市場scene | 初回対象外。不採用ではない |

この表は現在状態を示し、候補検証順または実装順を示さない。次工程は`Stage V-2 王都主要街路Geometry PoC`であり、対象導線と対象構造物の完成順を先に裁定する。

## 3. 記録対象

- 候補ID：
- Asset名：
- 検証Phase：
- 対象カテゴリ：
- 記録開始日時（JST）：
- 記録担当：
- Stage V-0 HEAD：`3ff9394d2fe8e57c2fb425ae84c52fdd90b311c6`
- 現在の正式実装基準点：`f08a16c489bdfdaed2e118f99c989ae88728c026`
- 現在の正式tag：`stage-v1-f5-paving-stones149-v0.1.0`
- 作業開始HEAD：
- 作業branch：
- 開始時git status：

## 4. Gate A：取得または直接ダウンロード前

- 取得経路：`Fab／ライブラリ型 / ambientCG等の直接ダウンロード型`

### 4.1 Asset識別

| 確認項目 | 記録値 | 状態 | 証拠path／URL |
|---|---|---|---|
| 候補ID |  |  |  |
| 正式Asset名 |  |  |  |
| Asset ID／listing ID |  |  |  |
| 配布元 |  |  |  |
| 制作者 |  |  |  |
| 一次URL |  |  |  |
| 価格 |  |  |  |
| 無料表示の確認日時（JST） |  |  |  |
| Stage V-0候補との同一性 |  |  |  |
| 外観Pre-Gate GO |  |  |  |

### 4.2 License種別

| 確認項目 | 記録値 | 状態 | 証拠path／URL |
|---|---|---|---|
| License種別 |  |  |  |
| License原文URL |  |  |  |
| 商用ゲーム利用 |  |  |  |
| 改変 |  |  |  |
| standalone再配布 |  |  |  |
| SourceArtローカル保全 |  |  |  |
| private repository共有 |  |  |  |
| 共同作業者共有 |  |  |  |
| Credit要否 |  |  |  |
| License証拠の保存可否 |  |  |  |

### 4.3 Fab提供tier（Fab候補のみ）

| 確認項目 | 記録値 | 状態 | 証拠path |
|---|---|---|---|
| 取得画面に表示された提供tier |  |  |  |
| Personal表示 |  |  |  |
| Professional表示 |  |  |  |
| Personal／Professionalの権利範囲同一 |  |  |  |
| Reference-Only表示 |  |  |  |
| 選択するtier |  |  |  |
| Reference-Onlyではないこと |  |  |  |
| source形式が提供されること |  |  |  |

`Reference-Only`はsource形式なしとして記録する。License種別がFab Standardか否かと、提供tierがPersonal／Professional／Reference-Onlyのどれかは別項目である。

## 5. Gate A判定

- [ ] Asset同一性と無料表示を確認した
- [ ] Gate Aの`UNKNOWN`：0
- [ ] Gate Aの`FAIL`：0
- [ ] License種別と提供tierを別々に確認した
- [ ] 商用利用、改変、保全、共有条件を確認した
- [ ] Reference-Onlyではない、またはReference-Onlyを理由に停止した
- [ ] source形式が提供され、Reference-Onlyではないことを確認した
- [ ] Fab型では豆虎Gateが無料ライブラリ取得を明示承認した
- [ ] 直接ダウンロード型では豆虎GateがAssetページ確認と直接ダウンロードを明示承認した

Gate判定：`GO / STOP`

判定日時（JST）：

豆虎Gate記録：

## 6. 無料ライブラリ取得

### 6.1 Fab／ライブラリ型

- [ ] Gate A PASS後である
- [ ] 取得画面を豆虎Gateが確認した
- [ ] License種別を豆虎Gateが確認した
- [ ] 提供tierを豆虎Gateが確認した
- [ ] 無料であることを豆虎Gateが確認した
- [ ] 無料ライブラリ取得ボタンを豆虎Gateが操作した
- [ ] この時点で実ファイルをダウンロードしていない
- [ ] Codexが契約・License選択・取得操作を代行していない

ライブラリ取得日時（JST）：

ライブラリ表示名：

### 6.2 ambientCG等の直接ダウンロード型

- 無料ライブラリ取得：`N/A`
- [ ] 公式AssetページでGate B相当項目を確認する
- [ ] 豆虎Gateが直接ダウンロードを承認・操作する
- [ ] Codexが取得操作を代行しない

## 7. Gate B：取得経路別のダウンロード／import前確認

### 7.1 Fab／ライブラリ型

| 確認項目 | 記録値 | 状態 | 証拠path／URL |
|---|---|---|---|
| ライブラリまたはLauncherに表示されたDownload size |  |  |  |
| 対応UE version |  |  |  |
| 配布形式 |  |  |  |
| 依存Plugin |  |  |  |
| 依存Asset |  |  |  |
| Download sizeと展開余裕を含む必要空き容量 |  |  |  |
| 外部保管先空き容量 |  |  |  |

- [ ] Gate Bの`UNKNOWN`：0
- [ ] Gate Bの`FAIL`：0
- [ ] 豆虎Gateが実ファイルダウンロードを明示承認した

Gate B判定：`GO / STOP`

判定日時（JST）：

### 7.2 ambientCG等の直接ダウンロード型：Assetページ上のGate B相当

| 確認項目 | 記録値 | 状態 | 証拠path／URL |
|---|---|---|---|
| Assetページ表示容量 |  |  |  |
| 取得形式 |  |  |  |
| 解像度 |  |  |  |
| 配布file構成 |  |  |  |
| 依存Plugin／依存Asset |  |  |  |
| 必要空き容量 |  |  |  |
| 豆虎Gate直接ダウンロード承認 |  |  |  |

直接ダウンロード型のGate B相当判定：`GO / STOP`

## 8. 実ファイルダウンロード

- [ ] Fab型はGate B PASS後、直接ダウンロード型はAssetページ上のGate B相当GO後である
- [ ] 豆虎Gateが承認した候補・version・形式だけをダウンロードした
- [ ] 無料ライブラリ取得日時とダウンロード日時を分けて記録した
- [ ] importはまだ実施していない

ダウンロード日時（JST）：

ダウンロード操作担当：

ダウンロード先：

## 9. Gate C：ダウンロード後、import前

### 9.1 原本保全・実容量

推奨root：`C:\Users\rinpa\Desktop\TITLE_ExternalAssets\StageV1\<CandidateID>_<ShortName>\`

| 確認項目 | 記録値 | 状態 |
|---|---|---|
| Original path |  |  |
| Original file名 |  |  |
| Original size |  |  |
| Original SHA-256 |  |  |
| Copy後SHA-256一致 |  |  |
| Expanded path |  |  |
| Expanded file数 |  |  |
| Expanded総容量 |  |  |
| Installed size実測 |  |  |
| 最大単体file |  |  |
| 50MB超file数 |  |  |
| 100MB超file数 |  |  |
| License証拠path／SHA-256 |  |  |
| Listing証拠path／SHA-256 |  |  |
| Manifest path |  |  |

### 9.2 実file inventory

| 確認項目 | 記録値 | 状態 |
|---|---|---|
| 配布形式の一致 |  |  |
| Source形式version |  |  |
| Mesh数 |  |  |
| Material数 |  |  |
| Texture数 |  |  |
| 最大Texture解像度 |  |  |
| LOD段数 |  |  |
| Nanite設定 |  |  |
| Collision方式 |  |  |
| 依存Plugin |  |  |
| 依存Asset |  |  |
| demo Map依存 |  |  |
| 必要subsetの分離可否 |  |  |
| missing file |  |  |

### 9.3 容量判定

- [ ] 50MB以上100MB未満のfile件数、容量、pathを記録した
- [ ] 50MB以上100MB未満はlocal検証のみとし、commit／push前報告対象にした
- [ ] 100MB以上の単体file：0
- [ ] 100MB以上を検出した場合、importせず停止した

Gate C判定：`GO / STOP`

最大単体fileとInstalled size実測はGate A項目ではなく、このGate Cで確定する。

## 10. 候補別の追加Gate

### F5 Paving Stones 149

- 状態：`完了／正式基準点固定済み／再検証禁止`
- commit：`f08a16c489bdfdaed2e118f99c989ae88728c026`
- tag：`stage-v1-f5-paving-stones149-v0.1.0`
- 街路表面Materialだけが合格範囲であり、geometry、縁石、排水溝、段差、建物際処理、曲がり、交差・分岐、Height、Variation、正式王都Map採用は合格範囲外である。

### F10 Roofing Tiles 011 A

- 状態：`対象建築、屋根Mesh、勾配、UV、Material Slot確定まで停止`
- [ ] 取得解像度を記録した
- [ ] 裁定済みの2K Textureを選択した
- [ ] 8K 196MBの既知riskを確認した
- [ ] 勾配屋根用の基準Meshで検証する

### F11 Plaster 002

- 状態：`対象建築、壁Mesh、壁構成、UV、Material Slot確定まで停止`
- [ ] 取得解像度を記録した
- [ ] 裁定済みの2K Textureを選択した
- [ ] 4K 95MB、8K 423MBの既知riskを確認した
- [ ] 汚れ・雨染み・経年decalは別途不足と理解した

### F4 Modular Fantasy Castle

**調査履歴／Inactive／実行禁止。** 外観Pre-Gate不合格によりActive候補から除外済みであり、取得、import、使用へ進まない。以下の既存項目は2026-07-17時点の調査履歴としてのみ保持する。

- [ ] FabのLicense種別と提供tierを取得画面で確認した
- [ ] Gate BでUE version、Download size、配布形式、Plugin、依存Asset、必要空き容量を確認した
- [ ] Gate CでInstalled size実測、最大単体file、LOD、Collision、Material、Texture、demo依存を確認した
- [ ] 通過可能な門洞、塔、城壁、屋根の収録可否を確認した
- [ ] 巨大平面・直方体反復だけでは不合格になることを確認した

### F6 Medieval Market / Tent Prop_01

- 状態：`実装順再裁定まで停止`

- [ ] FabのLicense種別と提供tierを取得画面で確認した
- [ ] Gate BでUE version、Download size、FBX配布、Plugin、依存Asset、必要空き容量を確認した
- [ ] Gate CでFBX version、Installed size実測、最大単体file、LOD、Collision、Material、Texture、demo依存を確認した
- [ ] 荷車、樽、麻袋、看板、照明、旗、ropeの不足を別記した

### F2 Medieval Village MegaKit

- 状態：`実装順再裁定まで停止`

- [ ] CC0原文を確認した
- [ ] 正式外観候補ではなくmodule比較対象と明記した
- [ ] Unreal source versionの取込経路を確認した
- [ ] Stylized low-poly傾向を外観riskとして維持した

### F8 Free Shrubs Pack

- 状態：`実装順再裁定まで停止`

- [ ] 新しい実装順と対象導線が確定するまで無料ライブラリ取得を含め保留した
- [ ] 約1.3GBの取得・保管を豆虎Gateが個別承認した
- [ ] Nanite、wind、UE version、License、tierを確認した
- [ ] 王城前広場の構造と現実的最大配置数を先に定義した
- [ ] 群衆・Dynamic Lightとの同時負荷条件を固定した

### F9 Free Nanite Tree

- 状態：`実装順再裁定まで停止`

- [ ] F8と混在させず単独検証する
- [ ] License種別、提供tier、容量、UE versionを確認した
- [ ] full-geometry Naniteの性能条件を固定した
- [ ] 王都景観に合う樹種・樹形かを豆虎Gateが判定する

### F1 特別保留解除

- [ ] License種別確認
- [ ] 提供tier確認
- [ ] Gate BでDownload size、UE version、配布形式、依存関係、必要空き容量を確認
- [ ] Gate CでInstalled size実測、最大単体file、50MB／100MB境界、file inventoryを確認
- [ ] 約12GB以上を見込む外部保管容量確認
- [ ] 必要subset抽出可否確認
- [ ] placeholder記載の解消
- [ ] 既存無料候補の建築品質不足を記録
- [ ] 豆虎GateのF1個別承認

全項目PASSでなければF1を取得しない。

## 11. 性能測定前Adapter Gate

このGateはimport後、配置数1の性能測定を開始する前に実施する。

| 確認項目 | 記録値 | 状態 | evidence path |
|---|---|---|---|
| UE5.8実描画RHI Adapter名 |  |  |  |
| Dedicated VRAM |  |  |  |
| UE5.8 log／RHI表示による証拠 |  |  |  |
| `NVIDIA GeForce RTX 3060`一致 |  |  |  |
| 12GB VRAM一致 |  |  |  |
| GTX 1660 SUPERではない |  |  |  |

- [ ] UE5.8実描画AdapterがRTX 3060である
- [ ] Dedicated VRAMが12GBである
- [ ] Adapter名とVRAMの証拠を保存した
- [ ] GTX 1660 SUPERまたは別Adapterなら性能測定を開始せず停止した

Adapter Gate判定：`GO / STOP`

## 12. 最終確認

- [ ] 一候補だけを対象としている
- [ ] 外観Pre-GateがGOである
- [ ] Fab型または直接ダウンロード型の取得経路を固定した
- [ ] 前候補の保全／撤去と豆虎Gate裁定が完了している
- [ ] 候補別local branch `stage-v1/validate-<candidate-id>`を使用している
- [ ] `Content/StageV1/External/<Candidate>/`以外へimportしない設計である
- [ ] 既存StageのContent、Map、Blueprint、C++、Configを変更しない
- [ ] SourceArtをGitへ追加しない
- [ ] Git LFSを導入しない
- [ ] 50MB以上100MB未満のfileをcommit／push前報告対象にした
- [ ] 100MB以上のfileは0。検出時は停止した
- [ ] 取得成功を外観承認として扱わない

最終判定：`GO / STOP`

未解消事項：

停止理由：
