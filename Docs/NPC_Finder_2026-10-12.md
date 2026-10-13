# 一键寻找 NPC（交付物品代价）

本批为正式游戏功能：玩家用背包中的一件物品作为代价，指定一位森林 NPC 并让玩家角色自动寻路过去。

## 范围

只增强 `AForestNPCInteraction`：新增 V 键入口、两步确认面板、代价扣除与自动寻路。不新增库存系统，不改动既有 E 交谈、第一日教程与 NPC 漫游逻辑。

F6/F7 测试相机与本次功能无关：它们只是编辑器内把镜头切到 NPC 身上，玩家角色不会移动，也不进入正式游戏。

## 玩家流程

1. 按 V 打开“要寻找谁？”面板，列出三位现有森林 NPC：寻找小说家（森林女巫）、寻找铁匠 Natta、寻找传信人 Fawnia，另有“取消”。
2. 选中一位后进入第二页确认，显示“需要交付背包中的 1 个物品”以及**本次将被扣除的具体物品名称**。背包为空时确认按钮禁用并提示“背包为空，暂时无法寻找。”
3. 只有在第二页点“确认交付并寻找”才执行校验与扣费；点“返回”回到第一步，不扣费。
4. 校验通过后扣除物品，玩家沿导航自动走向目标；进入交互距离后停止移动并直接打开该 NPC 的对话。

## 校验与扣费顺序

扣费发生在所有校验之后，避免玩家付费后才发现目标不可达：

- 目标不存在（未生成、已销毁或缺少 NPC 控制器）→ 提示“这位 NPC 当前不在场景中，没有扣除物品。”
- 既不能直接交互、又没有完整可达路径 → 提示“目前没有可到达这位 NPC 的完整路线，没有扣除物品。”

判定细节：

- 可直接交互沿用原有 `CanReach`：玩家视角在玩家 Pawn 上、距离小于 `InteractionRange`（默认 250）、且到 NPC 的相机通道射线无遮挡。
- 完整路径用 `UNavigationSystemV1::FindPathToActorSynchronously(World, 玩家位置, 目标, InteractionRange * 0.7f, 玩家 Pawn)`，要求返回路径有效、非 Partial、路径点数量大于 1。

## 取消与退回规则

| 情况 | 触发条件 | 已交付物品 |
|---|---|---|
| 玩家主动取消 | 行进中按下 W / A / S / D 或 Esc | 不退回 |
| 目标离开场景 | 行进中目标失效 | 自动退回 |
| 路线中断 | 行进中每秒复核发现无完整路径 | 自动退回 |

行进期间每 1 秒重发一次 `UAIBlueprintHelperLibrary::SimpleMoveToActor`，并在 HUD 显示“正在寻找 X”与“已交付：Y　WASD 或 Esc 取消”。

## 物品扣除实现

通过反射读取玩家 Pawn 上的 `Backpack`，不新增第二套库存：

- 读取 `Backpack`（`FMapProperty`，键为物品名 `FStrProperty`，值为数量 `FIntProperty`）。
- 选取规则：优先 `LastPicked`（若该物品仍在背包内且有数量），否则在可用物品中按名称排序取第一个。
- 数量大于 1 时减 1；等于 1 时从映射中移除该项。
- 变更后按 `BackpackList` 重建展示文本，并调用玩家的 `RefreshHUD`。
- 退回时数量加 1；键已被移除时重新插入。

上述字段名已与玩家蓝图 `BP_FirstPersonCharacter` 核对一致。

## 输入与界面

- V 键绑定在本 Actor 的输入组件上并设置 `bConsumeInput`，与既有 E 交谈并列；寻找面板打开或确认时接管输入（UI 输入模式并恢复鼠标光标），结束或取消后还原。
- Esc 在寻找面板内等于“返回 / 取消”，在交谈窗口内仍为结束交谈。
- 面板与提示文字接在既有 `UForestNPCInteractionWidget` 上，新增 `ShowFinderSelection`、`ShowFinderConfirmation`、`IsFinderVisible` 与五个按钮回调。

## 蓝图接口

- `OpenNpcFinder()`：按 V 的等价入口，可直接从蓝图打开寻找面板。
- `CancelNpcFinder()`：取消当前寻找流程。
- 面板按钮回调走内部状态机 `EFinderStage`（`Closed` / `Selecting` / `Confirming` / `Travelling`）。

## 验证

- UE 5.8 `demo3_UE58Editor Mac Development` 源码构建成功；末次构建编译 `ForestNPCInteraction.cpp` 并链接 `libUnrealEditor-demo3_UE58.dylib`，结果为 `Result: Succeeded`。
- 玩家蓝图 `BP_FirstPersonCharacter` 中确认存在 `Backpack`、`BackpackList`、`LastPicked`、`RefreshHUD`。
- 按仓库约定未启动 UE 编辑器，因此未做 PIE 的界面显示、扣费、寻路中断与退回回归；未实测行走节奏、面板可读性与到达判定手感。

## 未包含

- 三选一面板固定列出三位现有森林 NPC，未做动态 NPC 列表或运行时增删。
- 代价物品只显示名称，未做图标展示。
- 未接入评分惩罚或元素禁用机制；扣除物品与惩罚系统的关系留给后续批次。

## 2026-10-13 叙事扩展

交付物品后的寻路提示现已接入“涌现路引名称 + 短叙事”框架，复用项目共享 AI 网关，并保留无 Key / 请求失败时的本地兜底。元素与点数目前从当前日蘑菇表临时读取，后续可替换为背包条目的真实数值快照。结构、JSON 契约和限制见 [AI 涌现路引叙事框架](NPC_Finder_Emergent_Narrative_2026-10-13.md)。
