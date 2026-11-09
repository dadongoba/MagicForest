# 透特风格大牌图：切图源与运行时映射

源图是用户放在 Downloads 的 `森林科技塔罗牌阵列.png`，2 行 × 11 列，共 22 张。`Cards/` 下每张为独立 RGB PNG，统一 142 × 512 像素；只按黑色分隔线裁切并在少数窄列补齐黑色边距，未重绘或生成牌面。`Preview/contact_sheet.png` 用于人工核对，不是游戏资产。

`major_arcana_manifest.json` 给出每张牌的游戏 ID、中文名称、原图印刷英文名、文件名、源图裁切坐标和文件哈希。顺序以 `UAlchemyGameplaySubsystem::MakeMajorCards()` 的 0、I 至 XXI 为准。图中的英文与当前游戏内部的拉丁文名不完全相同，但 ID、序号和中文名对应。

当前运行时已将同一批 PNG 复制到完整工程的 `Content/UI/Textures/MajorArcana/`，由 `ForestMajorArt` 按清单的 `game_id` 映射为 Slate 图片；抽牌弹窗、背包、魔法书及合成台使用这组牌图。构建规则将这些 PNG 作为 NonUFS 文件一同分发。该接法不需要另外生成 `.uasset`，也不会在 UE 内容浏览器中出现 Texture2D。详见 [接入说明](../../Docs/Major_Arcana_Draw_Integration_2026-11-09.md)。

2026-11-08 切图时建议的 Texture2D 导入路径 `/Game/Forest/MajorArcana/` 属于历史方案；当前以文件图片绑定为准。之后若改用高分辨率 Texture2D，应统一替换图片映射和打包方式。

源图宽 1536 像素，单张有效图像约 136 像素宽。建议用于背包、图鉴等较小牌面；若要全屏大图，应另取高分辨率源图。可运行 `split_cards.py` 从 `Source/` 中的原图重建 22 张及清单；需要 Pillow。
