# 国家運営シミュレーションRPG 最小MVP 設計・実装指示書

## 0. 本書の位置付け

本書は、Codex Solが最小MVPを設計・実装するための拘束力を持つ指示書である。

本MVPの目的は、完成版ゲームを作ることではない。  
以下の中核機構が、破綻せず、再現可能な形で連鎖することを検証する。

- プレイヤーの自由行動
- AI NPCごとの固有状態遷移
- AI NPC同士の反応
- NON AI NPCを介した間接的な影響
- 国家・地域状態への波及
- 現実時間経過による自動進行
- セーブ／ロード後の再現性

本書にない機能、演出、設定、最適化、仕様変更を独断で追加してはならない。

---

## 1. 最小MVPの目的

### 1.1 検証対象

1国家内で、20人のAI NPCがそれぞれ固有の判断規則を持ち、プレイヤーの行動、他NPCの行動、経過時間、国家状態に応じて行動と会話を変化させること。

さらに、NON AI NPCに対するプレイヤーの言動をAI NPCが見た、聞いた、または関与した場合、そのAI NPCの判断を経由してゲーム進行へ影響が波及すること。

### 1.2 MVP成功条件

以下が確認できればMVP成功とする。

- 同一入力・同一シード・同一初期状態から同一結果を再現できる
- 20人のAI NPCが個別の状態を保持する
- 各AI NPCが最大255個の固有応答・行動状態を持てる
- プレイヤー行動によりAI NPCの状態が遷移する
- AI NPCの行動が他のAI NPCまたは国家状態へ波及する
- NON AI NPCへの行動が、AI NPCの観測・伝聞・関与を通じて間接的に波及する
- 現実時間経過分の世界更新をログイン時に反映できる
- セーブ／ロード後も状態、関係、イベント履歴が保持される
- 遷移理由と波及経路をログで追跡できる

---

## 2. MVPの範囲

### 2.1 実装するもの

- 国家：1
- AI NPC：20人
- プレイヤー：1人
- NON AI NPC：軽量な生成・配置・イベント媒介機構
- 国家状態
- AI NPC固有の255状態定義
- AI NPC状態遷移
- 会話選択
- 行動選択
- 目撃、伝聞、関与
- イベント伝播
- 現実時間連動
- セーブ／ロード
- デバッグ表示
- 自動テスト
- 状態遷移と因果経路の監査ログ

### 2.2 MVPでは実装しないもの

- 5国家
- 1国家500人のAI NPC
- 5,000万枠の定住型NON AI NPC
- 1億枠の流動型NON AI NPC
- 大規模戦争
- 完成版経済
- 完成版政治
- 王位継承制度
- 完成版冒険者制度
- 完成版商業制度
- 完成版勇者制度
- 3Dフィールド
- 高品質グラフィック
- 音声
- オンライン機能
- LLM API
- 生成AIによる動的台詞生成
- AI NPCの自由文章生成
- 本番用コンテンツ量産
- 最終版向けの大規模最適化

上記は将来拡張対象であり、MVPへ先行実装してはならない。

---

## 3. 絶対条件

### 3.1 LLM禁止

AI NPCの判断、会話、行動、状態遷移にLLM APIを使用してはならない。

以下も禁止する。

- OpenAI API
- Anthropic API
- Gemini API
- 外部推論API
- ローカルLLM
- 埋め込みモデルを使った判断
- 自由生成文章を前提とした設計

AI NPCは完全なルールベースで稼働させる。

### 3.2 AI NPCごとに255状態

255状態は全NPC共通の状態表ではない。

各AI NPCに、個別の255状態枠を割り当てる。

```text
AI_NPC_001
  State 001 ... State 255

AI_NPC_002
  State 001 ... State 255

...

AI_NPC_020
  State 001 ... State 255
```

同じ状態番号であっても、NPCが異なれば意味、応答、行動、遷移先は別物である。

状態番号 `0` は未初期化または無効値として予約し、実状態は `1..255` を使用する。

### 3.3 勝手な共通化禁止

実装効率を理由に、20人のAI NPCへ同一の255状態表を割り当ててはならない。

共通化してよいものは以下のみ。

- イベント種別
- 判定器
- データ構造
- ログ形式
- 遷移エンジン
- 条件式の評価機構
- 汎用アクション実行機構

NPC固有でなければならないものは以下。

- 状態の意味
- 会話
- 行動傾向
- 遷移条件
- 遷移先
- プレイヤー評価への反応
- 他NPCへの反応
- 国家状態への反応

---

## 4. ゲームの基本思想

プレイヤーは最初に所属国家を選ぶ構想だが、MVPでは国家が1つのため選択UIは不要とする。  
ただし、将来複数国家へ拡張できるよう `country_id` は必ず保持する。

プレイヤーの職業や人生経路を開始時に固定しない。

プレイヤーは行動の積み重ねによって、世界から以下のように認識される。

- 冒険者
- 商人
- 勇者候補
- 兵士
- 政治的人物
- 犯罪者
- 権力者
- 民衆の支持者
- 国家の敵

MVPでは各職業システムを完成させない。  
行動履歴、実績、評判、関係値を蓄積し、将来の役割判定に使用できる基盤のみ実装する。

---

## 5. 中核データモデル

## 5.1 Player

最低限、以下を保持する。

```text
player_id
country_id
current_location_id
status
inventory
funds
reputation
crime_record
achievement_tags
action_history_summary
last_login_at
created_at
updated_at
```

`achievement_tags` は固定職業ではなく、行動結果を示すタグとして扱う。

例：

```text
HELPED_CIVILIAN
DEFEATED_BANDIT
SUCCESSFUL_TRADE
ASSAULTED_CIVILIAN
SUPPORTED_GUARD
OPPOSED_AUTHORITY
```

## 5.2 AI NPC

最低限、以下を保持する。

```text
npc_id
country_id
current_location_id
role
faction_id
current_state_id
personality_traits
player_evaluation
relationships
current_goal
memory_summary
known_events
active_action
status
created_at
updated_at
```

`current_state_id` は `1..255`。

## 5.3 AI NPC State Definition

AI NPCごとの各状態は、最低限以下を持つ。

```text
npc_id
state_id
state_name
state_description
dialogue_candidates
action_candidates
transition_rules
goal_modifier
player_evaluation_modifier
relationship_modifiers
world_effect_candidates
time_based_rules
priority
is_terminal
```

状態は単なる感情値ではない。  
会話、次の行動、目標、対人評価、世界への作用をまとめた「応答・行動状態」とする。

## 5.4 Transition Rule

```text
rule_id
source_state_id
trigger_type
conditions
target_state_id
priority
cooldown
once_only
side_effects
```

`conditions` はデータとして定義し、コード内へ個別条件を直書きしない。

## 5.5 NON AI NPC

NON AI NPCは高度な状態遷移を持たない。

最低限、以下を保持またはシードから再生成できるようにする。

```text
non_ai_npc_id
seed
country_id
current_location_id
category
occupation
basic_profile
current_activity
temporary_memory
spawned_at
despawn_policy
```

NON AI NPCは以下を行える。

- 簡易移動
- 簡易会話
- プレイヤー行動の対象になる
- 事件の被害者になる
- 事件を目撃する
- 事件情報を一時保持する
- AI NPCへ事件情報を伝える
- AI NPCが介入する対象になる

NON AI NPC自身が、単独で国家状態、主要クエスト、AI NPC状態を直接変更してはならない。

## 5.6 World State

最低限、以下を持つ。

```text
world_id
world_seed
current_world_time
country_states
location_states
active_events
event_history
population_summary
last_simulated_at
simulation_version
```

## 5.7 Country State

MVPでは1国家のみだが、配列またはマップ構造とする。

```text
country_id
name
stability
security
economy
food
military
public_support
authority
crime_level
active_issues
updated_at
```

MVPでは数値を過度に増やさない。  
上記で連鎖検証に不足する場合のみ、追加理由を示して承認を得る。

---

## 6. イベントモデル

すべての重要な変化はイベントとして記録する。

```text
event_id
event_type
actor_id
actor_type
target_id
target_type
location_id
country_id
occurred_at
observed_by
heard_by
participants
evidence_level
credibility
source_event_id
payload
processed_by
resulting_events
```

### 6.1 必須イベント種別

最低限、以下を用意する。

```text
PLAYER_HELPED_NON_AI_NPC
PLAYER_HARMED_NON_AI_NPC
PLAYER_HELPED_AI_NPC
PLAYER_HARMED_AI_NPC
PLAYER_TRADED
PLAYER_STOLE
PLAYER_ENTERED_LOCATION
PLAYER_LEFT_LOCATION
AI_NPC_OBSERVED_EVENT
AI_NPC_HEARD_EVENT
AI_NPC_INTERVENED
AI_NPC_CHANGED_STATE
AI_NPC_STARTED_ACTION
AI_NPC_COMPLETED_ACTION
RUMOR_CREATED
RUMOR_TRANSFERRED
COUNTRY_STATE_CHANGED
TIME_ELAPSED
OFFLINE_SIMULATION_COMPLETED
```

イベント種別は拡張可能にするが、MVP中に無制限に増やしてはならない。

---

## 7. NON AI NPCからAI NPCへの間接影響

これはMVPの必須検証項目である。

### 7.1 原則

```text
プレイヤーがNON AI NPCへ行動
        ↓
イベント生成
        ↓
AI NPCが目撃、伝聞、または関与
        ↓
AI NPCが事件を解釈
        ↓
AI NPCの状態または目標が変化
        ↓
AI NPCが行動
        ↓
国家、地域、他NPC、プレイヤー評価へ影響
```

### 7.2 直接影響の禁止

以下は禁止する。

```text
プレイヤーがNON AI NPCを殴る
→ 即座に国家治安が低下
```

正しくは以下。

```text
プレイヤーがNON AI NPCを殴る
→ 事件発生
→ 衛兵AI NPCが目撃または通報を受理
→ 衛兵AI NPCが判断
→ 拘束、報告、見逃し、賄賂要求などを選択
→ その結果として国家状態が変化
```

### 7.3 目撃判定

最低限、以下を考慮する。

- 同一場所
- 視認可能範囲
- 時刻
- 障害物または遮蔽の有無を簡易化した可視判定
- AI NPCの注意状態
- 事件の目立ちやすさ

MVPでは高度な物理視線判定は不要。  
データ駆動の簡易判定でよい。

### 7.4 伝聞判定

NON AI NPCは事件情報を一時保持できる。

情報には以下を持たせる。

```text
event_id
original_actor
original_target
event_type
location
occurred_at
credibility
distortion_level
hop_count
```

AI NPCが伝聞を受け取った場合、必ず信じるとは限らない。

AI NPC固有ルールにより、以下を判断する。

- 信じる
- 疑う
- 無視する
- 調査する
- 利用する
- 他者へ伝える
- プレイヤーへ確認する

### 7.5 関与判定

AI NPCが事件へ関与した場合、役割と状態に応じた行動を取る。

例：

```text
衛兵        → 制止、拘束、報告
悪徳役人    → 賄賂要求、揉み消し
商人        → 取引拒否、噂の拡散
聖職者      → 被害者保護、仲裁
犯罪者      → 加担、勧誘、脅迫
一般有力者  → 政治利用、告発
```

これらは固定コード分岐ではなく、AI NPC固有状態と行動候補から決定する。

---

## 8. AI NPC判断処理

AI NPCの判断は、以下の順序で行う。

```text
1. 入力イベント取得
2. 自分が当事者、目撃者、伝聞受信者、無関係か判定
3. 現在状態を取得
4. 状態に紐づく遷移候補を取得
5. 条件評価
6. 優先順位評価
7. 同点時のみ決定論的乱数を使用
8. 次状態を確定
9. 会話候補を選択
10. 行動候補を選択
11. 副作用イベントを生成
12. ログへ記録
```

### 8.1 決定論的乱数

ランダム要素は使用可能だが、再現可能でなければならない。

乱数シードは最低限、以下から生成する。

```text
world_seed
npc_id
event_id
current_state_id
simulation_tick
```

システム時刻を直接乱数へ使用してはならない。

### 8.2 優先順位

同一イベントで複数の遷移条件が成立する場合、明示された `priority` に従う。

優先順位が同じ場合のみ決定論的乱数を使用する。

### 8.3 無限ループ防止

1イベント処理内で許可する連鎖回数に上限を設ける。

上限到達時は握り潰さず、以下を記録する。

```text
CHAIN_LIMIT_REACHED
root_event_id
processed_event_count
remaining_event_count
last_processed_actor
```

上限値は設定ファイル化する。

---

## 9. 現実時間連動

### 9.1 基本方式

ゲームを終了している間、常時プロセスを動かす必要はない。

次回起動時に以下を行う。

```text
last_simulated_at取得
↓
現在時刻との差分取得
↓
経過時間をゲーム内時間へ変換
↓
期間内に発生する時間イベントを算出
↓
AI NPC、国家状態、進行中イベントを更新
↓
結果を時系列ログへ記録
```

### 9.2 時間倍率

現実時間とゲーム内時間の倍率は設定ファイル化する。  
コード内へ固定しない。

### 9.3 オフライン処理

オフライン中の全秒を逐次シミュレーションしてはならない。

以下を使い分ける。

- 重要イベント：個別処理
- 定期行動：時間区間単位で処理
- 単純な数値変化：集約処理
- 関係変化：対象イベント発生時のみ処理

### 9.4 オフライン結果

起動時に最低限、以下を確認できるようにする。

```text
経過した現実時間
経過したゲーム内時間
発生した重要イベント
状態変化したAI NPC
国家状態の変化
未解決イベント
```

---

## 10. 会話システム

会話は自由生成しない。

各AI NPCの状態ごとに、複数の会話候補をデータとして保持できるようにする。

```text
dialogue_id
npc_id
state_id
trigger_type
conditions
text
tone
priority
once_only
cooldown
```

### 10.1 会話選択

会話は以下で選択する。

```text
現在状態
直前イベント
プレイヤー評価
関係値
場所
時刻
既出履歴
```

同一文章の過剰反復を防ぐため、直近使用履歴を保持する。

### 10.2 会話と行動の分離

会話表示だけで処理を終えてはならない。

会話と行動は別データとして扱う。

```text
会話：「ここで騒ぎを起こすな」
行動：警告状態へ遷移
行動：再犯時の拘束ルールを有効化
```

---

## 11. 行動システム

AI NPCの行動候補は、最低限以下を表現できるようにする。

```text
MOVE
TALK
WARN
HELP
ATTACK
ARREST
REPORT
INVESTIGATE
TRADE
REFUSE_TRADE
SPREAD_RUMOR
PROTECT
ESCORT
FLEE
JOIN_FACTION
LEAVE_FACTION
CHANGE_GOAL
CHANGE_RELATIONSHIP
CHANGE_COUNTRY_STATE
WAIT
```

`CHANGE_COUNTRY_STATE` は直接自由に実行させず、許可された効果定義を介して実行する。

---

## 12. 関係性

AI NPCは最低限、以下の関係を持てるようにする。

```text
trust
fear
respect
hostility
loyalty
debt
```

すべてを必須表示する必要はないが、内部状態として保持可能にする。

関係値の変更には必ず理由イベントを紐づける。

```text
relationship_change
source_event_id
old_value
new_value
reason
```

---

## 13. 国家状態への波及

国家状態の変化は、必ずAI NPCまたは国家システムの行動結果から生じる。

例：

```text
プレイヤーが一般人を襲う
→ 衛兵AI NPCが目撃
→ 拘束に失敗
→ 衛兵AI NPCが上官へ報告
→ 警戒命令発令
→ security変化
→ crime_level変化
```

国家状態変更には以下を記録する。

```text
country_id
parameter
old_value
new_value
source_event_id
actor_id
reason
```

---

## 14. データ駆動

以下は外部データとして定義し、コードへ埋め込まない。

- AI NPC定義
- 255状態定義
- 遷移ルール
- 会話
- 行動候補
- 国家初期値
- 場所
- イベント種別設定
- 時間倍率
- 連鎖上限
- 各種しきい値

形式は、既存リポジトリの技術構成に合わせる。  
新規プロジェクトで技術構成が未確定の場合、Codex Solは独断でエンジン、言語、フレームワークを選定せず、候補と理由を提示して停止する。

---

## 15. セーブ／ロード

最低限、以下を保存する。

- プレイヤー状態
- 20人のAI NPC状態
- AI NPC間関係
- AI NPCの既知イベント
- NON AI NPCの一時イベント情報
- 国家状態
- 世界時刻
- 最終シミュレーション時刻
- 進行中イベント
- 使用済み乱数シード情報または再現に必要な情報
- 会話使用履歴
- シミュレーションバージョン

### 15.1 保存の一貫性

保存途中で失敗しても、直前の正常保存を破壊してはならない。

### 15.2 バージョン

セーブデータに `simulation_version` と `schema_version` を持たせる。

---

## 16. 監査ログ

MVPでは演出より監査可能性を優先する。

最低限、以下を出力する。

```text
timestamp
simulation_tick
event_id
root_event_id
actor
target
AI NPC current_state
matched_rules
selected_rule
next_state
selected_dialogue
selected_action
generated_events
country_state_changes
random_seed
```

以下の問いへ答えられること。

- なぜこのNPCは怒ったのか
- なぜこの会話を選んだのか
- なぜ逮捕行動を取ったのか
- どの事件が国家状態変化の原因か
- NON AI NPCの事件がどの経路でAI NPCへ伝わったか
- オフライン中に何が起きたか

---

## 17. 最小デバッグ画面

本番UIは不要。  
既存プロジェクトにUIがない場合、最小限のCLIまたはデバッグUIでよい。

最低限、以下を操作・確認できること。

- プレイヤー行動を選ぶ
- 対象NPCを選ぶ
- 時間を進める
- セーブする
- ロードする
- AI NPC現在状態を見る
- AI NPCのプレイヤー評価を見る
- AI NPC同士の関係を見る
- 国家状態を見る
- イベント履歴を見る
- 因果経路を見る
- オフライン経過を擬似実行する

本番向けの画面デザインを行ってはならない。

---

## 18. 必須テストシナリオ

### Scenario A：NON AI NPCへの善行

```text
1. プレイヤーがNON AI NPCを助ける
2. AI NPCは現場を直接見ていない
3. NON AI NPCがAI NPCへ事件を伝える
4. AI NPCが情報を信じる
5. AI NPCのプレイヤー評価が上昇
6. AI NPCの会話または行動が変化
```

### Scenario B：NON AI NPCへの暴力を目撃

```text
1. プレイヤーがNON AI NPCを攻撃
2. 衛兵役AI NPCが目撃
3. 衛兵AI NPCの状態が変化
4. 警告、拘束、報告のいずれかを実行
5. 結果が国家状態またはプレイヤー犯罪記録へ波及
```

### Scenario C：伝聞を疑うAI NPC

```text
1. NON AI NPCが事件を伝える
2. AI NPC固有ルールにより信頼性を疑う
3. 即時処罰せず調査行動を選ぶ
4. 調査結果により後続状態が変化
```

### Scenario D：AI NPC同士の連鎖

```text
1. AI NPC Aが事件を認識
2. AI NPC AがAI NPC Bへ報告
3. AI NPC Bが別の判断を行う
4. AI NPC Bの行動が国家状態へ影響
```

### Scenario E：現実時間経過

```text
1. 保存
2. 擬似的に現実時間を進める
3. 再起動またはロード
4. 経過時間分の処理を実行
5. 重要イベントと状態変化を表示
```

### Scenario F：再現性

```text
1. 同一初期データ
2. 同一world_seed
3. 同一プレイヤー入力
4. 同一時間経過
5. 複数回実行
6. 結果とログが一致
```

### Scenario G：セーブ／ロード一貫性

```text
1. イベント連鎖途中で保存
2. ロード
3. 継続処理
4. 保存しなかった連続実行と結果が一致
```

---

## 19. 受入基準

以下をすべて満たすこと。

### 19.1 機能

- [ ] 1国家を生成できる
- [ ] 20人のAI NPCを生成できる
- [ ] 各AI NPCが独立した255状態枠を持つ
- [ ] 状態定義がデータ駆動である
- [ ] プレイヤー行動からイベントを生成できる
- [ ] AI NPCが目撃できる
- [ ] AI NPCが伝聞を受け取れる
- [ ] AI NPCが事件へ関与できる
- [ ] AI NPCの状態が変化する
- [ ] 会話が状態に応じて変化する
- [ ] 行動が状態に応じて変化する
- [ ] AI NPC同士でイベントが伝播する
- [ ] 国家状態へ波及する
- [ ] NON AI NPCが単独で国家状態を直接変更しない
- [ ] 現実時間経過を反映できる
- [ ] セーブ／ロードできる
- [ ] 同一条件で結果を再現できる
- [ ] 因果経路をログで追跡できる

### 19.2 品質

- [ ] LLMを使用していない
- [ ] 20人へ共通の255状態表を流用していない
- [ ] 個別NPC条件をコードへ大量直書きしていない
- [ ] 無限イベント連鎖を防止している
- [ ] エラーを握り潰していない
- [ ] セーブデータ破損対策がある
- [ ] テストが自動実行できる
- [ ] 未実装箇所を実装済みと報告しない
- [ ] 受入基準ごとに証拠を提示できる

---

## 20. Codex Solの作業手順

### Phase 0：リポジトリ確認

最初に以下を確認する。

- 使用言語
- 使用フレームワーク
- 既存ゲームエンジン
- 既存データ形式
- 既存テスト環境
- 既存セーブ形式
- 既存UI
- 既存CI
- 関連設計書
- 変更禁止領域

確認結果を報告する。

既存技術構成が存在する場合、それを優先する。  
存在しない場合、技術選定案を提示して停止し、承認前に実装を始めない。

### Phase 1：設計固定

以下を作成する。

- データモデル
- イベントモデル
- 状態遷移モデル
- 保存モデル
- シミュレーション処理順
- テスト構成
- ディレクトリ構成案

本書との対応表を示す。

### Phase 2：最小縦切り実装

最初に20人全員を作らない。

以下の縦切りを完成させる。

```text
プレイヤー
AI NPC 2人以上
NON AI NPC
目撃
伝聞
状態遷移
会話変化
行動変化
国家状態変化
保存
再現性テスト
```

縦切りが受入基準を満たした後、20人へ拡張する。

これは仕様削減ではなく、実装順序の指定である。

### Phase 3：20人へ拡張

- 20人分の固有状態定義を読み込めるようにする
- 各NPCへ固有の遷移を設定する
- 全員同一挙動になっていないことを検査する
- NPC間連鎖を検証する

MVPでは255状態すべてへ大量の完成版会話を埋める必要はない。  
ただし、各NPCが255状態を保持・読込・遷移可能な構造は完成させる。

未使用状態は明示的に `UNDEFINED` とし、使用時にエラーを出す。  
未定義状態を黙って初期状態へ戻してはならない。

### Phase 4：受入試験

本書18章および19章を実行し、結果を提出する。

---

## 21. 禁止事項

Codex Solは以下を行ってはならない。

- 本書外の機能追加
- ゲームジャンルの変更
- LLM導入
- AI NPC数の削減
- 255状態を共通表へ置換
- 255状態を単純な感情値だけへ縮退
- NON AI NPCの媒介機能削除
- 目撃と伝聞の統合
- イベントログ削除
- 再現性を犠牲にした乱数利用
- 本番UI制作
- 大規模国家実装
- 5,000万または1億NPC枠の先行実装
- 未承認ライブラリの大量導入
- 未承認のデータベース導入
- セーブ形式の独断決定
- テスト省略
- 未配線コードの提出
- ダミー実装を完成扱い
- TODOを残したまま完了報告
- 仕様不明点の推測補完
- 既存コードの無断削除
- 既存設計の無断変更

---

## 22. 報告形式

各作業報告は以下の形式にする。

```text
1. 実施内容
2. 変更ファイル
3. 本書の対応項目
4. 実装済み
5. 未実装
6. テスト結果
7. 既知の問題
8. 仕様確認が必要な点
9. 次に実施する内容
```

「完了」と報告する場合、受入基準のチェック結果と証拠を添付する。

---

## 23. 仕様確認が必要な項目

以下は本書では確定しない。  
既存リポジトリから判断できない場合、実装前に確認する。

- 使用言語
- ゲームエンジン
- UI方式
- データ保存方式
- AI NPC 20人の人物設定
- 各NPCの255状態の具体的内容
- 国家名
- 地域数
- 時間倍率
- プレイヤーがMVPで実行できる行動一覧
- 国家状態パラメータの初期値
- NON AI NPCのMVP同時出現数
- 目撃範囲
- 噂の伝播速度
- オフライン処理の最大対象期間

不明点を推測で埋めてはならない。

---

## 24. 将来拡張のための境界

MVPの内部構造は、将来以下へ拡張できるようにする。

```text
1国家
20 AI NPC
軽量NON AI NPC
```

から、

```text
5国家
1国家あたり500 AI NPC
5,000万枠の定住型NON AI NPC
1億枠の流動型NON AI NPC
```

へ拡張する。

ただし、MVP時点で巨大人口を生成・保存・常時処理してはならない。

将来版では以下を前提とする。

- AI NPC：永続個体として保存
- NON AI NPC：ID／シード空間から必要時生成
- 遠隔人口：個体処理ではなく統計処理
- 画面周辺：必要な個体だけ実体化
- 重要事件：AI NPCの認識を通じて進行へ反映

MVPでは、この拡張を妨げない境界設計のみ行う。

---

## 25. 最終定義

本ゲームの中核は、単なる自由度ではない。

```text
プレイヤーは自由に行動できる。
AI NPCも自分の状態と目的に従って反応する。
国家もプレイヤーの都合を待たず進行する。
NON AI NPCへの行動も、AI NPCへ届けば結果を生む。
```

最小MVPでは、この因果連鎖が技術的に成立することだけを証明する。

完成版の規模、演出、コンテンツ量を先に作ってはならない。
