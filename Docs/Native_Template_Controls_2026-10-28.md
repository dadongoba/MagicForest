# 第一、第三人称改用 UE 模板控制（2026-10-28）

本批替代前批三视角说明中第一、第三人称的手写移动、鼠标角度累加、逐帧强制角色转向和肩后镜头；C 找人、V 三视角循环及上帝视角控制保留。

## 模板来源与接入

以本机 UE 5.8 随引擎提供的 TP_FirstPersonCharacter.cpp、TP_ThirdPersonCharacter.cpp 为准，采用模板的 Enhanced Input 回调及原生 CharacterMovement/Controller/Camera 流程。模板类属于新建项目源码，并非当前项目可直接实例化的引擎内置类，因此在既有角色上接入对应模板方法和配置，保留背包、交互、存档和地图中的角色身份。

- 第一人称：复用角色原有 FirstPersonCamera（bUsePawnControlRotation=true），角色 bUseControllerRotationYaw=true，bOrientRotationToMovement=false。IA_Move 按角色 Forward/Right 加移动输入；IA_Look 通过 AddControllerYawInput/AddControllerPitchInput，由控制器正常更新视角。移动参数使用原角色默认值及模板的 AirControl/BrakingDecelerationFalling 设置。
- 第三人称：按模板以 ControlRotation 的水平角计算前后/左右移动；角色 bUseControllerRotationYaw=false，bOrientRotationToMovement=true，CharacterMovement 自动朝移动方向旋转。镜头使用居中弹簧臂、bUsePawnControlRotation=true、相机自身不重复应用控制器旋转、默认 400 距离和 90 度视野。转鼠标可自由环绕，角色站立时不被鼠标强制扭转；按 S/A/D 时朝对应移动方向转身，这是第三人称模板的标准行为。
- IA_Jump 的 Started/Completed 接入 Character 的 Jump/StopJumping。
- 复用工程已有 Witch_House/Demo/FirstPerson/Input 下的 IMC_Default 与 IA_Move/IA_Look/IA_Jump；读回动作类型分别为 Axis2D/Axis2D/Boolean。若映射已存在不重复添加，组件只移除自己补加的映射。
- 模板输入仍排除菜单/全图及失焦状态；原 Pawn 的重复输入保持禁用，避免双重输入。两种模板视角不再执行上帝视角的逐帧鼠标/移动计算，也不锁定 LookInput；上帝视角保留自己的右键观察、缩放和 5 秒空闲回正。
- 移除自定义第三人称肩偏移和滚轮缩放；鼠标灵敏度使用现有输入动作/映射的修改器，不再叠加 ObservationSensitivity。

## 文件与应用

修改 Source/demo3_UE58/ForestIsometricPlayer.h/.cpp；新增本说明；无需修改资产、地图或配置。依赖前批三视角组件及工程已有第一人称输入资产。应用源码后重新编译并重新打开编辑器，保留旧 Pawn 蓝图和输入资产。

## 验证

UE5.8.3 Win64 Development Editor UHT/源码编译通过；最终源码编译 8.37 秒、DLL 链接和元数据写入成功（Result: Succeeded，3.85 秒）。模板源码对照、输入动作类型读回完成。重新打开项目并启动 PIE 成功，日志确认 move/look/jump/context 均加载。原生 Slate 工具发送 V 返回成功但组件仍为 Overhead，因此不能算作视角切换验证通过。用户随后明确要求自己测试，已停止自动操作；第一/第三人称实际切换、移动手感、镜头及菜单/游泳组合均由用户验收，项目保持运行。
