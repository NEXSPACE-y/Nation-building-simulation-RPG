# Stage V-1 Asset実機検証記録テンプレート

## 1. 記録規則

- 一候補につき一記録を作成する。
- 未確認値は`UNKNOWN`とし、推測で補完しない。
- `PASS`は証拠pathまたは実測値がある場合だけ使用する。
- import成功、Automation PASS、Package PASSは外観承認を代替しない。
- 外観の最終裁定は豆虎Gateが行う。
- 現行G-1B～G-3B-Rは比較対象であり、正式ビジュアルではない。
- F5は2K Textureで正式合格・基準点固定済みである。F10／F11を将来再開する場合も2K方針を維持する。
- F10／F11は対象Mesh確定まで、F6／F2／F8／F9は実装順再裁定まで停止する。F4はActive候補除外済みであり本テンプレートの対象にしない。
- F5は正式合格・基準点固定済みである。本テンプレートをF5完了記録の再作成、再取得、再import、再検証に使用しない。
- 候補検証順と実装順を分離し、対象導線、対象建築、対象部位、対象Meshを特定してから記録を開始する。
- 現在の次工程は`Stage V-2 王都主要街路Geometry PoC`である。後続Assetの検索・取得は、対象構造物と実装順の別裁定後に行う。
- 物理構造の用語は`Docs/StageV0_Visual_Asset_Requirements.md`の15.3を正本とする。

## 2. Asset情報

| 項目 | 記録値 |
|---|---|
| 候補ID |  |
| Asset名 |  |
| Asset ID／listing ID |  |
| カテゴリ |  |
| 配布元 |  |
| 制作者 |  |
| 一次URL |  |
| 価格 |  |
| 無料ライブラリ取得日時（JST） |  |
| 無料ライブラリ取得担当 |  |
| 実ファイルダウンロード日時（JST） |  |
| ダウンロード担当 |  |
| 検証開始日時（JST） |  |
| 検証終了日時（JST） |  |
| 検証担当 |  |
| 検証Phase |  |
| 取得経路 | `Fab／ライブラリ型 / ambientCG等の直接ダウンロード型` |
| 対象導線 |  |
| 対象建築 |  |
| 対象部位 |  |
| 対象Mesh |  |
| Mesh path |  |
| 屋根勾配または面角度 |  |
| UV方式 |  |
| UV密度 |  |
| Material Slot名 |  |
| 使用目的 |  |
| 完成させる構造 |  |
| 今回の検証範囲外 |  |

対象導線、対象建築、対象部位は次の具体名から選び、該当しない場合だけ`なし`とする。

- 王都南門
- 王都城壁
- 主要街路
- 市場入口
- 市場付近
- 王城前広場
- 王城正門
- 王城正門門楼
- 王城正門左右塔
- 王城外郭城壁
- 王城中央棟
- なし

「門」「城門」「屋根」「壁」だけの記録は禁止し、対象建築または対象部位を付記する。

## 3. Git・基準点

| 項目 | 記録値 | 状態 |
|---|---|---|
| 上位要件基準点 Stage V-0 commit | `3ff9394d2fe8e57c2fb425ae84c52fdd90b311c6` |  |
| 上位要件基準点 Stage V-0 tag | `stage-v0-visual-direction-v0.1.0` |  |
| 現在の正式実装基準点 Stage V-1／F5 commit | `f08a16c489bdfdaed2e118f99c989ae88728c026` |  |
| 現在の正式実装基準点 Stage V-1／F5 tag | `stage-v1-f5-paving-stones149-v0.1.0` |  |
| 作業開始HEAD |  |  |
| origin/main |  |  |
| 候補別local branch | `stage-v1/validate-<candidate-id>` |  |
| 開始時git status |  |  |
| 前候補正式commit |  |  |
| 前候補tag |  |  |
| 前候補の未裁定差分 |  |  |
| 候補外差分 |  |  |

## 3A. 外観Pre-Gate

Gate A／B／CはAsset検査工程であり、物理的な門ではない。公式掲載情報だけで正式ビジュアル要件との明白な衝突を先に判定する。

| 項目 | 記録値 | 状態 | 証拠path／URL |
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

外観Pre-Gate：`GO / STOP`

裁定日時（JST）：

`STOP`の場合はLicense確認、無料ライブラリ取得、直接ダウンロード、Gate A以降を実施しない。

## 4. Gate A：取得または直接ダウンロード前／License・提供tier

| 項目 | 記録値 | 状態 | 証拠path／URL |
|---|---|---|---|
| License種別 |  |  |  |
| License原文 |  |  |  |
| Fab提供tier |  |  |  |
| Personal／Professionalの権利範囲 |  |  |  |
| Reference-Only該当 |  |  |  |
| source形式提供 |  |  |  |
| 商用ゲーム利用 |  |  |  |
| 改変 |  |  |  |
| standalone再配布 |  |  |  |
| SourceArtローカル保全 |  |  |  |
| private repository共有 |  |  |  |
| 共同作業者共有 |  |  |  |
| Credit |  |  |  |

License種別と提供tierは別項目として記録する。

### Gate A判定

| 確認項目 | 結果 | 状態 | evidence |
|---|---|---|---|
| Asset同一性 |  |  |  |
| 無料表示 |  |  |  |
| License種別 |  |  |  |
| 提供tier |  |  |  |
| 商用利用 |  |  |  |
| 改変・保全・共有条件 |  |  |  |
| Source形式提供 |  |  |  |
| 豆虎Gate承認 |  |  |  |

Gate A：`GO / STOP`

## 5. 無料ライブラリ取得

Fab／ライブラリ型だけが対象である。ambientCG等の直接ダウンロード型は全項目を`N/A`とし、Assetページ上のGate B相当確認へ進む。

| 項目 | 記録値 | 状態 |
|---|---|---|
| Gate A PASS後 |  |  |
| ライブラリ取得日時（JST） |  |  |
| ライブラリ取得担当 |  |  |
| ライブラリ表示名 |  |  |
| 実ファイル未ダウンロード |  |  |

## 6. Gate B：取得経路別のダウンロード／import前確認

Fab／ライブラリ型はライブラリまたはLauncher、直接ダウンロード型は公式Assetページを証拠とする。

| 項目 | 記録値 | 状態 | evidence |
|---|---|---|---|
| Download size表示 |  |  |  |
| 対応UE version |  |  |  |
| 配布形式 |  |  |  |
| 依存Plugin |  |  |  |
| 依存Asset |  |  |  |
| 必要空き容量 |  |  |  |
| 豆虎Gateダウンロード承認 |  |  |  |
| 直接ダウンロード型のAssetページ表示容量 |  |  |  |
| 直接ダウンロード型の解像度／取得形式 |  |  |  |

Gate B：`GO / STOP`

## 7. 実ファイルダウンロード

| 項目 | 記録値 | 状態 |
|---|---|---|
| Gate B PASS後 |  |  |
| ダウンロード日時（JST） |  |  |
| ダウンロード担当 |  |  |
| ダウンロード先 |  |  |
| import未実施 |  |  |

直接ダウンロード型では「Gate B PASS後」を「公式Assetページ上のGate B相当GO後」と読み替え、無料ライブラリ取得は`N/A`のまま維持する。

## 8. Gate C：ダウンロード後、import前／SourceArt原本保全

| 項目 | 記録値 | 状態 |
|---|---|---|
| 外部保管root |  |  |
| Original path |  |  |
| Original file名 |  |  |
| Original size |  |  |
| Original SHA-256 |  |  |
| 保全copy SHA-256 |  |  |
| SHA一致 |  |  |
| Expanded path |  |  |
| Expanded file数 |  |  |
| Expanded総容量 |  |  |
| Installed size実測 |  |  |
| 最大単体file |  |  |
| 50MB超file数 |  |  |
| 100MB超file数 |  |  |
| Source形式 |  |  |
| Source形式version |  |  |
| License証拠path／SHA-256 |  |  |
| Listing証拠path／SHA-256 |  |  |
| Source manifest path |  |  |
| SourceArtのGit追加 | `なし` |  |

### Gate C判定

| 確認項目 | 結果 | 状態 |
|---|---|---|
| 実容量 |  |  |
| Installed size実測 |  |  |
| 最大単体file |  |  |
| 50MB以上100MB未満のfile一覧 |  |  |
| 100MB以上のfile一覧 |  |  |
| file inventory |  |  |
| LOD／Collision |  |  |
| Material／Texture |  |  |
| demo Map依存 |  |  |

- 50MB以上100MB未満：local検証可、commit／push前報告
- 100MB以上：importせず停止

Gate C：`GO / STOP`

## 9. 配布物inventory

| 種別 | 件数 | 主な名前／path | 最大容量 | 備考 |
|---|---:|---|---:|---|
| Static Mesh |  |  |  |  |
| Skeletal Mesh |  |  |  |  |
| Material |  |  |  |  |
| Material Instance |  |  |  |  |
| Texture |  |  |  |  |
| Blueprint |  |  |  |  |
| Map |  |  |  |  |
| Plugin |  |  |  |  |
| その他 |  |  |  |  |

### Texture内訳

| 用途 | 件数 | 解像度 | 色空間／compression | 備考 |
|---|---:|---|---|---|
| Base Color |  |  |  |  |
| Normal |  |  |  |  |
| Roughness |  |  |  |  |
| Metallic |  |  |  |  |
| AO／ORM |  |  |  |  |
| Opacity／Mask |  |  |  |  |
| その他 |  |  |  |  |

## 10. Unreal import

| 項目 | 記録値 | 状態 |
|---|---|---|
| Unreal Engine | 5.8 |  |
| import日時（JST） |  |  |
| import担当 |  |  |
| import元 |  |  |
| import先 | `Content/StageV1/External/<Candidate>/` |  |
| Vendor原Folder |  |  |
| 候補専用Folder |  |  |
| Import Uniform Scale |  |  |
| Component Scale |  |  |
| forward axis |  |  |
| up axis |  |  |
| normal／tangent方式 |  |  |
| Material import |  |  |
| Texture import |  |  |
| Collision import |  |  |
| LOD import |  |  |
| Nanite |  |  |
| 依存Plugin |  |  |
| 依存Asset |  |  |
| demo Map依存 |  |  |
| import warning件数 |  |  |
| import error件数 |  |  |
| log evidence path |  |  |

### Vendor名対応表

| Vendor原名 | Unreal Asset名 | Unreal path | 用途 |
|---|---|---|---|
|  |  |  |  |

## 11. Import後技術inventory

| 項目 | 記録値 | 状態 |
|---|---|---|
| Asset Registry読込 |  |  |
| Static Mesh数 |  |  |
| vertex総数／代表値 |  |  |
| triangle総数／代表値 |  |  |
| LOD段数 |  |  |
| Nanite有効Asset数 |  |  |
| Collision方式 |  |  |
| CollisionなしAsset数 |  |  |
| Material数 |  |  |
| Texture数 |  |  |
| 最大Texture解像度 |  |  |
| shader compile |  |  |
| Material compile error |  |  |
| missing reference |  |  |
| Redirector |  |  |
| Texture streaming warning |  |  |
| 候補Folder容量 |  |  |
| 最大`.uasset`／`.umap` |  |  |
| 50MB超file数 |  |  |
| 100MB超file数 |  |  |

## 12. 検証Map

| 項目 | 記録値 | 状態 |
|---|---|---|
| Map | `Content/StageV1/Maps/StageV1_AssetValidation_PoC` |  |
| PLAYER_M | 175uu、Component Scale 1.0 |  |
| 100uu grid |  |  |
| 200uu扉frame |  |  |
| 17／20uu階段 |  |  |
| 90／110uu手すり |  |  |
| G-3B-R比較表示 |  |  |
| G-3B-Rを正式品質と誤表示していない |  |  |
| 候補外Asset混入 |  |  |

## 13. 尺度検証

初回はImport Uniform Scale 1.0、Component Scale 1.0で測定する。補正が必要な場合は一箇所へ集約し、補正前後を記録する。

| 対象 | 実測uu | 基準 | 比率／差 | 判定 | evidence |
|---|---:|---:|---:|---|---|
| PLAYER_M | 175 | 175 |  |  |  |
| 扉開口高 |  | 200 |  |  |  |
| 扉開口幅 |  |  |  |  |  |
| 階段蹴上げ |  | 17～20 |  |  |  |
| 階段踏面 |  |  |  |  |  |
| 手すり高 |  | 90～110 |  |  |  |
| 窓下端 |  |  |  |  |  |
| 市場台／counter |  |  |  |  |  |
| bench座面 |  |  |  |  |  |
| 城門開口 |  |  |  |  |  |
| その他 |  |  |  |  |  |

### Scale補正

- 補正要否：
- 原因：
- 補正箇所：
- 補正倍率：
- Import Uniform Scale最終値：
- Component Scale最終値：
- 二重補正なし：
- Camera／Actor Scaleによる見かけ補正なし：

## 14. Collision・NavMesh

| 確認項目 | 結果 | 状態 | evidence |
|---|---|---|---|
| 単純Collision |  |  |  |
| 複雑Collision |  |  |  |
| Per-poly collision使用 |  |  |  |
| Camera Collision |  |  |  |
| PLAYER通過 |  |  |  |
| 扉／城門通過 |  |  |  |
| 階段昇降 |  |  |  |
| NavMesh生成 |  |  |  |
| NavPath維持 |  |  |  |
| 左クリック移動 |  |  |  |
| 左クリック保持移動 |  |  |  |
| map外落下防止 |  |  |  |
| 見た目地面とCollisionの差 |  |  |  |

## 15. Camera検証

| 確認項目 | 結果 | 状態 | evidence |
|---|---|---|---|
| Standard近距離 |  |  |  |
| Standard通常距離 |  |  |  |
| Standard最大Zoom Out |  |  |  |
| Standard右ドラッグ |  |  |  |
| Standard自動Yaw追従 |  |  |  |
| Standard到着後Yaw維持 |  |  |  |
| Tactical切替 |  |  |  |
| Tactical Player追従 |  |  |  |
| Tactical自動Yaw非追従 |  |  |  |
| Tactical配置可読性 |  |  |  |
| Camera Collision |  |  |  |
| FOV／Pitch／Zoom仕様変更なし |  |  |  |

## 16. Material・外観技術確認

| 確認項目 | 結果 | 状態 | evidence |
|---|---|---|---|
| Base Color |  |  |  |
| Normal方向 |  |  |  |
| Roughness |  |  |  |
| Metallic |  |  |  |
| AO／ORM |  |  |  |
| sRGB設定 |  |  |  |
| UV密度 |  |  |  |
| tiling |  |  |  |
| seam |  |  |  |
| 反復感 |  |  |  |
| Material Instance調整可否 |  |  |  |
| 元Texture加工なし |  |  |  |
| F5完了値およびF10／F11再開時の2K固定 |  |  |  |
| 黒潰れ／白飛び |  |  |  |

## 17. 性能測定前Adapter Gate

性能測定前に、UE5.8が実際に描画へ使用しているRHI Adapter名とDedicated VRAMを証拠化する。OS上に存在するGPU名だけではPASSにしない。

| 確認項目 | 記録値 | 状態 | evidence path |
|---|---|---|---|
| UE5.8実描画RHI Adapter名 |  |  |  |
| Dedicated VRAM |  |  |  |
| UE5.8 log／RHI表示 |  |  |  |
| `NVIDIA GeForce RTX 3060`一致 |  |  |  |
| 12GB一致 |  |  |  |
| GTX 1660 SUPERではない |  |  |  |

Adapter Gate：`GO / STOP`

RTX 3060 12GBと確認できた場合だけ次節へ進む。GTX 1660 SUPER、別Adapter、Adapter名またはVRAMを証拠化できない場合は、配置数1を含む性能測定を開始せず停止する。

## 18. 性能測定条件

| 条件 | 記録値 |
|---|---|
| 解像度 | 1920×1080 |
| UE5.8実描画Adapter | Adapter Gateで確認済みのRTX 3060 |
| Dedicated VRAM | Adapter Gateで確認済みの12GB |
| GPU／CUDA構成 |  |
| driver |  |
| CPU |  |
| RAM |  |
| Scalability |  |
| VSync |  |
| frame cap |  |
| Lighting preset |  |
| Camera | Standard／Tactical |
| warm-up完了条件 |  |
| 1計測時間 | 60秒以上 |
| 背景process |  |

Adapter GateがGOでない場合は測定しない。比較途中でGPU設定、電力制限、driver、解像度、Scalabilityを変更した場合はFAILではなく測定停止とする。

## 19. 性能結果

### StandardCharacterCamera

| 配置数 | 計測秒 | FPS平均 | FPS最小 | frame time平均／最大 | GPU使用率 | VRAM | CPU | warning | 判定 |
|---:|---:|---:|---:|---|---:|---:|---:|---|---|
| 1 |  |  |  |  |  |  |  |  |  |
| 10 |  |  |  |  |  |  |  |  |  |
| 50 |  |  |  |  |  |  |  |  |  |
| 100／承認済み上限 |  |  |  |  |  |  |  |  |  |

### TacticalOverlookCamera

| 配置数 | 計測秒 | FPS平均 | FPS最小 | frame time平均／最大 | GPU使用率 | VRAM | CPU | warning | 判定 |
|---:|---:|---:|---:|---|---:|---:|---:|---|---|
| 1 |  |  |  |  |  |  |  |  |  |
| 10 |  |  |  |  |  |  |  |  |  |
| 50 |  |  |  |  |  |  |  |  |  |
| 100／承認済み上限 |  |  |  |  |  |  |  |  |  |

- 現実的上限を100未満にした場合の数：
- 変更理由：
- 事前豆虎Gate承認：
- 測定後に結果を隠す目的で上限変更していない：

## 20. 固定スクリーンショット

全Shotで同一解像度、受入済みCamera設定、175uu PLAYER_M、固定lighting presetを使用する。

| Shot | 必須内容 | path | 状態 |
|---|---|---|---|
| 1 | PLAYER_M横・近距離 | `shot_01_player_close` |  |
| 2 | Standard通常距離 | `shot_02_standard_default` |  |
| 3 | Standard最大Zoom Out | `shot_03_standard_max_zoom_out` |  |
| 4 | TacticalOverlook | `shot_04_tactical_overlook` |  |
| 5 | 既存G-3B-Rとの並置 | `shot_05_g3br_comparison` |  |
| 6 | 共通夕方照明 | `shot_06_evening_reference` |  |

- 昼間Directional Light：
- 昼間Exposure：
- 夕方Directional Light：
- 夕方Exposure：
- Shot間のAsset形状変更：なし／あり
- Shot間のTexture加工：なし／あり
- Camera歪曲／過剰DOF：なし／あり

## 21. 外観判定

| 判定項目 | 豆虎Gate判定 | コメント |
|---|---|---|
| 現実寄り中世として見える |  |  |
| LEGO／Minecraft／Primitive／豆腐建築に見えない |  |  |
| 175uu人物と尺度が合う |  |  |
| Standard近距離に耐える |  |  |
| Tacticalで形状が読める |  |  |
| 基準画像の世界観と衝突しない |  |  |
| 他候補と組み合わせられる |  |  |
| 非破壊の色調調整が可能 |  |  |
| 反復配置しても単調になりにくい |  |  |
| 王都導線での使用箇所が明確 |  |  |

使用候補箇所：`王都入口／主要街路／市場付近／王城前広場／王城正門／なし`

総合裁定：`合格／条件付き合格／不合格／確認不能`

裁定者：豆虎Gate

裁定日時（JST）：

条件付き合格の条件：

不合格／確認不能理由：

## 22. Automation・回帰・Package

| 検証 | 結果 | evidence |
|---|---|---|
| Stage V-1候補固有検査 |  |  |
| Asset Registry |  |  |
| missing reference |  |  |
| Material compile |  |  |
| Collision |  |  |
| NavMesh |  |  |
| Camera |  |  |
| Stage G-3B-R比較回帰 |  |  |
| Stage G-2B GUARD_M回帰 |  |  |
| Stage G-2A Camera回帰 |  |  |
| Stage G-1B PLAYER_M回帰 |  |  |
| Stage G-0回帰 |  |  |
| Stage F回帰 |  |  |
| Win64 Development Package |  |  |
| 引数なし検証Map起動 |  |  |

## 23. 保全／撤去結果

### 合格・条件付き合格の場合

| 項目 | 結果 | 状態 |
|---|---|---|
| 候補専用Folderに限定 |  |  |
| 不要demo Map除去 |  |  |
| 不要dependency除去 |  |  |
| SourceArtはリポジトリ外 |  |  |
| Candidate単位差分 |  |  |
| 50MB以上100MB未満のcommit／push前報告 |  |  |
| 100MB以上のfile 0 |  |  |
| commit承認待ち |  |  |

### 不合格・確認不能の場合

| 項目 | 結果 | 状態 |
|---|---|---|
| 専用Mapから参照除去 |  |  |
| Reference Viewer残存参照 |  |  |
| 候補Folder削除 |  |  |
| 候補由来Redirector整理 |  |  |
| missing reference 0 |  |  |
| Project内残存Asset 0 |  |  |
| Package |  |  |
| 既存Stage回帰 |  |  |
| SourceArt外部保全 |  |  |
| 不合格理由記録 |  |  |

撤去日時（JST）：

撤去Asset数：

## 24. 最終Git監査

| 項目 | 結果 |
|---|---|
| git status --short |  |
| staged一覧 |  |
| tracked差分一覧 |  |
| untracked一覧 |  |
| ignored候補関連一覧 |  |
| 候補外変更 |  |
| 候補別local branch |  |
| Content/StageV1外変更 |  |
| Source／Config／Plugins変更 |  |
| 50MB以上100MB未満fileと事前報告 |  |
| 100MB以上file 0。検出時停止 |  |
| Git LFS導入 | `なし` |
| commit | `豆虎Gate承認前はなし` |
| tag | `なし` |
| push | `なし` |

## 25. 既知の問題・UNKNOWN

### 既知の問題

1.
2.
3.

### UNKNOWN

1.
2.
3.

## 26. 完了要約

- 候補ID：
- import結果：
- 尺度結果：
- StandardCamera結果：
- TacticalOverlook結果：
- Collision／NavMesh結果：
- 性能結果：
- Package／回帰結果：
- 豆虎Gate裁定：
- 保全／撤去結果：
- ユーザー追加確認待ち：
