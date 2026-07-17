# Stage G-3B-R 王城専用モジュラー構築 設計書

## 1. 目的

Stage G-3A正式基準点を維持し、王城外観だけを自作軽量モジュールで再構築する。
旧Primitive王城の巨大箱・巨大平面・黒い直方体塔は再利用しない。

正式map：`/Game/Maps/StageG3B_ModularCastle_PoC`

## 2. 構築方式

- G-3A正本mapを新規mapへ複製する。
- 複製側の`G3A_Castle_*`外観だけを撤去する。
- G-3A正本map自体はSHA-256で処理前後を照合する。
- Blender 5.1で22種の軽量FBXを自作する。
- Unreal Engine 5.8で`/Game/StageG3B/ModularCastle/`へ新規importする。
- 外部素材、Marketplace、高精細Texture、Meshy、AI画像生成を使用しない。

## 3. モジュール契約

| 系統 | 主寸法 | 用途 |
|---|---:|---|
| WallBay | 幅300uu | 正面・翼棟・奥棟の反復壁面 |
| GateArch | 開口600×565uu | 尖頭アーチ正門 |
| TunnelWall | 奥行380uu | 実体のある門洞 |
| SideTower | 直径400／高さ1080uu | 左右八角塔 |
| CentralTower | 直径520／高さ1480uu | 奥側中央塔 |
| CornerTurret | 直径180／高さ560uu | 小塔 |
| SteepRoof | rise 285～520uu | 急勾配屋根・尖塔 |
| Buttress | 4段テーパー | 人間スケールの縦リズム |
| Window | 尖頭枠＋奥まった別部材 | 深さのある窓 |
| Crenellation | 長さ300uu | 小さい胸壁反復 |

単一の正面壁moduleを1000uu以上にしない。壁は300uuベイ11個以上で構成する。

## 4. 全体寸法

- 王城幅：2360uu
- 王城奥行き：1120uu
- 最高点：2000uu
- 王城前広場：1800×1100uu
- PLAYER_M身長：175uu（既存値を維持）

正面は「門洞→左右翼棟→左右塔→奥側高層棟→中央塔」の順に奥行きを付ける。
中央・左右・奥の段差をStandardCameraから読めることを優先する。

## 5. 外観構成

- 正門：尖頭アーチ1、門洞壁2、門洞屋根1、前階段1
- 塔：左右八角塔2、中央八角塔1、小塔4
- 屋根：翼棟4、門棟1、中央棟1、左右尖塔2、中央尖塔1、小尖塔4
- 扶壁：14
- 尖頭窓：20
- 胸壁module：11
- 旗：5
- 魔法marker：3

石壁は明るい青灰色、縁取りは淡灰色、屋根は暗いslate色とする。
完全な黒は門洞内にも使用せず、暗い青灰色に留める。

## 6. 維持境界

- PLAYER_M、Skeleton、Animation、Material、Textureを変更しない。
- GUARD_Mは1体、専用Skeletonを維持する。
- Stage G-2A Standard／Tactical cameraとF6を変更しない。
- Stage G-3A Standard camera自動追従を変更しない。
- 左クリック移動、保持移動、NavPath、WALK／RUN／DASHを変更しない。
- GameInstanceSubsystem、因果コア、Stage E状態、Stage F runtime、save schemaを変更しない。

## 7. 衝突とNavMesh

壁ベイ、門柱、門洞壁、左右塔、中央塔だけを物理衝突対象とする。
装飾、窓、屋根、旗、魔法markerは`NoCollision`とする。
G-3Aの物理groundをクリック移動面として維持し、王城前広場は視覚overlayだけとする。

## 8. 停止と承認

Automation合格は外観承認の代替ではない。16方向画像生成後、追加外観修正を行わず、
豆虎GateによるLEGO／Minecraft／豆腐建築判定を待つ。承認前のcommit／tag／pushは禁止する。

2026-07-17 JST、豆虎Gate目視判定PASS。Stage G-3B-R Modular Castle Exterior PoCを正式承認した。
