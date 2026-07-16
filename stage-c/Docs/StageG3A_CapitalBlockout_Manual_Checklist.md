# Stage G-3A 王都背景Blockout ユーザー実機チェックリスト

版：StandardCamera自動Yaw追従是正後 v0.3
是正日：2026-07-16 JST
確認日：2026-07-16 JST
確認者：ユーザー／豆虎Gate
正式承認：`PASS`（StandardCharacterCamera自動追従OK）
Package：`out\stage-g3a-capital-blockout\package\Windows\NationSimulationStageC.exe`

## 初回FAILと今回の再確認対象

- 初回判定：`FAIL`
- 初回問題1：左クリックで「通行可能地面ではありません」と表示され、PLAYER_Mが移動できない。
- 初回問題2：巨大な単色立方体と区画床が主体で、正本の中世王都シルエットとして読めない。
- 実機問題3：StandardCharacterCameraで移動中のカメラ自動Yaw追従が消失していた。
- 是正内容：クリック地面Traceを明示設定し、勾配屋根、木組み家屋、露店、煙突、扶壁・狭間付き城壁、門塔、尖塔付き王城へ再構成。
- Camera是正：Standard時だけVelocity／Destination方向へYawを補間し、右ドラッグ中と解放後1.25秒は手動Yawを優先する。Tacticalには自動追従を入れない。
- Camera再是正：内部Yawだけの判定を廃止し、G-3A専用Yaw Pivotから既存SpringArmの実Camera TargetRotationを進行方向へ向ける。横方向移動でもキャラクター背後へCameraが回り込むことを再確認対象とする。
- ユーザがまず確認する項目：「南門を左クリックで通過できる」と「中世王都Blockoutとして外形を許容できる」。

## 起動

- [ ] 引数なし起動でStage G-3A王都Blockout Mapが開く
- [ ] PLAYER_Mが南門外の街道上へ表示される
- [ ] Mannyへfallbackしていない
- [ ] GUARD_Mが南門付近に1体だけ表示される

## 南門・街路

- [ ] 南門の幅と高さがPLAYER_M基準で不自然ではない
- [ ] 南門を左クリック移動で通過できる
- [ ] 南門を左クリック保持移動で通過できる
- [ ] 門付近でGUARD_Mを確認できる
- [ ] メイン街路を南門から中央広場まで移動できる
- [ ] 道幅が狭すぎず、建物角で詰まらない

## 区画

- [ ] 中央広場と中心Markerが分かる
- [ ] 市場区画の店棚、店先、屋根付き商業建物が分かる
- [ ] 住宅区画の勾配屋根、木組み、横道が分かる
- [ ] 貴族区画の大きめの邸宅とスレート屋根が分かる
- [ ] 下町・工房区の工房、倹庫、煙突が分かる
- [ ] 王城前広場と王城の塔、尖塔、狭間が分かる
- [ ] 南門外の農地とForest Markerが分かる
- [ ] 限定的魔法文明Markerが過剰な演出になっていない

## NavMesh移動

- [ ] 中央広場から市場へ移動できる
- [ ] 中央広場から住宅へ移動できる
- [ ] 中央広場から王城前へ移動できる
- [ ] 王城前から貴族区画へ移動できる
- [ ] 南門付近から工房区へ移動できる
- [ ] 壁際を移動できる
- [ ] 建物角を回り込める
- [ ] 移動先MarkerとNavPathが途中で消失しない

## Camera

### 今回の必須再確認

- [ ] StandardCharacterCameraで左クリック移動中、進行方向へカメラYawが緩やかに追従する
- [ ] 画面左右方向へ移動させても、Cameraが側面に残らずキャラクター背後へ回り込む
- [ ] StandardCharacterCameraで左クリック保持移動中も自動追従する
- [ ] NavPathに沿って曲がる時も進行方向へ追従し、目的地とNavPathを破壊しない
- [ ] 到着後は勝手に回り続けず、その時点のYawを維持する
- [ ] 右ドラッグ中は手動の上下左右操作が最優先される
- [ ] 右ドラッグ解放直後は手動Yawを維持し、約1.25秒後に自動追従へ戻る
- [ ] カーソルがUI上にあるだけでは自動追従が停止しない
- [ ] UI上でボタンを実際に操作している間はCamera自動追従が入力を邪魔しない
- [ ] F6でTacticalOverlookへ切り替えると自動Yaw追従しない
- [ ] Tactical中もPlayer位置追従、Zoom、右ドラッグ回転が維持される
- [ ] F6でStandardへ戻った後、自動Yaw追従が再び機能する
- [ ] 南門から中央広場、王城前まで移動しながら上記動作を確認できる

### 既存Camera回帰

- [ ] StandardCharacterCameraで南門通過を確認できる
- [ ] StandardCharacterCameraで街歩き視点が成立する
- [ ] 右クリックドラッグの上下左右が動作する
- [ ] ホイールZoomが動作する
- [ ] F6でTacticalOverlookへ切り替わる
- [ ] TacticalOverlookで南門・広場・王城の位置関係が分かる
- [ ] TacticalOverlookで各区画を順に確認できる
- [ ] F6でStandardCharacterCameraへ戻る
- [ ] 壁際・門付近でCameraが極端にめり込まない
- [ ] 建物角で視界が回復不能にならない

## PLAYER_M／GUARD_M回帰

- [ ] PLAYER_Mの見た目、Material、影が劣化していない
- [ ] GUARD_Mの見た目、Material、Idle_11、影が劣化していない
- [ ] WALK 250uu/sが動作する
- [ ] RUN 500uu/sが動作する
- [ ] DASH 750uu/sが動作する
- [ ] WALK／RUN／DASH切替後も目的地とNavPathを維持する
- [ ] GUARD_MがPLAYER_MのSkeletonを共有していない

## 総合判定

- [ ] 王都の代表区画と主導線をBlockoutとして理解できる
- [ ] 巨大な箱の羅列ではなく、正本に沿う中世王都の屋根群・城門・王城シルエットが読める
- [ ] PLAYER_M／GUARD_M基準で街の寸法がPoC受入範囲である
- [ ] Standard／Tactical両方で王都確認が成立する
- [ ] 完成背景制作の寸法基準として次工程へ進められる

判定：`PASS`
コメント：StandardCharacterCamera自動追従を含め、Stage G-3A 王都背景BlockoutをPoC受入範囲として正式承認。

豆虎Gateによる明示承認とcommit／tag／push許可：2026-07-16 JST 受領済み。
