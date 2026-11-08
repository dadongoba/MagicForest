# 透特风格大牌图：UE 导入源文件

源图是用户放在 Downloads 的 `森林科技塔罗牌阵列.png`，2 行 × 11 列，共 22 张。`Cards/` 下每张为独立 RGB PNG，统一 142 × 512 像素；只按黑色分隔线裁切并在少数窄列补齐黑色边距，未重绘或生成牌面。`Preview/contact_sheet.png` 用于人工核对，不是游戏资产。

`major_arcana_manifest.json` 给出每张牌的游戏 ID、中文名称、原图印刷英文名、文件名、源图裁切坐标和文件哈希。顺序以 `UAlchemyGameplaySubsystem::MakeMajorCards()` 的 0、I 至 XXI 为准。图中的英文与当前游戏内部的拉丁文名不完全相同，但 ID、序号和中文名对应。

导入建议：将 `Cards/` 下 22 张 PNG 导入完整工程的 `/Game/Forest/MajorArcana/`；作为 Slate/UMG 图片时设为 UI 纹理组、关闭 mipmap，并保留 sRGB。随后按清单的 `game_id` 把各张 Texture2D 绑定到运行时大牌定义或 UI 映射。**目前只准备了图像源资产，尚未生成 `.uasset`，也未改游戏运行时绑定。**

源图宽 1536 像素，单张有效图像约 136 像素宽。建议用于背包、图鉴等较小牌面；若要全屏大图，应另取高分辨率源图。可运行 `split_cards.py` 从 `Source/` 中的原图重建 22 张及清单；需要 Pillow。
