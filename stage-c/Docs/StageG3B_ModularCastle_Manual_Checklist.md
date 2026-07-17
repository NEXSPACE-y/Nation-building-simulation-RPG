# Stage G-3B-R 王城専用モジュラー構築 実機確認チェックリスト

確認者：豆虎Gate　確認日：2026-07-17 JST

## 事前技術検証（Codex）

- [x] G-3B-R Automation 33/33 PASS
- [x] NavMesh route 5/5 PASS
- [x] 実PLAYER_M移動 4,330.6uu PASS
- [x] Camera Collision 16/16 PASS
- [x] Stage C～G-3A全回帰PASS
- [x] Win64 Development Package PASS
- [x] 引数なしG-3B-R map起動PASS
- [x] G-3A／G-2B／G-2A／G-1B明示map起動PASS
- [x] PLAYER_M fallback=false
- [x] GUARD_M専用Skeleton維持
- [x] 関連プロセス0

## 起動

- [x] Win64 Development Packageが引数なしで起動する
- [x] `StageG3B_ModularCastle_PoC`が表示される
- [x] PLAYER_Mが表示され、Mannyへ戻っていない
- [x] GUARD_Mが1体だけ存在する

## StandardCharacterCamera

- [x] 正門前から見て王城として輪郭を判別できる
- [x] 中央塔、左右塔、翼棟、門洞の前後差が読める
- [x] 巨大な一枚壁が画面を占有しない
- [x] プレイヤー移動中にカメラYawが進行方向へ自動追従する
- [x] 右ドラッグ中は手動操作が優先される
- [x] 右ドラッグ終了後に自動追従へ戻る
- [x] 王城壁・門付近でCamera Collisionが機能する

## TacticalOverlookCamera

- [x] F6で戦略俯瞰へ切り替えられる
- [x] 王城が王都北端のlandmarkとして読める
- [x] Tactical中は自動Yaw追従しない
- [x] F6でStandardへ戻れる
- [x] Standardへ戻った後に自動追従が再開する

## 王城外観

- [x] 正門が貼り付け模様ではなく奥行きのある門洞に見える
- [x] 左右塔が黒い直方体ではなく八角塔として読める
- [x] 中央塔が奥側・高所の主塔として読める
- [x] 急勾配屋根と尖塔がsilhouetteを作っている
- [x] 12本以上の扶壁で壁面に縦のリズムがある
- [x] 10個以上の奥まった尖頭窓が見える
- [x] 胸壁が巨大ブロックではなく小さい反復になっている
- [x] 石壁が明るい灰色～青灰色で、黒い塊になっていない
- [x] PLAYER_M比較で人間スケールを把握できる
- [x] LEGO／Minecraft／豆腐建築に見えない

## 移動と既存機能

- [x] 左クリック移動ができる
- [x] 左クリック保持移動ができる
- [x] 王城前広場から正門までNavPathが成立する
- [x] 正門開口を通過できる
- [x] WALK／RUN／DASHを切り替えられる
- [x] 移動中切替で目的地／NavPathを維持する
- [x] 到着後にIdle_11へ戻る

## 豆虎Gate判定

- [x] 外観：PASS
- [x] 表示基準：PASS
- [x] 移動・Camera：PASS
- [x] Stage G-3B-R正式承認：PASS

2026-07-17 JST、豆虎Gateにより正式承認。以後、指定されたcommit／tag／pushを実施可能とする。
