# 可独立控制的隐形区块 · 2026-10-31

基线 `e75fa9a`。本批将上一批分区草案中的归属、读取、控制与按天保存做成原生运行时功能。默认不启用任何控制，不调用AI，不匹配合成评分阈值。

## 默认划分

| 编号 | 用途 | 保护 |
|---|---|---|
| home_safe | 小屋周围 | 资源及物件隐藏/碰撞控制受保护 |
| fire_entry_safe / wind_entry_safe / water_entry_safe | 三个大区面向小屋的入口扇区 | 同上 |
| fire_gather_a / fire_gather_b | 火区的两片可控区域 | 可单独控制 |
| wind_gather_a / wind_gather_b | 风区的两片可控区域 | 可单独控制 |
| water_gather_a / water_gather_b | 水区的两片可控区域 | 可单独控制 |

**这是逻辑边界，不是新墙、地形、触发碰撞或加载关卡。** 地图普通物件按最近符文石划分三个大区，再按朝向小屋的方向分入口120度扇区及两侧各120度；因此原产区周围也有归属。资源仍在原40/40/60米生产范围内生成，沿用原所属符文石，不扩大生产面积。

这是无需打开UE即可建立的第一版稳定边界，不冒称已按每一条真实坡坎精修。`Config/ForestZones.json` 的 PolygonOverrides 可按列表优先级给某个区块配置世界XY多边形，之后细化水岸、坡坎等边界，无需改变资源生成器或区块编号。多边形之外继续使用自动边界，小屋保护圆优先于其他归属。

当前HomeCenter沿用已记录小屋位置(44820,13210,-8950)，保护半径900厘米。入口方向使用符文石实时位置指向HomeCenter。同元素有多个符文石时当前采用名称排序后的第一个；当前地图是三块符文石各一个。移动小屋后需更新HomeCenter，移动符文石或调整表后可调用ReloadLayout。

## 无控制时保持现状

- 随附JSON的Controls为空，不写入默认控制存档。首次使用所有区块为normal。
- 未修改任何umap/uasset、灯光、材质资产、UI、NPC行为或地形。没有分区可视化标记或新碰撞体。
- 保留产区候选生成顺序、PlacementRandom调用、生成数量/变换、ResourceId、CollectionKey、每日再生和采集账本。资源生成完毕才登记区块信息，不重抽、不搬动、不增加资源。
- 中性状态下Available=true、隐藏标记=false，与资源初始值相同，ApplyZoneAvailability直接返回，不再次调用显示/碰撞/漂浮设置。普通场景物件未绑定时也不会改状态。
- 历史产区采集存档保持原格式；区块控制另存ForestWorldZones_v1。已有显式控制存档会恢复，那属于“已经施加控制”，不是新的默认表现。

## 已实际接入的控制

`Get World Subsystem → ForestWorldZones`，可用蓝图或C++：

| 接口 | 行为 |
|---|---|
| GetZones | 获取10个区块编号、中心、入口方向、保护标记 |
| GetZoneAtLocation / GetActorZone | 查询位置/当前Actor的实时归属，NPC移动后查询跟随其位置 |
| GetZoneState | 读取指定区块和日期的状态；Day=0表示当天 |
| SetZoneState | 单独设置区块状态；TargetDay=0立即作用，未来日期在换天后作用；保存失败不应用 |
| ResetZone | 指定日期恢复normal，恢复未采资源和受控物件原始显示/碰撞；不会复活已采点 |
| SetActorZoneControlled | 显式选择一个场景物件是否接受区块控制 |
| ReloadLayout | 重读表、重算资源归属并应用状态 |
| OnZoneStateChanged | 当前状态设置或换天后发事件，供后续环境/NPC效果消费者使用 |

状态字段：ResourceAvailabilityScale(0–1)、MaxAvailableResources(-1不限)、bHarvestEnabled、bHideUnavailableResources、bHideTaggedActors、bDisableTaggedActorCollision，以及StateId、NpcStateId、MapStateId、SourceReceiptId。

- **资源**：蘑菇和贝壳自动登记。比例采用ResourceId的稳定哈希筛选，不消耗生产随机序列。上限按区块内稳定候选序号限制，不额外补生已采数量。停用可隐藏资源，或保留其外观但禁止采集。控制恢复时原未采点恢复，已采点仍隐藏。
- **保底**：入口区资源不受负面控制；另外每个实际产区至少保护第一个成功生成的采集点，即使水域入口扇区没有合格水面也不把全部贝壳封掉。ProtectedResourcesPerProductionRegion可调大，最小1。这是技术保底，不是已定好的数值平衡；只保护现有点，不会凭空生成或无限刷新。
- **保护区状态**：可以设置NPC/地图预设键等非破坏性状态。API拒绝降低保护区资源、关闭其采集或隐藏/禁用其物件；配置读取也会屏蔽这些破坏性字段。
- **场景物件**：仅控制显式标记Forest.ZoneControlled或经接口绑定的Actor。恢复时还原绑定物件原先的隐藏/碰撞值；不强制显现原本隐藏的物件。Pawn/Controller、资源Actor、Landscape、导航、PlayerStart和含实例化网格的Actor不走这条物件开关；Forest.ZoneProtected标签额外保护。当前没有自动批量给场景树木/建筑添加标签，默认不改变它们。
- **NPC/地图预设**：预设键和事件已经可读取，但没有自动实现疲惫、驻留计划、手势、局部天气或雾效。后续可按GetActorZone绑定各自效果。不能把这些键填写了就称为动画/天气已实装。
- **天数**：API保存值覆盖同区同日的配置行，重复设置同值不重复发事件。没有为某日设置的区块自然返回normal，不将前一天控制永久延续。SetZoneState替换同区同日旧值，不自动叠加多条评分效果。

## 配置表示例

以下仅是开发接法示例，**没有写入随附Controls**：

```json
{
  "ZoneId": "fire_gather_a",
  "TargetDay": 2,
  "bEnabled": true,
  "State": {
    "StateId": "reduced",
    "ResourceAvailabilityScale": 0.5,
    "MaxAvailableResources": -1,
    "bHarvestEnabled": true,
    "bHideUnavailableResources": true
  }
}
```

将类似行放入Controls可在第二天只减少火区A的资源；火区B和其他元素不受影响，保护点不受限制。API设置也可指定SourceReceiptId为合成收据，之后评分系统即可调同一接口。**目前仍未自动从左侧评分计算条件和惩罚，也没有让AI文本回写状态。**

## 文件与应用

新增Source/demo3_UE58/ForestWorldZones.h/.cpp、ForestZonePolicy.h、Config/ForestZones.json、Tools/Tests/ForestZonePolicyTests.cpp；修改ForestElementRegions.h/.cpp、ForestPlayerActions.cpp、demo3_UE58.Build.cs。按原路径更新并重新编译；Build.cs增加JSON的NonUFS打包项，需保留其他既有依赖。没有地图或动画资产需要覆盖。

原草案Docs/World_Zone_Control_Draft_2026-10-31.json继续作为历史设计，不是运行时配置；真正读取的是Config/ForestZones.json。

## 验证与边界

- macOS UE5.8 Development Editor编译链接通过，覆盖新USTRUCT/子系统、资源接入和动作取消；结果时间见对应CHANGELOG批次。
- 独立C++回归检查通过：中性状态全部候选可用、扇区中心/边界、减量哈希分界、上限、保底不被关闭、隐藏/禁采/水面漂浮，以及恢复控制不复活已采点。测试不启动UE、不访问网络、不加载用户地图。
- 对比资源生成器差异：没有修改随机调用、候选位置、资产选择、数量目标、旋转/缩放、资源编号或原采集存档格式。JSON语法、差异及凭据检查通过。
- **未打开UE、未运行PIE、未做Windows或打包验收**。未在真实地图实测边界、事件、存档往返或视觉对比，不以策略测试代替这些验收。仍需完整工程的符文石、贝壳等资源依赖；分区框架不弥补缺失模型，也不移动资源以凑保底量。
