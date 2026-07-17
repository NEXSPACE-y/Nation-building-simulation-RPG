# Stage G-3B-R 王城専用モジュラー構築 検証報告

## 現在状態

- Stage：G-3B-R Modular Castle Exterior PoC
- 基準点：`0505735ddd65c494986c0eca11d77e813e0b6aa5`
- 正式map：`/Game/Maps/StageG3B_ModularCastle_PoC`
- 豆虎Gate実機確認日：2026-07-17 JST
- 外観承認：PASS
- Stage G-3B-R正式承認：PASS
- commit／tag／push：未実施

## 技術結果

| 項目 | 結果 |
|---|---|
| 旧試行保全 | 5,284件、SHA-256 PASS |
| 自作FBX module | 22種 |
| 外部素材 | 0 |
| imported Content Asset | 33件 |
| G-3B-R SourceArt | 47件 |
| 最大ファイル | 1,238,240 bytes |
| 50MB超Asset | 0 |
| 壁ベイ | 11 |
| 門開口 | 600×565uu |
| 門洞奥行き | 380uu |
| 扶壁 | 14 |
| 尖頭窓 | 20 |
| 急勾配屋根／尖塔 | 13 |
| 胸壁module | 11 |
| 旗 | 5 |
| 魔法marker | 3 |
| GUARD_M | 1体 |
| PLAYER_M fallback | false |
| GUARD_M Skeleton共有 | false |
| NavMesh route | 5/5 PASS |
| 実移動距離 | 4,330.6uu PASS |
| Camera Collision | 16/16 PASS |
| G-3B-R Automation | 33/33 PASS |
| 全回帰 | PASS |
| Win64 Development Package | PASS |

## 回帰結果

| Stage | 結果 |
|---|---:|
| Stage C | 11/11 PASS |
| Stage D | 9/9 PASS |
| Stage D Fix | 8/8 PASS |
| Stage F | 5/5 PASS |
| Stage G-0 | 33/33 PASS |
| Stage G-1A | 22/22 PASS |
| Stage G-1B | 30/30 PASS |
| Stage G-2A | 18/18 PASS |
| Stage G-2B | 18/18 PASS |
| Stage G-3A | 29/29 PASS |
| Stage G-3B-R | 33/33 PASS |

## Package結果

- executable：`out/stage-g3br-modular-castle/package/Windows/NationSimulationStageC.exe`
- SHA-256：`7931C90FA1B8C11845D8A6782B02B03BEF463DE800FFBB7DA258F37D1E2BCC64`
- 引数なしG-3B-R map：PASS
- G-3A明示map：PASS
- G-2B明示map：PASS
- G-2A明示map：PASS
- G-1B明示map：PASS
- 関連プロセス：0
- commit／tag／push：未実施

## 視覚判定

次の16画像を同一成果物群として確認する。

1. Tactical全景
2. Standard進入
3. Standard正門
4. 王城前広場
5. 左右塔正面
6. 門洞奥行き
7. 壁面細部
8. 側面silhouette
9. 魔法marker
10. 正門Camera Collision
11. 壁Camera Collision
12. 広場→門Nav route
13. Tactical復帰
14. Standard自動追従復帰
15. PLAYER_M尺度比較
16. 王都市街入口からの正門

2026-07-17 JST、豆虎Gate目視PASSによりStage G-3B-Rを正式承認した。

## 既知の制限

- 中精度PoCのため石・屋根は無地Materialで、高精細Textureは未使用。
- 内装、扉開閉、NPC追加、戦闘、LOD本制作は未実装。
- 16画像とPackageは技術証拠であり、外観受入は2026-07-17 JSTの豆虎Gate実機確認PASSを正本とする。
