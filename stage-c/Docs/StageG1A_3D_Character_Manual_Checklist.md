# Stage G-1A 標準3Dキャラクター 実機確認チェックリスト

Package: `out/stage-g1a/package/Windows/NationSimulationStageC.exe`

## 初回ユーザー実機確認

```text
初回ユーザー実機確認：
WALK／RUN切替手段が存在しないため、
WALKのみ確認不能。
その他の項目はPASS。
```

| # | 確認項目 | 判定 | 観察メモ |
|---:|---|---|---|
| 1 | 引数なし起動でG-1A専用mapが開く | PASS | 初回確認 |
| 2 | 標準3D Playerが表示される | PASS | 初回確認 |
| 3 | Characterと240uu扉の縮尺が自然 | PASS | 初回確認 |
| 4 | 左クリックで任意方向へ移動 | PASS | 初回確認 |
| 5 | 左クリック保持で目的地を更新 | PASS | 初回確認 |
| 6 | WASDでは通常移動しない | PASS | 初回確認 |
| 7 | カメラがPlayerへ自動追従 | PASS | 初回確認 |
| 8 | 停止時にIDLE | PASS | 初回確認 |
| 9 | 低速域にWALK | PENDING | 切替手段がなく確認不能 |
| 10 | 通常移動中にRUN | PASS | 初回確認 |
| 11 | 足滑りが受入可能 | PASS | 初回確認（RUN） |
| 12 | 方向転換が自然で瞬間回転しない | PASS | 初回確認 |
| 13 | 緩斜面を移動可能 | PASS | 初回確認 |
| 14 | 4段階段を移動可能 | PASS | 初回確認 |
| 15 | dynamic/contact shadowが自然 | PASS | 初回確認 |
| 16 | 足裏と床の接地が自然 | PASS | 初回確認 |
| 17 | 壁・柱による遮蔽が破綻しない | PASS | 初回確認 |
| 18 | 代表3D NPCをクリック選択可能 | PASS | 初回確認 |
| 19 | map外へ落下しない | PASS | 初回確認 |
| 20 | 通常UIが小型で見やすい | PASS | 初回確認 |

## WALK/RUN是正後ユーザー再確認

| # | 確認項目 | 判定 | 観察メモ |
|---:|---|---|---|
| 1 | 起動時がWALK | PASS | 是正後ユーザー実機確認でCLEAR |
| 2 | WALKアニメーションが自然 | PASS | 是正後ユーザー実機確認でCLEAR |
| 3 | WALK時の足滑りが受入可能 | PASS | 是正後ユーザー実機確認でCLEAR |
| 4 | ボタンでRUNへ切替可能 | PASS | 是正後ユーザー実機確認でCLEAR |
| 5 | RUNアニメーションが自然 | PASS | 是正後ユーザー実機確認でCLEAR |
| 6 | RUN時の足滑りが受入可能 | PASS | 是正後ユーザー実機確認でCLEAR |
| 7 | 移動中の切替で目的地を失わない | PASS | 是正後ユーザー実機確認でCLEAR |
| 8 | WALKへ戻せる | PASS | 是正後ユーザー実機確認でCLEAR |
| 9 | 到着後IDLEへ戻る | PASS | 是正後ユーザー実機確認でCLEAR |
| 10 | UIが見やすい | PASS | 是正後ユーザー実機確認でCLEAR |

## 承認

- 初回ユーザー実機確認: WALKのみ確認不能、その他PASS
- 是正後ユーザー実機確認: 10/10 PASS
- Stage G-1A総合ユーザー実機確認: PASS
- 既知の受入阻害問題: なし
- Stage G-1A完了承認: PASS
