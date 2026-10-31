# 区块状态的俯视可见表现（2026-10-31）

基于 c01249b 的十区控制，本批提供真实运行时消费者，读取同一份当日区块状态；不修改关卡或二进制资产，不启用结算惩罚。

## 已实现

| 预设键 | 区块大面积表现 | 资源表现 |
|---|---|---|
| weakened | 灰褐色局部雾斑，无漂浮光点 | 可采资源略缩小、暖暗叠色；停用资源灰化 |
| overgrown | 与火/风/水所属产区对应的暖橙/青绿/蓝色雾斑与漂浮光点 | 可采资源放大到原来的1.2倍并叠加元素光色 |
| disturbed | 紫色异色雾斑、漂浮光点 | 可采资源紫色叠光；停用资源灰化 |
| inactive | 冷灰色雾斑，无漂浮光点 | 停用资源变灰并缩小，保留位置供玩家辨认 |
| normal / 空键 | 移除本批效果 | 恢复原材质槽、覆盖材质和原始大小 |

资源是否停用依然由 ResourceAvailabilityScale、MaxAvailableResources、bHarvestEnabled 决定。外观预设不暗中减少资源，也不改变元素、数值或库存。bShowInactiveResources=true 时，已停用但未采走的资源会显现且不可采；已采集资源始终保持消失。保底资源和保护区不应用异常外观。

大面积表现使用 UE 原生 LocalFogVolume：在原产区生成半径内按网格探测自然地面/水面，每区最多6个软边雾斑，采样周围8个边缘点也必须属于同区，尽量避开保护区。不是整块地表换材质，也没有修改整片植被材质；雾斑有软边，不能当作精确分区边界。无地面时稍后重试。多边形覆盖仍受产区范围限制。

光点用引擎球体和已有引擎叠加材质，合批绘制，每雾斑5个，10Hz轻微漂浮。没有引入第三方新资产、全局后处理或额外粒子插件。雾体在起伏地形的覆盖、相邻区软边和远近视觉效果需要实际地图验收。

## 数据与调用

Config/ForestZones.json 新增 VisualPresets，可调整雾颜色、密度、半径、资源大小、叠光颜色和亮度；这些是外观参数，不是结算阈值。MapStateId 非空时优先，否则读取 StateId；未知键或 normal 不套用外观。修改外观表后调用 ReloadLayout。

现有 SetZoneState(ZoneId, State, TargetDay) 已同时接通外观与资源控制，无需额外手工生成雾或修改资源。未来日期到当日自动生效；ResetZone 恢复，收集存档不回滚。

下面仅为调用示例，不在默认配置启用：

```cpp
FForestZoneState State;
State.StateId = TEXT("weakened");
State.ResourceAvailabilityScale = 0.5f; // 演示值，正式数值仍待设计。
Zones->SetZoneState(TEXT("fire_gather_a"), State, NextDay);
// 下一天：局部灰褐雾，约半数普通资源失活变灰，保底节点仍可采。
// 完全失活需另设置 bHarvestEnabled=false；仅设置外观键不改变可采性。
```

默认 Controls=[]，不随机制造异常。结算数据到区块效果的条件与幅度尚未启用；NPC当天服务状态、前往异常区块动作、资源数值增减和元素替换不属于本批已实现内容。不要把过盛放大模型误认为已提高产出数值。

## 应用、验证与边界

- 前置：2026-10-31-05；按原路径更新 ForestWorldZones、ForestElementRegions、新增 ForestZoneVisuals.h/.cpp，合并 JSON 的 VisualPresets 并保留自己的 Controls / PolygonOverrides，重新编译。无存档迁移或资产重存要求。
- 外观使用的引擎网格/材质由 CDO 硬引用；JSON 沿用既有 NonUFS 打包。Local Fog Volumes 引擎默认启用；若目标工程自行关闭 r.SupportLocalFogVolumes 或 r.LocalFogVolume，需恢复支持，否则区域雾不显示。光点和资源外观是独立的。
- 本次 macOS UE5.8 Development Editor 编译与链接通过；分区/可采性独立回归通过；JSON数值/唯一键/中性默认验证通过。
- 未打开 UE、未运行 PIE，未进行实际画面、Windows或打包验收；没有提供伪造的游戏截图。完整工程仍需原本的区域资源资产，尤其 Corals2 贝壳网格。
