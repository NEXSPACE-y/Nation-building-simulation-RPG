# Stage F 性能・安定性検証報告

## 1. 検証条件

| 項目 | 実測値 |
|---|---|
| OS | Windows 11 |
| CPU | AMD64 Family 25 Model 33、16 logical cores |
| RAM | 34,272,993,280 bytes |
| compiler | MSVC 19.50.35728 |
| Unreal | 5.8.0 |
| core build | Release |
| package build | Win64 Development |
| worker count | 1 |
| dataset SHA-256 | `d8fe2891b6faf63b703f417a97c79a93d4adbd13ce90287e0deae939984ca2b3` |

生成scale packは5,010ファイル、879,817,053 bytesである。国家manifest 5、AI manifest 2,500、state shard 2,500を含む。

## 2. F-P1 完成規模ロード

| 項目 | 結果 |
|---|---:|
| 国家 | 5 |
| AI NPC | 2,500 |
| 状態枠 | 637,500 |
| 仮想NON AI人口枠 | 350,000,000 |
| load時間 | 409 ms |
| load後working set | 8,630,272 bytes |
| 判定 | PASS |

world manifest、country manifest、AI manifest、state index、definition hash manifest、population spaceのSHAと件数を読込時に検証した。状態本文は必要NPCだけを遅延ロードし、cache上限は32 shardである。

## 3. F-P2 イベント負荷

決定論的fixture event 10,000件を228 msで処理した。処理件数10,000、pending 0で、全root eventは終端した。既存因果コアの`CHAIN_LIMIT_REACHED`監査試験も10/10回帰内でPASSした。

canonical SHA-256:

```text
7a222cd4af12fa49c46e07dcf4b80e7ad18a0db96e60a9d44f399a71afaed328
```

## 4. F-P3 オフライン7日

現実604,800秒を区間集約で処理し、全秒逐次処理は行っていない。

| 項目 | 実測値 |
|---|---:|
| due action | 833,280 |
| country interval | 50,400 |
| population interval | 10,080 |
| 計測時間 | 0 ms未満 |
| canonical SHA-256 | `424bbd00310db4117884a88bce43b38e55db78efda33b0f020216575868a3e17` |
| 判定 | PASS |

## 5. F-P4 保存・ロード100回

100世代を保存・再読込し、各回でcanonical snapshotを比較した。

| 項目 | 実測値 |
|---|---:|
| 反復 | 100 |
| 所要時間 | 77,372 ms |
| hash不一致 | 0 |
| 最終世代 | 100 |
| 判定 | PASS |

AI runtimeはscale manifest読込時に確保した固定2,500 IDへインプレース復元する。監査segmentは固定64 KiBバッファのストリーミングSHAで検証し、世代間コピー時に全segmentをメモリへ常駐させない。

## 6. F-P5 中断復旧

次の6点すべてで新世代を拒否し、直前正常世代を維持した。

- `BEFORE_SHARD_WRITE`
- `DURING_SHARD_WRITE`
- `BEFORE_MANIFEST_WRITE`
- `DURING_MANIFEST_WRITE`
- `BEFORE_CURRENT_SWITCH`
- `AFTER_SWITCH_READBACK_FAILURE`

各結果は`STAGE_F_SAVE_GENERATION_REJECTED`として監査可能で、判定は6/6 PASSである。

## 7. F-P6 30分soak

headless完成規模runtimeを1,800秒連続実行した。warmupは前半900秒、判定区間は後半900秒である。

| 項目 | 実測値 |
|---|---:|
| events | 16,054 |
| save/load | 29 |
| warmup working set | 13,619,200 bytes |
| 終了working set | 14,163,968 bytes |
| warmup後増加率 | 4.0% |
| 最大working set | 17,117,184 bytes |
| 終了pending | 0 |
| crash | 0 |
| deadlock | 0 |
| canonical SHA-256 | `177450713d4b8284e58f4466f10b00b4e33fb5fc9521e71bcdf206185734211d` |
| 判定 | PASS |

保存直後には一時的なworking setピークがあるが、固定バッファによるSHA検証後に解放され、終了値はwarmup比4.0%である。queueは各iterationで0を検査し、恒常増加はなかった。

## 8. F-P7 worker一致

並列処理は導入していない。production pathと検証はworker 1であり、本項目は適用外である。activity tier処理と全ACTIVE参照処理の一致は別試験で確認した。

## 9. 証拠

- `out/stage-f/performance/stage_f_performance.json`
- `out/stage-f/performance/stage_f_soak.json`
- `out/stage-f/test-output/stage_f_determinism_evidence.json`
- `out/stage-f/test-output/stage_f_offline_evidence.json`
- `out/stage-f/test-output/stage_f_save_recovery_evidence.json`
- `out/stage-f/package_launch.txt`

