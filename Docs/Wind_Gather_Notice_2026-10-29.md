# 风区碰撞、单次采集动画与 ESC 关闭提醒

完整工程：MagicForest，UE 5.8.3。未同步或发布 MagicForest-upload。本批全程仅使用 Unreal MCP、命令行和文件工具。

## 风属性区域

锚点 `SM_RuneStone_01_WIP_2`，位置约 (30719, 48600, -6289)，区域默认半径 4000 厘米。检查附近 23 个地形/岩石网格组件，已有 Cliff/Terrain 精确碰撞配置仍在。围绕锚点四个高度、每层 24 个方向的射线大多首先命中风符文石。原网格 `SM_RuneStone_01` 使用一个凸包且名称不符合攀爬识别条件；这是确认的抓附拒绝条件。单凸包还可能在可见凹面外形成阻挡，不能仅据此认定所有风区碰撞问题已排除。

通过 MCP 新建 `Content/ForestMigration/Climbing/SM_WindRuneClimbSurface.uasset`，来自符文石原网格，设置 ComplexAsSimple，仅将 DemoMap 中风符文石组件替换为该副本。原网格碰撞仍为 UseDefault，火区、水区不替换。地图修改前确认没有未保存修改，原地图备份 `Saved/CodexBackups/wind-gather-20261029/DemoMap.umap`。

`ForestElementRegions.cpp` 在识别风锚点后，只给其静态网格组件添加运行时 `ForestClimbable` 和 `ForestClimbOverhang` 标记。`ForestPlayerActions.cpp` 对带后者标记的组件允许法线 Z>-0.65，仍拒绝近水平底面，并保留胶囊扫掠及登顶检测。没有关闭树木、链条或其它障碍物的碰撞。

## 采集动画

原 Start/Loop/End 时长分别约 0.333/0.667/0.333 秒，Start 与完整动作前半段包含重叠采样。以前按三段顺序播放会重放伸手动作。

现在只播放完整动作一次（不循环），默认用 `GatherDuration=1` 秒完成；水中、陆地各用对应完整动作。只在动作完成后提交一次收获；菜单、状态变化、目标失效等原有取消检查保留。准备脚本及旧 Start/End 资产保留兼容，但不再拼接播放。

## 收获提醒

`ForestHarvestMagic.cpp/.h` 移除 X 按钮，提示改为“按 ESC 关闭”。通过带弱引用的 Slate 输入预处理器处理可见提醒的 ESC，避免被编辑器 PIE 停止快捷键先截获；键盘焦点不在游戏视口、背包、炼药台或对话菜单打开时不抢占 ESC。处理对应按键释放和长按，并在子系统销毁时注销。

`ForestIsometricPlayer.cpp/.h` 移除原来为了点击 X 而设置的 Alt 显示鼠标逻辑。提醒仍自动消失（原 6/8 秒），ESC 只关闭提醒，不撤销已入包的物品。

## 验证与边界

- 所有修改源码编译、标准 DLL 链接成功，日志 `Saved/ClimbAudit/wind-gather-build.txt`。
- MCP 保存后读回新网格碰撞、风组件网格引用、原网格碰撞配置通过；另一个只读 UE 命令行进程重新加载保存地图、执行区域射线查询成功。
- 用户关闭编辑器后通过 Unreal MCP 重新启动，加载新编译模块。
- 未启动 PIE，也没有代替用户操作角色。风区实际攀爬路线、凹凸过渡和登顶、单次采集观感、ESC 在三视角及菜单组合下的行为仍需游戏内验收。精确碰撞增加该风符文石的三角碰撞查询成本。

涉及源码：ForestElementRegions.cpp、ForestPlayerActions.cpp、ForestIsometricPlayer.cpp/.h、ForestHarvestMagic.cpp/.h；资产：DemoMap.umap、新风符文石碰撞副本。
