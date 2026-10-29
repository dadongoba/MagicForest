# 2026-10-29 攀爬、采集和提醒更新应用说明

本批是截至“攀爬仅忽略当前组件移动碰撞”的累计最终版本，覆盖当天早期方案。适用基线：完整 demo3_UE58 1.5、UE 5.8.3 Windows，增量功能前置 `eb210bc` / 2026-10-28-12；仓库整理基线 `303c381`。仅上传源码、开发测试、脚本和说明。

## 最终行为

- F 抓附有 0.35 秒缓冲，多点探测、贴面距离与朝向平滑、支撑丢失容错和松手冷却。
- 攀爬胶囊只忽略当前支撑组件的移动碰撞，独立世界探测仍命中该组件，完整胶囊仍阻挡其它物体；中部小球核心防止穿过岩壁。跨组件、松手及登顶先检查安全空间，异常结束清理本次忽略项。Cliff_02 专用绕石沿逻辑已被通用机制取代，角度规则保留。
- 上爬使用新 `AS_ClimbUpLoop` 连续动作；采集完整动作只播放一次，默认 1 秒；提醒按 ESC 关闭，移除 X 按钮及 Alt 鼠标逻辑。
- 风符文石运行时加入可攀爬和外挑面标记；本地还使用独立网格副本精确碰撞，按下述脚本重建。

## 文件清单

修改源码：`ForestElementRegions.cpp`、`ForestHarvestMagic.cpp/.h`、`ForestIsometricPlayer.cpp/.h`、`ForestPlayerActions.cpp`，均位于 `Source/demo3_UE58/`。

新增源码：`Source/demo3_UE58/ForestClimbCollisionTests.cpp`。开发自动化测试，非正式运行必需功能。

脚本：修改 `Tools/PreparePlayerActions.py`；新增 `Tools/OptimizeClimbingSurfaces.py` 和 `Tools/PrepareWindClimbingSurface.py`。

历史诊断及验证说明：当天 `Climbing_Optimization`、`Cliff04_Climbing`、`Wind_Gather_Notice`、`Cliff02_Climbing`、`Climbing_Component_Ignore` 五篇说明；以最后一篇及本文的最终行为为准。更新 README、CHANGELOG。

## 应用步骤及未上传资产

不上传第三方动画包、ChengYi 骨架、动作派生资产、11 份第三方地形网格、风符文石派生网格或本地修改后的 DemoMap。本仓库既有地图不随本批替换。也不上传 Saved、测试原始日志、机器信息、缓存、编译产物或原有未跟踪的 DefaultInput.ini。

1. 备份匹配的完整工程。自备并按原路径导入 `LaddersAndLedges`、`ResourceGatheringAnimationPack`、ChengYi、DreamscapeTower 等前置素材，沿用上批骨架兼容要求。
2. 在 UE Python 中执行 `Tools/PreparePlayerActions.py`，生成包括 `AS_ClimbUpLoop` 的 12 份当前动作依赖。新上爬动作来源为 `/Game/LaddersAndLedges/Animations/Ladder/ladder_up_loop`，锁定根骨骼第一帧；旧 `AS_ClimbUp` 不需删除。已有未知副本不会被覆盖。
3. 地形碰撞脚本默认仅检查。确认目标素材后，在 UE Python 控制台执行以下命令（脚本路径相对于项目根目录）：

```python
import runpy
surface_tools = runpy.run_path(unreal.Paths.project_dir() + 'Tools/OptimizeClimbingSurfaces.py', run_name='surface_tools')
surface_tools['optimize'](apply=True)
```

这会备份并将 Terrain_01～03、Cliff_01～08 的碰撞设为 ComplexAsSimple。该配置影响所有引用这些共享网格的实例，会增加三角碰撞查询成本；保留原包备份供回退。

4. 打开并先保存 `/Game/DreamscapeSeries/DreamscapeTower/Maps/DemoMap`。重建风区专用碰撞副本及该处地图引用：

```python
wind_tools = runpy.run_path(unreal.Paths.project_dir() + 'Tools/PrepareWindClimbingSurface.py', run_name='wind_tools')
wind_tools['prepare'](apply=True)
```

脚本先核对地图无未保存修改、风锚点唯一且仍使用预期网格，备份原地图，再从本机已有 `SM_RuneStone_01` 复制出 `/Game/ForestMigration/Climbing/SM_WindRuneClimbSurface` 并应用精确碰撞，仅替换风锚点组件引用。火区、水区仍使用原网格。自定义地图或不同命名需先人工核对，不能绕过断言批量替换。新脚本只做语法检查；对应资产修改已通过本地 MCP 执行并读回，重建脚本的 apply 分支未额外执行覆盖当前地图。

5. 关闭编辑器，按相同相对路径合并本批源码和测试，重新编译 `demo3_UE58Editor Win64 Development`，再打开工程。源码构造器依赖新动作，务必先完成动作生成。本批不要求合并配置，不更改存档格式。

## 验证与边界

完整工程源码编译、DLL 链接通过并已通过 Unreal MCP 重新启动确认加载。独立 UE 物理世界自动化测试 `Forest.Climbing.ComponentCollision`：成功 1、失败 0、警告 0，覆盖当前组件忽略、独立支撑探测、防穿墙、其它障碍、跨面恢复、正常/受阻/强制退出、保留原有忽略关系及目标销毁清理。

地形和风区碰撞设置在本地保存后重新加载检查通过；各类岩石的离线射线与胶囊诊断见历史说明。没有在用户关卡启动 PIE 或代操作角色；手感、实际凹凸过渡、跨面、登顶、三视角、采集和 ESC 组合以及打包仍需人工验收。本批未实现手脚 IK。
