# 国家運営シミュレーションRPG
# Stage G-0 3D背景＋2.5Dキャラクター描画PoC 実装指示書
## Codex Sol向け拘束仕様

---

## 0. 本書の位置付け

本書は、Stage Fの受入およびGit基準点固定が完了した後にのみ、Codex Solへ投入するStage G-0実装指示書である。

Stage G-0の目的は、以下の描画方式がUnreal Engine上で破綻せず成立することを小規模に証明することである。

```text
3D背景・地形・建物
＋
2.5D人物・牙鼠
＋
斜め見下ろしカメラ
＋
3D空間での移動・衝突・経路探索
```

Stage G-0は王都本制作ではない。

王都7区画、複数国家、本番戦闘、本番アセット量産へ進んではならない。

---

# Part I：開始条件

## 1. Stage F基準点

作業開始前に以下を確認する。

```text
- Stage Fの全受入がPASS
- Stage Fの実機確認が必要な場合は完了済み
- Stage Fがcommit済み
- Stage Fのannotated tagが存在
- local main == remote main
- tag target == main HEAD
- working tree clean
```

投入時にStage F受入タグ名が明示されていない場合、変更せず停止して報告する。

以下の場合も変更せず停止する。

- Stage Fタグが軽量タグ
- Stage Fタグとmain HEADが不一致
- localとremoteが不一致
- working treeがcleanではない
- Stage F以外の未反映変更がある

## 2. 必須入力資料

以下を正本として使用する。

```text
- 統合要件定義書
- 世界観基礎仕様書 v0.1
- 王都・2.5Dビジュアル仕様書 v0.1
- Stage F設計書
- Stage F検証報告
- Stage Eまでの既存設計・回帰基準
- 承認済み王都ビジュアルコンセプト
```

## 3. 画像素材パック

本指示投入時に、司令塔またはユーザーから以下の素材パックを別途与える。

```text
StageG0_Visual_Asset_Pack_v0.1
```

最低限の予定内容：

```text
- プレイヤー代表スプライト
- GUARD代表スプライト
- CAPTAIN代表スプライト
- BROKER代表スプライト
- 一般住民代表スプライト
- 牙鼠代表スプライト
- 足元影素材
```

素材パックが存在しない場合：

- 最終アートを独断生成しない
- Marketplace素材を独断導入しない
- 外部画像を無断取得しない
- 技術用の明示的な仮スプライトで配線まで実施してよい
- ビジュアル受入は未完了として停止する

---

# Part II：技術方式

## 4. 採用方式

Unreal EngineのPaper 2Dを使用する。

```text
Actor / Character
├─ Capsule Collision
├─ Character Movement または既存移動Component
├─ 既存interaction/target境界
├─ UPaperFlipbookComponent
└─ 足元簡易影
```

Paper 2D Pluginを有効化し、SpriteとFlipbookを使用する。

既存のUnreal Actor責務と因果コア責務を維持する。

## 5. 3D側

3D側で処理する。

```text
- world position
- movement
- collision
- NavMesh
- height
- wall/building occlusion
- interaction range
- location volume
```

スプライト画像の輪郭を衝突判定として使用しない。

## 6. 2.5D側

2.5D側で処理する。

```text
- current visual direction
- current animation
- Flipbook selection
- playback rate
- sprite scale
- billboard orientation
- foot alignment
- blob shadow
```

2.5D表示Componentは、因果判断、状態遷移、イベント生成を行ってはならない。

---

# Part III：クラスと責務

## 7. 基本表示Component

既存命名と構造を調査し、最小変更で以下相当を実装する。

```text
UStageGDirectionalFlipbookComponent
```

責務：

```text
- 8方向Flipbookの参照保持
- animation stateごとの参照保持
- world/camera方向から8方向を決定
- 現在Flipbookの切替
- 再生速度変更
- foot pivot基準の表示
- debug情報の公開
```

判断や行動選択は行わない。

## 8. 表示状態

最低限、以下を定義する。

```text
EStageGVisualAction
- Idle
- Walk
- Talk
- Warn
- Report
- Arrest
- Flee
- MonsterCharge
- MonsterBite
- Hit
- Death
```

既存因果コアのactionと1対1である必要はない。

表示専用の明示的なmapping tableを設ける。

例：

```text
WAIT            -> Idle
TALK            -> Talk
WARN            -> Warn
REPORT          -> Report
ARREST          -> Arrest
FLEE            -> Flee
REFUSE_TRADE    -> Talk または Idle + UI表示
INVESTIGATE     -> Walk または Idle
```

mapping tableに判断条件を入れてはならない。

## 9. 対象Actor

既存Actorを破壊的に置換しない。

以下のいずれかの最小構成を採用する。

```text
A. 既存Player/NPC ActorへFlipbook Componentを追加
B. 表示専用派生ActorをStage G-0マップだけで使用
C. 表示Adapter Componentとして追加
```

選択理由を`StageG0_Design.md`へ記録する。

---

# Part IV：カメラ

## 10. 斜め見下ろし

Stage G-0用カメラを実装する。

初期値は検証用設定として外部化する。

```text
yaw: 45 degrees
pitch: 50〜60 degrees downward
zoom: 制限付き
rotation: 原則固定
```

透視投影を初期採用とする。

ただし、正投影との比較が低コストで可能な場合はデバッグ切替を設け、結果を証拠へ記録してよい。

正投影比較は必須ではない。

カメラ値をコードへ複数箇所直書きしない。

## 11. Billboard

スプライト面はカメラへ向ける。

要件：

```text
- Z軸周りを基準にカメラへ正対
- 足元位置を変えない
- カメラpitchによってスプライトが倒れない
- Actorの論理的な移動方向を破壊しない
```

---

# Part V：8方向切替

## 12. 方向判定

方向は、Actorの移動または向きベクトルを、カメラ基準の水平座標へ変換して決定する。

手順：

```text
1. Actorの水平移動ベクトルまたは論理向きを取得
2. Camera Forward/Rightを水平面へ投影
3. camera-local vectorへ変換
4. atan2相当で角度を算出
5. 45度単位の8sectorへ量子化
6. 対応Flipbookを選択
```

停止中は最後の有効方向を保持する。

速度が閾値未満の場合はIdleへ切り替える。

方向閾値と速度閾値は外部化する。

## 13. 方向一覧

```text
Front
FrontRight
Right
BackRight
Back
BackLeft
Left
FrontLeft
```

方向名とasset suffixを統一する。

例：

```text
FB_Guard_Walk_F
FB_Guard_Walk_FR
FB_Guard_Walk_R
FB_Guard_Walk_BR
FB_Guard_Walk_B
FB_Guard_Walk_BL
FB_Guard_Walk_L
FB_Guard_Walk_FL
```

既存プロジェクトの命名規則がある場合は、それへ合わせる。

---

# Part VI：アニメーション

## 14. 人物

Stage G-0で最低限、以下を接続する。

```text
IDLE
WALK
TALK
WARN
REPORT
FLEE
```

`ARREST`は以下のどちらかを実装する。

```text
- 専用Flipbook
- 明示的なStage G-0暫定Flipbook
```

暫定の場合はHUD/F1へ`VISUAL_PLACEHOLDER`を表示する。

## 15. 牙鼠

牙鼠は以下を接続する。

```text
IDLE
MOVE
CHARGE
BITE
HIT
DEATH
```

Stage G-0では戦闘ロジックを新設しない。

動作再生は以下のどちらかに限定する。

```text
- 既存event/actionからの表示mapping
- Stage G-0専用のdebug fixture
```

debug fixtureは本番因果イベントを生成してはならない。

## 16. Flipbook切替

`UPaperFlipbookComponent::SetFlipbook`相当の切替を一元化する。

要件：

```text
- 同一Flipbookを毎frame再設定しない
- 状態変更時だけ切替
- loop/non-loopを明示
- non-loop終了後のfallbackを明示
- animation stateとdirection変更を監査可能
```

---

# Part VII：描画・遮蔽・影

## 17. Sprite Material

PoCでは、透明描画順の破綻を避けるため、原則としてMasked系materialを使用する。

要件：

```text
- 背景色を含まないalpha
- 輪郭が崩れないclip設定
- unnecessary translucencyを避ける
- lighting方針を統一
```

半透明が必要な特殊素材はStage G-0対象外とする。

## 18. 3D遮蔽

人物が建物の前後を移動した際、3D深度に従って自然に隠れること。

以下を禁止する。

```text
- 常時最前面表示
- depth test無効化
- 全キャラクターの強制Translucent sort priority固定
```

建物の裏へ完全に隠れて操作不能になる場合だけ、PoC用の輪郭表示または屋根fadeを検討できる。

独断で本番仕様へ固定せず、比較証拠を提示する。

## 19. 足元影

人物と牙鼠へ簡易Blob Shadowを追加する。

要件：

```text
- 足元中央へ追従
- 地面からの浮遊感を抑える
- sprite directionで影が回転しない
- collisionへ影響しない
- 過剰な透明面積を持たない
```

---

# Part VIII：Stage G-0検証マップ

## 20. マップ方針

既存`StageD_Capital`を直接破壊しない。

以下のどちらかを選ぶ。

```text
A. StageD_Capital複製からStageG0_VisualPoCを作成
B. Stage G-0専用の小規模mapを新規作成
```

正本マップを直接変更する場合は、既存Stage D/E/F回帰が完全に維持される根拠が必要であり、原則として選択しない。

## 21. 必須環境

```text
都市セル：
- 3D地面
- 市場用小物
- 建物壁
- 門またはアーチ
- 狭い通路
- 広場

郊外セル：
- 街道
- 農地
- 柵
- 林縁
- 高低差または傾斜
```

本番品質の王都7区画を制作してはならない。

## 22. 必須表示個体

通常確認：

```text
Player 1
ai_npc_001 / GUARD
ai_npc_002 / CAPTAIN
ai_npc_012 / BROKER
NON AI NPC 5以上
牙鼠 1以上
```

表示負荷確認：

```text
既存AI NPC 20
既存NON AI NPC 20
牙鼠 1
Player 1
```

Stage F内部の2,500 AI NPCをActor化してはならない。

---

# Part IX：既存システム接続

## 23. 因果コア

既存の因果コア、Stage E行動、Stage F activity tier、save、offline、determinismを変更しない。

表示層は読み取り専用Viewまたは既存Subsystem projectionを使用する。

## 24. Player操作

既存の以下を維持する。

```text
- 移動
- target選択
- TALK
- HELP
- HARM
- TRADE
- STEAL
- WAIT
- MOVE
- save/load
- F1 debug
```

入力キーの独断変更は禁止する。

## 25. NPC行動表示

Stage Eで成立した以下を画面へmappingする。

```text
REPORT
WARN
REFUSE_TRADE
FLEE
ARREST
TALK
WAIT
INVESTIGATE
HELP
PROTECT
```

専用アニメーションがない行動は、暫定mappingであることをF1へ明示する。

---

# Part X：F1デバッグ

## 26. 追加表示

F1へ最低限、以下を追加する。

```text
visual_actor_id
visual_asset_id
visual_action
visual_direction
flipbook_name
flipbook_frame
play_rate
is_placeholder
sprite_world_location
capsule_world_location
camera_yaw
camera_pitch
distance_to_camera
occlusion_state
```

Stage Fの既存デバッグ情報を削除してはならない。

---

# Part XI：自動検証

## 27. Stage G-0 Automation

最低限、以下を自動検証する。

### G0-1 Component Contract

- Paper2D plugin有効
- required component存在
- collisionとspriteが分離
- sprite側に因果型参照なし

### G0-2 Direction Quantization

8方向の代表ベクトルが正しい方向へ分類される。

境界角度も検証する。

### G0-3 Idle/Walk

速度閾値の上下でIdle/Walkが正しく切り替わる。

### G0-4 Action Mapping

既存actionから表示actionへのmappingが期待どおり。

### G0-5 Flipbook Stability

同一状態で毎frame再設定されない。

### G0-6 Non-loop Fallback

WARN、REPORT、ARREST、BITE、HIT、DEATH等の非loop方針が正しい。

DEATHはIdleへ戻さない。

### G0-7 Save/Load Independence

表示状態をsave正本にしない。

load後、コア状態から表示を再構成できる。

### G0-8 Actor Count Boundary

Stage F 2,500 AI runtimeを読み込んでも、表示Actor数がPoC上限を超えない。

### G0-9 Causal Separation

Stage G-0 sourceへ因果判断、transition rule、国家作用が複製されていない。

### G0-10 Package Asset Load

packaged Development buildで必要Sprite/Flipbookを解決できる。

---

# Part XII：手動受入

## 28. 手動チェック

以下を実機で確認する。

```text
1. StageG0_VisualPoCを起動
2. 斜め見下ろしカメラを確認
3. PlayerのIDLEを確認
4. 8方向へ移動し、方向別WALK切替を確認
5. 停止時に最後の方向を保持してIDLEへ戻る
6. GUARD、CAPTAIN、BROKERが2.5D表示される
7. TALK、WARN、REPORT、FLEEを表示確認
8. ARRESTの専用または暫定表示を確認
9. 建物の前後で正しく遮蔽される
10. 門または狭路を衝突判定付きで通過する
11. spriteとcollision位置が乖離しない
12. 足元影が追従する
13. 傾斜または高低差で浮遊・埋没しない
14. 牙鼠のIDLE、MOVE、CHARGE、BITE、HIT、DEATHを確認
15. AI NPC 20＋NON AI NPC 20を同時表示
16. target選択と既存Player actionを確認
17. F5/F9または既存save/loadを確認
18. F1でStage E/F/G-0情報を確認
19. package版で同じ表示・操作を確認
20. 2,500 AI NPC Actorが生成されていないことを確認
```

---

# Part XIII：性能証拠

## 29. 記録対象

ユーザーの開発PC上で以下を記録する。

```text
resolution
window mode
GPU
CPU
RAM
average FPS
average frame time
1% low相当値を取得可能なら記録
VRAM usage
rendered character count
draw calls
sprite/flipbook count
```

Stage G-0では絶対性能要件を本番最低仕様として固定しない。

以下は必須とする。

```text
- 40 NPC＋Player＋牙鼠で操作不能な継続的stutterがない
- memory/VRAMの継続的増加がない
- sprite方向切替による著しいframe spikeがない
- 透明描画順の恒常的な破綻がない
```

最終的な採用判断にはユーザーの実機目視を必要とする。

---

# Part XIV：回帰

## 30. 必須回帰

Stage G-0受入時、Stage Fまでの全回帰を維持する。

```text
C++ causal core
Stage C
Stage D
Stage D corrective
Stage E
Stage E migration
Stage E validator
Stage F scenarios
Stage F validator
Stage F save/recovery
Stage F offline
Stage F scale/runtime
Stage F package/smoke
```

既存期待値を変更してPASSさせてはならない。

---

# Part XV：成果物

## 31. 必須設計・報告

```text
stage-c/Docs/StageG0_Design.md
stage-c/Docs/StageG0_Asset_Contract.md
stage-c/Docs/StageG0_Verification_Report.md
stage-c/Docs/StageG0_Manual_Verification_Checklist.md
```

## 32. 推奨配置

既存構造に合わせて調整してよい。

```text
stage-c/Source/NationSimulationStageC/Public/StageG0/
stage-c/Source/NationSimulationStageC/Private/StageG0/
stage-c/Source/NationSimulationStageC/Private/Tests/StageG0*
stage-c/Content/StageG0/
stage-c/Build/RunStageG0Acceptance.ps1
stage-c/Build/PackageStageG0.ps1
```

## 33. 証拠

```text
out/stage-g0/stage_g0_acceptance_results.txt
out/stage-g0/test-output/stage_g0_direction_evidence.json
out/stage-g0/test-output/stage_g0_animation_mapping.json
out/stage-g0/test-output/stage_g0_actor_count.json
out/stage-g0/performance/stage_g0_performance.json
out/stage-g0/manual/stage_g0_manual_verification.txt
out/stage-g0/package_launch.txt
out/stage-g0/screenshots/
```

`out/`をGit追跡へ追加してはならない。

---

# Part XVI：受入条件

## 34. 自動受入

- [ ] Paper2Dを使用
- [ ] 3D collisionと2.5D表示を分離
- [ ] 8方向判定PASS
- [ ] IDLE/WALK切替PASS
- [ ] action mapping PASS
- [ ] non-loop fallback PASS
- [ ] save/load再構成PASS
- [ ] Stage F 2,500 runtimeとActor数分離PASS
- [ ] 因果責務分離PASS
- [ ] package asset load PASS
- [ ] Stage Fまでの全回帰PASS

## 35. 実機受入

- [ ] 斜め見下ろしで違和感なく操作可能
- [ ] 3D背景内で2.5D人物が浮いて見えない
- [ ] 建物前後の遮蔽が破綻しない
- [ ] 8方向切替が目視で自然
- [ ] 既存40 NPCを表示可能
- [ ] 牙鼠の6動作を確認
- [ ] 既存target/action/save/load/F1が動作
- [ ] package版で再現
- [ ] ユーザーが描画方式を承認

全自動試験PASSだけではStage G-0完了扱いにしない。

ユーザーの実機目視承認を必須とする。

---

# Part XVII：禁止事項

## 36. 絶対禁止

```text
- Stage F完了前の実装開始
- Stage F基準点未固定での開始
- 3D人物モデル追加
- 5国家分のビジュアル制作
- 王都7区画の本制作
- 複数モンスター追加
- 本番戦闘実装
- ダメージ、経験値、戦利品の独断実装
- 魔法体系追加
- クエスト実装
- 冒険サイクル実装
- 2,500 AI NPC Actor生成
- NON AI人口枠の全描画
- Actor/UIへの判断ロジック追加
- 既存因果コアの描画都合による変更
- 既存テスト期待値変更
- Marketplace素材の独断導入
- 外部素材の無断取得
- 未承認plugin追加
- placeholderを本番素材として報告
- commit
- tag
- push
- Stage G本制作への移行
```

---

# Part XVIII：作業順序

## 37. Phase G0-0 Baseline

1. Stage F基準点確認
2. 必須文書確認
3. asset pack確認
4. Paper2D plugin状態確認
5. 既存Actor/Subsystem責務調査
6. `StageG0_Design.md`作成

## 38. Phase G0-1 Technical Skeleton

1. directional component
2. visual action enum
3. asset contract
4. camera
5. billboard
6. blob shadow
7. F1 fields

## 39. Phase G0-2 Player

1. capsule維持
2. sprite表示
3. 8方向
4. IDLE/WALK
5. collision/occlusion
6. save/load再構成

## 40. Phase G0-3 NPC

1. GUARD
2. CAPTAIN
3. BROKER
4. NON AI代表
5. action mapping
6. 既存40 NPC表示

## 41. Phase G0-4 牙鼠

1. display Actor
2. 8方向
3. 6動作
4. collision
5. debug fixture
6. 因果コア非変更確認

## 42. Phase G0-5 Verification

1. G0 Automation
2. Stage Fまでの全回帰
3. Development package
4. launch smoke
5. performance evidence
6. Computer Use手動確認
7. ユーザー実機確認
8. 検証報告完成

---

# Part XIX：完了報告

## 43. 報告形式

```text
1. Stage F基準点
2. 実装方式
3. 変更ファイル
4. 使用asset一覧
5. placeholder一覧
6. camera設定
7. 8方向判定結果
8. animation mapping
9. occlusion結果
10. collision結果
11. 40 NPC表示結果
12. 牙鼠6動作結果
13. performance実測
14. Stage Fまでの回帰結果
15. package結果
16. Computer Use手動結果
17. ユーザー実機確認待ち項目
18. 既知の問題
19. 未実装
20. git status
```

## 44. 終了条件

自動受入とComputer Use確認が完了したら、ユーザーの実機確認を求めて停止する。

ユーザー実機確認後、検証報告を更新して停止する。

以下を行わない。

```text
commit
tag
push
Stage G本制作
```

---

## 45. 最終原則

Stage G-0で証明するのは、絵の豪華さではない。

```text
3D世界の中で、
2.5D人物と牙鼠が、
3Dの移動・衝突・遮蔽に従って自然に存在し、
既存の因果システムを壊さず動くこと
```

これが成立し、ユーザーが実機画面を承認した場合だけ、後続Stageで王都7区画の本制作へ進む。
