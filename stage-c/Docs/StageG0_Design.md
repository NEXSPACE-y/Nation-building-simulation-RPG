# Stage G-0 3D背景＋2.5D描画PoC 技術設計

## 1. 目的と境界

Stage G-0は、Stage F commit `79d7223462e5eeadf479d1b3af50759917075331`を基準に、3D world position・Capsule collision・移動と、Paper 2Dによる人物／牙鼠表示を分離して成立させる技術PoCである。王都7区画、本番戦闘、本番asset、Stage G本制作は対象外とする。

承認画像は画風と構成の参照だけに使用し、画像データを切り抜き・加工・runtime取込しない。完成asset packは未提供なので、全表示は`VISUAL_PLACEHOLDER`として明示する。

## 2. 採用方式

既存Player／NPC Actorへ表示Adapter `UStageGDirectionalFlipbookComponent`を追加する方式を採用する。既存interaction、target、因果Subsystemを維持でき、Stage G-0専用派生Actorへ処理を複製しないためである。

```text
AStageDPlayerCharacter / AStageDNpcActor / AStageG0FangRatActor
├─ UCapsuleComponent: 3D collision、world position
├─ movement: CharacterMovementまたは表示専用fixture sweep
├─ UStageGDirectionalFlipbookComponent: 2.5D表示のみ
├─ UStaticMeshComponent: Blob Shadow、collisionなし
└─ Screen-space UMG: 日本語family／ID／検証地点表示
```

`UStageGDirectionalFlipbookComponent`は`UPaperFlipbookComponent`派生であり、因果コア、state transition、国家作用、event生成を参照しない。

## 3. Asset方式

`/Paper2D/MaskedUnlitSpriteMaterial`を維持し、Stage G-0専用のtransient `UTexture2D`／`UPaperSprite`／`UPaperFlipbook`を実行時に生成する。画像は外部入力を使わず、透明背景、足元中央pivot、family色の非対称body、黒縁、8方向別の白い矢印をコードで描画する。

全placeholderは`is_placeholder=true`、`visual_asset_id=VISUAL_PLACEHOLDER:<family>:PROC_ARROW_<direction>`、Flipbook名は`FB_VISUAL_PLACEHOLDER_<family>_<action>_<direction>`としてF1へ表示する。完成素材が存在するようなasset名や報告は行わない。

family色はPLAYER=黄、GUARD=青、CAPTAIN=赤、BROKER=紫、RESIDENT=緑、FANG_RAT=濃灰とする。頭上表示は日本語family名とIDを併記する。

## 4. 3Dと2.5Dの分離

- Playerの物理衝突はCapsuleだけが担当する。Stage G-0の40体NPCと牙鼠は密集検証でPlayerを閉じ込めないよう`QueryOnly`とし、専用`ClickTargetable` queryだけを受ける。Stage D以外の因果・保存境界は変更しない。
- FlipbookとBlob Shadowは`NoCollision`である。
- sprite pivot相当位置はCapsule中心基準で固定し、足元影はCapsule下端へ置く。
- spriteはcamera yawへbillboardし、camera pitchでは倒さない。
- depth testを無効化せず、3D wallの前後関係に従う。

## 5. 8方向

world velocityをcamera forward/rightの水平基底へ投影し、`atan2(right, forward)`を45度単位へ量子化する。方向はFront、FrontRight、Right、BackRight、Back、BackLeft、Left、FrontLeftである。速度が`MovementDirectionThreshold`未満なら最後の有効方向を保持する。

Stage G-0では8方向ごとに矢印先端が異なるprocedural spriteを割り当てる。左右反転は使用しない。本番採用可否は完成8方向素材で再評価する。

## 6. Animation mapping

| core/debug action | visual action | loop | fallback |
|---|---|---|---|
| WAIT／停止 | IDLE | yes | - |
| MOVE／INVESTIGATE／PROTECT | WALK | yes | - |
| TALK／REFUSE_TRADE／HELP | TALK | no | IDLE |
| WARN | WARN | no | IDLE |
| REPORT | REPORT | no | IDLE |
| ARREST | ARREST | no | IDLE、placeholder明示 |
| FLEE | FLEE | yes | - |
| 牙鼠MOVE | MonsterMove | yes | - |
| 牙鼠CHARGE／BITE／HIT | 対応action | no | IDLE |
| 牙鼠DEATH | Death | no | fallbackなし |

mappingは文字列対応表だけで、判断条件を持たない。牙鼠6動作はdebug fixtureが順番に再生し、因果eventを生成しない。

## 7. Camera

透視投影、yaw 45度、pitch -55度、arm 1100uuを初期値とする。右マウスorbitはユーザー実機評価により廃止し、Spring ArmがPlayerへ固定追従する。placeholder geometryでarmが短縮されないようStage G-0だけcollision testを無効にする。ホイールzoomは700～1500uuへclampし、日本語パネルの「カメラ初期位置へ戻す」で初期値へ復帰する。値は`UStageG0Settings`と`DefaultGame.ini`へ集約し、因果コア、Actor論理方向、save schemaへ書かない。

移動はStage G-0だけ左クリックのポイント移動とする。押下時はUI、`ClickTargetable`、`ClickMoveSurface`の順で排他的に解決する。対象Actorなら選択だけを行い、地面なら`DeprojectMousePositionToWorld → ClickMoveSurface trace → ProjectPointToNavigation → 到達可能な非partial path`を経て目的地を確定する。保持中は0.075秒間隔かつ前回目的地から50uu以上変化した場合だけ再計算し、解放後も最後の有効目的地まで移動を続ける。

経路追従は`PlayerController + UAIBlueprintHelperLibrary::SimpleMoveToLocation`を採用する。既存`CharacterMovement`、PlayerController、8方向velocity表示をそのまま利用でき、Actor内へ独自操舵を複製せずRecast NavMeshの障害物迂回を利用できるためである。`AddMovementInput`による固定方向直進はStage G-0では使用しない。cyanの「移動先」markerは実際のNavMesh目的地と同じworld座標へ置く。

経路中に1.0秒進捗がなければ同一目的地へ1回だけ再計算し、再度進めなければ現在地で停止する。スタック時に安全位置へteleportしない。最後の安全位置への復帰はZ=-500未満の落下時だけである。既存Stage D mapのWASD互換は維持するが、Stage G-0 map内ではWASD移動を明示的に無効化する。

## 8. PoC map

`StageG0_VisualPoC.umap`を専用mapとして作成する。mapはPlayerStartと`StageG0_NavMeshBounds`を持ち、GameModeが実行時に以下の3D placeholderを構築する。Recastは`RuntimeGeneration=Dynamic`とし、実行時生成床と障害物からnavigationを再構築する。

- 都市セル: 地面、壁、門型障害物、狭路、広場、露店相当box
- 郊外セル: 街道、柵、林縁、高低差相当の傾斜box

`StageD_Capital.umap`は変更しない。既存20 AI＋20 NON AIを表示し、牙鼠1体を追加する。Stage F内部2,500 AIはActor化しない。

通行可能床だけが`ClickMoveSurface`をBlockし、壁、屋根、人物sprite、影、文字、装飾は反応しない。目的地はX/Y ±4200uu内、NavMesh投影可能、到達可能であることを必須とする。日本語glyph fallbackを使うscreen-space UMG立札で「門・衝突試験」「狭路試験」「傾斜試験」「遮蔽試験」「牙鼠動作試験」の5地点を表示し、検証パネルから移動できる。頭上family名／IDも同じUMG widgetを使用し、距離650uu外のNPCは非表示にする。外周4辺は`BlockAll`のBoxで囲み、Z=-500未満では最後の安全位置へ復帰して`STAGE_G0_FALL_RECOVERY`を記録する。傾斜地点には歩行可能な傾斜boxと上端landingを置く。

## 9. 日本語検証パネル

Stage G-0 mapだけでnative UMG panelを表示し、Player／衛兵／隊長／商人／一般住民／牙鼠、人物8方向、人物7動作、牙鼠6動作と自動再生、5検証地点、影ON/OFF、カメラresetを選べる。牙鼠はworld click用135uu QueryOnly hit areaと選択ringを持ち、panelの「表示対象→牙鼠」からも確実に選択できる。UI hit pathをSlateで先に判定し、UIクリックをworld入力へ貫通させない。

AI NPC、NON AI NPC、牙鼠は`IStageG0Targetable`を実装し、ID、種別、日本語名、国家、location、有効性、選択可能性、interaction originを返す。Capsule／専用hit areaは`ClickTargetable`をOverlapし、spriteにはtarget判定を持たせない。重なりはray depth、画面cursor距離、target IDの順で決定論的に選ぶ。地面クリックは既存targetを保持する。牙鼠は選択と表示fixture切替だけを許可し、既存Player Actionを因果コアへ送らない。

## 10. Debugと性能

F1はStage E/F情報を残し、visual actor／asset／action／direction／flipbook／frame／rate／placeholder／sprite・capsule位置／camera／distance／occlusion／switch countを追加表示する。さらにクリック地点、地面判定、NavMesh投影地点、現在目的地、経路状態／点数、移動中、左保持中、更新回数、拒否理由、対象ID／種別／日本語名／距離／同一location／行動可否を日本語表示する。

5秒のruntime samplingでresolution、window mode、CPU、GPU、RAM、average FPS、frame time、1% low相当、表示数を記録する。自動PoCで取得できないVRAMとdraw callsは`NOT_AVAILABLE_IN_AUTOMATED_POC`として明示し、値を偽装しない。

F1のユーザー向け項目名は日本語とし、内部IDは括弧付きで併記する。足元影はEngine Cylinderと単色materialを扁平化した楕円で、ground traceにより地面／傾斜へ追従する。影は`NoCollision`で方向回転から独立する。

## 11. 保存境界

visual action、direction、playback position、placeholder flipbookはsaveへ書かない。load後にcoreのselected actionとActor velocityから再構成する。Stage F save schemaは変更しない。
