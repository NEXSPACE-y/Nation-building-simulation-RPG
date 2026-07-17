# Stage V-1 Asset取得前チェックリスト

## 1. 使用方法

- 一候補につき本チェックリストを一部複製して記録する。
- `PASS`、`FAIL`、`UNKNOWN`、`N/A`のいずれかを記入する。
- Gate Aに`UNKNOWN`または`FAIL`が一つでもある場合、無料ライブラリへ追加しない。
- Gate Bに`UNKNOWN`または`FAIL`が一つでもある場合、実ファイルをダウンロードしない。
- Gate Cに`UNKNOWN`または`FAIL`が一つでもある場合、importしない。
- 「無料ライブラリ取得」と「実ファイルダウンロード」を別工程として記録する。
- FabのLicense種別と提供tierを混同しない。
- Fab取得画面の確認と取得ボタン操作は豆虎Gateが行う。Codexは代行しない。

## 2. 候補分類と順序

| 順序 | 候補 | 目的 | 状態 |
|---:|---|---|---|
| 1 | F5 Paving Stones 149 | 街路Material | 初回検証 |
| 2 | F10 Roofing Tiles 011 A | 屋根Material | 初回検証 |
| 3 | F11 Plaster 002 | 壁Material | 初回検証 |
| 4 | F4 Modular Fantasy Castle | 王城・城壁module | Fab取得前Gate必須 |
| 5 | F6 Medieval Market / Tent Prop_01 | 市場・生活小物 | Fab取得前Gate必須 |
| 6 | F2 Medieval Village MegaKit | module・尺度・配置作業性比較 | 正式外観を前提にしない |
| 7 | F8 Free Shrubs Pack | 植生・Nanite・性能 | Phase 5まで無料ライブラリ取得を含め保留 |
| 8 | F9 Free Nanite Tree | 植生・Nanite・性能 | F8と分離して検証 |
| 保留 | F1 ElderBoom Hollow Massive Medieval Village Environment | 建築統合候補 | 8条件と個別承認が必要 |
| 対象外 | F3 Medieval Village Megascans Sample | 統合sample | 初回対象外。不採用ではない |
| 対象外 | F7 Low Poly Market Pack | 市場scene | 初回対象外。不採用ではない |

## 3. 記録対象

- 候補ID：
- Asset名：
- 検証Phase：
- 対象カテゴリ：
- 記録開始日時（JST）：
- 記録担当：
- Stage V-0 HEAD：`3ff9394d2fe8e57c2fb425ae84c52fdd90b311c6`
- 作業開始HEAD：
- 作業branch：
- 開始時git status：

## 4. Gate A：無料ライブラリ取得前

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
- [ ] 豆虎Gateが無料ライブラリ取得を明示承認した

Gate判定：`GO / STOP`

判定日時（JST）：

豆虎Gate記録：

## 6. 無料ライブラリ取得

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

## 7. Gate B：ライブラリ取得後、ダウンロード／import前

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

## 8. 実ファイルダウンロード

- [ ] Gate B PASS後である
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

- [ ] 取得解像度を記録した
- [ ] 裁定済みの2K Textureを選択した
- [ ] 4K 59MB、8K 215MBの既知riskを確認した
- [ ] geometry、縁石、排水溝、段差は含まれないと理解した

### F10 Roofing Tiles 011 A

- [ ] 取得解像度を記録した
- [ ] 裁定済みの2K Textureを選択した
- [ ] 8K 196MBの既知riskを確認した
- [ ] 勾配屋根用の基準Meshで検証する

### F11 Plaster 002

- [ ] 取得解像度を記録した
- [ ] 裁定済みの2K Textureを選択した
- [ ] 4K 95MB、8K 423MBの既知riskを確認した
- [ ] 汚れ・雨染み・経年decalは別途不足と理解した

### F4 Modular Fantasy Castle

- [ ] FabのLicense種別と提供tierを取得画面で確認した
- [ ] Gate BでUE version、Download size、配布形式、Plugin、依存Asset、必要空き容量を確認した
- [ ] Gate CでInstalled size実測、最大単体file、LOD、Collision、Material、Texture、demo依存を確認した
- [ ] 通過可能な門洞、塔、城壁、屋根の収録可否を確認した
- [ ] 巨大平面・直方体反復だけでは不合格になることを確認した

### F6 Medieval Market / Tent Prop_01

- [ ] FabのLicense種別と提供tierを取得画面で確認した
- [ ] Gate BでUE version、Download size、FBX配布、Plugin、依存Asset、必要空き容量を確認した
- [ ] Gate CでFBX version、Installed size実測、最大単体file、LOD、Collision、Material、Texture、demo依存を確認した
- [ ] 荷車、樽、麻袋、看板、照明、旗、ropeの不足を別記した

### F2 Medieval Village MegaKit

- [ ] CC0原文を確認した
- [ ] 正式外観候補ではなくmodule比較対象と明記した
- [ ] Unreal source versionの取込経路を確認した
- [ ] Stylized low-poly傾向を外観riskとして維持した

### F8 Free Shrubs Pack

- [ ] Phase 5開始まで無料ライブラリ取得を含め保留した
- [ ] 約1.3GBの取得・保管を豆虎Gateが個別承認した
- [ ] Nanite、wind、UE version、License、tierを確認した
- [ ] 王城前広場の構造と現実的最大配置数を先に定義した
- [ ] 群衆・Dynamic Lightとの同時負荷条件を固定した

### F9 Free Nanite Tree

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
