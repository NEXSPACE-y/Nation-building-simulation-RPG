# Stage G-0 左クリック移動・ターゲティング 手動受入チェックリスト

本チェックはWin64 Development Packageを人間操作して判定する。Automation PASSだけで実機PASSにしない。2026-07-12、ユーザーが実機で全20項目を確認し、すべて問題なしと判定した。

1. [x] PASS: 画面内の任意の通行可能地点をクリックできる
2. [x] PASS: クリックした有効地点へ「移動先」ポインタが出る
3. [x] PASS: Playerが表示ポインタと同じNavMesh地点へ進む
4. [x] PASS: Player周囲360度の任意方向へ移動できる
5. [x] PASS: 左クリック保持中、0.075秒間隔の目的地更新でcursorへ追従して方向転換する
6. [x] PASS: 障害物へ直進せずNavMesh経路で迂回する
7. [x] PASS: 壁、空、UI、NavMesh外、検証範囲外のクリックで移動命令を発行しない
8. [x] PASS: 外周へ目的地を設定できず、通常操作でマップ外へ落下しない
9. [x] PASS: 衛兵（GUARD）をクリック選択できる
10. [x] PASS: 隊長（CAPTAIN）をクリック選択できる
11. [x] PASS: 商人（BROKER）をクリック選択できる
12. [x] PASS: NON AI NPCをクリック選択できる
13. [x] PASS: 牙鼠をクリック選択できる
14. [x] PASS: 対象クリック時に新しい移動命令を誤発行しない
15. [x] PASS: HUD／検証パネル上のクリックがworld移動・対象選択へ貫通しない
16. [x] PASS: 対象ring、HUD日本語名、F1対象ID／種別が同じActorを示す
17. [x] PASS: 同一location外またはinteraction range外のNPC行動が日本語理由付きで拒否される
18. [x] PASS: 同一locationかつinteraction range内のNPC行動が既存因果コアへ正常送信される
19. [x] PASS: 実移動方向と既存8方向placeholder表示が一致する
20. [x] PASS: 移動中もcameraがPlayerへ自動追従し、右ドラッグを必須としない

## 自動証拠との対応

- G0-M1～M8: cursor ray、16方向、保持更新、pointer一致、無効面、迂回契約、落下防止、8方向表示
- G0-T1～T6: NPC／牙鼠target、地面分離、対象クリック非移動、UI分離、重なり決定順
- `out/stage-g0/test-output/stage_g0_click_move_evidence.json`
- `out/stage-g0/test-output/stage_g0_targeting_evidence.json`
- `out/stage-g0/manual/stage_g0_click_move_manual.txt`

## 現在の判定

- Automation: 33/33 PASS
- Development Package navigation smoke: PASS（`ready=1`、`path_points=2`、非partial）
- Computer Use: 部分確認PASS（移動先marker、地点移動、camera追従、隊長クリック選択、対象クリック非移動、F1日本語監査表示）。未実施項目をユーザーPASSへ代用しない
- ユーザー実機受入: 20/20 PASS
- 既知の受入阻害問題: なし
- Stage G-0受入: 完了
