# Stage G-1A 標準3Dキャラクター技術PoC 設計

## 判定範囲

Stage G-1Aは、UE 5.8標準Third Person共有コンテンツのManny/Quinnを用い、標準3D Characterが既存のNation Simulation基盤上で成立するかを確認する技術PoCである。オリジナル男性PLAYER、牙鼠3D化、王都7区画本制作、戦闘、Stage G-1Bは対象外とする。

## 隔離境界

- 新規コードは`Source/NationSimulationStageC/{Public,Private}/StageG1A`に隔離する。
- Stage D Player/GameMode、Stage G-0 Paper2D表示、Stage E状態定義、Stage F runtime、共有fixture・受入契約は変更しない。
- Player/NPCのMesh、AnimBP、表示状態はsave正本へ追加せず、実行時に標準資産から再構成する。
- G-1A map内でPaper2D、旧2.5D素材、牙鼠をロードしない。

## 3D Character

Playerは`ACharacter`で、Capsule 42/96、CharacterMovement、Manny Skeletal Mesh、`ABP_Unarmed`、SpringArm、Perspective Cameraを持つ。Mannyの一括scaleは`UStageG1ASettings::CharacterScale`だけで管理する。AnimBPは`BS_Idle_Walk_Run`を使用し、Actor移動はNavMesh完全経路のwaypointをCharacterMovementで追従し、animationはin-placeとする。移動方向へ自然に回頭し、回転速度も設定へ外出しする。

移動モードは表示・操作層の一時状態`EStageG1AMovementMode`としてWALK/RUNを明示的に保持し、起動時はWALKとする。`WalkSpeed=250.0`、`RunSpeed=500.0`は`UStageG1ASettings`と`DefaultGame.ini`へ外部化し、save schema・因果コアへ追加しない。停止時はIDLE、移動中は速度閾値ではなくモードを優先してWALK/RUN状態を選ぶ。再生倍率はWALK時`current_horizontal_speed / WalkSpeed`、RUN時`current_horizontal_speed / RunSpeed`を設定範囲へclampする。

移動中の切替は`MaxWalkSpeed`とanimation表示状態だけを更新し、現在の目的地、NavMesh path、path index、目的地更新数、選択対象には触れない。RUNからWALKへ戻す際は水平速度をWalkSpeedへ制限するが、停止・経路再計算・teleportは行わない。

代表NPCはQuinnと同じ標準AnimBPを使う1体だけで、縮尺・障害物・クリック対象の確認に限定する。既存の表示用Targetable契約だけを利用し、因果Subsystemへ状態を書き込まない。

## 入力・カメラ・安全

- 左クリック: 対象channelを先に判定し、対象でなければ歩行可能地面をtraceしてNavMesh完全経路へ移動する。
- 左クリック保持: 75ms周期かつ50uu以上の差がある場合だけ目的地を更新する。
- UI上のクリックはSlate widget pathで遮断する。
- G-1A PlayerはMoveForward/MoveRightをbindせず、WASD通常移動を無効にする。
- カメラはyaw 45°、pitch -50°、FOV 30°、arm 1800uu、Player追従、1200～2600uuの制限付きホイールzoomとする。
- 外周床、4面blocking boundary、NavMesh範囲検査、最終安全地点へのfall recoveryを持つ。

## 専用map

`/Game/Maps/StageG1_3DCharacterPoC`は、石畳広場、高さ・幅240uuの開口を持つ中世壁、4本の柱、緩斜面、4段階段、経路障害物、外周安全床、NavMesh、Movable Directional Light/Sky Light/Sky Atmosphereで構成する。初期位置から扉と代表NPCを比較しやすい配置にする。巨大cubeだけの空間、黒い虚空、旧Calibration map、王都7区画本制作にはしない。

## UI・証拠

通常UIは操作説明・現在モード・小型切替ボタン・状態だけを表示し、移動先markerと選択markerを残す。WALK中は`移動：歩行 / 走行へ切替`、RUN中は`移動：走行 / 歩行へ切替`とする。ボタンはF1欄から分離し、Slate widget pathでworld clickへの貫通を遮断する。キーボード切替は追加しない。

速度、play rate、目的地、復帰回数などの技術情報はF1時だけ表示する。F1には移動モード、設定歩行/走行速度、現在最大速度、現在水平速度、Animation状態、Animation再生倍率を日本語ラベル付きで表示する。Automation 22区分、実行時JSON 10種、Win64 Developmentの引数なし起動、Computer Use画像、初回ユーザー20項目と是正後10項目を別々に判定する。Automation PASSだけではStage G-1A完了としない。
