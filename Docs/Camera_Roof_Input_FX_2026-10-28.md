# 相机、采集特效与输入报错修正（2026-10-28）

- 按住鼠标右键拖动观察，松开后平滑恢复原始俯视角度。打开菜单或地图时退出观察（按最新操作要求从左键改为右键）。
- 室内遮挡隐藏仅匹配屋顶名称、ForestCameraRoof 标签，以及 Witch House 的屋顶板 SM_Shop_Cube_092；不再按物件大小和高度隐藏任意家具。原有植被淡出与游泳逻辑保留。
- 采集取消前段 FXS_Portal_01 和 FXS_Dust_Spots_01，保留后续光效。
- Dreamscape 预览 Pawn 蓝图 BP_PlayerPawn、CinemaCameraPawn 的 EnableInput 节点取消外部 PlayerController 连线，使用空 Controller 参数。APawn::EnableInput 接受自身 Controller 或空值，原来绑定玩家 Controller 会在未被其控制时产生 LogPawn 错误。执行链保留，蓝图已编译并保存。

## 验证范围

右键版本已在 Windows UE 5.8.3 Development Editor 编译并重新打开。两个预览 Pawn 蓝图已编译保存；按用户要求未启动新的 PIE 测试。合并远程界面更新后的构建结果见本批上传说明。
