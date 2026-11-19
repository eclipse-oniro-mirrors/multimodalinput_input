# 触控板原始报点知识

应用通过 `InputManager::EnableTouchpadRawEvent` 使能后，服务端在归一化之前旁路采集触控板
原始触点，以 `sourceType = SOURCE_TYPE_TOUCHPAD_RAW` 的 `PointerEvent` 直投给已使能的
焦点进程（@since 26.1.0）。本文只记录该链路容易被改错的边界。

## 使能与门控边界

- 使能状态按 pid 记录在 `EventNormalizeHandler::rawTouchpadEnabled_`。重复使能/重复
  去使能返回 `MMI_RAW_ERR_REPEAT_OPERATION`（4900103）；IPC 失败返回
  `MMI_RAW_ERR_IPC_FAILED`（4900102），错误码定义见 `TouchpadRawEventErrCode`。
- 会话断开时由 `MMIService` 调 `RemoveTouchpadRawEventEnabled(pid)` 清理，不要依赖
  应用主动去使能。
- 采集门控在 `CollectRawTouchpadEvent`：仅当显示组焦点进程（`GetFocusPid(groupId)`）
  已使能且焦点窗口存在时才上报；失焦或未使能不产生任何事件。
- 使能 IPC 走 `IMultimodalInputConnect.idl` 的 `SetTouchpadRawEventSwitch`，链路为
  `InputManagerImpl` → 连接管理 → `MMIService::OnSetTouchpadRawEventSwitch` →
  `EventNormalizeHandler`。改动签名前先看"协议与数据格式兼容性"约束。

## 数据与分发边界

- 原始触点数据只放在 `PointerEvent::TouchpadRawExt`（`deviceWidth`/`deviceHeight`
  单位 mm、触发指 `seatSlot`、`rawPoints`），不进 `PointerItem`；单个
  `PointerItem`（pointerId=0，TOOL_TYPE_TOUCHPAD）只承载触发时刻的光标位置。
  触点坐标与设备宽高必须保持同一单位（libinput 已按轴分辨率换算为 mm）。
- 直投路径 `EventDispatchHandler::DispatchRawTouchpadPointerEvent` 用
  `MmiMessageId::ON_POINTER_EVENT` 按 pid 发送，绕过拦截器、监视器和 ANR 定时器。
  不要在该路径引入对责任链设施的依赖。
- 事件对象在同一触控序列内复用：`FillRawPointerEvent` 对 pointerId=0 走
  `UpdatePointerItem` 而非重复 `AddPointerItem`，改动时不要破坏该复用逻辑。
- `SOURCE_TYPE_TOUCHPAD_RAW` 的 switch 分支有三处需同步维护：
  `PointerEvent::IsValid`（原始 PointerItem 无 downTime，不做按下时间校验）、
  `InputEvent::DumpSourceType`、`EventStatistic::ConvertSourceTypeToString`
  （均为 `"touch-pad-raw"`）。

## 事件序列完整性

- 原始报点必须保持完整且时序单调的事件序列：每个 seatSlot 的触点从 DOWN 经 MOVE 到
  UP 结束，序列被手势接管时以 CANCEL 收尾，不能缺项；应用侧依赖配对的 UP/CANCEL
  结束触点序列。
- 服务端 UP 兼作 `rawTouchpadPoints_`/`rawTouchpadDownedSlots_` 的清理时机，任何
  过滤逻辑都不得丢弃 UP：触点残留会导致序列无法结束、`rawTouchpadPaused_` 无法复位。
- CANCEL 只负责给出终态，不清理触点表；暂停期间的 UP 仍被触点表维护逻辑消费，
  直至全部手指抬起。

## 手势接管边界

- 系统手势接管（`LIBINPUT_EVENT_GESTURE_SWIPE_BEGIN`、边缘滑动、指关节双击）会触发
  `DispatchTouchpadRawCancel`：先向已使能焦点进程下发 `POINTER_ACTION_CANCEL`
  （携带 fingerCount），再置 `rawTouchpadPaused_ = true`。
- 暂停期间 DOWN/MOTION 不上报，UP 仅用于维护内部触点表；`rawTouchpadPoints_` 清空
  （全部手指抬起）后 `rawTouchpadPaused_` 复位、恢复上报。调整 cancel 策略时保持
  "先 cancel、后暂停、抬完恢复"的顺序。

## 客户端边界

- 接收复用既有窗口输入通道：`SetWindowInputEventConsumer` 设置的 `IInputEventConsumer`
  回调；应用靠 `GetSourceType() == SOURCE_TYPE_TOUCHPAD_RAW` 加 `HasTouchpadRawExt()`
  识别原始报点。
- 该能力不暴露给 NDK/C API（`interfaces/kits/c/input`）和 ArkTS 侧；js monitor 走
  `REPORT_POINTER_EVENT` 通道，收不到原始报点。

## 测试指引

- 使能开关、采集与分发：`service/event_handler/test/event_normalize_handler_ex_test.cpp`
  （`SetTouchpadRawEventEnabled_*`、`RemoveTouchpadRawEventEnabled_*` 及 raw 用例）。
- 序列化：`util/common/test/input_event_data_transformation_test.cpp`。
- 连接管理 mock：`service/connect_manager/test/mock_multimodal_input_connect_stub.h`。
- 板侧验证：HiLog 过滤 `EnableTouchpadRawEvent`、`FillRawPointerEvent`、
  `DispatchRawTouchpadPointerEvent`；`hidumper` 事件统计中 sourceType 显示
  `touch-pad-raw`。板侧证据要求见 `board-verification.md`。
