# Stage G-0 描画PoC 検証報告

## 1. 判定

Stage G-0の技術配線、自動受入、Stage A～F回帰、Win64 Development package、起動smokeはPASSした。2026-07-12、ユーザーがWin64 Development Packageを実機操作し、手動受入20/20をすべてPASSと判定した。既知の受入阻害問題はない。完成Flipbook素材は未提供であり、全人物・牙鼠表示を明示的な`VISUAL_PLACEHOLDER`として扱う。

## 2. 基準点

- Stage F commit: `79d7223462e5eeadf479d1b3af50759917075331`
- annotated tag: `stage-f-production-scale-runtime-v0.1.0`
- branch: `main`

## 3. 正本

- `Docs/国家運営シミュレーションRPG_王都・2.5Dビジュアル仕様書_v0.1.md`
- `Docs/国家運営シミュレーションRPG_StageG-0_描画PoC実装指示書.md`
- `Docs/ChatGPT Image 2026年7月11日 21_50_21.png`

承認画像は画風・構成参照だけに使用した。切り抜き、加工、runtime取込、外部素材取得、Marketplace導入は行っていない。

## 4. 技術構成

`Paper2D` pluginを有効化し、`UPaperFlipbookComponent`派生の`UStageGDirectionalFlipbookComponent`へ表示責務を集約した。ActorはCapsule、world position、移動、衝突を保持し、表示Componentはaction／velocityをvisual actionと8方向へ変換するだけである。方向別procedural sprite、screen-space日本語label、検証panelも表示層へ限定し、因果判断、状態遷移、国家処理、保存処理を追加していない。

## 5. 3Dと2.5Dの分離

- Player: `ACharacter`の既存CapsuleとCharacterMovementを維持
- NPC: Capsule rootを追加し、旧3D仮bodyを非表示化。Stage G-0では40体の密集による移動封鎖を防ぐため`QueryOnly`とする
- 牙鼠: Capsule sweep移動を持つ検証専用Actor。物理blockせずVisibility queryを受ける
- PaperFlipbookとBlob Shadow: `NoCollision`
- save schema: 無変更。visual action、direction、frameを保存しない

## 6. 8方向

camera forward/rightの水平基底へworld velocityを投影し、45度単位でFront、FrontRight、Right、BackRight、Back、BackLeft、Left、FrontLeftへ量子化する。停止時は最後の有効方向を保持する。各方向は透明背景のprocedural矢印assetで形状差を持ち、G0-2境界試験とG0-12の8 asset差異試験をPASSした。Computer Useでは右方向の矢印変化を目視した。

## 7. 人物action配線

IDLE、WALK、TALK、WARN、REPORT、FLEEを配線した。ARRESTは専用完成素材がないため`VISUAL_PLACEHOLDER`として配線した。non-loop actionはIDLEへ戻り、FLEEはloopする。同一状態のtickでFlipbookを再設定しない。

## 8. 牙鼠

牙鼠1体を専用PoC mapへ配置し、IDLE、MOVE、CHARGE、BITE、HIT、DEATHを配線した。検証fixtureは6動作を順に切り替えるが、因果イベントや戦闘処理を生成しない。DEATHは最終frameを保持し、自動fallbackしない。ワールドクリック用135uu QueryOnly hit area、選択ring、頭上`【選択中】`表示を追加した。日本語panelの`表示対象→牙鼠`でも同じ描画targetへ確実に接続し、HUDは因果NPC targetと区別して`描画対象: 牙鼠 fang_rat_001 | 非因果fixture`と表示する。Computer Useで牙鼠選択と`噛みつき`の個別再生を確認した。

## 9. Camera

初期値は透視投影、yaw 45度、pitch -55度、arm 1100uuである。ユーザー実機評価を正本として右マウスorbitを撤回し、Player固定追従へ変更した。placeholder geometryでarmが短縮されないようStage G-0だけSpring Arm collision testを無効化した。ホイールは700～1500uu zoom、日本語buttonは初期値へ戻す。Computer Useで起動直後からPlayerを中心とする一定距離の斜め見下ろしを確認した。

移動は左クリック開始時にUI、`IStageG0Targetable`、`ClickMoveSurface`を排他的に判定するポイントドラッグ方式へ変更し、Stage G-0 map内のWASD移動は無効化した（既存Stage D mapは無変更）。地面はmouse ray、専用trace、NavMesh投影、到達可能な完全経路の順に検査し、受理した正確な目的地へcyanの「移動先」markerを表示して`SimpleMoveToLocation`へ渡す。保持中は0.075秒間隔かつ50uu以上変化した場合だけ目的地を更新し、release後も最後の目的地まで継続する。1秒進捗がない場合は同一目的地を1回だけ再計画し、再失敗時はその場で停止する。通常の詰まり回復にteleportは使用せず、Z落下時だけ最後の安全位置へ復帰する。

Cook済みpackageで最初に`ready=0 partial=1`が検出された。原因は、動的生成した外周Blocking Boxをroot登録した後にActor座標を再適用しておらず、4枚がworld原点へ重なってNavMeshを分断したこと、およびPlayerStart Capsuleがruntime NavMeshへ影響していたことである。外周Boxのworld transform再適用とPlayerStartのnavigation relevance無効化後、同一package smokeは`STAGE_G0_NAVIGATION_READY ready=1 path_points=2`を記録した。

## 10. Mapと環境

専用`/Game/Maps/StageG0_VisualPoC`を追加した。既存`StageD_Capital.umap`は変更していない。GameModeは専用mapでだけ、都市セル、郊外セル、壁、門、狭路、露店box、街道、柵、林縁、傾斜を`VISUAL_PLACEHOLDER`として生成する。5検証地点は日本語UMG立札と移動buttonを持ち、外周4面をBlocking Boxで囲む。Z=-500未満は最後の安全位置へ復帰する。王都7区画の本制作は行っていない。

## 11. 遮蔽と足元影

Paper2DはMasked materialとdepth testを使用する。F1へocclusion状態を表示し、実画面で3D障害物との前後関係を確認した。Blob ShadowはEngine Cylinderと単色materialを楕円へ扁平化し、ground traceで傾斜へ追従する。Computer UseでON/OFFによる明確な出現／消失を確認した。完成影素材ではない。

## 12. F1 debug

既存Stage E／F情報を維持し、表示対象ID、表示素材ID、表示動作、表示方向、Flipbook名、仮素材、sprite位置、Capsule位置、camera角、距離、遮蔽状態、切替回数、落下復帰回数を日本語表示した。内部IDは括弧付きで併記する。実画面で`VISUAL_PLACEHOLDER`、表示動作=報告、表示方向=右、sprite／Capsule座標、camera yaw／pitchを確認した。

## 13. 自動検証

- G0-1 Component Contract: PASS
- G0-2 Direction Quantization: PASS
- G0-3 Idle/Walk: PASS
- G0-4 Action Mapping: PASS
- G0-5 Flipbook Stability: PASS
- G0-6 Non-loop Fallback: PASS
- G0-7 Save/Load Independence: PASS
- G0-8 Actor Count Boundary: PASS
- G0-9 Causal Separation: PASS
- G0-10 Package Asset Load: PASS
- G0-11 Camera Follow Zoom Limits: PASS
- G0-12 Directional Placeholder Difference: PASS
- G0-13 Japanese Display Names: PASS
- G0-14 Verification Panel Non-causal: PASS
- G0-15 Fall Recovery: PASS
- G0-16 Verification Points: PASS
- G0-17 Fang Rat Manual Actions: PASS
- G0-18 Blob Shadow Toggle: PASS
- G0-19 Fixed Follow Camera Input: PASS
- G0-M1 Cursor Ray Contract: PASS
- G0-M2 Full Direction Destination: PASS
- G0-M3 Hold Update: PASS
- G0-M4 Pointer Consistency: PASS
- G0-M5 Invalid Surface: PASS
- G0-M6 Path Around Obstacle: PASS
- G0-M7 No Fall: PASS
- G0-M8 Directional Visual: PASS
- G0-T1 NPC Click Target: PASS
- G0-T2 Fang Rat Click Target: PASS
- G0-T3 Ground Does Not Select: PASS
- G0-T4 Target Does Not Move: PASS
- G0-T5 UI Click Isolation: PASS
- G0-T6 Overlap Priority: PASS
- summary: 33/33 PASS

証拠は`out/stage-g0/test-output/`へ生成した。

## 14. Stage A～F回帰

- Existing C++ causal core: 10/10 PASS
- Stage C: 11/11 PASS
- Stage D: 9/9 PASS
- Stage D corrective: 8/8 PASS
- Stage E behavioral: 8/8 PASS
- Stage E save migration: 4/4 PASS
- Stage E validator: 22/22 PASS
- Stage F core scenarios: 9/9 PASS
- Stage F validator: ERROR=0
- Stage F validator negative: 21/21 PASS
- Stage F Unreal boundary: 5/5 PASS
- Stage F 10,000 events／7-day offline／100 save-load／failure injection: PASS
- accepted Stage F soak 1800 seconds: PASS evidence retained
- Stage F Development package／StageD_Capital launch smoke: PASS

Stage F標準性能試験は前回生成物と世代番号が衝突しないG-0専用空ディレクトリで再実行した。テスト、期待値、fixture、Stage Fコードは変更していない。

## 15. Development package

- executable: `out/stage-g0/package/Windows/NationSimulationStageC.exe`
- executable SHA-256: `7931c90fa1b8c11845d8a6782b02b03bef463de800ffbb7da258f37d1e2bcc64`
- configuration: Development
- map: `/Game/Maps/StageG0_VisualPoC`
- Stage F dataset SHA-256: `d8fe2891b6faf63b703f417a97c79a93d4adbd13ce90287e0deae939984ca2b3`
- presentation NPC: 40
- Player: 1
- 牙鼠: 1
- rendered character／sprite／flipbook: 42／42／42
- Paper2D: PASS
- visual placeholder marker: PASS
- 引数なし既定起動redirect: PASS
- 日本語検証panel／頭上名／立札: PASS
- test point／outer blocker: 5／4
- runtime navigation probe: `ready=1`、`path_points=2`、非partial
- launch smoke: PASS

## 16. 性能証拠

NullRHI自動smokeの値は描画性能の採用判断に使わず、runtime wiringと42表示体の集計証拠としてのみ扱う。Computer Use実画面ではoverlay上で約114～117 FPSを観測したが、短時間目視値であり正式benchmarkではない。自動PoCで取得していないVRAM usageとdraw callsは`NOT_AVAILABLE_IN_AUTOMATED_POC`／`-1`として明示し、推測値を記録していない。

## 17. Computer Use結果

2026-07-12に再生成したWin64 Development packageをComputer Useで引数なし起動した。空き地の左クリックでcyanの「移動先」markerが出現し、Playerがmarker位置へ移動して固定cameraが追従することを目視した。続けて隊長`ai_npc_002`をクリックし、新規移動markerを出さず対象ring、HUDの`選択対象: 隊長 ai_npc_002`、範囲外理由を表示することを確認した。F1ではクリック地点、NavMesh移動地点、現在の移動先、移動状態、再計算回数、対象ID／種別が日本語名付きで表示された。牙鼠worldクリック、保持dragの連続更新、全方位／全通路、迂回、UI貫通防止はこのComputer Use回では未確認であり、PASSへ繰り上げていない。

## 18. ユーザー実機最終受入

ユーザー実機確認は20/20 PASSである。

- 左クリック自由移動／保持追従／360度任意方向移動: PASS
- 移動先ポインタ／カメラ自動追従: PASS
- 障害物迂回／全通路移動／落下防止: PASS
- NPC／牙鼠クリック選択: PASS
- 対象クリック／UIクリック時の誤移動なし: PASS
- 範囲内外の行動判定: PASS
- 8方向表示／Capsule collision／傾斜追従: PASS
- 遮蔽／足元影／牙鼠6動作: PASS
- Development Package実機動作: PASS
- 既知の受入阻害問題: なし

## 19. 完成素材が必要な項目

人物family `PLAYER`、`GUARD`、`CAPTAIN`、`BROKER`、`RESIDENT`について、8方向のIDLE、WALK、TALK、WARN、REPORT、FLEEが必要である。合計は5 family × 8方向 × 6 action = 240 Flipbookである。ARRESTは専用素材またはユーザー承認済み代替が別途必要である。

牙鼠は8方向のIDLE、MOVE、CHARGE、BITE、HIT、DEATH、合計48 Flipbookが必要である。さらに人物・牙鼠の足元影、foot pivot、透明背景、方向間の身長・接地位置、alpha輪郭、loop指定、source SHA manifestが必要である。

3D背景については、王都7区画、建築、街路、門、露店、郊外、植生、地面、遮蔽物の本番assetが未制作である。Stage G-0ではEngine Cube等のplaceholderだけを使用した。

## 20. 停止条件

技術配線、仮素材による自動受入、ユーザー実機手動受入20/20は完了した。Stage G-0は受入済みである。完成Flipbook素材は未提供のため、本成果物を完成ビジュアル素材として扱わない。Stage G本制作へ移行せず、Stage G-0基準点固定後に停止する。
