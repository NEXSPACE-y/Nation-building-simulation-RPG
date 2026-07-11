# Stage D 人手操作検証チェックリスト

## ビルド識別情報

- 状態: `18/18合格`
- 実行ファイル: `out/stage-d/package/Windows/NationSimulationStageC.exe`
- ビルド日時: `2026-07-11 11:07:25 +09:00`
- SHA-256: `7931C90FA1B8C11845D8A6782B02B03BEF463DE800FFBB7DA258F37D1E2BCC64`
- 構成: `Win64 Development`

## 実施記録

- 操作者: Codex Computer Use（デスクトップ上での人間操作相当入力）／項目17はユーザーによる実機の人間操作
- 開始日時（日本時間）: `2026-07-11 10:52:00 +09:00`
- 終了日時（日本時間）: `2026-07-11 11:10:59 +09:00`
- 総合結果: `合格`
- 停止理由: なし。項目17はユーザーが実機の人間操作で確認済みとしたため、受入記録へ反映した。Codex Computer Useでは軸入力の押下状態を保持できず同じ距離確保を再現できなかったが、この制約は項目17の不合格理由としては扱わない。
- 保存した実行ログ: `out/stage-d/manual-verification/20260711-111059/NationSimulationStageC.log`（SHA-256 `EF35DACF3802D9A44DCD1FF02DEDE00FA631364F3C33B6B39618F711C52FBC7B`）

## 必須確認項目

- [x] 1. `NationSimulationStageC.exe`を起動する。— 合格。
- [x] 2. `StageD_Capital`が読み込まれることを確認する。— 合格。実行ログ899行目に`STAGE_D_PLAYABLE_READY ... npc_count=40 map=StageD_Capital location=market`を確認した。
- [x] 3. WASDで移動できることを確認する。— 合格。ユーザーが直接確認済み。Computer Useでは長時間の軸入力を保持できないため、項目17の距離確保にだけ制約が残った。
- [x] 4. `market`へ移動する。— 合格。7キー後、HUDの現在地が`market`へ変化した。
- [x] 5. `non_ai_npc_002`から350 Unreal Units以内に入り、対象として選択する。— 合格。market到着直後のTabでHUDへ`対象: non_ai_npc_002`と表示された。
- [x] 6. HARMを実行する。— 合格。3キー後、HUDへ`PLAYER_HARMED_NON_AI_NPC`が表示された。
- [x] 7. AI NPC 001、007、012、017が反応することを画面上で確認する。— 合格。HUDの`AI反応`欄へ4 IDを表示した。
- [x] 8. REPORT、WARN、REFUSE_TRADE、FLEEが実行されることを画面上で確認する。— 合格。4アクションを`AI反応`欄で確認した。
- [x] 9. 直近イベント表示またはデバッグ表示で、AI NPC 001からAI NPC 002への報告を確認する。— 合格。HUDに`ai_npc_001 -> ai_npc_002 REPORT`を表示した。
- [x] 10. 国家securityが50から48へ変化することを確認する。— 合格。HUDで`治安 48`を確認した。
- [x] 11. 国家crime_levelが10から12へ変化することを確認する。— 合格。HUDで`犯罪 12`を確認した。
- [x] 12. イベント連鎖の途中でF5を押し、未処理イベントを含む保存が作成されることを確認する。— 合格。保存証拠`stage_d_save.json`に未処理イベント1件を確認した。
- [x] 13. ゲームを終了する。— 合格。Alt+F4で終了した。
- [x] 14. 同じビルドを再起動する。— 合格。
- [x] 15. 未処理イベントが再開されることを確認する。— 合格。再起動後のHUDで未処理3件から0件への進行を確認した。
- [x] 16. 再開後の最終国家結果が、保存せず連続実行した場合の結果と一致することを確認する。— 合格。HUD最終値48/12、自動証拠の連続実行と再開後SHA-256はいずれも`855d142c881e5f7218843793ef007fbe19cd171cd5b5a8949cb4a2ed43e00016`。
- [x] 17. 別の場所にいるNPC、または350uuより遠いNPCへの行動が拒否され、その理由がUIに表示されることを確認する。— 合格。ユーザーが現行Development Packageを実機の人間操作で確認済み。Codex Computer Useでは軸入力の押下状態を保持できず同一操作の再現証拠を取得できなかったが、当該制約は補足記録とする。
- [x] 18. 7キーを1回押し、場所ボリュームとの重複によってMOVEイベントが二重作成されないことを確認する。— 合格。実行ログ966行目に`destination=tavern reason=MOVE to tavern is already pending`を確認し、HUD上の場所遷移は1回だった。

## 必須証拠値

- 最終security: `48`
- 最終crime_level: `12`
- F5保存時の未処理イベント数: `1`（保存ファイルSHA-256 `2250C28BE337CF23B976B4DD295A330425D4F7B85353B5D647EA7CF19B607ABC`）
- 保存再開後の最終結果: `security=48 / crime_level=12 / pending=0`
- 連続実行・保存再開スナップショットSHA-256: `855d142c881e5f7218843793ef007fbe19cd171cd5b5a8949cb4a2ed43e00016`（一致）
- 7キー1回の移動に対するMOVEイベント数: `1`。重複候補は`STAGE_D_MOVE_SUPPRESSED`。
- 遠隔・範囲外拒否時のUI表示: ユーザーによる実機の人間操作で確認済み。Computer Useでは軸入力保持の制約により同一ログ行を取得できなかった。
- `STAGE_D_ACTION_REJECTED`ログ行: 自動是正テストで監査済み。人手確認分はユーザー操作による受入記録とする。
- `STAGE_D_MOVE_SUPPRESSED`ログ行: `[2026.07.11-02.10.13:835][524] ... destination=tavern reason=MOVE to tavern is already pending`
- オフライン反映: HUDで`+651秒`を確認した。

## 受入署名

- 操作者判定: `18/18合格`
- 操作者名: Codex Computer Use／項目17はユーザー実機操作確認
- 署名日時（日本時間）: `2026-07-11 11:10:59 +09:00`
- 備考: 項目17のユーザー実機操作確認を反映した。Stage Dのコミット、`stage-d-playable-v0.1.0`タグ作成、プッシュ、およびPhase E-Designへ進行可能。
