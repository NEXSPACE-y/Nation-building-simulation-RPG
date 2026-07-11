# Stage F 保存・復旧設計

## 1. 論理slotとschema

ランタイム保存先は既存の`Saved/SaveGames/stage_d_save.json`を継続使用する。このファイルは`stage_f_save_schema_v1 / stage-f-0.1.0`のcurrent generation manifestであり、Stage F専用slotへ分離しない。

各世代は`stage_f_generations/gen_NNNNNNNNNNNN/`にworld、countries、AI runtime、promoted NON AI、pending events、visible core、timestamp、audit segments、generation manifestを持つ。

## 2. transaction

1. `.tmp`世代とincomplete markerを作る。
2. 全shardを書く。
3. audit segmentを世代内へコピーする。
4. shard SHA付きgeneration manifestを書く。
5. 全shardを読戻し、SHAを再計算する。
6. `.tmp`を完成世代名へrenameする。
7. current manifestの`.next`を読戻す。
8. 旧currentを世代ID＋SHA付き不変backupへ保存する。
9. currentを原子的renameで切り替える。

失敗時は成功を返さず、旧currentを維持する。起動時はcurrent世代をSHA検証し、破損時は新しい順に直前正常世代を選ぶ。`.tmp`世代数は`INCOMPLETE_GENERATIONS`監査markerへ記録する。

## 3. Stage E migration

Stage D saveは既存`FStageESaveMigration`でStage Eへ上げた後、Stage Fへ移行する。Stage E saveとmetadataは、それぞれsource SHAを含む不変名へcopyし、同名存在時は内容SHA一致だけを許可して上書きしない。

Stage E JSON全文は`visible_core.json`へ保持するため、player、country_001、既存20 AIのstate、relationships、evidence、timers、used rules、pending、history、audit、causal chainを失わない。Stage F軽量runtimeには既存20 AIの現在値を投影し、残る2,480 AIと4国家をfixture既定値で追加する。migration recordはsource schema、simulation version、save SHA、metadata SHA、両backup pathを記録する。

移行中に失敗した場合、元saveとmetadataへ書込みを行わない。Stage F schemaを検出した再実行は二重移行しない。

## 4. failure injection

次の6点を自動試験する。

- `BEFORE_SHARD_WRITE`
- `DURING_SHARD_WRITE`
- `BEFORE_MANIFEST_WRITE`
- `DURING_MANIFEST_WRITE`
- `BEFORE_CURRENT_SWITCH`
- `AFTER_SWITCH_READBACK_FAILURE`

各試験は失敗結果、旧current byte一致、generation 1再ロード、監査messageを証拠化する。

## 5. log segment

hot logは1,000 rowごとにJSONL segmentへ確定し、segment ID、country、event/tick範囲、件数、SHAを索引化する。segment本体もsave shardとしてhash検証する。ロード後はsegment pathを復元し、root_event_idによる因果経路検索を継続できる。
