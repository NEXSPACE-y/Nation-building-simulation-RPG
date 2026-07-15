# Stage G-2B GUARD_M ユーザー実機チェックリスト

確認日：＿＿＿＿＿＿＿＿

確認者：＿＿＿＿＿＿＿＿

総合判定：`PASS／FAIL`

## 起動方法

以下を引数なしで起動する。

`C:\Users\rinpa\Desktop\TITLE\out\stage-g2b-guardm\package\Windows\NationSimulationStageC.exe`

起動後、Stage G-2B専用mapとGUARD_Mが表示されることを確認する。

## 1. 起動とモデル

- [ ] 引数なし起動に成功する
- [ ] PLAYER_Mが表示される
- [ ] PLAYER前方にGUARD_Mが1体だけ表示される
- [ ] GUARD_MがManny、Quinn、PLAYER_Mの代用品ではない
- [ ] GUARD_Mが上下反転していない
- [ ] GUARD_Mが地面へ正しく接地している
- [ ] GUARD_Mが宙に浮いたり、深く埋まったりしていない

判定／メモ：

＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿

## 2. 外観と質感

- [ ] GUARD_Mの兜、鎧、盾、衣装が正しく表示される
- [ ] 青、銀、金系の色が極端に黒潰れしていない
- [ ] 鎧のNormalが反転して見えない
- [ ] Metallic／Roughnessが不自然に真っ白、真っ黒、鏡面一色になっていない
- [ ] Dynamic Shadowが表示される
- [ ] Contact Shadowが足元に表示される
- [ ] GUARD_MとPLAYER_Mの身長・体格差が世界観上許容できる

判定／メモ：

＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿

## 3. Idle

- [ ] GUARD_Mが停止状態でIdle_11を再生する
- [ ] 肩、腕、膝、足首が大きく破綻しない
- [ ] 鎧や盾が身体へ大きく貫通しない
- [ ] GUARD_MのActor位置がIdleによって移動し続けない
- [ ] しばらく待ってもAnimationが停止しない

判定／メモ：

＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿

## 4. StandardCharacterCamera

- [ ] 起動時は通常のStandardCharacterCameraである
- [ ] GUARD_Mを近距離で確認できる
- [ ] 右クリック保持＋左右ドラッグで視点を回せる
- [ ] 右クリック保持＋上下ドラッグでPitchが変わる
- [ ] ホイールでZoomできる
- [ ] GUARD_M周辺でCamera Collisionが極端に破綻しない

判定／メモ：

＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿

## 5. TacticalOverlookCamera

- [ ] F6でTacticalOverlookCameraへ切り替わる
- [ ] Tactical表示でGUARD_Mの配置を確認できる
- [ ] Tactical表示でも右ドラッグYaw／Pitchが機能する
- [ ] Tactical表示でもホイールZoomが機能する
- [ ] F6でもう一度StandardCharacterCameraへ戻る
- [ ] F6往復後もGUARD_Mが正常表示される

判定／メモ：

＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿

## 6. PLAYER_M回帰

- [ ] 左クリック地点移動が機能する
- [ ] 左クリック保持移動が機能する
- [ ] GUARD_M付近でもNavMesh移動が破綻しない
- [ ] WALK 250uu/sが機能する
- [ ] RUN 500uu/sが機能する
- [ ] DASH 750uu/sが機能する
- [ ] WALK／RUN／DASH切替時に目的地が維持される
- [ ] 切替時にNavPathが維持される
- [ ] 到着後にPLAYER_MがIdle_11へ戻る
- [ ] PLAYER_Mの外観とAnimationがStage G-2Aから劣化していない
- [ ] Mannyへのsilent fallbackがない

判定／メモ：

＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿

## 7. PoC受入判定

- [ ] GUARD_Mを正式3D NPC候補として次の検討へ進められる
- [ ] 1体表示時の負荷と操作感がPoC受入範囲である
- [ ] 複数配置前にLODが必要であることを理解した
- [ ] AI、会話、保存、国家作用が未実装であることを確認した

外観判定：`PASS／FAIL`

Animation判定：`PASS／FAIL`

Camera判定：`PASS／FAIL`

PLAYER回帰判定：`PASS／FAIL`

総合判定：`PASS／FAIL`

追加指摘：

＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿

＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿＿

## 確認後

確認結果を豆虎Gateへ伝える。明示承認が出るまでcommit、tag、pushは行わない。
