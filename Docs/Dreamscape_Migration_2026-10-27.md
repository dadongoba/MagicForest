# Dreamscape DemoMap 迁移记录 · 2026-10-27

本次直接修改完整工程 `demo3_UE58/demo3_UE58.uproject`。UE 5.8.3，Windows。

## 启动与角色

- 编辑器启动地图和游戏默认地图：`/Game/DreamscapeSeries/DreamscapeTower/Maps/DemoMap`。
- 默认游戏模式：`/Game/ForestMigration/Blueprints/BP_DreamscapeGameMode`。
- 默认玩家：`/Game/ForestMigration/Blueprints/BP_ChengYiPlayer`，继承原 FirstPersonCharacter，保留 Backpack、LastPicked、RefreshHUD 和旧系统接口。
- 运行时使用 `SK_CHENGYI_01` 和素材自带 `ThirdPerson_AnimBP`，隐藏原第一人称手臂，停用原第一人称相机。
- 俯视相机：俯角 55°、方位角 -45°、初始距离 1350 cm，滚轮范围 700–2200 cm；WASD 以固定相机方向移动，人物身体单独朝移动方向旋转。
- 新增 `ForestIsometricPlayer.h/.cpp`；修改 `ForestAlchemy.cpp` 的俯视邻近交互，以及 `ForestHarvestMagic.cpp` 的操作提示。
- E 按键优先处理药瓶/炼药台，其次 NPC，再处理附近当天可采集蘑菇，屏蔽旧模板的重复 E 采集。B 背包，V 找人，滚轮缩放。

## 地图内容

新增常驻子关卡 `/Game/ForestMigration/Maps/Dreamscape_Gameplay`，承载从 Demo_Day 复制的 1114 个玩法 Actor：911 个蘑菇、199 个道路偏好区域、3 个 NPC、1 个交互管理器。三位 NPC 为森林女巫、Natta、Fawnia，继续使用原生 AI 控制器和原有对话/记忆/找人系统。

根据已手动放置的小屋重新布置，并逐一做地面射线检测；28 个原映射位置落在不适合区域的 Actor 改放到邻近可站立地面。没有保存或改写 Demo_Day；其 SHA256 与增量仓库基线一致。

用户手动迁移的 Witch_House 保留原实例与变换 `(44820, 13210, -8950), yaw -180°`。只在 DemoMap 另加出生点、导航范围、木质门口平台和坡道。运行时打开没有门控脚本的装饰门碰撞，连通室内外通路；屋顶/遮挡物在游戏中按玩家位置隐藏，离开或结束游戏后恢复显示。预览飞行 Pawn 关闭自动占有，避免抢走主角控制权。

旧地图的森林布景、测试摄像机和依赖旧森林障碍位置的 7 处蹲伏通道未复制到新地形。蹲伏和道路寻路的原生系统仍保留。新地图采用自己的光照与动态导航，主方向光指定前向渲染优先级。

## 验证与边界

- Development Editor 编译通过。
- 多次重开工程后，主地图、游戏模式、常驻玩法子关卡、三位 NPC 和原小屋实例均读回一致。
- PIE 确认实际占有 ChengYi，身体网格和动画实例正确，相机俯角正确。
- 注入原 IA_Move：沿 `(正 X, 负 Y)` 稳定移动，胶囊 yaw 持续 -45°，没有随角色转向改变移动基准。
- 采集得到一个水 6 魔法道具；旧背包接口正常衔接魔法书系统。
- 三位 NPC 的近距离对话均打开，找人菜单正常打开。对话检查使用运行时离线测试模式，没有测试新的在线聊天内容。
- 拿取药瓶：今日大牌数从 0 增到 1；炼药台显示正常。
- 桌面实际按键：B 开关背包、E 打开炼药台、滚轮使相机距离从 1350 改为 1470。最后版本另验证 E 单次采集只增加一件道具，以及 E 近距离打开 Natta 对话。
- 小屋出生点到室外的导航路径为完整路径（13 个路径点）。NPC 漫游已观察到；整个大型地图的全部漫游位置没有逐一走查。
- 保存槽在测试前备份，测试结束后恢复并逐文件校验。
- 未执行打包发布、联网对话全流程、完整调配交付跨天回归。动态加载的 ChengYi 资源在正式打包前应补充显式 Cook 引用或资源标签并验证成品包。
- 现有资源仍有 `GameFeatureData` AssetManager 配置提示、ChengYi 骨架旧 `/Engine/EngineMeshes/Humanoid` 引用提示和未重建反射捕捉提示；PIE 的角色和动画已实际运行。

## 回滚与证据

- 原地图、配置和源代码备份：`Saved/CodexBackups/dreamscape-migration-20261027/`。
- 测试前保存槽：该目录下 `SaveGames-before-validation/SaveGames/`。
- 放置记录、运行时读回与验证结果：`Saved/MigrationTools/`。
- 验证脚本通过独立 Python 命名空间执行，结束后释放 PIE 引用；曾发生一次脚本持有关卡引用导致的退出断言，修正后已正常结束 PIE。
- 本次迁移保持在完整本地工程，未复制第三方素材到增量上传仓库，也未执行 Git 提交或推送。

最后版本正常结束 PIE，编辑器保留在 DemoMap，测试保存槽已再次恢复且 SHA256 校验无差异。
