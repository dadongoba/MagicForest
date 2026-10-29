# 攀爬容错、贴面与连续上爬优化

工程：MagicForest/demo3_UE58.uproject，UE 5.8.3 Windows。仅修改完整工程；没有同步或发布 MagicForest-upload。

## 本次修改

- `Source/demo3_UE58/ForestIsometricPlayer.h`：增加 F 输入缓冲（0.35 秒）、支撑丢失容错（0.15 秒）及可攀爬状态。
- `Source/demo3_UE58/ForestPlayerActions.cpp`：五点小球扫掠、前方 ±20 度抓附搜索、限定接近距离、抓附前扫掠贴面、同组件且相近法线的采样合并、贴面距离和朝向平滑。探测使用与胶囊移动一致的简单碰撞查询（ComplexAsSimple 网格仍使用三角碰撞）。停留时也保持贴面；短暂丢失支撑时停留重探，超时下落。松手后 0.4 秒内不重新抓附，清除缓冲。菜单和失焦清除待抓附请求。
- `Source/demo3_UE58/ForestHarvestMagic.cpp`：有效抓附范围内显示 F 抓附提示。
- `Content/Characters/Actions/AS_ClimbUpLoop.uasset`：从原包 `Ladder/ladder_up_loop` 新建动作副本，禁用根运动、锁定第一帧根骨骼；原 `AS_ClimbUp` 与下载包保持原样。按实际移动速度调整播放速度，被挡住时回到待机；每次切换动作重置播放速度。
- `Tools/PreparePlayerActions.py`：今后准备动作时生成新循环资产。
- `Tools/OptimizeClimbingSurfaces.py`：默认只报告，调用 `optimize(apply=True)` 才备份并修改指定 11 个资产。

## 地形碰撞调整

检查 DemoMap 中 422 个相关静态网格组件，共 18 种网格。3 个 Terrain 和 8 个 Cliff 网格原来各只有一个凸包；该凸包会包住视觉凹面，导致角色无法贴近实际表面。

已将以下 11 个网格的碰撞复杂度改为 `Use Complex Collision As Simple`，角色碰撞和攀爬探测都使用网格三角表面：

- `/Game/DreamscapeSeries/DreamscapeTower/Meshes/Terrain/SM_Terrain_01` 至 `03`。
- `/Game/DreamscapeSeries/DreamscapeTower/Meshes/Stones/Cliffs/SM_Cliff_01` 至 `08`。

没有重新建模、降低视觉网格精度或重存关卡。该设置影响引用这些资产的所有实例，并增加三角碰撞查询成本；性能和既有行走路线需实际游玩确认。其余岩石保持原配置。

11 份原始包备份在 `Saved/CodexBackups/climbing-20261029/Content/`，按原相对路径保存。回退时关闭编辑器，将指定备份包复制回对应 Content 位置后重新加载。

## 动画问题依据

原 `climb_U` 时长 1.333 秒，起步 pelvis 高度从约 96.76 降至 69.56，然后升至约 159.93，根骨骼一轮上升 150 厘米，属于明显蓄力跃升动作。禁用根运动后依然保留这段身体起伏。

新循环时长 1.767 秒，检查多个时间点 pelvis 高度均约 96.76；原根位移一轮约 70 厘米，游戏内由胶囊承担移动。该循环来自梯子动作，可消除蓄力跃升；不包含针对岩石的手脚 IK。

## 验证与验收边界

- 源码编译、DLL 链接通过；最终结果另见 `Saved/ClimbAudit/final-build.txt`。
- 单独启动 UE Python 命令行进程重新加载：11 个碰撞设置、11 个原包备份、新动画骨架/时长/根锁定/根运动设置与采样姿态检查通过。记录在 `Saved/ClimbAudit/verified.json`。
- 没有启动 PIE 或执行玩家操作；输入手感、动画姿态、转角、登顶、第一/第三/俯视视角和性能仍需游戏内验收。未实现手脚 IK 或动画切换混合。
- UE 命令行启动存在工程原有 GameFeatureData 规则缺失报错；本次 Python 验证以明确断言和保存后的重新加载结果为据，没有将该既有配置报错改作攀爬修复范围。

建议依次检查：提前按 F 接近岩壁、W 连续上爬、小凹凸和接缝、停留贴面、碰壁停止、正常登顶、F 松手后不会立即重抓、三视角及菜单暂停恢复。
