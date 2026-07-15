# Stage G-2A カメラ再設計書

作成日：2026-07-15 JST
状態：技術検証完了、ユーザー実機確認待ち

## 1. 目的

Stage G-1B正式基準点のPLAYER_Mと移動基盤を維持し、通常探索用の非俯瞰カメラと、F6でのみ起動する戦略俯瞰補助ビューを分離する。

旧Stage G-2Aの「俯瞰を標準視点とする方式」は正式不採用であり、本実装の基準点・初期状態・設定値として使用しない。

## 2. 正式基準点

- branch：`main`
- HEAD／origin/main：`985155fa86586295b804601bbee33d01bf0b115f`
- annotated tag：`stage-g1b-meshy-original-player-v0.1.0`
- PLAYER_M、Skeleton、Material、Texture、Idle_11、WALK／RUN／DASHは変更しない
- Stage G-1A／G-1BのPlayer、map、因果コア、save schemaは変更しない

旧G2Aはリポジトリ外へ5,370ファイル、1,993,620,739 bytesを保全し、SHA-256一致5,370／5,370を確認後に旧対象だけを撤去した。

## 3. 実装構成

```text
Stage G-1A Player（受入済み）
├─ Capsule／CharacterMovement（変更なし）
├─ Meshy PLAYER_M（Stage G-1B、変更なし）
├─ StageG1ACameraBoom（参照して制御）
└─ StageG1AFollowCamera（参照して制御）

StageG2ACameraModeAdapter（新規）
├─ StandardCharacterCamera
├─ TacticalOverlookCamera
├─ 右ドラッグYaw／Pitch
├─ ホイールZoom
├─ F6モード切替
├─ UI入力遮断
└─ Runtime／画像証拠生成
```

カメラAdapterは表示層だけを担当し、移動命令、目的地、NavPath、対象選択、WALK／RUN／DASH、因果イベント、保存状態を所有しない。

## 4. StandardCharacterCamera

初期モード。PLAYER_M背後寄りの非俯瞰視点として使用する。

| 項目 | 値 |
|---|---:|
| 初期距離 | 520uu |
| Zoom範囲 | 320～850uu |
| 初期Pitch | -15度 |
| Pitch範囲 | -30～+8度 |
| FOV | 75 |
| 注視中心 | Capsule中心 + Z 115uu |
| Collision Probe | 12uu |
| Collision最小距離 | 180uu |
| Collision復帰基準 | 0.20秒 |
| Camera Lag Speed | 14 |
| Yaw感度 | 0.50度／raw MouseX |
| Pitch感度 | 0.15度／raw MouseY |

初期Yawは固定角度ではなく、起動時PLAYERの背後となるActor Yawを採用する。カメラ回転はPLAYER Actor Yawを変更しない。

## 5. TacticalOverlookCamera

F6でのみ起動する戦略確認用の補助ビュー。起動時の標準視点にはしない。

| 項目 | 値 |
|---|---:|
| 初期距離 | 1200uu |
| Zoom範囲 | 800～2000uu |
| 初期Pitch | -55度 |
| Pitch範囲 | -65～-45度 |
| FOV | 60 |
| 注視中心 | Capsule中心 + Z 120uu |
| Collision Probe | 16uu |
| Collision最小距離 | 400uu |
| Collision復帰基準 | 0.30秒 |
| Camera Lag Speed | 12 |
| Yaw感度 | 1.25度／raw MouseX（旧観測値0.50の2.5倍） |
| Pitch感度 | 0.18度／raw MouseY（旧観測値0.15の1.2倍） |

右ドラッグ中の回転は即時反映し、入力終了後だけ軽い減衰処理を許可する。

## 6. F6切替と状態分離

- StandardでF6：Tacticalへ切替
- TacticalでF6：Standardへ切替
- 各モードはYaw、Pitch、距離を別々に保持する
- FOV、Pitch clamp、Zoom clamp、Probe、Lagは切替先の値を即時適用する
- PLAYER位置、目的地、NavPath、移動モード、選択対象、Idle_11を変更しない
- Tacticalの値をStandardへ、Standardの値をTacticalへ漏らさない

## 7. 入力

Win64専用mappingは`Config/Windows/WindowsInput.ini`へ分離する。

- 右クリック保持：カメラ回転開始
- MouseX：Yaw
- MouseY：Pitch
- 右クリック解放：回転終了、カーソル復帰
- ホイール：現在モードの距離Zoom
- F6：カメラモード切替
- 左クリック／左クリック保持：Stage G-1B移動処理をそのまま使用
- UI上の右クリック／ホイールはWorldカメラへ送らない

## 8. Camera Collision

既存SpringArmのCamera channel sweepを使用する。

- `bDoCollisionTest=true`
- `ProbeChannel=ECC_Camera`
- モード別Probe値を適用
- Runtime証拠では520uu設定時に実測537.1uuから244.3uuへ短縮し、障害撤去後537.1uuへ復帰した
- Character衝突、Capsule、per-poly collisionは変更しない

## 9. 専用成果物

- map：`/Game/Maps/StageG2A_CameraRedesignPoC`
- Blueprint：`/Game/StageG2A/CameraRedesign/Blueprints/BP_StageG2A_CameraModeAdapter`
- C++：`Source/NationSimulationStageC/**/StageG2A/`
- Automation：`NationSimulation.StageG2A.CameraRedesign`
- 証拠：`SourceArt/StageG2A/CameraRedesign/v0.1/`

commit、tag、pushはユーザー実機確認と豆虎Gate承認まで行わない。
