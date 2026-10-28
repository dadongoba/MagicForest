# 采集动作、收获提示关闭与自然地形攀爬

基线：元素产区提交 e3fb876、文档提交 2504993；UE 5.8.3 Windows。

## 操作

- E：靠近元素物资采集。播放起手、拾取、收手，约 1.33 秒后重新确认原目标及距离、视线、水中条件，再入包。动作期间重复 E 不会重复入包。打开菜单、离开有效运动状态、物资换日销毁等会取消，不发放物资。
- 陆地使用 PickupCrouchedRight 三段动作；水中使用 PickupStandingRight 三段伸手动作，保持水面位置，结束后恢复游泳。第一人称保持原模板隐藏身体的规则，动作与采集计时仍执行；查看全身动作可用 V 切到第三人称。
- 收获提示右上角 X 可立即关闭，仍保留原有 6/8 秒自动隐藏。第一/第三人称按住 Alt 暂时显示鼠标；松开或关闭提示后恢复原输入模式。上帝视角可直接点 X。关闭仅影响提示，不影响背包。
- F：面对近处岩石/陡峭地形抓附，再按 F 松手下落。W/S 上下爬，A/D 横移，支持左摇杆。第一/第三人称也可用跳跃键松手。镜头仍按当前模式观察；攀爬或采集期间暂不切换视角。
- 上爬到能站立且有胶囊空间的边缘，自动播放登顶。缓坡继续使用 CharacterMovement 正常行走。攀爬菜单打开/失去焦点时停留，关闭后继续。

## 范围与实现

`ForestPlayerActions.cpp` 是现有 UForestIsometricPlayer 的动作实现，不创建第二个移动输入组件。模板行走/相机仍由原实现负责；动作期间屏蔽普通移动。攀爬使用 CharacterMovement 的 Flying 模式与 SafeMoveUpdatedComponent 胶囊扫掠，退出恢复 Falling，自然落地；不关闭胶囊碰撞。

默认识别静态 Landscape，以及模型名含 Rock / Cliff / Terrain / Ground / Boulder 的静态网格。探测距离 150 厘米，攀爬速度默认 110 厘米/秒，表面法线 Z 在 -0.2 至 0.65。可给其他 Actor/组件加 ForestClimbable 标签；ForestNoClimb 优先禁止。查询采用角色胶囊自身的碰撞通道，装饰性不阻挡表面不会成为攀爬支点。地形必须具有角色碰撞。

登顶在规划时检测向上、向内的完整胶囊路径及终点空间，执行时继续扫掠；遇阻停止并下落。大幅悬垂、动态物体、跨缝跳跃、梯子交互、手脚 IK、多玩家网络同步不在本批范围内。

## 本地导入与增量依赖

两个下载包按原始包路径导入完整工程：

- `Content/LaddersAndLedges/`：162 个文件，含 143 个可加载动画序列。
- `Content/ResourceGatheringAnimationPack/`：58 个文件，含 29 个可加载动画序列。
- 从中生成 `Content/Characters/Actions/AS_GatherCrouched{Start,Loop,End}`、`AS_GatherStanding{Start,Loop,End}`、`AS_Climb{Idle,Up,Down,Left,Right,Top}` 共 12 份动作副本；禁用根运动、启用根锁定，位移交给胶囊碰撞。
- `Content/ChengYi/Mesh/SKEL_ChengYi_Skeleton.uasset` 本地增加上述两包 UE4 skeleton 兼容引用；原文件备份于 Saved/CodexBackups/gather-climb-20261028。

公开增量仅含本批源码、本文档和 `Tools/PreparePlayerActions.py`；不上传第三方动画包、动作派生资产或 ChengYi 骨架。下载者需自行准备两包，在原始 Content 路径导入，使用 UE Python 执行准备脚本，检查输出后编译。脚本检查对应骨骼名及主要骨骼参考旋转，已有动作只校验，不覆盖未知资产。C++ 构造器保存动作硬引用，仍需实际打包验证。

## 实际验证与边界

- 编辑器加载两个包中全部 172 个动画序列，均有有效 skeleton 和时长；12 个副本的时长、骨架引用、根锁定/根运动设置读回通过。
- 两包 UE4 骨骼名均存在于 ChengYi，检查的 pelvis/spine/upperarm/thigh 参考旋转一致；不是另行生成新动作或修改原动画数据。
- 首轮源码编译通过。最终编译、DLL 链接/重开结果以 CHANGELOG 本批记录为准。
- 按用户“测试我自己来”的要求，没有启动 PIE、播放游戏、移动角色或自动进行采集/攀爬测试。动画实际姿态、水中伸手、岩石凹凸处碰撞、登顶观感、X 点击与三视角组合、保存/换日及打包待用户验收。
