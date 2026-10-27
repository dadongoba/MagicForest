# 封瓶后的偏离与世界推演

基线 main `70e93b3`，前置批次 2026-10-27-01/02。完整工程 demo3_UE58 1.5，UE 5.8。对应批次 2026-10-27-03。

## 玩家流程

1. 夜间合成台保留元素小物、今日大牌、通用修饰、顺序槽和委托；不显示偏离、灵验、剂量比、评级和成品试算。
2. 二次确认封瓶后，实际扣料并保存带唯一编号的本次成品记录，切入同一套纸页主题的结算界面，调配槽不可再编辑。
3. 左侧显示目标/成品火风水、总剂量、灵验、偏离、方向吻合、剂量比、评级和判定原因；右侧异步显示使用现场与森林余波。无三维轴。容器沿用1180×740、按可用宽高等比缩小，长文本独立滚动。
4. 生成中继续按钮禁用。缺Key、接口失败、响应结构不合适或45秒超时后显示本地物理结算及中性森林收尾；不编造成功后果。
5. 点击“承接新局 · 开始下一天”才执行原换天和小屋出生流程。Esc不会跳过结算。接口完成回调本身不换天。

## AI接入

复用 UDialogueAIServiceSubsystem::RequestChatCompletion，与既有NPC/采摘/寻路共用配置与接口，不新增地址、密钥或模型配置。沿用 UAlchemyGameplaySubsystem::BuildWorldConsequencePrompt 的委托映射和评级分支，并补充秘密背景、实际元素符号含义、死头/空瓶/违约/普通交付分支。

请求在封瓶时冻结：委托身份、愿望、情境、秘密背景、目标/实际向量、各项本地评分、评级原因和既有记忆。blacksmith/operator/novelist分别读取natta/fawnia/forest_witch记忆，避免职业ID与场景NPC ID错配。背景缺失时拒绝提交，不发送残缺请求。

提示要求第三人称、冷峻克制的近代工矿/流亡幻想质感、200–300字、两段纯正文。字数和风格是生成约束，不能保证每次严格满足；消费侧接受80–1800字符并拒绝代码围栏，其余不合格响应走本地结算。AI不改数值、奖励或世界规则。

评分仍由原有本地评价器计算。本次只将“违约”的提前返回移到指标计算之后，保持违约优先级、奖励和理由；避免有实际成品向量但灵验/偏离全部为零。

## 保存与回调

ForestDailyArcana存档新增PendingResultId；每条Brews记录新增ReceiptId、Customer、Narrative、bNarrativeFallback和冻结提示词。旧字段保留，旧存档新增字段使用默认值。历史无编号条目不触发补扣或补推演。

结果先保存才发AI请求。未读结果在下次进入世界时恢复并打开；已经保存的文本不重复生成，未完成的请求可按冻结提示词重新请求。回调同时核对请求对象和成品编号；超时或世界销毁后清除Owner，迟到回复被丢弃。这里取消的是消费回调，不强制中断底层HTTP。

确认进入次日前先保存原PendingAdvanceFromDay标记；日系统已换天时不再次推进。错误时保留结算可重试，不再扣料。旧版已存在的待换天标记仍按原恢复逻辑处理。

旧元素背包仍为既有蓝图Backpack数量表，本批没有给它增加完整跨进程数量存档或跨多个存档文件的原子事务。成品数值、推演和大牌是持久化记录；未将叙事自动写成真实地图变化、报酬发放或NPC交付记忆。

## 文件与应用

合并Source/demo3_UE58/ForestAlchemy.h/.cpp、ForestAlchemyUI.cpp及Plugins/DialogueMemoryTable/Source/DialogueMemoryTable/Private/AlchemyGameplaySubsystem.cpp并重新编译。沿用共享主题、纸页PNG、原插件和既有关卡。无配置、地图、蓝图、模型变更，无删除或迁移要求。不要覆盖完整工程中无关修改。

Docs/Brew_Result_Preview.html及.jpg为新结算离线预览；Daily_Arcana_Brewing_Preview.html已移除操作中的评分。此前Daily_Arcana_Brewing_Preview.jpg和批次01描述属于历史状态，以本批为准。截图不是UE实机。

## 验证与未验证

- 本次完成UE5.8 Mac Development Editor源码/UHT/链接编译；未启动编辑器或PIE。
- 逐文件核对增量与完整工程一致，检查变更范围和空白错误。
- 离线预览检查生成完成、生成中禁用继续、失败后可继续三种状态，保存整页截图。
- 源码路径审查：完整输入校验先于派发；请求使用冻结提示；失败/超时都写本地结算；对象+成品编号拒绝迟到回复；销毁解绑Owner；推进只在玩家继续时调用。
- 评级分支静态审查：magnum_opus精准成功；side_effect具体副作用；under_dosed愿望未解决；over_dosed/reversed_effect失控或相反后果；caput_mortuum/empty_bottle/contract_breach不可用或拒收；deliverable普通满足。未调用真实AI逐项生成验收。
- 仍需游戏内验收：点击/焦点/DPI、真实AI返回、45秒超时、世界销毁中请求回调、退出恢复、保存故障重试、重复点击不扣料/不重复换天。离线预览和源码审查不等同于这些运行测试。
