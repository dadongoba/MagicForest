# 小地图与蘑菇朝向修复 · 2026-10-27

完整本地工程：`demo3_UE58/demo3_UE58.uproject`，UE 5.8.3。

## 小地图

- 右上角常驻显示附近 160 米范围，固定北向，跟随 ChengYi。
- 按 M 切换世界地图；世界地图覆盖 DemoMap 主地形约 960 × 960 米。
- 玩家箭头显示位置和身体朝向；金色菱形标记小屋，其他颜色标记森林女巫、Natta 和 Fawnia。范围外标记停留在边缘并显示距离。
- 显示世界 X/Y 坐标、距小屋距离、指北和比例尺。
- 底图为当前场景的 2048 × 2048 正交俯视截图，已在蘑菇修正后刷新。运行时使用静态纹理，动态更新图标，不保留额外 SceneCapture 或实时全场景渲染。
- 小地图不拦截鼠标；M 键不改变原 WASD、交互、背包和找人的按键。其他交互菜单打开时，M 不触发地图切换。

新增 `ForestMinimap.h/.cpp`，在 `ForestIsometricPlayer` 中创建和清理地图 Widget；更新操作提示。底图为 `/Game/ForestMigration/UI/T_DemoMap_Minimap`，由 Widget 默认对象显式引用。

## 蘑菇朝向

检查当前 DemoMap 的所有普通 StaticMeshComponent 和实例化网格组件，共找到 911 个蘑菇，全部位于 Dreamscape_Gameplay 子关卡；未发现实例化蘑菇。

迁移时错误地把水平旋转写入了俯仰轴。899 个蘑菇与向上轴明显不一致，另有 2 个存在负缩放。本次全部清零俯仰和侧倾，恢复绕世界 Z 轴的随机角度，并使缩放分量为正。所有锚点再次射线检查，地面高度无需调整。原位置和缩放大小保留，原 Demo_Day 未修改。

修正通过独立编辑器事务执行，已保存玩法子关卡。旧迁移辅助脚本也改为明确设置 yaw 字段，避免再次混淆轴。

## 验证

- Development Editor 编译通过。
- 运行时只有一个地图 Widget，使用实际底图；主角实时坐标与地图追踪坐标一致。
- 通过原 IA_Move 移动约 366.7 cm，地图中心持续与玩家位置一致，观测误差为 0；M 键实际关闭全图。
- PIE 检查 911 个蘑菇，向上向量与世界 Z 轴点积最小值为 1.0。编辑器再次检查通过。
- 已检查小地图/全图的实际显示，并将全图置于普通交互提示上方。
- 测试前保存槽备份位于 `Saved/CodexBackups/minimap-20261027/SavesBeforeMinimapValidation/SaveGames/`，测试后恢复。

备份与逐 Actor 朝向记录：`Saved/CodexBackups/minimap-20261027/` 和 `Saved/MigrationTools/mushroom-orientation-before.json`、`mushroom-orientation-after.json`。未执行 Git 提交或推送。

最终版本重开工程后再次验证通过；普通提示未覆盖全图。PIE 已正常结束，测试保存槽逐文件恢复，哈希差异为 0，编辑器保留在 DemoMap。
