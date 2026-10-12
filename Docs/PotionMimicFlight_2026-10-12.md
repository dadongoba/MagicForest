# 工作台药瓶与 MimicBook 飞舞联动

## 范围

本批只控制女巫房间炼药工作台附近的七个药瓶。架子、高柜和两个橱窗中的其他 107 个药瓶保持原状。

运行时子系统不依赖关卡里的固定世界坐标。它查找药瓶模型与柜台锅，在每只锅附近统计药瓶，选择数量最接近七个的药瓶簇作为工作台组；因此关卡整体平移或作为 Level Instance 放入其他地图后仍可识别。

## 最终行为

- 工作台七瓶在进入游戏约两秒后自动升空，围绕锅进行错相环绕、上下浮动、半径呼吸和轻微倾斜。
- 检测到 `BP_Mimic-book-Demo` 一类 MimicBook Actor 后，飞行中心改为 MimicBook；书移动越快，药瓶环绕速度越快。
- 读取 MimicBook 的 `isFolded` / `IsFolded` 布尔属性：折叠时七瓶缓动回到各自原始位置，展开时再次升空。
- 飞行期间关闭瓶子碰撞，回位后恢复原碰撞设置，避免围绕书移动时推挤玩家或环境。
- 工作台没有放置 MimicBook 时使用自动演示模式；可在蓝图中关闭。

## 蓝图接口

从当前 World 获取 `PotionMimicWorldSubsystem` 后可调用：

- `StartWorkbenchPotionFlight(MimicBookActor)`：立即开始飞舞，可传入领飞书。
- `StopWorkbenchPotionFlight(bSnapHome)`：缓动或立即回位。
- `SetMimicBookActor(MimicBookActor)`：运行时更换领飞书。
- `SetAutomaticWorkbenchFlight(bEnabled)`：控制无书时的自动演示。
- `GetBoundWorkbenchPotionCount()`：正常应返回 7。
- `IsWorkbenchPotionFlightActive()`：读取当前目标状态。

## MimicBook 迁移边界

本地 MimicBook 演示蓝图没有可复用的独立 `CurveFloat` 资产；原逻辑主要使用 Flying Movement、速度切换和直接更新 Actor 位置。本批将这些运动原则迁移为静态瓶子可使用的 C++ 缓动与正弦轨迹，没有复制书的骨骼攻击动画或输入绑定。

## 摔碎表现的后续接入

当前药瓶资源只有完整瓶模型，没有碎片模型，也没有破碎 Niagara、药液贴花或玻璃音效。本批不把“隐藏完整瓶子”冒充已完成的摔碎效果。

后续推荐在瓶子落地或收到明确破碎指令时执行：隐藏完整瓶体，生成玻璃碎片实例，按药液类型选择颜色，生成短时飞溅 Niagara 与地面贴花，播放玻璃破裂声，并把破碎状态交给存档或任务系统决定是否永久保留。

## 验证

- UE 5.8 `demo3_UE58Editor Mac Development` 源码构建成功。
- 从女巫房间 114 个药瓶的关卡 Actor 元数据复核，工作台锅附近识别结果为七个；其余瓶子分属多层药架、高柜与两个橱窗展示架。
- 按用户要求没有启动 UE 编辑器，因此尚未进行 PIE 视觉节奏、碰撞与折叠/展开联动实测。
