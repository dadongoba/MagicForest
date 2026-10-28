# Dreamscape 项目增量应用说明 · 2026-10-28

本批基于 main `ac8142b` 与完整 demo3_UE58 1.5，目标 UE 5.8.3 Windows。仓库依旧是增量文件集。文件及 SHA256 见 [清单](Dreamscape_Upload_2026-10-28_files.json)。

## 内容

- 默认启动 DreamscapeSeries/DreamscapeTower/Maps/DemoMap，常驻玩法子关卡 ForestMigration/Maps/Dreamscape_Gameplay，保留原 Demo_Day。
- ChengYi 为默认主角，沿用原角色的背包和交互接口；固定俯视移动方向、滚轮缩放，按住右键拖动观察，松开平滑恢复。打开菜单或地图时退出观察。
- 小地图、M 世界地图、坐标和主角/小屋/NPC 标记。底图为静态纹理，不增加持续场景捕捉。
- 迁移三位 NPC、交互管理器、道路区域；蘑菇按世界 +Z 修正，取消对玩家/NPC 的阻挡及导航影响；移除 39 个独立 Mushroom_Cap 场景 Actor，保留 872 个可采集蘑菇。此操作在玩法子关卡中体现，不要求删除任何源网格资产。
- 室内遮挡隐藏只匹配屋顶，桌子和书架不再因为大小/高度被隐藏；原有植被材质淡出、游泳实现及 AS_Swimming 动画依赖随现有主角代码保留，并非本次新增运行验收项目。
- 采集移除前段 Portal、Dust 粒子，保留书本、溶解和后段元素光效。
- BP_PlayerPawn、CinemaCameraPawn 的 EnableInput 取消错误外部 Controller 参数，保留执行链；预览 Pawn 不抢占主角。
- 将远程已有的炼药面板、自适应排版、居中物品/引路字条更新合并回完整工程。上传差异保留这些界面行为。

## 依赖与应用

1. 备份完整工程及地图，退出编辑器。先应用到本批基线为止的仓库更新。
2. 自行准备匹配的 DreamscapeSeries、ChengYi、Witch_House、PNB_Enchanted_Forest 与既有 NPC、魔法书、粒子和主题素材。**本批未收录完整第三方素材包、ChengYi 网格/动画蓝图依赖或 Witch_House 关卡。** 新地图引用这些原路径；缺少资源时不能只靠本仓库运行。
3. 执行 `git lfs pull`，确认两份 `.umap` 为二进制文件而非 LFS 指针；按清单相对路径应用源码和选定资产。本批只带修改过的 Dreamscape 主地图、两个预览 Pawn 和三份植被主材质，以及 ForestMigration 自建资产、MPC 和游泳动画。
4. 对照合并 DefaultEngine.ini 的三个地图/游戏模式设置；保留目标工程自己的平台、网络和密钥配置。其他碰撞配置沿用现有 ForestDecoration、ForestNPC 通道。未提交工作区已有的 DefaultInput.ini，本功能的右键绑定由 C++ 创建。
5. 重新生成项目文件并构建 demo3_UE58Editor。打开 DemoMap 检查常驻玩法关卡与主角引用；小屋使用地图内的原 LevelInstance，必须保留 Witch_House 资源。屋顶规则包括 roof 名称、ForestCameraRoof 组件标签及小屋屋顶板 SM_Shop_Cube_092。
6. 正式打包前补充软路径加载的 ChengYi、游泳动画等显式 Cook 规则，并验证成品包。本批没有新增打包配置或完整素材发布。

## 验证边界

- 本批上传前执行合并源码的 Windows Development Editor 编译；具体结果记在 CHANGELOG 本批条目。
- 两个 EnableInput 蓝图在此前编辑中已编译、保存并读回。迁移和小地图的早期 PIE 记录见 [迁移记录](Dreamscape_Migration_2026-10-27.md)、[小地图记录](Minimap_and_Mushroom_Orientation_2026-10-27.md)，它们是当时版本的历史证据。
- 根据用户“测试我自己来”的要求，本批不启动新 PIE。最终右键、屋顶过滤、粒子简化、蘑菇碰撞与远程界面合并组合尚未运行验收，也未打包或在线 AI 联调。
- 本地磁盘空间处理是工作站维护，没有把目录联接、备份、存档或本机路径纳入仓库。
- 其他两份历史说明中“未上传”描述指其撰写时状态；本次上传以 CHANGELOG 与 Git 远程 SHA 核对结果为准。
