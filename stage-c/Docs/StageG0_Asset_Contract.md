# Stage G-0 Asset Contract

## 1. 現在のasset状態

`StageG0_Visual_Asset_Pack_v0.1`は未提供である。承認画像は構成・画風参照であり、Sprite／Flipbook素材ではない。画像からの切り抜き、外部取得、Marketplace導入は行わない。

## 2. 技術placeholder

| asset ID | source | 用途 | 完成扱い |
|---|---|---|---|
| `VISUAL_PLACEHOLDER:<family>:PROC_ARROW_<direction>` | 外部入力なしのruntime procedural texture／sprite | Player、人物、牙鼠の方向配線 | 否 |
| `/Paper2D/MaskedUnlitSpriteMaterial` | Unreal Paper2D同梱material | depth test付きmasked描画 | 技術materialのみ |
| Engine Cylinder＋BasicShapeMaterial | Engine基本shapeを扁平化 | 楕円Blob Shadow placeholder | 否 |
| Engine Cube | Engine基本shape | PoC 3D障害物 | 否 |
| Engine Cylinder＋cyan単色material＋日本語`移動先`label | Engine基本shape／runtime widget | NavMeshで受理したクリック目的地の技術marker | 否 |

runtime flipbook名は`FB_VISUAL_PLACEHOLDER_<family>_<action>_<direction>`とする。8方向で矢印先端は変化するが、完成animationの見た目・frame品質・人体表現・8方向自然さを証明しない。

procedural placeholderの契約は透明背景、96x128、足元中央pivot `(48,120)`、Masked描画、family別色、黒縁、白い方向矢印である。承認参考画像の画素は一切使用しない。

## 3. 完成asset受入契約

人物familyはPLAYER、GUARD、CAPTAIN、BROKER、RESIDENTを必要とする。各familyは8方向でIDLE、WALK、TALK、WARN、REPORT、FLEEを必要とする。ARRESTは専用素材または承認済み代替が必要である。

牙鼠は8方向でIDLE、MOVE、CHARGE、BITE、HIT、DEATHを必要とする。

各Flipbookは以下を満たすこと。

- transparent background
- foot pivot中央固定
- 方向間の身長・接地位置一致
- Masked描画で輪郭が破綻しないalpha
- actionごとのloop/non-loop指定
- 左右反転が必要な場合は装備・紋章・利き手の承認
- 本番asset IDとsource SHAのmanifest化

## 4. 差替境界

完成assetはaction×direction tableへ割り当て、Actor、Capsule、因果Subsystem、save schemaを変更せず差し替える。表示Componentへ判断規則を追加してはならない。
