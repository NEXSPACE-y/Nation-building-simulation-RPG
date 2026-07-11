# Stage F Production Scale Runtime Foundation 設計

## 1. 目的と境界

Stage Fは、Stage Eの因果・状態・表示を維持したまま、5国家、2,500 AI NPC、637,500状態枠、350,000,000 NON AI人口枠を扱うランタイム基盤である。生成コンテンツはすべて`fixture_only: true`、`production_content: false`であり、本番世界設定ではない。

既存`nation_sim::Simulation`は、画面へ表示するcountry_001、既存20 AI、既存20 NON AIの因果正本として変更せず利用する。`nation_sim::StageFProductionRuntime`は、その外側で国家partition、規模runtime、activity tier、due scheduler、人口仮想化、世代保存、ログsegmentを管理する。Unrealはlifecycle、入力、40 Actor、UMGだけを担当し、判断規則を持たない。

## 2. モジュール境界

| モジュール | 責務 |
|---|---|
| `src/simulation.cpp` | Stage A～Eの可視因果コア |
| `src/stage_f_runtime.cpp` | Stage F partition、scheduler、population、save/recovery |
| `StageFScaleDataGenerator` | 5国家・2,500 NPC・637,500状態fixture生成 |
| `StageFDataPackValidator` | schema、件数、SHA、索引、状態規則の検査 |
| `UNationSimulationGameInstanceSubsystem` | 2つのコアの所有とUnreal lifecycle接続 |
| Actor / HUD | 入力・表示。判断、遷移、routingは行わない |

## 3. データ分割

`world_manifest → country manifest → AI manifest → state shard`の参照階層とし、`state_index`と`definition_hash_manifest`を別に持つ。状態shardはNPC単位で255枠を格納する。状態は要求されたNPCだけ読み、SHA-256を照合してからLRU cacheへ置く。cache上限は32 shardである。

## 4. AI activity tierとscheduler

- ACTIVE: 既存20 NPC。可視因果コアのevent入力時に処理する。
- BACKGROUND: `next_due_time`を持ち、`(due_time, country_id, npc_id)`順のordered setで期限到来時だけ処理する。
- DORMANT: dueを持たず、eventや昇格要求がない限り処理しない。

本番経路は2,500 NPCの毎tick走査を行わない。受入専用の`all_active_reference_mode`だけが2,500件を全走査し、階層処理とのcanonical hash一致を検証する。

## 5. country partitionと決定論

AI、event、country state、schedule、save shard、audit rowは`country_id`を持つ。国内eventはsourceとtarget countryの一致を必須とする。国家間伝播は`STAGE_F_FIXTURE_CROSS_COUNTRY`だけが明示targetへ届く。

同時処理順は`scheduled_at → target_country_id → priority → event_id → actor_id`で固定する。map/setの順序、固定world seed、SHAで識別されたscale data、simulation versionをcanonical snapshotへ含める。並列workerは導入していないためworker数差分は存在しない。

## 6. NON AI virtualization

定住人口は各国家50,000,000 index、流動人口は100,000,000 indexとして表現する。`world_seed | country_id | population_class | index`のSHA-256からID、seed、fixture属性を再生成する。近傍MATERIALIZEDは最大512件、重要事件対象はPROMOTEDとして永続化する。全人口ID、runtime、saveは生成しない。

## 7. 保存世代、migration、log segment

論理slotは従来どおり`stage_d_save.json`である。Stage Fでは同ファイルをcurrent generation manifestとし、world、5 country、5 AI runtime、promoted NON AI、pending event、visible Stage E core、timestamp、audit segmentを世代directoryへ分割する。全shard読戻しとSHA検証後だけcurrentを切り替える。

Stage E saveは元saveとmetadataをSHA付き不変名へバックアップし、元JSONを`visible_core.json`として無変更で保持する。これによりplayer、country_001、既存20 AIの全詳細、pending、event history、audit、causal chainを維持する。新規country_002～005とAI 021～2500はscale fixture既定値で追加する。

audit/causal rowは1,000件でsegment化し、segment SHAと範囲索引を保存する。hot logはsegment化後に解放し、`archived_causal_path`が保存世代内のsegmentから経路を復元する。

## 8. 計算量

- 規模runtime load: O(2,500)。637,500状態本文は読まない。
- state参照: index O(1)、shard内配列 O(1)、cache上限32。
- pending event: ordered multisetの挿入・削除 O(log E)。
- background due: 到来件数Kに対してO(K log B)。
- NON AI materialize: O(1)、常駐最大512。
- save: 2,500軽量AI runtimeと現在のpromoted/pending/log segmentに比例する。
- offline: 全秒逐次ではなくdue件数と時間区間数に比例する。

## 9. Stage E互換境界

共有fixture、共有受入契約、Stage E overlay、既存期待値は変更しない。Stage E状態1～25は生成器が元JSON objectをそのまま複製する。Stage Fへの入口はstate object外の`external_transition_rules`へ置き、既存state objectを改変しない。Unreal画面はStageD_Capital、Third Person操作、20 AI＋20 NON AIを維持する。
