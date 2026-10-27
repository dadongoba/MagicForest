# 今日大牌、药瓶抽牌和夜间合成台

批次2026-10-27-01，基线 main 9b90c83。依赖前序纸页主题、采摘小物、AlchemyGameplaySubsystem牌库/委托/运算和DailyMushroomSubsystem世界日。不启动UE、不改二进制关卡或蓝图。

## 玩家流程

1. 每次游戏世界启动，以及世界日变化后，查找现有小屋PlayerStart并将玩家放回室内。优先使用带 `Forest.CabinSpawn` 标签的PlayerStart；其次识别Witch_House子关卡出生点，再回退为与室内锅同一关卡、距离1200单位内的出生点。不使用旧文档里的固定坐标；关卡稍后加载会继续重试，找不到合适出生点时不会猜坐标或强行传送。
2. 看向药瓶按E，随机获得一张22张牌库中的大牌。当前匹配所有现有 `SM_Potions_MDL_Potion*` / `SM_EmptyPotions_MDL_EmptyPotion*` 模型，不限原飞舞的7瓶，也不按桌面/货架限制；隐藏的瓶子和隔墙目标不可取。每个实体瓶子每天一次，允许重复牌型，每张以独立实例记录。
3. 瓶子收取后隐藏并关闭Actor碰撞，提示获得的大牌。没有增加持瓶手部动画。用原始Actor路径剥离PIE前缀作为来源键，保存当天已领取记录，普通同日读档不重新抽取。流式关卡生成名称变化仍需实机验证。
4. B背包增加“元素小物 / 今日大牌”两栏。大牌以牌框、编号、名字、效果、额度呈现，点击看描述；未添加牌面插画。牌不混进原Backpack字符串数量表，不能被原寻路费用误扣。
5. 看向室内锅按E进入“夜间合成台”。本批的夜间指调配界面阶段，不改天空光照、不新增真实时间计时器。关闭旧坩埚面板，旧锅中已有材料先返还背包，以免新旧两个面板争用。
6. 选择当天委托，在元素小物、今日大牌与四种通用修饰中点击放入调配槽。槽位上限沿用委托，额度沿用原型每晚5点；元素小物按实际库存数量，大牌同一实例只能放一次，通用修饰仍按原型可重复、受额度约束。
7. 点击槽位后前移、后移、移除或清空；这些操作不扣实际库存。左到右试算调用现有EvaluateAlchemySequence，显示委托、火/风/水数值、总剂量、灵验、偏离、剂量比和结果。没有3D坐标轴。原型已有的非可视计算规则沿用，并未重做平衡。
8. 封瓶交付需再次点击确认。再次验证库存、大牌和额度；扣槽内小物和大牌，保存成品数值记录，再调用ConfirmDryPotSynthesis换天。新一天清空昨日可用大牌/领瓶记录、恢复药瓶并回到室内。未使用的大牌也过期。关闭面板或取消配方不扣料。

## 数据、配置和恢复

新存档槽 `ForestDailyArcana` 保存日、随机种子、逐瓶领取记录、大牌实例和 `FForestBrewRecord` 成品数值账本。抽牌用本地FRandomStream，每瓶来源键+当日持久种子决定结果；不调用LLM，抽取顺序不改变该瓶结果。只有存档成功才隐藏瓶子。

`Config/DefaultForestAlchemy.ini` / `UForestAlchemyRules`：

- MaxDailyDraws=0：无每日总量限制（每瓶仍一天一次）。
- AllowedMajorIds空数组：允许22张全部；可填编号白名单。
- RequiredBottleTags空数组：所有匹配瓶子；可配置必须具备的Actor标签。
- InteractionDistance=280：看向并拿取的距离，运行时限制50–500。

未来的评分、熟悉度、概率/权重限制尚未定义，没有私自添加。当前可用的限制只有上述真实配置项。

合成记录先持久化，再换天，保存PendingAdvanceFromDay作为中断恢复标记；恢复时完成未完成的换天而不再次扣料。保存或换天失败会尝试撤回本次库存、大牌和记录。DailyMushroomSubsystem::ConfirmDryPotSynthesis增加保存失败时的日状态回滚。

**边界：**元素库存数量仍使用旧角色Backpack，本批没有把整个旧背包改造成跨退出存档，也不宣称跨文件写入具有完整事务保证。成品先保存配方和结算数值，可通过GetBrewRecords读取；尚未生成可持有的成品药瓶、发放玛瑙报酬、写NPC交付记忆或生成次日后果故事。没有把NPC搬进室内，也未新增合成台内聊天框。每日蘑菇场景可采刷新、室内外导航和台阶问题不在本批修改。

## 实现文件

新增：Source/demo3_UE58/ForestAlchemy.h、ForestAlchemy.cpp、ForestAlchemyUI.cpp；Config/DefaultForestAlchemy.ini。

修改：Source/demo3_UE58/ForestHarvestMagic.h/.cpp（背包页签、自适应容器与面板互斥）；PotionMimicWorldSubsystem.h/.cpp（被收取瓶子跳过飞行更新，读取原始桌面位置）；Plugins/DialogueMemoryTable/Source/DialogueMemoryTable/Private/DailyMushroomSubsystem.cpp（换天保存失败回滚）。

文档：本说明、CHANGELOG.md、Daily_Arcana_Brewing_Preview.html与同名jpg。预览只展示排版示例，不读取真实库存、不做运行验收；UI由Slate实现，HTML截图不是引擎截图。

应用：按相对路径合并以上9份源码/配置后编译；保留原有地图、插件和纸页PNG依赖。完整工程旧源码备份在Saved/CodexBackups/major-brewing-20261027。没有上传缓存、编译产物、地图或API凭据。

## 验证

- 本次UE 5.8 Mac Development Editor构建：UHT、源码编译与模块链接成功，最终日志Result: Succeeded。
- 增量源码/配置与本次编译的完整工程逐文件校验一致；差异空白和凭据模式扫描。
- 22张大牌编号唯一，额外配置默认为全牌库/无总量限制；静态核对抽牌保存先于隐藏、配方调整不写库存、最终确认复查库存/额度、换天回滚以及MimicBook跳过已领取瓶子。
- 未启动UE或PIE，未实际操作出生、看向按E、浮空瓶子取用、读档、背包页签、投料与封瓶/换天。未做打包验收。HTML仅检查视觉栏目，不能替代上述功能验收。

待运行检查：同瓶二次领取/同日读档不增牌；不同瓶可出相同牌型；次日恢复并清空旧牌；流式子关卡下出生点识别；采蘑菇E/NPC交谈E不被吞；元素库存不足/大牌重复/超槽/超预算拒绝；退出不扣；封瓶仅一次扣料；保存失败恢复；旧锅返还不丢材料。现有评分系统的必用料与副锅定义仍需后续数值审订，本批保留现有实现，不宣称已完成全部炼药系统。
