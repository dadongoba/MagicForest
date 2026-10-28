# 更新记录

本文件记录增量仓库每次提交上传的内容。最新更新放在最前面；每次上传前必须把本条记录与对应文件一起提交。验证状态应区分“本次验证”“历史记录”与“未验证”。

## 2026-10-28-06：C 找人、V 三视角与独立移动控制

- **目的 / 基线 / 前置**：基于 main `410a29b`、相机批次 2026-10-28-01/05；C 找人，V 循环上帝/第一/第三人称。保留已有三笔本地提交与未跟踪 DefaultInput.ini。
- **文件 / 类型**：修改 Source/demo3_UE58/ForestIsometricPlayer.h/.cpp、ForestNPCInteraction.cpp；新增 Docs/Three_View_Controls_2026-10-28.md；修改本记录。无资产、配置或存档格式变更。
- **行为**：上帝视角屏幕方向移动、身体朝速度转向，右键观察和五秒空闲回正；第一/第三人称鼠标直接转头，W/S 沿水平视线前后移动，A/D 横移。第一人称眼高隐藏本人身体，第三人称右肩跟随/碰撞避障/独立滚轮距离。斜向限速；菜单/地图及失焦停止手动移动/转头，关闭菜单恢复对应鼠标模式；忽略捕获切换首帧位移。
- **冲突处理 / 应用**：三视角组件运行期间禁用导入 Pawn 的模板输入，统一处理移动、转头和跳跃，原背包数据/独立 B/E/M 组件保留；NPC 导航不再重复写入三视角玩家的朝向。应用三份源码后重新编译并重新打开项目；依赖和验收点见[操作说明](Docs/Three_View_Controls_2026-10-28.md)。
- **验证 / 未验证**：UE5.8.3 Win64 Development Editor 最终 UHT/源码编译（-NoLink）成功（33.19 秒），仅保留既有采集源码浮点截断警告；用户保存关闭编辑器后 DLL 链接、元数据写入成功（Result: Succeeded，3.50 秒）。重新启动 DemoMap 并成功进入 PIE，运行组件读回 Overhead / 5 秒 / 第三人称 400，确认新模块已加载。源码差异检查通过；C/V 实际按键、三视角移动手感、避障、找人/菜单/游泳组合仍待人工验收。
- **上传状态**：待上传；制作前 fetch 成功，远程 main 未出现新提交。推送成功后以远程 SHA 核对结果为准。

## 2026-10-28-05：右键观察防误释放与五秒无操作回正

- **目的 / 基线 / 前置**：修正右键旋转可能提前结束、松开即回正的问题；基于 main `1203aea`，相机前置批次 2026-10-28-01。保留已有两笔本地提交及未跟踪 DefaultInput.ini。
- **文件 / 类型**：修改 Source/demo3_UE58/ForestIsometricPlayer.h/.cpp；新增 Docs/Camera_Idle_Return_2026-10-28.md；修改本记录。无资产或配置变更。
- **行为**：使用 Slate 实际右键状态判断释放，取消控制器释放事件及逐帧光标强写；松开保留角度，连续五秒无操作后平滑回到初始方向。鼠标/键盘操作、持续按键重置计时，菜单/全图期间暂停；保留滚轮距离。CameraReturnIdleSeconds 默认 5 秒。
- **依赖 / 应用**：同步两份源码并编译，重新加载 DLL；沿用原组件和地图，无删除或迁移。详见[说明](Docs/Camera_Idle_Return_2026-10-28.md)。
- **验证 / 未验证**：本次 UE5.8.3 Win64 UHT、最终源码编译通过；首次链接被运行中编辑器占用 DLL 阻挡。用户保存关闭后重新构建成功（Result: Succeeded，2.47 秒），下次打开项目加载新 DLL。未启动 PIE，旋转手感、计时及菜单切换待游戏内验收；源码差异与增量同步已检查。
- **上传状态**：待上传；制作前 fetch 因 GitHub 连接重置失败。





## 2026-10-28-04：修复纸页首帧绘制断言崩溃

- **目的 / 基线 / 前置**：修复批次2026-10-28-03（本地提交 `8eec735`）引入的启动游戏即崩溃；以本批覆盖该版本。
- **文件 / 类型**：修改 Source/demo3_UE58/ForestUITheme.cpp、Docs/Adaptive_Paper_2026-10-28.md、本记录；无资产或配置变化。
- **原因 / 修正**：两份最新本地崩溃报告均指向SForestPaperBorder::OnPaint闭合边框时TArray自身元素引用追加的CheckAddress断言。改为先把首点复制到独立局部变量，再追加到数组，保留按尺寸绘制背景。
- **依赖 / 应用**：同步cpp并重新编译，关闭旧编辑器进程后加载新DLL；保留前批h文件。不要使用前批未修正版运行。
- **验证 / 未验证**：崩溃调用栈与UE本机TArray检查源码吻合；修正后Win64 Development Editor编译链接成功（4.97秒）。按用户此前要求未启动新的PIE，运行恢复仍待用户验证。
- **上传状态**：待上传；前批GitHub网络失败，当前先修复本地项目。

## 2026-10-28-03：按纸页实际尺寸重绘公共背景

- **目的 / 基线 / 前置**：修正所有纸页背景与文字范围不一致；遵照用户要求按UI尺寸重新设计同风格背景，基于main `4dc6b89`，前置2026-10-28-01/02。
- **文件 / 类型**：修改 Source/demo3_UE58/ForestUITheme.h/.cpp；新增 Docs/Adaptive_Paper_2026-10-28.md；修改本记录。无资产删除或配置变化。
- **行为**：背景以当前Slate局部宽高实时绘制暖纸渐变、细纸边与植物装饰，不再直接使用原PNG；共享文字安全区左右至少16/上下14，边框、装饰和子内容约束在同一容器。既有更大边距与窗口响应布局保留。
- **依赖 / 应用**：合并两份源码并重新编译；已有SForestPaperBorder界面自动生效。旧PNG保留，元素图标依赖不变。各界面尺寸及未验证边界见[适配说明](Docs/Adaptive_Paper_2026-10-28.md)。
- **验证 / 未验证**：Windows UE5.8.3 Development Editor编译链接成功（13.84秒）；未启动新PIE，实际DPI、长文与不同窗口下的显示由用户验收。
- **上传状态**：待上传；推送并核对远程SHA后补记。

## 2026-10-28-02：补记 Dreamscape 增量上传结果

- **目的 / 基线 / 前置**：补记批次 2026-10-28-01，基于功能提交 `e1a1e27`。
- **文件 / 类型**：仅修改 CHANGELOG.md；无额外应用步骤、功能文件或依赖变化。
- **验证**：功能与两份地图的 Git LFS 对象已推送；远程 main 完整 SHA `e1a1e271e45e89ebef649b029ed2780b0e5ebb45` 与本地一致。合并源码编译成功，编辑器已重新打开并确认目标项目 MCP 连接。
- **未验证 / 保留**：沿用前批未做新 PIE 和打包的边界；原有未跟踪 Config/DefaultInput.ini 保持原位、未提交。本条推送结果以最终远程 SHA 核对为准。

## 2026-10-28-01：Dreamscape 主地图、ChengYi 俯视角色与交互修正

- **目的 / 基线 / 前置**：汇总本地尚未发布的场景与操作更新；完整 demo3_UE58 1.5、UE 5.8.3，基于 main `ac8142b`，前置截至 2026-10-27-15。已先快进同步远程界面更新，并三方合并本地采集修改，保留远程居中字条及炼药/NPC界面修订。
- **新增源码**：Source/demo3_UE58/ForestIsometricPlayer.h/.cpp、ForestMinimap.h/.cpp。
- **修改源码 / 配置**：Source/demo3_UE58/ForestAlchemy.cpp、ForestHarvestMagic.h/.cpp、ForestNPCSetupLibrary.cpp；Config/DefaultEngine.ini 仅三个默认地图/模式路径。
- **新增资产**：Content/ForestMigration 下角色/模式蓝图、Dreamscape_Gameplay.umap、小地图纹理；Content/DreamscapeSeries 下修改后的 DemoMap.umap、两个预览 Pawn 蓝图、三份植被主材质（相对本增量仓库为新增）；Content/Forest/Camera/MPC_ForestCameraOcclusion.uasset；Content/Characters/Swimming/AS_Swimming.uasset。完整路径及 SHA256 见 [文件清单](Docs/Dreamscape_Upload_2026-10-28_files.json)。地图采用既有 Git LFS 规则。
- **文档**：新增 Docs/Dreamscape_Upload_2026-10-28.md、对应清单、Dreamscape_Migration_2026-10-27.md、Minimap_and_Mushroom_Orientation_2026-10-27.md、Camera_Roof_Input_FX_2026-10-28.md；修改 README.md 与本记录。
- **行为**：DemoMap 默认启动、ChengYi 固定俯视移动、右键拖动观察并松开回正、滚轮缩放；M全图及NPC/小屋定位；玩法/NPC迁移；蘑菇朝上且不挡玩家/NPC，删除39个独立菌盖Actor后保留872个采集对象；屋顶专用遮挡；移除前段漩涡/尘粒，保留后段光效；修正预览Pawn的EnableInput Controller参数。
- **依赖 / 应用**：先备份并准备匹配第三方素材；LFS下载地图，按原路径应用清单，合并配置后重新编译。保留原Demo_Day与源蘑菇网格；不上传完整Dreamscape/ChengYi/Witch_House素材包、存档、缓存或二进制程序。已有未跟踪DefaultInput.ini保持原位且不纳入本次提交。详见 [应用与边界](Docs/Dreamscape_Upload_2026-10-28.md)。
- **本次验证**：三方合并无冲突；Windows UE5.8.3 Development Editor UHT、源码编译及链接成功（Result: Succeeded，29.20秒）。上传前逐文件同步/哈希、差异及凭据检查；保留原有浮点截断编译警告。
- **历史 / 未验证**：迁移、小地图早期PIE及蓝图编译读回见各说明；按用户要求本批不启动新PIE。最终相机、家具遮挡、采集粒子、碰撞、游泳与界面合并组合的运行测试及打包仍由用户验收。
- **上传状态**：已上传 main，功能提交 `e1a1e27`，远程完整 SHA 已核对；补记见批次 2026-10-28-02。

## 2026-10-27-15：补记28号字条上传结果

- **目的 / 基线 / 前置**：补记批次14，基于功能提交 `5f98c8c`。
- **文件 / 类型 / 应用**：仅修改CHANGELOG.md，无依赖或应用步骤变化。
- **验证**：推送成功，远程main完整SHA `5f98c8ca2ff8d573eba314f2a2c05e8993f643ea` 与本地一致；编译、同步与离线样例验证见前批。
- **未验证**：沿用前批原生DPI与实机阅读边界，未启动UE。

## 2026-10-27-14：两张居中字条正文提高到28号

- **目的 / 基线 / 前置**：回应用户正文仍小；基于main `54ddf94`，前置批次12/13。
- **修改**：Source/demo3_UE58/ForestHarvestMagic.cpp、ForestNPCInteraction.cpp；Docs/Item_Emergence_Preview.html/.jpg、NPC_Travel_Centered_Preview.html/.jpg、HarvestMagic_2026-10-13.md、NPC_Finder_UI_2026-10-14.md；本记录。
- **行为**：物品涌现/交付后的引路故事正文22→28，常规行进框高400→480；保留居中、大字不整体缩小、窄窗高度限制。其他界面和游戏逻辑不变。
- **依赖 / 应用**：合并两份cpp并编译，沿用主题/PNG及前序声明，无地图/配置/存档变化。
- **验证**：本次Mac UE5.8源码编译与链接成功；源码同步、差异检查；物品预览三尺寸样本在视口内，引路预览1280×720/800×600样例28px且完整可见；更新截图。
- **未验证**：未开UE/PIE；实际DPI、移动阅读、长故事和极矮窗口仍待验收。HTML字号像素不等同于Slate物理像素。
- **上传状态**：已上传main，功能提交 `5f98c8c`，远程完整SHA已核对。

## 2026-10-27-13：补记字条居中修订上传结果

- **目的 / 基线 / 前置**：补记批次12，基于功能提交 `82ce2e0`。
- **文件 / 类型 / 应用**：仅修改CHANGELOG.md，无依赖与应用步骤变化。
- **验证**：推送成功，远程main完整SHA `82ce2e0e43b407f0fdf75eda7cbabc9a0aa029d3` 与本地一致；源码编译/同步及离线居中检查见前批。
- **未验证**：沿用前批原生DPI、遮挡和运行验收边界，未启动UE。

## 2026-10-27-12：物品涌现与交付后的NPC引路故事居中

- **目的 / 基线 / 前置**：按用户明确位置与入口，将两张字条居中；main `03bfd46`，前置批次09/10。
- **修改**：Source/demo3_UE58/ForestHarvestMagic.cpp、ForestNPCInteraction.cpp；Docs/Item_Emergence_Preview.html、Item_Emergence_Preview.jpg、HarvestMagic_2026-10-13.md、NPC_Finder_UI_2026-10-14.md；本记录。
- **新增**：Docs/NPC_Travel_Centered_Preview.html、NPC_Travel_Centered_Preview.jpg，独立展示交付物品后的引路文本。
- **行为**：物品涌现Overlay垂直居中、对称24边距；NPC Travelling故事页也居中，保留既有大字、尺寸和鼠标穿透。普通对话/附近提示原位置保留，不改交付、AI、寻路或退款。
- **依赖 / 应用**：合并两份cpp并编译，沿用纸页PNG及前批Widget声明，无地图/配置/存档变更。旧顶部/底部展示为历史，以本批独立预览为准。
- **验证**：Mac UE5.8源码编译与链接成功；源码同步、差异检查；两张离线字条三尺寸中心差值均0且在视口内，保存对应截图。
- **未验证**：未开UE/PIE，原生DPI、实际遮挡与行进切换待实机验收；截图为离线样例。
- **上传状态**：已上传main，功能提交 `82ce2e0`，远程完整SHA已核对。

## 2026-10-27-11：补记物品涌现与NPC引路界面上传结果

- **目的 / 基线 / 前置**：补记批次09/10；基于 `5243a1d`，包含 `2a74fd2`。
- **文件 / 类型 / 应用**：仅修改CHANGELOG.md，无额外应用步骤和依赖变化。
- **验证**：首次上传因LFS锁检查域名解析超时失败；本次无LFS资产变更，仅对推送命令临时禁用该远程锁检查后成功，未修改仓库配置。远程main完整SHA `5243a1d1d67a247026333ec7d55e09e105eda901` 与本地一致，并包含两批功能。源码编译/同步/离线预览验证见前批。
- **未验证**：未打开UE/PIE，沿用两批原生DPI、焦点、实际背包/移动/退款验收边界。

## 2026-10-27-10：修复消耗物品寻找NPC的界面尺寸与字号

- **目的 / 基线 / 前置**：继续修复用户指出的选人/交付/行进UI问题；基于本地批次09提交 `2a74fd2`，依赖前序NPC寻找与共享纸页主题。
- **修改**：Source/demo3_UE58/ForestNPCInteraction.h/.cpp；Docs/NPC_Finder_Preview.html、NPC_Finder_Preview.jpg、NPC_Finder_Preview_800.jpg、NPC_Finder_UI_2026-10-14.md、HarvestMagic_2026-10-13.md；本记录。后者补记用户已确认批次09入口。
- **新增**：Docs/NPC_Finder_Payment_Preview.jpg、NPC_Finder_Travel_Preview.jpg。
- **行为**：取消整体缩小，按Slate可用尺寸布局；选人/物品卡换行，交付页窄窗上下排列，按钮独立保留，文字至少18、描述/故事22；行进纸页扩大且保持鼠标穿透。共享对话宿主跟随可用尺寸，但原对话字号/逻辑保留。
- **依赖 / 应用**：合并两份源码编译，沿用纸页/元素PNG；无资产、配置、存档变更。详细行为与边界见NPC_Finder_UI追加说明。
- **验证**：Mac UE5.8 UHT、源码/链接成功；两份源码同步、差异检查；Actor扣物品/寻路/退款逻辑与基线逐字一致；离线四尺寸无横向越界、交付按钮在面板内，演示手动选物与生成目标故事切换通过。
- **未验证**：未开UE/PIE；原生DPI/焦点/长内容及实际扣料、移动、退款待实机验收，HTML不是游戏运行测试。
- **上传状态**：已上传main，本批功能提交 `5243a1d`，同时包含批次09 `2a74fd2`；远程完整SHA已核对。

## 2026-10-27-09：修复物品涌现提示的固定小框

- **目的 / 基线 / 前置**：针对“涌现时框特别小”追查发现物品提示被锁620×160；基于main `9cd8fa4`，前置采摘炼化与纸页主题。此前两批修的是不同界面。
- **修改**：Source/demo3_UE58/ForestHarvestMagic.h/.cpp；Docs/HarvestMagic_2026-10-13.md；本记录。
- **新增**：Docs/Item_Emergence_Preview.html、Item_Emergence_Preview.jpg。
- **行为**：仅调整“魔法书捎来一件小物 / 获得：名称·属性”提示。移除固定视口尺寸和一次性物理像素定位；全视口Slate锚定，宽度上限960随窗口更新，高随文案展开。标题18/正文22，保留AI、入包、8秒时长与鼠标穿透。
- **依赖 / 应用**：合并两份源码编译，沿用纸页主题/PNG；无资产、配置、存档改动，不修改合成台/推演页或旧锅蓝图。
- **验证**：本次Mac UE5.8 UHT、源码编译和链接成功；源码同步与差异检查；离线样例三尺寸框随文案增高、文本在视口内且无横向越界。
- **未验证**：未开UE/PIE；用户已确认物品名称/属性/描述入口，原生DPI/连续回复/最长文案和极矮窗口待运行验收。截图非引擎；极端窗口可通过背包查看完整描述，本提示未加交互滚动。
- **上传状态**：已上传main，功能提交 `2a74fd2`，随批次10一并推送且远程HEAD祖先关系已核对。

## 2026-10-27-08：补记常规字号放大上传结果

- **目的 / 基线 / 前置**：补记2026-10-27-07，基于功能提交 `c2a3a61`。
- **文件 / 类型 / 应用**：仅修改CHANGELOG.md，无依赖或应用步骤变更。
- **验证**：推送成功，远程main完整SHA `c2a3a61d598699c2922e7a58bdabdb4d7c09e885` 已核对；编译与三尺寸离线检查见前批。
- **未验证**：沿用前批原生字体/DPI与运行验收边界，未启动UE。

## 2026-10-27-07：放大合成台与结算页常规字号

- **目的 / 基线 / 前置**：用户明确正常窗口下字也小；基于main `9eb3e29`，前置2026-10-27-05/06。
- **修改**：Source/demo3_UE58/ForestAlchemyUI.cpp；Docs/Brew_Result_Preview.html、Brew_Result_Preview.jpg、Brew_Result_Adaptive_800.jpg、Daily_Arcana_Brewing_Preview.html、Brew_Result_2026-10-27.md；本记录。
- **行为**：直接放大正常字号，Slate普通正文14→18、推演18→22、按钮13→18，小字至少18；按钮上下内边距增加。保留换行/滚动与固定操作区，不改共享主题及其他界面。
- **依赖 / 应用**：合并一份源码并编译；沿用主题字体与前批布局，无配置/资产/存档迁移。
- **验证**：本次Mac UE5.8源码编译与链接成功；源码同步/差异检查；离线结算三尺寸均保留故事22px/按钮18px、继续按钮在窗口内且无横向越界；更新截图。
- **未验证**：未开UE/PIE；原生DPI与实际阅读仍需验收。字号不是物理像素，HTML截图不替代Slate实机。
- **上传状态**：已上传main，功能提交 `c2a3a61`，完整远程SHA已核对。

## 2026-10-27-06：补记自适应修复上传结果

- **目的 / 基线 / 前置**：补记2026-10-27-05；基于功能提交 `7a6e01a`。
- **文件 / 类型 / 应用**：仅修改CHANGELOG.md，无额外依赖或应用步骤。
- **验证**：首次推送连接中断，重试成功；远程main完整SHA `7a6e01a7a6f48c645f80be81191657991d21355e` 已核对。源码编译和离线预览验证见前批。
- **未验证**：沿用前批原生UI、DPI与运行验收边界，未启动UE。

## 2026-10-27-05：修复合成台与结算页窗口自适应

- **目的 / 基线 / 前置**：修复固定画布整体缩放和预览只按宽度缩放；基于main `e8c05e8`，前置2026-10-27-03/04。
- **修改**：Source/demo3_UE58/ForestAlchemy.h、ForestAlchemyUI.cpp；Docs/Brew_Result_Preview.html、Brew_Result_Preview.jpg、Daily_Arcana_Brewing_Preview.html、Brew_Result_2026-10-27.md；本记录。
- **新增**：Docs/Brew_Result_Adaptive_800.jpg（离线窄窗截图）。
- **行为**：按实际Slate逻辑宽高调整页面，保留正文大小；低于900宽度纵向重排，选择/工具/操作按钮可换行；内容滚动、底部按钮独立保留。布局变化不重置选料/确认/AI请求。不修改其他界面。
- **依赖 / 应用**：合并两份源码并编译，沿用纸页主题及PNG，无资产/配置/存档变更。详见原接入说明追加的自适应修正节。
- **验证**：本次Mac UE5.8 UHT、源码编译和链接成功；两份源码同步、差异检查；离线结算预览四尺寸均无横向越界、继续按钮均在视口内，保存800/1280宽截图。
- **未验证**：未打开UE/PIE；原生DPI、多屏、实际输入/滚动/缩放仍需运行验收；截图为HTML，非Slate实机。布局阈值切换会重置滚动位置。
- **上传状态**：已上传main，功能提交 `7a6e01a`，远程完整SHA已核对。

## 2026-10-27-04：补记封瓶结算与推演上传结果

- **目的 / 基线 / 前置**：补记2026-10-27-03；基于功能提交 `1b3756e`。
- **文件 / 类型**：仅修改CHANGELOG.md，无额外应用步骤或依赖变更。
- **验证**：功能推送成功，远程main完整SHA `1b3756e47cbd9390d57e9337a3814d4a8e8e8fd4` 与本地一致；最终源码编译成功、差异检查通过。
- **未验证**：沿用前批未做UE/PIE、真实AI和故障恢复运行验收的边界；截图为离线预览。

## 2026-10-27-03：封瓶后显示偏离结果并接入世界推演

- **目的 / 基线 / 前置**：按用户要求将偏离面板从调配操作移到封瓶后，与推演同时显示；基于main `70e93b3`，前置2026-10-27-01/02。
- **修改**：Source/demo3_UE58/ForestAlchemy.h/.cpp、ForestAlchemyUI.cpp；Plugins/DialogueMemoryTable/Source/DialogueMemoryTable/Private/AlchemyGameplaySubsystem.cpp；Docs/Daily_Arcana_Brewing_Preview.html；本记录。
- **新增**：Docs/Brew_Result_2026-10-27.md、Brew_Result_Preview.html、Brew_Result_Preview.jpg。
- **行为**：封瓶确认扣料并保存成品；结果页并排显示药性/偏离/评级与推演。共享AI接口读取冻结委托、成品、背景和记忆，失败/超时显示本地记录；阅读后点击继续才换天。存档保存未读结果并防止迟到回调覆盖、重复扣料/推进。违约计算保留原判定、补齐指标。
- **依赖 / 应用**：合并4份源码并编译，沿用主题PNG、插件、关卡；无二进制资产/配置覆盖或删除。详见[接入与验证边界](Docs/Brew_Result_2026-10-27.md)。旧预览截图为历史版本。
- **验证**：本次Mac UE5.8源码/UHT/链接成功；文件同步、差异检查；离线等待/完成/失败三态及整页截图；评级分支与恢复路径源码审查。
- **未验证**：未开UE/PIE，未做真实AI联调、存读档/并发故障运行测试或打包。生成风格/字数仅为提示约束；未新增实际地图后果、奖励或完整元素库存存档。截图非Slate实机。
- **上传状态**：已上传main，功能提交 `1b3756e`，远程完整SHA已核对。

## 2026-10-27-02：补记大牌与夜间合成台上传结果

- **目的 / 基线 / 前置**：补记2026-10-27-01；基于功能提交 `6ea058e`。
- **文件 / 类型**：仅修改 CHANGELOG.md。
- **验证**：功能推送成功，远程 main 完整SHA与本地功能提交一致，差异检查通过。
- **应用 / 未验证**：无额外文件；沿用前批全部应用步骤、依赖与未做UE运行验收的限制。

## 2026-10-27-01：今日大牌背包、药瓶抽牌与夜间合成台

- **基线 / 前置**：main `9b90c83`，前置纸页主题、采摘小物、委托与世界日；完整demo3_UE58 1.5。
- **新增**：Source/demo3_UE58/ForestAlchemy.h、ForestAlchemy.cpp、ForestAlchemyUI.cpp；Config/DefaultForestAlchemy.ini；Docs/Daily_Arcana_Brewing_2026-10-27.md、Daily_Arcana_Brewing_Preview.html、Daily_Arcana_Brewing_Preview.jpg。
- **修改**：Source/demo3_UE58/ForestHarvestMagic.h/.cpp、PotionMimicWorldSubsystem.h/.cpp；Plugins/DialogueMemoryTable/Source/DialogueMemoryTable/Private/DailyMushroomSubsystem.cpp；本记录。
- **行为**：背包新增今日大牌页；拿起现有药瓶本地随机抽22张大牌，每瓶每天一次、允许重复牌型，持久化逐瓶领取记录。开始游戏/换天查找室内出生点。E开夜间合成台，按原型栏目放入大小牌、调序、移除、额度/槽位/库存限制和本地试算，不显示3D轴。
- **封瓶**：二次确认后扣槽内材料/大牌、保存成品数值并换天；新日恢复药瓶、清空旧牌、回小屋。增加换天保存失败的回滚。成品为数值记录，未新增实体药剂、玛瑙奖励或NPC交付记忆。
- **依赖 / 应用**：合并9份源码/配置并编译，保留既有关卡、PlayerStart、小屋药瓶模型与共享主题PNG。无地图/蓝图/缓存覆盖。详见[接入说明](Docs/Daily_Arcana_Brewing_2026-10-27.md)。
- **验证**：本次UE 5.8 Mac Development Editor的UHT、编译、链接成功；源码同步、差异及凭据扫描；离线栏目预览截图。
- **未验证 / 边界**：未开UE/PIE，出生/拿瓶/读档/扣料/换天仍需运行验收。夜间为面板阶段，不改天空；沿用既有评分未重新平衡，旧元素背包未增加完整数量存档。截图非引擎实机。
- **上传状态**：已上传 main，功能提交 `6ea058e`，远程完整SHA已核对。

## 2026-10-14-09：补记NPC寻找自适应界面上传结果

- **目的 / 基线 / 前置**：补记批次 2026-10-14-08，基于功能提交 `6a110a6`。
- **文件 / 类型**：仅修改 CHANGELOG.md，补齐上传状态。
- **验证**：推送成功，远程 main 完整 SHA 与本地功能提交一致；工作区差异检查通过。
- **应用 / 未验证**：无额外文件；沿用前批依赖、应用方式与运行限制，未启动UE。

## 2026-10-14-08：NPC寻找纸页界面与窗口自适应

- **目的 / 基线 / 前置**：用户确认三阶段预览并要求自适应尺寸；基于 main `4d8b4ab`，前置 2026-10-14-06/07。
- **修改**：Source/demo3_UE58/ForestNPCInteraction.h/.cpp；Content/UI/ForestUI.uss（设计清单）；本记录。
- **新增**：Docs/NPC_Finder_Preview.html、NPC_Finder_Preview.jpg、NPC_Finder_Preview_800.jpg、NPC_Finder_UI_2026-10-14.md。
- **行为**：人物三选一带头像空框；支付从文字下拉改为物品格与右侧详情，手动选择后启用交付；移动故事独立成纸页。头像后续由游戏截图补充。既有AI、费用、路线和退款逻辑保留。
- **尺寸**：SScaleBox按实际可用宽高等比缩小、限制最大尺寸、留24 Slate单位边距；物品和详情可滚动。找人与交付居中，移动/对话靠下。本批不修改其他界面的尺寸策略。
- **依赖 / 应用**：合并两份源码并编译；沿用前批共享主题与两张PNG，不改地图/存档。详细说明见 [NPC寻找界面](Docs/NPC_Finder_UI_2026-10-14.md)。
- **验证**：本次 Mac UE 5.8 Development Editor 编译、UHT与链接成功；离线预览已检查切换人物清空选择、选择物品进入叙事及四种窗口尺寸，无横向越界/按钮越界；源码与完整工程逐文件同步核对。
- **未验证**：未启动UE/PIE；截图为HTML离线预览，非Slate实机。游戏内DPI/点击命中/长内容/费用与退款仍待运行验收。
- **上传状态**：已上传 main，功能提交 `6a110a6`，远程完整 SHA 已核对。

## 2026-10-14-07：补记纸页主题上传结果

- **目的 / 基线 / 前置**：补记批次 2026-10-14-06；基于功能提交 `9ab81f5`。
- **文件 / 类型**：仅修改 CHANGELOG.md，补齐上传状态。
- **验证**：功能提交推送成功，远程 main SHA 与本地功能提交一致。
- **应用 / 未验证**：无额外文件，沿用前批依赖、应用步骤与全部运行/美术限制；未启动UE。

## 2026-10-14-06：纸页主题、紧凑背包与手绘元素标记

- **目的 / 基线 / 前置**：按用户参考图修订原深色 Slate UI，并纠正被否定的未发布平面卡片方案。基于 main `9980c58`，前置 2026-10-14-04/05。
- **新增**：Source/demo3_UE58/ForestUITheme.h/.cpp；Content/UI/Textures/WoodlandPaper.png、ElementEmblems.png（运行素材）；Docs/UI_Reference_Warm.png（仅原图参考）、UI_Warm_Preview.jpg（HTML截图）、Warm_UI_2026-10-14.md（依赖、限制、生成提示词）。
- **修改**：Source/demo3_UE58/ForestNPCInteraction.cpp、ForestHarvestMagic.cpp/.h、demo3_UE58.Build.cs；Content/UI/ForestUI.uss；Docs/UI_Redesign_Preview.html、Slate_UI_2026-10-14.md；本记录。
- **行为**：统一暖棕文字/纸底/橄榄绿操作，实际Slate容器接纸页贴图；背包改为四列紧凑格子、右侧说明，三种元素用水彩通用图标。手动选择、描述、支付与原有玩法保留；炼药仍保留原蓝图布局。
- **依赖 / 应用**：按相对路径合并六份源码及两张PNG，重新编译；Build.cs将PNG作为NonUFS松散文件分发。USS仅设计清单。无地图、存档、缓存或编译产物，未覆盖本地地图。详见 [修订说明](Docs/Warm_UI_2026-10-14.md)。
- **验证**：本次 Mac UE 5.8 Development Editor 编译与链接成功；HTML离线视觉预览并保存截图；差异检查、JSON解析、源码同步校验通过。
- **未验证 / 完成度**：未启动UE/游戏，未做PIE、低分辨率、PNG运行加载与打包验收；截图不是Slate实机截图。暂无专属道具插画/头像/皮革分类页签，未达到参考图完整美术完成度。
- **上传状态**：已上传 main，功能提交 `9ab81f5`，远程完整 SHA 已核对。

## 2026-10-14-05：补记界面更新与新设计上传结果

- **目的 / 基线 / 前置**：补记批次 2026-10-14-04 的上传结果，基于功能提交 `b2d3b8b`。
- **文件 / 类型**：仅修改 CHANGELOG.md，补齐功能与设计文件的上传状态。
- **验证**：157 MB 地图 LFS 上传成功；远程 main 与本地功能提交完整 SHA 一致。本次差异检查与凭据模式扫描通过。
- **应用 / 未验证**：无需额外应用文件，沿用 2026-10-14-04 的依赖与运行验收限制。

## 2026-10-14-04：森林手札 Slate UI、DeepSeek 在线与追踪朝向

- **基线 / 前置**：main `21168d6`，完整 demo3_UE58 1.5，前置 2026-10-14-01/02。
- **修改**：ForestNPCInteraction.h/.cpp、ForestHarvestMagic.h/.cpp（均在 Source/demo3_UE58）；Content/Biomes/PNB_Enchanted_Forest/Map/Demo_Day.umap；本记录。
- **新增**：Docs/Slate_UI_2026-10-14.md；设计参考 Content/UI/ForestUI.uss 与 Docs/UI_Redesign_Preview.html（HTML 可在浏览器中直接打开）。USS 为 JSON 样式设计参考，目前源码未接入其加载，不代表运行界面与小样完全一致。
- **行为**：NPC 寻找、交谈、背包、采摘提示和采集 HUD 改用原生 Slate；炼药面板仅统一主题，仍保留蓝图布局。自动追踪按玩家实际 XY 移动方向平滑更新身体与视角。关卡管理器关闭离线模式。
- **Key**：仅本机环境变量配置，KEY.env 与凭据未收录。
- **历史验证（来自本地开发说明）**：Windows UE 5.8 Editor 编译与链接成功；项目启动和 Slate 常驻提示条已检查；DeepSeek 最小在线请求成功。
- **本次上传检查**：四份源码与地图和完整工程 SHA256 一致；USS JSON 解析通过；检查增量差异与凭据。本次未启动编辑器或重复运行验收。
- **未验证**：所有面板输入与点击、扣费/退款、追踪朝向、完整 NPC 在线对话和记忆总结仍需运行验收；炼药布局不是完整 Slate 重构。
- **应用 / 回退**：覆盖四份源码并编译，地图可仅合并管理器 Offline Test Mode=false；本机源码地图已有备份。详见新增说明。
- **上传状态**：已上传 main，功能与新设计提交 `b2d3b8b`，地图 LFS 已上传，远程 SHA 已核对。

## 2026-10-14-03：补记快捷键与 NPC 接入修复上传结果

- **目的 / 基线 / 前置**：补记 2026-10-14-01/02 上传；基于功能提交 `7b78843`。
- **文件 / 类型**：仅修改 `CHANGELOG.md`，补齐上一批已上传状态。
- **核对 / 应用**：`8bb0467` 与 `7b78843` 均已上传 main，远程 SHA 核对一致；无额外玩法文件，沿用前序批次应用步骤。
- **验证 / 未验证**：仅源码编译和链接通过，未运行验收；UE 保持关闭，不因发布记录而启动。

## 2026-10-14-02：补齐 B 入口与实际 NPC 控制器绑定

- **目的 / 基线 / 前置**：回应 B 仍不可用和 NPC 功能未绑定的反馈；基于已上传 `8bb0467`，前置 2026-10-14-01。
- **修改**：`Source/demo3_UE58/ForestNPCInteraction.h`、`.cpp`、`Source/demo3_UE58/ForestHarvestMagic.cpp`、`Docs/NPC_Finder_Fix_2026-10-14.md`；本记录同时补记上一批上传。
- **行为**：B 仅避让实际打开的 NPC 对话/找人面板，不因鼠标可见无声拒绝；主游戏世界每秒自动将三类现有 NPC 绑定到功能所用的 ForestRoadAIController，已有绑定跳过，排除玩家与子世界，保证 V 管理器存在。接管旧蓝图控制器占有但不删除资产或旧控制器。
- **依赖 / 应用**：合并三份源码并重编译；仍需既有导航/道路数据，不生成导航。原生控制器停用旧漫游计时器以避免两套移动争抢。说明见 [修复记录](Docs/NPC_Finder_Fix_2026-10-14.md)。
- **验证 / 未验证**：UE 5.8 Mac Development 编译、生成代码与链接通过；按用户要求 UE 保持关闭，未做运行与 UI 验收，不把编译当作场景接入验收。
- **上传状态**：已上传 main，功能提交 `7b78843`，远程 SHA 已核对。

## 2026-10-14-01：修复 V 入口、启动崩溃与手动选择代价物品

- **目的 / 基线 / 前置**：修复 V 无响应的初始化依赖并要求玩家明确选择交付物品；基于 main `90cec96`，前置采摘与寻路故事批次。
- **修改**：`Source/demo3_UE58/ForestNPCInteraction.h`、`.cpp`、`Source/demo3_UE58/ForestHarvestMagic.h`、`.cpp`、`Docs/NPC_Finder_2026-10-12.md`。
- **新增**：`Docs/NPC_Finder_Fix_2026-10-14.md`。
- **入口 / 兼容**：主游戏世界自动保证交互管理器存在，不依赖 NPC 原生控制器先生成；支持原蓝图 NPC 作为寻找目标及普通 AI 控制器交谈，排除未映射角色；E 不吞采摘输入。
- **物品选择**：确认页新增可用物品下拉列表，无默认项；未选择禁止确认，只扣明确选定的一件；库存变动要求重选，不自动替换。叙事继续使用实际消耗键和属性快照。
- **崩溃修复**：历史 PIE 日志定位直接创建抽象 UUserWidget 失败后访问空 WidgetTree；改为具体采摘提示控件子类，并检查创建结果。
- **应用 / 依赖**：合并四份源码后重编译，无二进制资产、地图或配置覆盖。详见 [修复说明](Docs/NPC_Finder_Fix_2026-10-14.md)。
- **验证 / 未验证**：UE 5.8 Mac Development 编译与链接通过；曾启动项目准备测试，收到用户禁止打开 UE 后关闭本次进程，未运行本批 PIE。V 界面、选择焦点、扣费/退款、导航、普通 NPC 停步恢复及真实 AI 仍待运行验收。
- **上传状态**：已上传 main，功能提交 `8bb0467`，远程 SHA 已核对。

## 2026-10-13-05：补记寻路故事更新上传结果

- **目的 / 基线 / 前置**：记录批次 2026-10-13-04 的发布；基于功能提交 `cb0d06a`。
- **文件 / 类型**：仅修改 `CHANGELOG.md`，补记已上传状态。
- **验证**：功能提交已推送至 main，本地与远端 SHA 一致；无需额外应用，验证与未验证项沿用 2026-10-13-04。

## 2026-10-13-04：寻路故事由交出物品主导

- **目的 / 基线 / 前置**：把临时路引占位替换为已交出物品带玩家找到已选 NPC 的故事；基于 main `6427a6e`，前置批次 2026-10-13-01 与 2026-10-13-02。
- **修改**：`Source/demo3_UE58/ForestTravelNarrativeSubsystem.h`、`.cpp`、`Source/demo3_UE58/ForestNPCInteraction.cpp`、`Docs/NPC_Finder_Emergent_Narrative_2026-10-13.md`。
- **提示词**：带路主体只允许原物品、其部件、连续变化后的形态；加入纽扣滚动的正例和新造衣袖的反例。传入名称、道具描述、目标 NPC 和固定属性；第二人称短故事，不再生成新路引名称。
- **接入 / 兜底**：沿用扣费后共享 AI 请求与行进 HUD；只解析 narrative，显示名固定为消耗物品名；检查长度及物品/NPC 名，失败保留原物品直接引路的本地故事。迟到请求隔离和移动不阻塞逻辑保持。
- **应用 / 依赖**：合并三份源码并重编译，无蓝图、关卡或新特效资产。详见 [当前接入说明](Docs/NPC_Finder_Emergent_Narrative_2026-10-13.md)。
- **验证 / 未验证**：UE 5.8 Mac Development 编译与链接通过；未开 UE，未做 PIE 或真实 AI 质量验收。主体来源属于提示词约束，格式校验不等于完整语义校验。
- **上传状态**：已上传 main，功能提交 `cb0d06a`，远程 SHA 已核对。

## 2026-10-13-03：补记采摘魔法化形上传结果

- **目的 / 基线 / 前置**：确认批次 2026-10-13-02 的上传；基于 `4f757c6`，功能提交 `d5b8091`。
- **文件 / 类型**：仅修改 `CHANGELOG.md`，补记上传结果，不涉及玩法或资产。
- **核对**：功能与上传记录已推送至 main，远程 SHA 核对一致；工作区干净。
- **应用 / 验证 / 未验证**：无需应用额外功能文件，沿用 2026-10-13-02 的依赖、编译成功与未做 PIE/真实 AI 验收限制。

## 2026-10-13-02：采摘魔法化形、具象 AI 道具与可点击背包

- **目的 / 基线**：采摘后由魔法书炼出火/风/水，并化形成具象小物；基于 main `750a059` 与批次 2026-10-13-01。
- **新增**：`Source/demo3_UE58/ForestHarvestMagic.h`、`Source/demo3_UE58/ForestHarvestMagic.cpp`、`Docs/HarvestMagic_2026-10-13.md`。
- **修改**：`Source/demo3_UE58/ForestNPCInteraction.cpp`、`Source/demo3_UE58/demo3_UE58.Build.cs`。
- **接入**：WorldSubsystem 自动监听既有成功采摘后的 Backpack 增量，将新增蘑菇替换为“具象名称 · 火5/风3/水6”等条目；B 打开背包，点击条目查看道具描述；沿用原背包投料、扣费与退款。
- **AI / 数据**：复用共享 AI 接口，提示词为“具体物件 + 魔法异常”，返回名称与道具描述；元素/点数/三轴值/采摘日从每日表冻结。无 Key 或失败用本地具象道具；迟到响应不重建已移出物品。同属性同名记录防冲突。元数据账本保存描述与属性，不新增完整库存数量存档。
- **动效**：使用现有 MimicBook 模型和展开动作、封面发光、Portal、尘粒和元素粒子；短暂原料模型悬浮缩小消失；临时魔法书影像不移动房间原书。未新增褪色材质或专用特效资产。
- **依赖 / 应用**：合并四份源码后重编译；新增 Niagara 模块依赖；需要完整 1.5 工程已有 MimicBook、森林 FX 与 StarterContent 粒子。无需覆盖蓝图或地图。详见 [接入说明](Docs/HarvestMagic_2026-10-13.md)。
- **验证 / 未验证**：UE 5.8 Mac Development 源码编译与链接成功；未开 UE 编辑器，未做 PIE 视觉、背包点击、投料/退款、真实 AI 与网络回归。历史库存不批量转换，原有合成评分占位与每日可采限制不在本批实现。
- **上传状态**：已上传 main，功能提交 `d5b8091`，远程 SHA 已核对。

## 2026-10-13-01：寻找 NPC 的 AI 涌现路引叙事框架

- **目的 / 基线**：在玩家交付一个背包物品并自动寻找 NPC 时，按被消耗物品的元素和数值生成“临时路引名称 + 短叙事”；基于 main `bd567a4` 与批次 2026-10-12-03。
- **新增**：`Source/demo3_UE58/ForestTravelNarrativeSubsystem.h`、`Source/demo3_UE58/ForestTravelNarrativeSubsystem.cpp`、`Docs/NPC_Finder_Emergent_Narrative_2026-10-13.md`。
- **修改**：`Source/demo3_UE58/ForestNPCInteraction.h`、`Source/demo3_UE58/ForestNPCInteraction.cpp`、`Docs/NPC_Finder_2026-10-12.md`。
- **共享接口**：新子系统只负责输入快照、提示词、结构化 JSON 解析与本地兜底；网络调用复用 `UDialogueAIServiceSubsystem::RequestChatCompletion`，沿用 NPC 对话和世界后果功能的地址、模型与 Key，不复制 HTTP 实现。
- **输入 / 输出**：输入包含实际消耗物品、火/风/水元素、1–9 点数、三轴数值、目标 NPC、数值来源和占位标记；AI 严格输出 `item_name` 与 `narrative`。结果只是寻路期间的叙事，不新增背包物品或场景 Actor。
- **当前占位接线**：现有背包没有保存采摘时的元素快照。每日蘑菇暂按当前日表读取元素、点数和三轴值，并标为 `current_day_mushroom_table_placeholder`；无法解析的物品使用“未定形路引”，不猜测元素。后续只需向同一输入结构传真实快照。
- **运行规则**：扣费成功后立即显示确定性本地叙事并异步请求 AI；成功且 JSON 合格时替换显示。无 Key、网络失败、无效 JSON 时保留本地兜底。请求用递增 ID 隔离，取消或结束后迟到回复不会污染下一次寻找；叙事生成不阻塞移动、退款或到达后的对话。
- **验证 / 未验证**：UE 5.8 `demo3_UE58Editor Mac Development` 构建成功，`ForestTravelNarrativeSubsystem.cpp`、`ForestNPCInteraction.cpp` 与生成代码编译并链接通过。按约定未启动 UE 编辑器，未做 PIE UI、真实 DeepSeek、迟到响应和元素快照回归。
- **上传状态**：已上传 `main`，功能提交 `540e263`，远程分支已核对。

## 2026-10-12-03：一键寻找 NPC（交付物品代价）

- **目的 / 基线**：让玩家在正式游戏中用背包里的一件物品作为代价，指定一位森林 NPC 并让玩家角色自动寻路过去；基于 main `986dc9c` 与批次 2026-10-12-02。
- **修改**：`Source/demo3_UE58/ForestNPCInteraction.h`、`Source/demo3_UE58/ForestNPCInteraction.cpp`。
- **新增**：`Docs/NPC_Finder_2026-10-12.md`。
- **行为**：按 V 打开“要寻找谁？”面板，三选一（小说家/森林女巫、铁匠 Natta、传信人 Fawnia）；选中后进入第二页确认，显示需要交付 1 个物品以及本次将被扣除的具体物品名，背包为空时确认按钮禁用。点“确认交付并寻找”后才校验并扣费，点“返回”回到第一步且不扣费。校验通过后扣除物品，玩家沿导航自动走向目标，进入交互距离后停止移动并直接打开该 NPC 的对话。
- **校验顺序**：目标不存在 → 提示“这位 NPC 当前不在场景中，没有扣除物品。”；既不能直接交互又没有完整可达路径 → 提示“目前没有可到达这位 NPC 的完整路线，没有扣除物品。”两种情况都不扣费。完整路径判定使用 `FindPathToActorSynchronously(World, 玩家位置, 目标, InteractionRange*0.7, 玩家 Pawn)`，要求路径有效、非 Partial、路径点大于 1。
- **取消 / 退回**：行进中按 WASD 或 Esc 主动取消，已交付物品不退回；目标离开场景或路线中断时自动退回物品。行进期间每 1 秒重发一次 `SimpleMoveToActor`。
- **代价实现**：反射读取玩家 Pawn 的 `Backpack`（键 `FStrProperty`、值 `FIntProperty`），优先取仍在背包内的 `LastPicked`，否则按名称排序取第一个可用物品；数量大于 1 减 1，等于 1 移除该项；随后重建 `BackpackList` 并调用 `RefreshHUD`。不新增第二套库存。
- **输入 / 界面**：V 键绑定在本 Actor 输入组件上并设置 `bConsumeInput`，与既有 E 交谈并列；面板打开时切换为 UI 输入模式并恢复鼠标光标，结束或取消后还原。Esc 在面板内为返回/取消，在交谈窗口内仍为结束交谈。
- **验证 / 未验证**：UE 5.8 `demo3_UE58Editor Mac Development` 构建成功，末次编译 `ForestNPCInteraction.cpp` 并链接 `libUnrealEditor-demo3_UE58.dylib`（`Result: Succeeded`）；玩家蓝图 `BP_FirstPersonCharacter` 中确认存在 `Backpack`、`BackpackList`、`LastPicked`、`RefreshHUD`。按仓库约定未启动 UE 编辑器，未做 PIE 的界面显示、扣费、寻路中断与退回回归。
- **未包含**：三选一面板固定列出三位现有森林 NPC，未做动态 NPC 列表；代价物品只显示名称、无图标；未接入评分惩罚或元素禁用机制。详见 [一键寻找 NPC 说明](NPC_Finder_2026-10-12.md)。
- **上传状态**：已上传 `main`，功能提交 `a8e08e3`，远程分支已核对。

## 2026-10-12-02：工作台药瓶失活状态

- **目的 / 基线**：在不消耗、不替换为空瓶且不摔碎的情况下，让工作台药瓶具有明显的不可用状态；基于 main `7d98c51` 与批次 2026-10-12-01。
- **修改**：`Source/demo3_UE58/PotionMimicWorldSubsystem.h`、`Source/demo3_UE58/PotionMimicWorldSubsystem.cpp`、`Docs/PotionMimicFlight_2026-10-12.md`。
- **行为**：单瓶失活后退出 MimicBook 飞舞、缓动落回桌面并向一侧倾斜；液体三层流速、发光强度和对比度降低，颜色变为暗灰浑浊。恢复时还原原始材质、姿态、碰撞和飞舞资格。失活不替换空瓶模型，为后续“耗尽”保留独立状态。
- **接口**：支持按绑定序号、Actor 或全部七瓶切换失活；提供单瓶查询和失活数量查询，便于后续评分惩罚按元素或角色控制。
- **验证 / 未验证**：UE 5.8 `demo3_UE58Editor Mac Development` 构建成功；按用户要求未启动 UE 编辑器，未做 PIE 材质显示、倾斜落点与恢复回归。
- **上传状态**：已上传 `main`，功能提交 `0d3687d`，远程分支已核对。

## 2026-10-12-01：工作台七瓶与 MimicBook 飞舞联动

- **目的 / 基线**：让女巫房间炼药工作台上的七个药瓶升空飞舞，并与 MimicBook 的位置、移动速度和折叠状态联动；基于 main `e31f5d5`。
- **新增**：`Source/demo3_UE58/PotionMimicWorldSubsystem.h`、`Source/demo3_UE58/PotionMimicWorldSubsystem.cpp`、`Docs/PotionMimicFlight_2026-10-12.md`。
- **行为**：运行时按锅附近药瓶数量识别工作台七瓶，不控制架子、柜子和橱窗中的其他 107 瓶；无书时自动展示升空环绕，有书时围绕书飞行，书移动会加速，`isFolded` 折叠时回位、展开时再起飞；飞行碰撞临时关闭，回位恢复。
- **MimicBook 迁移**：本地演示蓝图没有独立运动曲线资产，本批迁移 Flying Movement、速度切换和直接位移的原则，以缓动、正弦浮动和错相环绕实现静态瓶子飞舞。
- **依赖 / 应用**：合并两份源码后重新构建 `demo3_UE58Editor`；子系统自动创建，无需覆盖女巫房间关卡。需要书控制时在房间放置 MimicBook Actor，或通过蓝图调用 `SetMimicBookActor`。
- **验证**：UE 5.8 `demo3_UE58Editor Mac Development` 构建成功；关卡 Actor 元数据复核工作台簇为七瓶。按用户要求未启动 UE 编辑器，未做 PIE 视觉、碰撞和折叠状态回归。
- **未包含**：药瓶资源没有碎片模型、破碎特效、药液贴花或玻璃音效，本批未实现摔碎表现；后续接口建议见说明文档。
- **上传状态**：已上传 `main`，功能提交 `0e1778a`，远程分支已核对。

## 2026-10-09-02：干锅确认换天、蘑菇小牌表与元素叙事

- **目的 / 基线**：把每日蘑菇绑定为火/风/水三种小牌元素，点数由本地随机数生成 1–9；干锅合成结果确认后进入下一天；NPC 委托叙事改用元素药性。基于 main `231c5ed`。
- **修改**：`Plugins/DialogueMemoryTable/Source/DialogueMemoryTable/Public/DailyMushroomSubsystem.h`、`Private/DailyMushroomSubsystem.cpp`、`Public/AlchemyGameplayTypes.h`、`Private/AlchemyGameplaySubsystem.cpp`；`Plugins/DialogueMemoryTable/README.md`；`Docs/DailyMushroomSystem_2026-10-09.md`。
- **行为**：每日蘑菇表行包含 `CardId`、`Element`、`PipValue`、`Value`、`Weight`、`Tags`，便于后续增加条件字段；蘑菇类型固定元素，天数只改变可用状态、1–9 点数和权重；`ConfirmDryPotSynthesis` 是换天入口；NPC Prompt 和委托 `ElementRequest` 使用火、风、水描述，不把牌当作世界实体。
- **验证 / 未完成**：静态检查待执行；未在 UE 5.8 编译，未把 `BP_FirstPersonCharacter` 的干锅确认和采摘节点接到新接口，未做 PIE 换天回归。
- **上传状态**：待上传。

## 2026-10-09-01：上传状态更正

- **更正**：每日蘑菇接口已由提交 [231c5ed](https://github.com/daddongoba/MagicForest/commit/231c5ed3c9b2436185fd1dda703a28d377416401) 上传到 `main`，远程 SHA 已核对。

## 2026-10-09-01：全局天状态与每日蘑菇数值接口

- **目的 / 基线**：为 demo3_UE58 1.5 增加可保存的全局“天”，让蘑菇可采摘集合和绑定的火/风/水数值按天稳定变化；基于 main `f6463a4`。
- **新增**：`Plugins/DialogueMemoryTable/Source/DialogueMemoryTable/Public/DailyMushroomSubsystem.h`、`Private/DailyMushroomSubsystem.cpp`；`Docs/DailyMushroomSystem_2026-10-09.md`。
- **修改**：`Source/demo3_UE58/ForestNPCDialogueSubsystem.cpp`、`Plugins/DialogueMemoryTable/Tools/validate_package.py`。
- **行为**：`WorldDayState` 保存当前天；`AdvanceDay` / `SetCurrentDay` 管理日期；19 种现有基础蘑菇每天得到稳定的可采摘状态和 `FAlchemyVector` 数值；NPC 记忆写入使用同一世界日。
- **原型核对**：桌面原型包含 22 张大牌、4 张宫廷牌、9 个委托和三轴数值，但没有每日蘑菇表；原型小牌目前由委托必需牌加随机 1–8 小牌生成。
- **验证 / 未完成**：插件静态包检查和差异检查待执行；未在 UE 5.8 编译，未修改二进制采摘蓝图，未完成蓝图接线、换天刷新和 PIE 验证。完整接线步骤见 [每日蘑菇系统说明](DailyMushroomSystem_2026-10-09.md)。
- **上传状态**：已上传 main，功能提交 [231c5ed](https://github.com/daddongoba/MagicForest/commit/231c5ed3c9b2436185fd1dda703a28d377416401)，远程分支已核对；本次叙事与换天修改另记于 2026-10-09-02。

## 2026-10-07-05：绑定完整 AI 人设与自动熟悉度升级

- **目的 / 前置**：修正森林 NPC 只有通用性格、未调用升级接口的问题；基于 2026-10-07-03，demo3_UE58 1.5。
- **映射**：forest_witch → novelist，natta → blacksmith，fawnia → operator。复制插件完整人设，保留场景名字与独立记忆 ID；继承基础语气、日常状态、背景、三层提示和触发关键词。
- **行为**：有效玩家消息发送前调用 AdvancePersonaFromPlayerText，三级优先，可直接升级，只升不降并立即保存；联网回复读取升级后层级。无 Key 的在线发送及无效输入不会触发升级。AI 请求失败不会回退已保存的升级，与原型先升级后请求一致。
- **文件**：Source/demo3_UE58/ForestNPCDialogueSubsystem.cpp、Docs/NPC_Dialogue_Integration_2026-10-07.md、CHANGELOG.md。
- **验证 / 限制**：原型关键词与对应内置人设静态比对、插件包检查、差异检查通过；未执行 UE 编译、PIE、真实 API 或打包。沿用现有离线模式与存档槽。
- **应用 / 回退**：合并源码并编译完整工程；要体验真实人设回复需配置 Key 并关闭离线模式。回退源码不会自动降低已保存熟悉度，需使用应用前存档备份。
- **上传状态**：已上传 main；功能提交 [bf5e5c4](https://github.com/daddongoba/MagicForest/commit/bf5e5c4)，远程分支已核对。

## 2026-10-07-04：补记 NPC 对话更新上传结果

- **文件范围 / 目的**：只更新 CHANGELOG.md，记录功能批次 2026-10-07-03 的发布结果。
- **基线 / 前置批次**：2026-10-07-03，功能提交 [c4402a3](https://github.com/daddongoba/MagicForest/commit/c4402a37f28553c7ee199a7fb8bf8adfb52711ef)。
- **核对**：Git 推送成功；Git LFS 1/1、157 MB 上传完成；远程 main 与本地功能提交 SHA 均为 c4402a37f28553c7ee199a7fb8bf8adfb52711ef。
- **整理检查**：16 个增量文件，源码和关卡与完整工程一致，地图 LFS SHA256 核对，暂存差异检查和凭据/生成文件扫描通过。
- **应用 / 验证 / 回退**：无新增玩法文件，沿用 2026-10-07-03 的接入步骤和验证限制。本记录随文档维护提交发布，交付时再次核对远程；回退仅影响文档。

## 2026-10-07-03：森林 NPC 对话、独立记忆与故障恢复

- **目标 / 基线**：DialogueMemoryTable 安装到完整工程并接入三个森林 NPC；demo3_UE58 1.5，UE 5.8.3 Windows，main 67d6097；前置 NPC 批次 2026-10-07-01、插件批次 2026-10-06-05。
- **上传状态**：已上传 main，功能提交 [c4402a3](https://github.com/daddongoba/MagicForest/commit/c4402a37f28553c7ee199a7fb8bf8adfb52711ef)，关卡 Git LFS 上传成功，远程 SHA 已核对；见 2026-10-07-04。
- **新增**：Source/demo3_UE58/ForestNPCDialogueSubsystem.{h,cpp}；Docs/NPC_Dialogue_Integration_2026-10-07.md。
- **修改**：Source/demo3_UE58/ForestNPCInteraction.{h,cpp}、demo3_UE58.Build.cs；Plugins/DialogueMemoryTable/Source/DialogueMemoryTable/DialogueMemoryTable.Build.cs、Public/AlchemyGameplaySubsystem.h、Private/AlchemyGameplaySubsystem.cpp、Private/DialogueAIServiceSubsystem.cpp、Private/DialogueMemoryTableSubsystem.cpp；插件 Resources/DialogueMemoryPatch.schema.json、README.md；根 README.md、本记录。
- **关卡**：Content/Biomes/PNB_Enchanted_Forest/Map/Demo_Day.umap（Git LFS），现有管理器开启离线模拟。本机 uproject 已启用插件，仓库 uproject 原已启用。
- **功能**：E 交谈、Enter/发送、Esc/结束；暂停和恢复漫游，文字输入隔离；自定义人设、独立历史和长期记忆；JSON 校验、会话幂等、失败总结保存与重试；超时/取消恢复。环境变量配置 DeepSeek 兼容网关，无 Key 不发送。
- **应用 / 依赖**：合并源码/插件并构建 Editor 模块；地图依赖原完整资源，有个人关卡编辑时可只开启现有 NPC_EncounterInteraction 的 Offline Test Mode。在线设置 FOREST_DIALOGUE_API_KEY 后重启 UE 并取消离线模式。详见 [接入与实测表](Docs/NPC_Dialogue_Integration_2026-10-07.md)。
- **本次验证**：Editor 编译、三 NPC PIE 离线交谈/记忆/恢复漫游、本机模拟 HTTP 回复/总结、超时与非法响应、取消延迟回复、总结跨 PIE 重试、独立记忆重新加载、无 Key 拒绝、身份/编号/字段校验、熟悉度不回退、Slate 输入/Enter/Esc/按钮/E、插件静态包检查均通过；独立测试槽。
- **未验证 / 限制**：无真实服务 Key，未验证真实 DeepSeek；未打包、其他平台/多人/长期负载未测。森林人设为基础性格，无升级关键词或剧情秘密。原委托/卡牌接口保留，不表示已接入场景。
- **范围 / 回退**：不含测试存档、临时 HTTP 服务、缓存、构建产物或令牌。源码/插件/地图配套回退，本机已保留源码配置备份。

## 2026-10-07-02：补记 NPC 更新上传结果

- **目的 / 文件范围**：仅更新 `CHANGELOG.md`，记录功能批次 `2026-10-07-01` 的 GitHub 发布结果；本条随上传核对文档提交发布。
- **基线 / 前置批次**：`2026-10-07-01`，功能提交 [c7ce5e8](https://github.com/daddongoba/MagicForest/commit/c7ce5e8b275a20ce18e89f520277f17da6c8bc5a)。
- **核对结果**：Git 推送成功，Git LFS 报告 1/1 对象、157 MB 上传完成；远程 main SHA 与本地功能提交均为 `c7ce5e8b275a20ce18e89f520277f17da6c8bc5a`。
- **整理检查**：24 个增量文件的资产与源文件已比对；地图 LFS SHA256 与文件一致；凭据/生成文件扫描无发现，`git diff --cached --check` 通过。
- **应用 / 未验证项**：无新增游戏文件，功能与验证限制沿用前置批次。未从另一台机器重新下载 LFS 或运行打包验证。
- **回退**：只影响上传记录，不改变工程功能。

## 2026-10-07-01：NPC 道路漫游、相遇交互与步行修正

- **目的**：让三个 NPC 持续在道路上活动，玩家能够遇见或寻找并交谈；不可达路线自动换路。取代七目的地固定巡逻。
- **基线 / 前置更新**：demo3_UE58 1.5 完整工程，UE 5.8.3 Windows；增量基线 main `5e81998`，沿用 `2026-10-06-01` NPC 和 `2026-10-06-05` 既有更新。
- **上传状态**：已上传到 [daddongoba/MagicForest](https://github.com/daddongoba/MagicForest) 的 `main`；功能提交 [c7ce5e8](https://github.com/daddongoba/MagicForest/commit/c7ce5e8b275a20ce18e89f520277f17da6c8bc5a)，Git LFS 关卡上传成功，远程 SHA 已核对。
- **新增文件**：`Source/demo3_UE58/ForestRoadAIController.{h,cpp}`、`ForestCrouchNavigation.{h,cpp}`、`ForestNPCInteraction.{h,cpp}`、`ForestRoadTestCamera.{h,cpp}`、`ForestNPCSetupLibrary.{h,cpp}`；`Content/NPC/Navigation/SM_Log_NPCCollision.uasset`、`SM_Portal_NPCCollision.uasset`；`.gitattributes`；`Docs/NPC更新应用说明_2026-10-07.md`、`Docs/NPC_FreeRoaming_Interaction.md`、`Docs/NPC_Camera_Test.md`。
- **关卡新增入库**：`Content/Biomes/PNB_Enchanted_Forest/Map/Demo_Day.umap` 是完整工程已有但本次修改的关卡，约 157 MB，使用 Git LFS。包含用户编辑的道路、三 NPC、交互管理器、跟随相机和小物件碰撞修改。
- **修改文件**：`Content/NPC/AI/BP_NPC_WanderTest.uasset`、`BP_NPC_Natta_Wander.uasset`、`BP_NPC_Fawnia_Wander.uasset`；`Source/demo3_UE58/demo3_UE58.Build.cs`；`Config/DefaultEngine.ini`；`README.md`；本更新记录。
- **完整工程侧删除操作**：从 Demo_Day 关卡删除 NPC01–NPC07 七个箱子，已体现在关卡文件中；未删除独立资产。旧 Arrival 辅助点保留但不再控制移动。
- **最终行为**：速度 93.75 cm/s，模型朝向 Yaw=-90，Natta/Fawnia 模型高度 -88 cm；恢复原走路动画并停用实验性下蹲骨骼叠加；小物件对 NPC 放行，大障碍保留碰撞与导航；E 基础交谈，E/Esc 结束；F6/F7/F8/F9 提供 NPC 观察工具。
- **依赖与应用**：需要匹配完整工程原有模型、动画、场景与玩家资产；先获取 Git LFS 关卡，合并源码和导航/碰撞配置、编译模块，再应用资产和关卡。ECC_GameTraceChannel1 分配给 ForestNPC，已有通道需审查。完整步骤与文件表见 [本批应用说明](Docs/NPC更新应用说明_2026-10-07.md)。既有插件和 uproject 保留。
- **验证来源**：本开发会话原生模块构建成功，漫游阶段 PIE 确认三 NPC 连续完成路段、基础交互暂停和恢复移动输入。最后的碰撞、朝向、慢步行、删除箱子等在编辑器读取确认并保存；本次上传整理不额外运行游戏测试。
- **未验证项 / 限制**：最后修正未做完整 PIE 回归或打包；真实下蹲动画、足部 IK、剧情对话尚未接入，E 与原采摘交互的冲突未检查。
- **发布范围**：只包含上述增量，未包含缓存、Saved、构建产物、调试截图、完整资源包或本机令牌。上传配置保留 SecurityToken 为空。
- **回退**：完整工程恢复应用前备份，地图、NPC 蓝图、源码与配置须配套回退。

## 2026-10-06-05：坩埚投料合成（蓝图）与 DialogueMemoryTable 插件入库

- **功能更新范围**：2026-10-06 晚间，玩家走近坩埚后的投料—合成链路；以及一组非本次制作的第三方插件源码入库。
- **适用基线 / 引擎版本**：`demo3_UE58` 1.5 版完整工程，UE 5.8（历史记录为 5.8.3，macOS / Apple 芯片）。
- **前置更新**：本仓库批次 `2026-10-06-01`（首批 NPC 游走与采摘功能）；流程约定批次 `2026-10-06-04`。
- **上传状态**：已上传至 [daddongoba/MagicForest](https://github.com/daddongoba/MagicForest)，分支 `main`；批次提交 [020c429](https://github.com/daddongoba/MagicForest/commit/020c429bb70351af0689847494b7931bedabcd39)，远程分支已核对。
- **上传核对（2026-10-07）**：接受仓库协作邀请后，通过 SSH 完成推送；`git ls-remote origin refs/heads/main` 已返回 `020c429bb70351af0689847494b7931bedabcd39`。
  **离线交付与防误推（2026-10-07 00:15 补充）**：
  ① 已导出 `MagicForest_2026-10-06-05.bundle`（167 KB，含本批唯一提交，依赖父提交 `a31dbd5`），
  可在任何具备推送权限的环境中用 `git pull <bundle路径> main` 导入后补推，无需依赖本机凭据；
  ② 桌面的 `推送到GitHub.command` 会轮询 SSH 认证状态，**公钥登记后自动完成推送**，无需再执行任何命令；
  ③ 完整工程侧的 `origin` 已把 `pushurl` 设为禁用占位值 —— 两套 Git 历史无合并基，
  此举用于防止误用完整工程历史覆盖本仓库（保留 `fetch` 以便按 `AGENTS.md` 检查更新）。
  ④ **仓库归属已核实（2026-10-07 00:55）**：本仓库归 `dadongoba <285905484@qq.com>` 所有，
  经确认**属于协作方，本机操作者并非该账号持有人，且没有 GitHub 账号**。⇒ 本批的推送权在仓库所有者一侧。
  **因此本批以 Git bundle 形式交付**：`MagicForest_2026-10-06-05.bundle` 可由仓库所有者用
  `git pull <bundle 路径> main` + `git push origin main` 两条命令并入（已实测：从远端干净克隆后导入成功，
  `main` 前进到本批提交，仓库共 36 个文件）。
  ⑤ 复查结论：本机仍无任何 GitHub 凭据 —— 无 `gh` CLI、无 `credential.helper`、无 `.netrc`／
  `.git-credentials`、钥匙串无 `github.com` 条目、Xcode 源码控制账号列表为空（`IDESourceControlHostAccounts_10` 为 `()`）、
  未安装 GitHub Desktop。⇒ **剩余唯一缺环是账号侧授权，本机无法自行取得。**
- **目的与行为变化**：让玩家在保留原有 E 键采摘的基础上，能走近柜台坩埚打开面板，把手上的东西投入锅中、取回、并合成；合成判定单独抽成一层函数，便于将来换成外部 AI 或接插件内的炼药子系统。

### 文件范围

| 类型 | 工程相对路径 | 说明 |
|---|---|---|
| 新增 | `Content/Gameplay/Harvest/WBP_CauldronPanel.uasset` | 坩埚合成面板；含 `Txt_Bag`、`Txt_Hint` 与函数 `Refresh(InText)` |
| 修改 | `Content/Witch_House/Demo/FirstPerson/Blueprints/BP_FirstPersonCharacter.uasset` | 新增 `CauldronContents`、`CauldronList`、`CauldronOpen`、`CauldronPanel`、`CraftResult`、`LastPicked`、`LastPutIn`；新增函数 `ToggleCauldronPanel`、`UpdateCauldronUI`、`PutInCauldron(InKey)`、`TakeOutCauldron(InKey)`、`PutHeldInCauldron`、`TakeLastFromCauldron`、`ResolveCraft()`、`DoCraft()`；`TraceAndHarvest` 改为按命中对象分派（`Mushroom`→采摘，`Cauldron`→开关面板） |
| 新增 | `Plugins/DialogueMemoryTable/`（16 个文件） | ⚠️ **非本次制作**：他人于 2026-10-06 21:38–21:41 装入完整工程，`.uplugin` 标注 `CreatedBy: Codex`。含 `DialogueMemoryTableSubsystem`（AI 对话记忆，存 `USaveGame` 槽 `DialogueMemoryLedger`）与 `AlchemyGameplaySubsystem`（火/风/水元素、卡牌类型、`FAlchemyVector`、委托 Commissions、Personas）两个子系统。已做凭据扫描，未发现硬编码密钥或令牌 |
| 修改 | `demo3_UE58.uproject` | 启用 `DialogueMemoryTable` 插件（该改动同样不是本次制作） |
| 新增 | `Docs/DialogueMemoryTable-接入说明.md` | 插件自带的接入说明；其中也自述未启动编辑器验证 |
| 修改 | `Docs/更新汇总_2026-10-05至10-06.md` | 补记“阶段 16”一节，与完整工程记录对齐 |

### 依赖与应用

- **按键**：`E` 交互分派（采摘 / 开面板）· `F` 投料 · `R` 取回 · `G` 合成。完整工程需在角色蓝图中保留这些键盘事件绑定。
- **未包含的依赖**：锅体模型位于 `Witch_House` 关卡（未纳入本仓库）；合成 UI 依赖本面板资产，缺 `WBP_CauldronPanel` 时角色蓝图将出现空引用。
- **应用步骤**：关闭编辑器并备份后，按相对路径复制上表两个蓝图；再复制 `Plugins/DialogueMemoryTable/` 并合并 `demo3_UE58.uproject` 的插件启用项，**然后重新生成项目文件并编译 C++**。
- **插件与本次合成的关系**：本批次只是把现成文件入库与同步，未修改插件源码；是否采用 `AlchemyGameplaySubsystem` 替代本次的占位 `ResolveCraft()` 尚未决定。

### 验证与已知问题

- **本次验证**：对新增与修改文件做凭据扫描，未发现密钥、令牌或个人路径泄漏；`git diff --check` 无空白错误；文件清单与上述表格逐项核对。
- **历史记录**：本批蓝图的 `compile_blueprint` 返回无错误、`save_assets` 返回成功，来自完整工程晚间操作记录。
- **未验证项**：**未运行 UE，未做 PIE 实测**——面板排版、按键触发、投料扣减与产物入包均未经过运行验证。**插件未在 Unreal Build Tool 中编译过**，下次打开编辑器会触发编译，存在失败可能，建议优先确认。
- **已知问题**：① 合成产物目前是占位字符串 `"未知的混合物"`，`ResolveCraft()` 待接入正式规则或 AI；② MCP 侧无数组索引取值节点（`Utilities|Array|Get` 不存在），因此面板暂时无法按编号点选第 N 个物品，当前用 `LastPicked` / `LastPutIn` 记录最近操作的物品绕开；③ 完整工程记录中的女巫小屋 3.5 米高差与室内外导航未连通问题本批未处理。
- **回退方式**：删除 `Plugins/DialogueMemoryTable/`、还原 `demo3_UE58.uproject` 可去掉插件；蓝图可用 Git 恢复上一版本，但先前批次已包含的 `BP_FirstPersonCharacter.uasset` 也会一并回到旧版。

## 2026-10-06-04：明确 AI 开发与 Git 更新流程

- **目的**：让参与项目的 AI 先同步并检查 Git 进度，再制作功能，最后整理增量、维护记录并提交推送。
- **文件范围**：更新 `AGENTS.md`，补充必需工作顺序、安全同步方式、验证和交付要求；更新 `README.md`，增加可直接提供给 AI 的任务提示。
- **上传状态**：已上传至公开仓库 [daddongoba/MagicForest](https://github.com/daddongoba/MagicForest)，分支 `main`；实现提交 [94e6a9c](https://github.com/daddongoba/MagicForest/commit/94e6a9c51e18b42d36af0ed0495722471a3e1663)，远程分支已核对。
- **验证结果**：检查仓库当前约定与 README 后编写；`git diff --check` 通过；本次只改流程文档，不涉及 UE 资产或功能验证。
- **未验证项**：尚未在其他 AI 编码工具中实测其对 `AGENTS.md` 的读取情况。工具若不自动读取该文件，须在任务提示中附上 README 提供的指令并确保文件处于其上下文中。
- **回退方式**：可通过 Git 恢复这两份文档的上一版本。

## 2026-10-06-03：仓库改名并公开

- **目的**：将 GitHub 仓库名改为 `MagicForest` 并设为公开，让其他人可以查看迭代文件与记录。
- **文件范围**：本次修改 `README.md` 与 `CHANGELOG.md` 的仓库名称、链接和可见性说明；UE 功能资产没有变化。
- **上传状态**：GitHub 仓库设置已变更为公开，名称已核实为 `daddongoba/MagicForest`；文档提交待上传。
- **验证与应用**：仓库 API 返回 `private=false` 且名称匹配。使用新仓库地址克隆；应用功能文件仍需匹配的 UE 5.8 完整工程。
- **回退方式**：可将仓库可见性恢复为私有并恢复旧名；公开期间仓库内容可能已被访问或克隆。

## 2026-10-06-02：补记首批上传结果

- **适用基线 / 前置更新**：本仓库批次 `2026-10-06-01`；引擎与功能资产未变。
- **目的与文件范围**：仅修改 `CHANGELOG.md`，补记首批文件上传成功及验证结果。
- **依赖与应用**：无新增依赖，无需向完整 UE 工程覆盖任何文件。
- **验证结果**：首批 17 个文件通过 GitHub API 上传，每个文件的 Git blob SHA 与本地提交一致，远程 `main` 已核对为首批提交 `a834c21c10ae4cf98b7708d4eca73961361d09b2`。本条随后续文档提交同步至同一远程仓库。
- **未验证项 / 已知问题**：未运行 UE；功能限制沿用首批记录。
- **回退方式**：只影响文档，可通过 Git 恢复上一版 `CHANGELOG.md`。

## 2026-10-06-01：首批增量文件整理

- **功能更新范围**：2026-10-05 至 2026-10-06。
- **适用基线**：`demo3_UE58` 1.5 完整工程，UE 5.8（历史记录为 5.8.3）。这是首批增量文件，无本仓库前置批次。
- **上传状态**：已上传至 [daddongoba/MagicForest](https://github.com/daddongoba/MagicForest)，分支 `main`；首批提交 [a834c21](https://github.com/daddongoba/MagicForest/commit/a834c21c10ae4cf98b7708d4eca73961361d09b2)，上传结果已核对。仓库目前公开。
- **目的**：保存 NPC 游走蓝图、玩家采摘蘑菇与背包 HUD 的迭代文件，并明确增量仓库的用途及后续记录要求。

### 文件范围

以下功能文件分类依据 [原始清单](_清单.md)，路径已经与当前目录核对；未对蓝图内部逻辑重新验证。

| 类型 | 工程相对路径 | 说明 |
|---|---|---|
| 新增 | `Content/Gameplay/Harvest/WBP_MushroomHUD.uasset` | 采摘计数、背包与交互提示 HUD |
| 新增 | `Content/NPC/AI/BP_NPC_Natta_Wander.uasset` | Natta 游走蓝图 |
| 新增 | `Content/NPC/AI/BP_NPC_Fawnia_Wander.uasset` | Fawnia 游走蓝图 |
| 修改 | `Content/Witch_House/Demo/FirstPerson/Blueprints/BP_FirstPersonCharacter.uasset` | 玩家射线采摘、蘑菇隐藏及关闭碰撞、按种类入背包、HUD 刷新；E 键绑定采摘 |
| 修改 | `Content/NPC/AI/BP_NPC_WanderTest.uasset` | 原记录判定为编辑器重新保存、无语义变化 |
| 修改 | `Config/DefaultEngine.ini` | 启动关卡设为 `Demo_Day`，默认 GameMode 设为第一人称 GameMode |
| 参考，未修改 | `Source/demo3_UE58.Target.cs` | 工程构建目标 |
| 参考，未修改 | `Source/demo3_UE58Editor.Target.cs` | 编辑器构建目标 |
| 参考，未修改 | `Source/demo3_UE58/demo3_UE58.Build.cs` | 模块构建规则 |
| 参考，未修改 | `Source/demo3_UE58/demo3_UE58.h` | 模块头文件 |
| 参考，未修改 | `Source/demo3_UE58/demo3_UE58.cpp` | 模块实现 |

本批仓库维护文件：新增 `README.md`、`CHANGELOG.md`、`AGENTS.md` 与 `.gitignore`；附带既有 `_清单.md` 和 `Docs/更新汇总_2026-10-05至10-06.md`。上传准备时清空 `Config/DefaultEngine.ini` 中本机 Android File Server 的 `SecurityToken`，目标工程需在本地配置自己的令牌。本批没有声明需从完整工程删除或重命名的文件。

### 依赖与应用

- 完整工程需提供原素材、第一人称 GameMode、NPC 相关依赖及 `Demo_Day` 等关卡；本仓库未提供完整工程下载地址。
- `Content/Biomes/PNB_Enchanted_Forest/Map/Demo_Day.umap` 未纳入本批。NPC 摆放、路点及导航状态需由完整工程提供或自行配置。
- 关闭编辑器并备份后，按原路径复制本批蓝图；审查并合并 `DefaultEngine.ini` 中启动关卡与默认 GameMode 的设置。未修改的 `Source/` 文件无需作为功能更新覆盖。
- 详细应用流程见 [仓库说明](README.md#如何应用更新)，背景见 [历史更新汇总](Docs/更新汇总_2026-10-05至10-06.md)。

### 验证与已知问题

- **本次检查**：核对当前文件清单与文档路径；本次仅整理仓库文档，没有改动功能资产，也没有运行 UE、编译或打包。
- **历史记录**：macOS 编译通过，三个 NPC 游走和采摘至 HUD 刷新的逻辑链路已做 PIE 验证。这些结果来自完整工程历史汇总。
- **仍需验证**：人工在 PIE 中按 E 采摘；应用到目标完整工程后的资产引用、蓝图编译、运行及打包。
- **已知问题**：历史记录指出女巫小屋室内外导航未连通、门口约 3.5 米高差；本批未修复。NPC 游走路点距离小屋较远，复制蓝图不会自动改变路点或摆放。

---

## 后续更新模板（复制到文件顶部并填写）

```markdown
## YYYY-MM-DD-NN：更新标题

- 功能更新范围：
- 适用基线 / 引擎版本：
- 前置更新：无 / 批次编号
- 上传状态：待上传 / 已上传（已确认的提交或标签，如有）
- 目的与行为变化：

### 文件范围

| 类型（新增/修改/删除/重命名/参考） | 工程相对路径 | 说明 |
|---|---|---|
| | | |

### 依赖与应用

- 未包含的依赖：
- 复制、配置合并、删除、迁移或关卡操作步骤：
- 详细说明链接（如有）：

### 验证与已知问题

- 本次验证方式与结果：
- 历史验证依据（如引用）：
- 未验证项：
- 已知问题与回退方式：
```
