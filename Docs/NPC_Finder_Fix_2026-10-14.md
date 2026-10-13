# V 入口与手动交付物品修复

## 问题证据

历史日志显示：2026-10-13 的 PIE 启动在 UForestHarvestMagic::Setup 中因直接创建抽象 UUserWidget 失败，随后空对象作为 WidgetTree 的外层造成致命错误。此前源码编译没有暴露这个运行错误。

静态检查发现 V 的 AForestNPCInteraction 仅由 ForestRoadAIController::OnPossess 生成；原蓝图 AI 尚未接入该控制器时，既没有 V 管理器，也不会被寻找目标过滤器认可。

原收费流程自动选择 LastPicked 或名称排序第一项，不符合手动选择要求。

## 修改

- 主游戏世界的 WorldSubsystem 自动确保唯一交互管理器存在；已有管理器时不重复创建，流式子世界与专用服务器不生成。
- NPC 识别按现有 Natta、Fawnia、NPC_WanderTest/KindWitch 名称映射，其他角色返回空 ID，避免把玩家或其他角色当小说家。
- 找人不再要求特定原生控制器。到达后交谈支持普通 AAIController：交谈期间停止其移动；原生控制器仍沿用已有 BeginInteraction/EndInteraction。
- E 的交谈绑定不吞掉原采摘输入；V 保持独立绑定。
- 选 NPC 后出现物品下拉列表，只列数量大于零的 Backpack 项；初始无选择，确认按钮禁用。玩家可选择或更换，确认时重新核对数量，只扣明确选定的一件。物品已被移出或改名时要求重新选择，不自动换成另一个物品。
- 实际扣除键继续传入现有 AI 带路故事，快照描述与元素保持一致，原退款规则继续使用同一个键。
- 采摘提示改为具体 UForestHarvestNoticeWidget，由 RebuildWidget 构建文本，创建失败时不访问空对象。

## 应用与验证

新增类均在现有 ForestNPCInteraction.h/.cpp 与 ForestHarvestMagic.h/.cpp 内。合并这四份源码并重新编译，无地图、蓝图、插件配置覆盖。

UE 5.8 demo3_UE58Editor Mac Development 编译、生成代码与链接通过。检查移除了自动选择分支，并保留确认前目标/路径/库存检查。

本次曾启动编辑器准备验证，但用户要求不要打开 UE 后立即关闭本次进程，未运行修复版本的 PIE。V 是否显示、下拉选择焦点、扣费/取消/退款、原 NPC 停步恢复以及实际路径均尚未运行验收。编译成功不能替代这些测试。未做新一轮真实 AI 请求。

## 补充：B 与三位 NPC 的实际运行绑定

2026-10-14-02 在上一修复基础上继续补齐：

- 原 B 入口用 bShowMouseCursor 一概拒绝打开，导致普通游戏中鼠标可见也会无提示失效。现仅在 NPC 对话或找人面板实际打开时避让，不再用鼠标是否可见作为阻止条件。
- 交互 WorldSubsystem 每秒检查主游戏世界，保证管理器存在，并将 NPC_WanderTest、NPC_Natta_Wander、NPC_Fawnia_Wander 三类已有场景角色绑定 ForestRoadAIController。已有原生控制器时跳过，玩家控制角色和流式子世界不处理；旧控制器只解除占有、不删除，未修改关卡资产。
- 由实际 NPC 的新控制器接通既有教程主动接近、对话暂停/恢复以及人设/记忆入口。旧漫游计时器由现有原生控制器停用，避免两套移动同时争抢。
- 这会在运行时接管三位 NPC 的原蓝图 AI；实际场景若尚未有道路辅助 Actor 或可用导航，漫游/教程接近仍依赖已有导航数据，源码绑定本身不会生成导航。
- 补充改动编译与链接通过；UE 已关闭，没有运行验证。不能声称 B/UI/新控制器在用户当前场景已通过验收。
