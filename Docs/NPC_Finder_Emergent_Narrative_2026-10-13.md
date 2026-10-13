# 寻找 NPC：AI 涌现路引叙事框架

本批为“交付一个物品寻找 NPC”增加叙事生成层。它只生成一件临时路引的名称和一段短叙事，用于玩家自动寻路期间的界面文字；不会向背包添加新物品，也不会改变寻路、扣费、退款或 NPC 对话结果。

## 玩家可见流程

玩家确认交付物品并开始寻找后，寻路提示增加两行：

1. 已交付的真实背包物品名称；
2. `涌现路引名称：短叙事`。

请求开始时立即显示本地兜底文本，不等待网络。共享 AI 接口成功返回后，在玩家仍处于寻路状态时替换为 AI 结果。到达 NPC、玩家主动取消、目标失效或路线中断，都按原寻找系统的规则结束；AI 请求不会阻塞这些状态。

## 输入契约

`FForestTravelNarrativeInput` 是一次请求的冻结快照：

| 字段 | 用途 |
|---|---|
| `RequestId` | 拒绝旧请求的迟到结果 |
| `ConsumedItemId` / `ConsumedItemName` | 实际扣除的背包物品 |
| `bHasElementValue` | 是否已有可用的元素数值 |
| `Element` | 火 / 风 / 水 |
| `PipValue` | 当前 1–9 点数入口 |
| `Value` | 火、风、水三轴数值，给后续复杂规则留口 |
| `TargetNpcId` / `TargetNpcName` | 本次寻找目标 |
| `bUsesPlaceholderValue` / `ValueSource` | 标记当前数值是否仍为临时接线 |

当前背包只保存物品名和数量，没有保存采摘当天的数值快照。因此现阶段若消耗的是每日蘑菇表中的物品，就临时读取**当前日**的 `Element`、`PipValue` 和 `FAlchemyVector`，并写入 `ValueSource=current_day_mushroom_table_placeholder`。该来源明确标为占位；后续背包条目保存真实元素快照后，直接填充同一输入结构并把 `bUsesPlaceholderValue` 设为 `false`。

无法在每日蘑菇表中解析的物品不会被猜成火元素：`bHasElementValue=false`，本地结果为“未定形路引”，叙事明确说明数值仍待补全。

## AI 接口与输出

`UForestTravelNarrativeSubsystem` 负责提示词、JSON 解析、兜底与最后结果；网络仍统一调用 `UDialogueAIServiceSubsystem::RequestChatCompletion`，沿用 NPC 对话和世界后果功能的 Base URL、模型与 `FOREST_DIALOGUE_API_KEY`。

要求 AI 只返回：

```json
{"item_name":"2–10个汉字的名称","narrative":"35–80个汉字的短叙事"}
```

解析后的 `FForestTravelNarrativeResult` 包含 `EmergentItemName`、`ShortNarrative`、`bGeneratedByAI`、`bUsedPlaceholderValue` 和 `Error`。名称与叙事设置长度上限，Markdown 围栏会被清理；缺字段、非法 JSON、网络错误和无 Key 都转入确定性本地兜底。

## 本地兜底

- 火：生成“微/鸣/盛烬路签”，用余烬落点指路。
- 风：生成“微/鸣/盛风路羽”，用风羽沿可走道路飘行。
- 水：生成“微/鸣/盛潮引露”，用水痕绕开阻碍延伸。
- 未解析元素：生成“未定形路引”。

前缀暂按点数 `1–3 / 4–6 / 7–9` 分为微、鸣、盛，只是可运行占位，不代表最终数值平衡。

## 接入位置

`AForestNPCInteraction::ConfirmNpcFinding` 在目标和路径校验通过、背包扣费成功后调用 `RequestTravelNarrative`。本地兜底立即写入寻路 HUD；异步结果通过带 `RequestId` 的回调更新。取消或结束寻找时清除活动请求编号，迟到结果不会覆盖后续一次寻找。

## 验证与限制

- UE 5.8 `demo3_UE58Editor Mac Development` 构建成功，已编译新子系统、交互接线并链接编辑器模块。
- 按既有约定未启动 UE 编辑器，未做 PIE 界面换行、异步返回时序或真实 DeepSeek 请求验证。
- 当前没有真实背包元素快照；当前日蘑菇表接线只用于占位。
- 生成的路引只有名称和叙事，不创建 Actor、图标、特效、可拾取物或背包条目。
