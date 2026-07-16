# Stage G-3A 王都背景Blockout 検証報告

初回検証日：2026-07-15 JST
是正検証日：2026-07-16 JST
ユーザー実機再確認日：2026-07-16 JST
判定：`PASS`（豆虎Gate正式承認）

## 基準点

- branch：`main`
- HEAD／origin/main：`e413ce6deae04ce7dfe295dc0004b4b33d4b9d10`
- tag：`stage-g2b-guardm-poc-v0.1.0`
- 開始時未追跡PDF：ユーザ裁定によりG-3A正式成果物へ含める。

## 初回ユーザーFAIL

- 症状1：左クリック時に「通行可能地面ではありません」と表示され、PLAYER_Mが移動できなかった。
- 原因：`ClickMoveSurface` は既定でIgnoreだが、G-3A地面に`ECC_GameTraceChannel1=Block`を明示設定していなかった。
- 検査漏れ：初回Automationは座標間NavMesh pathだけを見ており、マウスクリックが使う地面Traceと実Playerの移動量を検証していなかった。
- 症状2：単色の巨大な立方体・区画床・英字TextRenderに依存し、正本の中世王都の外形として読めなかった。

## 是正結果

- G-3A専用GameModeで`WalkableSurface` 1面だけが`ECC_GameTraceChannel1`をBlockするよう修正した。
- 建物・城壁・装飾はクリック地面Traceを奪わず、Character衝突、NavMesh obstacle、Camera Collisionは維持した。
- クリック地面Trace：6/6 PASS。
- 実移動命令：PASS。
- PLAYER_M実移動距離：422.27uu、PASS基準100uu以上。
- NavMesh主要導線：10/10 PASS。
- 旧箱形背景は、勾配屋根、木組み漆喰家屋、窓、戸、露店、煙突、扶壁、狭間、門塔、王城尖塔を持つ中世王都Blockoutへ再構成した。
- 過度に白く飛んでいた日照と色を抑え、土、石、漆喰、木材、テラコッタ、スレートの色分けへ変更した。

## Map／Asset

- Map：`/Game/Maps/StageG3A_CapitalBlockout_PoC`
- 王都Blockout：6000uu × 6000uu基準
- Blockout Actor：527件
- Engine Primitive：Cube、Cylinder、Cone、Sphereのみ
- Material：Master 1件、Instance 20件
- G-3A Content：21ファイル、最大9,191 bytes
- G-3A Map：1,138,701 bytes
- PLAYER_M：南門外のPlayerStart
- GUARD_M：南門に1体
- 旧G-1A技術広場／Quinn：G-3Aでは生成しない

## NavMesh runtime結果

- PLAYER開始地点 → 南門：PASS
- 南門 → メイン街路：PASS
- メイン街路 → 中央広場：PASS
- 中央広場 → 市場：PASS
- 中央広場 → 住宅：PASS
- 中央広場 → 王城前：PASS
- 王城前 → 貴族：PASS
- 南門 → 工房：PASS
- 建物角：PASS
- 壁際：PASS
- 合計：10/10 PASS

## Camera／Collision

- StandardCharacterCamera：runtime維持PASS
- Standard自動Yaw追従：PASS。Velocity優先、Destination／NavPath fallback、到着後Yaw維持。
- 初回是正判定の問題：G-3A内部ManagedYawの変化だけをPASS条件にしており、SpringArm Camera socketへ反映されない横移動を見逃した。
- 最終是正：Player rootと既存SpringArmの間へG-3A専用Yaw Pivotを挿入し、SpringArm計算前に実Camera world Yawを構成する方式へ変更。
- 実Camera検証：`USpringArmComponent::GetTargetRotation().Yaw`と進行方向の最終誤差0.0015度未満。
- Yaw Interp Speed：5.0
- 右ドラッグManual Override：PASS。入力中は手動優先、解放後1.25秒の保持を確認。
- Manual Override後の自動追従復帰：PASS
- UI hover誤停止：是正。hoverだけでは追従継続、UI上でボタン入力中だけ停止。
- TacticalOverlookCamera：runtime到達PASS
- TacticalOverlook自動Yaw追従なし：PASS。実測非指示Yaw変化0.0度。
- F6往復：PASS
- Standard復帰後の自動追従：PASS
- 南門外 → 中央広場 → 王城前のPlayer実走：4,155.5uu、PASS
- G-3A Collision ActorのCamera channel Block：PASS
- PLAYER fallback：false
- GUARD_M／PLAYER_M Skeleton共有：false
- Computer Use：`native pipe` BLOCKED。既知の環境制約として維持し、ユーザ実機のマウス操作確認で補完する。

## Automation／回帰／Package

- Stage C：11/11 PASS
- Stage D：9/9 PASS
- Stage D Fix：8/8 PASS
- Stage F：5/5 PASS
- Stage G-0：33/33 PASS
- Stage G-1A：22/22 PASS
- Stage G-1B：30/30 PASS
- Stage G-2A：18/18 PASS
- Stage G-2B：18/18 PASS
- Stage G-3A：29/29 PASS
- Win64 Development Package：PASS
- 引数なしG-3A Map起動：PASS
- G-2B明示Map回帰：PASS
- G-2A明示Map回帰：PASS
- G-1B明示Map回帰：PASS
- 評価画像：22/22、1280×720、合計21,945,068 bytes。自動追従6場面を追加。

## Package

- 実行ファイル：`C:\Users\rinpa\Desktop\TITLE\out\stage-g3a-capital-blockout\package\Windows\NationSimulationStageC.exe`
- 実行ファイルSHA-256：`7931C90FA1B8C11845D8A6782B02B03BEF463DE800FFBB7DA258F37D1E2BCC64`
- Package内G-3A runtime：クリック地面6/6、南門外から王城前まで実移動4,155.5uu、主要導線10/10を再取得してPASS。
- 明示Map起動：G-2B、G-2A、G-1Bのruntime証拠を再取得してPASS。

## Asset管理／保護境界

- 新規50MB超背景Asset：0
- 外部素材：0
- Marketplace：0
- Meshy追加生成：0
- Foliage：0
- Git LFS：未使用
- PLAYER_M資産変更：0
- GUARD_M資産変更：0
- G-2Aカメラ：受入済みC++、設定、Mapの変更0。SHA-256／byte一致PASS。
- G-2B GUARD仕様変更：0
- GameInstanceSubsystem変更：0
- 因果コア変更：0
- Save schema変更：0

## Git／プロセス

- staged：0
- tracked変更：G-3A Map cook／既定起動MapのConfig差分だけ
- untracked：69ファイル。G-3A成果物67件とユーザー指定正本文書2件。
- 範囲外変更：0
- `git diff --check`：PASS
- Unreal／ゲーム／Blender関連プロセス：0
- commit／tag／push：未実施

## 未解決事項

- 完成背景美術、石積みTexture、瓦、建物内部、完成植生は未実装。
- NPC、店機能、経済、国家UI、戦闘、魔法機能は未実装。
- StandardCharacterCameraの自動Yaw追従消失は是正済み。ユーザー実機で自動追従OKを確認し、豆虎Gateにより正式承認された。
- 全景、王城俯瞰、郊外の評価画像ではG-3A証拠撮影専用の広域フレームを使用する。受入済みG-2A設定と通常操作範囲は未変更。
- ユーザー実機確認と豆虎Gate承認を完了し、Stage G-3Aを正式完了扱いとする。
