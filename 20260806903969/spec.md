# Spec

## 概述

| 属性 | 值 |
|------|-----|
| 特性名称 | 支持鼠标和触控笔输入事件再分发 |
| 特性编号 | FEAT-30094739 |
| 关联需求 | 20260806903969（SR 20260806903969） |
| 优先级 | P1 |
| 复杂度 | 标准（引用 proposal.md 初始分级判断结果） |
| 目标版本 | OpenHarmony-7.1-Release |

## 用户故事或场景

### US-1: 触控笔接近与远离事件再分发

**作为** 全局手势窗口, **我想要** 将未消费的触控笔接近和远离事件再分发到下层窗口, **以便** 下层窗口感知触控笔的接近和远离状态。

**验收标准：**

- **AC-1.1:** Given 全局手势窗口处于 Z 序顶层且已通过注入权限校验 When 手势窗口接收触控笔接近事件（PointerAction=POINTER_ACTION_PROXIMITY_IN, ToolType=TOOL_TYPE_PEN）后未形成手势并调用 RedispatchInputEvent 归还该事件并指定 Z 序 Then 下层窗口（Z 序下一个可见窗口）收到该触控笔接近事件，事件 ToolType=TOOL_TYPE_PEN，EVENT_FLAG_REDISPATCH 已设置
- **AC-1.2:** Given 全局手势窗口处于 Z 序顶层且已通过注入权限校验 When 手势窗口接收触控笔远离事件（PointerAction=POINTER_ACTION_PROXIMITY_OUT, ToolType=TOOL_TYPE_PEN）后未形成手势并调用 RedispatchInputEvent 归还该事件并指定 Z 序 Then 下层窗口收到该触控笔远离事件，事件 ToolType=TOOL_TYPE_PEN

### US-2: 触控笔按下、移动、抬起事件再分发

**作为** 全局手势窗口, **我想要** 将未消费的触控笔按下、移动、抬起事件再分发到下层窗口, **以便** 下层窗口正常响应触控笔操作。

**验收标准：**

- **AC-2.1:** Given 全局手势窗口已通过注入权限校验 When 手势窗口接收触控笔按下事件（PointerAction=POINTER_ACTION_DOWN, ToolType=TOOL_TYPE_PEN）后未消费并调用 RedispatchInputEvent 归还 Then 下层窗口收到该触控笔按下事件
- **AC-2.2:** Given 全局手势窗口已通过注入权限校验 When 手势窗口接收触控笔移动事件（PointerAction=POINTER_ACTION_MOVE, ToolType=TOOL_TYPE_PEN）后未消费并调用 RedispatchInputEvent 归还 Then 下层窗口收到该触控笔移动事件
- **AC-2.3:** Given 全局手势窗口已通过注入权限校验 When 手势窗口接收触控笔抬起事件（PointerAction=POINTER_ACTION_UP, ToolType=TOOL_TYPE_PEN）后未消费并调用 RedispatchInputEvent 归还 Then 下层窗口收到该触控笔抬起事件

### US-3: 触控笔悬停移动事件再分发

**作为** 全局手势窗口, **我想要** 将未消费的触控笔悬停移动事件再分发到下层窗口, **以便** 下层窗口正确更新触控笔悬停位置。

**验收标准：**

- **AC-3.1:** Given 全局手势窗口已通过注入权限校验 When 手势窗口接收触控笔悬停移动事件（PointerAction=POINTER_ACTION_LEVITATE_MOVE, ToolType=TOOL_TYPE_PEN）后未消费并调用 RedispatchInputEvent 归还 Then 下层窗口收到该触控笔悬停移动事件

### US-4: 衍生事件丢弃

**作为** 全局手势窗口, **我想要** 再分发时不产生窗口进入/离开衍生事件, **以便** 避免下层窗口收到冗余的进入/离开窗口衍生事件（POINTER_ACTION_LEVITATE_IN_WINDOW / POINTER_ACTION_LEVITATE_OUT_WINDOW）。

**验收标准：**

- **AC-4.1:** Given 再分发路径中存在 PointerAction=POINTER_ACTION_LEVITATE_IN_WINDOW 的触控笔衍生事件 When 调用 RedispatchInputEvent 归还该事件 Then 该事件被丢弃，不进入目标选择和分发，下层窗口不收到该再分发事件
- **AC-4.2:** Given 再分发路径中存在 PointerAction=POINTER_ACTION_LEVITATE_OUT_WINDOW 的触控笔衍生事件 When 调用 RedispatchInputEvent 归还该事件 Then 该事件被丢弃，不进入目标选择和分发，下层窗口不收到该再分发事件

### US-5: 中间环节不感知再分发事件

**作为** 中间监听/拦截环节, **我不应** 感知再分发事件, **以便** 避免干扰正常拦截逻辑。

**验收标准：**

- **AC-5.1:** Given 全局手势窗口已通过注入权限校验 When 调用 RedispatchInputEvent 归还触控笔事件 Then 中间监听环节回调未收到该事件
- **AC-5.2:** Given 全局手势窗口已通过注入权限校验 When 调用 RedispatchInputEvent 归还触控笔事件 Then 中间拦截环节回调未收到该事件，且下层目标窗口正常收到该事件

### US-6: 权限校验

**作为** 系统安全管控, **我想要** 再分发接口需注入权限校验, **以便** 防止未授权应用调用再分发接口。

**验收标准：**

- **AC-6.1:** Given 调用方无 ohos.permission.INJECT_INPUT_EVENT 权限 When 调用 RedispatchInputEvent 接口 Then 返回权限错误码（201），事件不被再分发
- **AC-6.2:** Given 调用方有 ohos.permission.INJECT_INPUT_EVENT 权限 When 调用 RedispatchInputEvent 接口归还触控笔事件 Then 权限校验通过，事件正常进入再分发流程

**验证方式说明：** US-6 仅通过单元测试（UT）覆盖验证。理由：正式版本中调用方均为持注入权限的系统应用，无法构建无权限场景进行板侧验收；AC-6.1/AC-6.2 通过 UT 模拟（mock）调用方权限状态覆盖验证，板侧仅保留有权限正常路径的现象验证（由 US-1~US-7 覆盖）。

### US-7: 现有行为保持不变

**作为** 现有再分发业务方, **我想要** 鼠标和触屏手指再分发现有行为保持不变, **以便** 现有功能不受影响。

**验收标准：**

- **AC-7.1:** Given 鼠标再分发场景 When 形成手势的鼠标输入事件分发到手势窗口 Then 鼠标手势正常触发，行为无变化
- **AC-7.2:** Given 触控笔再分发场景 When 形成手势的触控笔输入事件分发到手势窗口 Then 触控笔手势正常触发，行为无变化
- **AC-7.3:** Given 鼠标未消费事件再分发场景 When 手势窗口未消费鼠标事件并调用 RedispatchInputEvent 归还 Then 下层窗口收到完整的鼠标按下/移动/抬起/滚轮事件序列，行为无变化

## 业务规则

| 规则 ID | 规则描述 | 约束条件 | 关联 AC |
|---------|----------|----------|---------|
| BR-1 | 触控笔再分发事件源类型为 SOURCE_TYPE_TOUCHSCREEN，工具类型为 TOOL_TYPE_PEN 或 TOOL_TYPE_PENCIL | 再分发处理按工具类型区分触控笔和手指状态 | AC-1.1~AC-3.1 |
| BR-2 | 衍生事件 POINTER_ACTION_LEVITATE_IN_WINDOW / POINTER_ACTION_LEVITATE_OUT_WINDOW（进入/离开窗口）在再分发路径统一丢弃 | 丢弃仅在再分发路径生效，正常分发路径不受影响 | AC-4.1, AC-4.2 |
| BR-3 | 再分发事件仅送达目标窗口，中间监听/拦截环节回调不收到该事件 | 该隔离仅对再分发路径生效，正常分发路径行为不变 | AC-5.1, AC-5.2 |
| BR-4 | 触控笔与手指的再分发活跃状态相互隔离 | 同一设备和指针下，触控笔与手指各自独立维护活跃状态，互不干扰 | AC-1.1~AC-3.1 |
| BR-5 | 再分发按业务方指定 Z 序重新选择目标窗口 | 目标窗口为指定 Z 序之下的下一个可见窗口 | AC-1.1~AC-3.1 |
| BR-6 | RedispatchInputEvent 需注入权限 ohos.permission.INJECT_INPUT_EVENT | 权限校验在接口入口执行，无权限直接返回错误不进入分发 | AC-6.1, AC-6.2 |

## 异常与边界规则

| 编号 | 场景 | 触发条件 | 系统行为 | 关联 AC |
|------|------|----------|----------|---------|
| EX-1 | Z 序不存在目标窗口 | 业务方指定的 Z 序下方无可见窗口 | 再分发事件丢弃，记录警告日志，返回成功（事件已处理但无目标） | AC-1.1~AC-3.1 |
| EX-2 | 触控笔与手指同时活跃 | 同一 deviceId 下触控笔和手指均有活跃指针 | 按 toolType 隔离管理，互不干扰，各自独立维护活跃状态 | AC-1.1~AC-3.1 |
| EX-3 | 再分发事件序列不完整 | 触控笔接近事件未先于按下事件到达 | 不做序列完整性强制校验，按到达顺序逐事件再分发 | AC-1.1~AC-3.1 |
| EX-4 | 衍生事件在正常分发路径 | 非 EVENT_FLAG_REDISPATCH 标记的 LEVITATE_IN_WINDOW / LEVITATE_OUT_WINDOW 事件 | 正常分发，不丢弃（丢弃仅对再分发路径生效） | AC-4.1, AC-4.2 |
| EX-5 | 再分发接口并发调用 | 多线程同时调用 RedispatchInputEvent | 再分发处理串行化执行，活跃状态操作线程安全 | AC-1.1~AC-3.1 |
| EX-6 | 无权限调用 | 调用方缺少 ohos.permission.INJECT_INPUT_EVENT | 返回错误码 201，事件不进入再分发流程 | AC-6.1 |

## 错误码定义

| 错误码 ID | 错误码值 | 含义 | 关联 AC |
|-----------|----------|------|---------|
| PERMISSION_DENIED | 201 | 调用方缺少 ohos.permission.INJECT_INPUT_EVENT 权限，RedispatchInputEvent 返回权限拒绝错误，事件不被再分发 | AC-6.1 |

## API 规格定义

本次变更不涉及 API/SDK 设计。理由：使用现有内部接口 `RedispatchInputEvent`（InnerAPI），仅在 C++ 实现层扩展触控笔事件处理逻辑，不新增或修改任何公开 API 声明文件。现有 `RedispatchInputEvent` 接口签名不变，本次仅扩展其内部实现（`RedispatchInputEventInner`）对触控笔工具类型的识别和衍生事件丢弃逻辑。

## 接口变更分析

### 新增接口

| 接口名称 | 开放级别 | 参数概要 | 返回值 | 错误码 | 关联 AC |
|----------|----------|----------|--------|--------|---------|
| 无新增公开接口 | — | — | — | — | — |

本次不新增公开或内部接口，仅在现有 InnerAPI `RedispatchInputEvent` / `RedispatchInputEventInner` 内部实现中扩展逻辑。

### 变更/废弃接口

| 接口名称 | 开放级别 | 变更类型 | 影响场景 | 迁移指引 | 关联 AC |
|----------|----------|----------|----------|----------|---------|
| RedispatchInputEvent | Inner | 行为扩展（不改签名） | 触控笔事件再分发：现有接口内部实现增加触控笔工具类型识别和衍生事件丢弃 | 无需迁移，接口签名不变，现有调用方无感知 | AC-1.1~AC-4.2 |
| RedispatchInputEventInner | Inner | 行为扩展（不改签名） | 触屏分支增加触控笔 ToolType 识别逻辑 | 无需迁移，内部函数，外部不可见 | AC-1.1~AC-3.1 |

## 兼容性声明

- **已有 API 行为变更:** 否。RedispatchInputEvent 接口签名不变，鼠标和触屏手指再分发现有行为保持不变（运行时分支隔离）。
- **配置文件格式变更:** 否。不涉及配置文件变更。
- **数据存储格式变更:** 否。不涉及持久化数据变更。

## 验证映射

| AC | 关联规则 | 验证方式 | 证据 |
|----|----------|----------|------|
| AC-1.1 | BR-1, BR-4, BR-5 | 单元测试 + 板侧功能测试 | 单元测试：触控笔接近事件再分发到目标窗口；板侧：触控笔接近手势窗口，下层窗口收到接近事件 |
| AC-1.2 | BR-1, BR-4, BR-5 | 单元测试 + 板侧功能测试 | 单元测试：触控笔远离事件再分发到目标窗口 |
| AC-2.1 | BR-1, BR-4 | 单元测试 + 板侧功能测试 | 单元测试：触控笔按下事件再分发到目标窗口 |
| AC-2.2 | BR-1, BR-4 | 单元测试 + 板侧功能测试 | 单元测试：触控笔移动事件再分发到目标窗口 |
| AC-2.3 | BR-1, BR-4 | 单元测试 + 板侧功能测试 | 单元测试：触控笔抬起事件再分发到目标窗口 |
| AC-3.1 | BR-1, BR-4 | 单元测试 + 板侧功能测试 | 单元测试：触控笔悬停移动事件再分发到目标窗口 |
| AC-4.1 | BR-2 | 单元测试 | 单元测试：再分发路径 LEVITATE_IN_WINDOW 事件被丢弃，不进入分发 |
| AC-4.2 | BR-2 | 单元测试 | 单元测试：再分发路径 LEVITATE_OUT_WINDOW 事件被丢弃，不进入分发 |
| AC-5.1 | BR-3 | 单元测试 + 板侧验证 | 单元测试：中间监听环节回调未收到再分发事件 |
| AC-5.2 | BR-3 | 单元测试 + 板侧验证 | 单元测试：中间拦截环节回调未收到再分发事件，目标窗口正常收到 |
| AC-6.1 | BR-6 | 单元测试（仅 UT） | 单元测试：UT 模拟无权限调用返回错误码 201（正式版本无法构建板侧无权限场景） |
| AC-6.2 | BR-6 | 单元测试（仅 UT） | 单元测试：UT 模拟有权限调用正常进入再分发流程 |
| AC-7.1 | — | 回归测试 | 回归测试：鼠标形成手势事件行为无变化 |
| AC-7.2 | — | 回归测试 | 回归测试：触控笔形成手势事件行为无变化 |
| AC-7.3 | — | 回归测试 | 回归测试：鼠标未消费事件再分发行为无变化 |

## 关键 HiLog 日志（US-1~US-5）

> 下表列出 US-1~US-5 各 AC 关键实现路径对应的关键 HiLog 日志，用于确认实现按规格执行。日志位于多模输入服务进程 hilog（部分为 DEBUG 级别，查看时需开启 hilog debug）。

| AC | 关键实现点 | 关键 HiLog 日志 | 来源 |
|----|-----------|-----------|-----------|
| AC-1.1 | 触控笔接近自动激活（PROXIMITY_IN） | 日志 `Auto-activate pen on PROXIMITY_IN` | 现有（TouchRedispatchStore::Abandon） |
| AC-1.1 | 再分发事件末端分发进入 | 日志 `Redispatch, id:` 与 `source:` | 现有（RedispatchInputEventInner） |
| AC-1.1 | Z 序目标过滤 | 日志 `Skip the invalid zOrder window` | 现有（InputWindowsManager::DispatchTouch） |
| AC-1.2 | 触控笔远离去活（PROXIMITY_OUT） | 日志 `Deactivate pen on PROXIMITY_OUT` | 现有（TouchRedispatchStore::Abandon） |
| AC-2.1 | 再分发接受；真实手指同窗冲突放弃 | 日志 `Redispatch, id:`；冲突场景日志 `Redispatch DOWN conflicts with real finger in same window` | 现有（RedispatchInputEventInner） |
| AC-2.2 | 再分发接受 | 日志 `Redispatch, id:` | 现有（RedispatchInputEventInner） |
| AC-2.3 | 抬起后触控笔状态保持（仅 PROXIMITY_OUT 去活） | 日志 `Redispatch, id:`，且直至远离事件前无 `Deactivate pen on PROXIMITY_OUT` | 现有（RedispatchInputEventInner、TouchRedispatchStore::Abandon） |
| AC-3.1 | 悬停移动末端分发 | 日志 `Redispatch, id:`；窗口进出衍生触发时 `Levitate in by horizon` / `Levitate out by horizon` / `Levitate in by Z-` / `Levitate out by Z+` | 现有（RedispatchInputEventInner、InputWindowsManager::HandleLevitateInOutEvent） |
| AC-4.1 | 进入窗口衍生事件入口丢弃 | 日志 `Redispatch: derived event discarded` | 规划（随本变更丢弃逻辑实现） |
| AC-4.2 | 离开窗口衍生事件入口丢弃 | 日志 `Redispatch: derived event discarded` | 规划（随本变更丢弃逻辑实现） |
| AC-5.1 | 结构性跳过：末端进入且中间监听节点未执行 | 存在日志 `Redispatch, id:`，且 EventMonitorHandler 回调无日志 | 现有 |
| AC-5.2 | 结构性跳过：末端进入且中间拦截节点未执行，目标窗口正常收到 | 存在日志 `Redispatch, id:`，且 EventInterceptorHandler 回调无日志 | 现有 |

## 测试设计提示

> US-1~US-5 各行的关键实现路径可通过『关键 HiLog 日志』表中对应 AC 的日志关键字确认；AC-6.x 为 UT 权限模拟场景。

| AC | 测试类型 | 测试文件 | 测试名称 | 输入/触发 | 期望输出/错误 | Red 条件 |
|----|----------|----------|----------|-----------|---------------|----------|
| AC-1.1 | 单元测试 | service/window_manager/test/touch_redispatch_store_test.cpp | RedispatchStylusApproachEvent | 构造触控笔接近事件（PROXIMITY_IN, PEN），调用 RedispatchInputEventInner | 下层窗口收到该事件，ToolType=PEN | 实现前：触控笔事件未被识别，触屏分支不处理 PEN 工具类型 |
| AC-1.2 | 单元测试 | service/window_manager/test/touch_redispatch_store_test.cpp | RedispatchStylusDepartEvent | 构造触控笔远离事件（PROXIMITY_OUT, PEN），调用 RedispatchInputEventInner | 下层窗口收到该事件 | 实现前：触控笔远离事件未被正确再分发 |
| AC-2.1 | 单元测试 | service/window_manager/test/touch_redispatch_store_test.cpp | RedispatchStylusDownEvent | 构造触控笔按下事件（DOWN, PEN） | 下层窗口收到按下事件 | 实现前：PEN 工具类型在触屏分支未区分 |
| AC-2.2 | 单元测试 | service/window_manager/test/touch_redispatch_store_test.cpp | RedispatchStylusMoveEvent | 构造触控笔移动事件（MOVE, PEN） | 下层窗口收到移动事件 | 实现前：PEN 移动事件与手指状态混淆 |
| AC-2.3 | 单元测试 | service/window_manager/test/touch_redispatch_store_test.cpp | RedispatchStylusUpEvent | 构造触控笔抬起事件（UP, PEN） | 下层窗口收到抬起事件 | 实现前：PEN 抬起事件状态清理不正确 |
| AC-3.1 | 单元测试 | service/window_manager/test/touch_redispatch_store_test.cpp | RedispatchStylusLevitateMoveEvent | 构造触控笔悬停移动事件（LEVITATE_MOVE, PEN） | 下层窗口收到悬停移动事件 | 实现前：PEN LEVITATE_MOVE 未走再分发路径 |
| AC-4.1 | 单元测试 | service/module_loader/test/redispatch_derived_event_test.cpp | DiscardLevitateInWindowOnRedispatch | 构造 POINTER_ACTION_LEVITATE_IN_WINDOW 事件标记 EVENT_FLAG_REDISPATCH | 事件被丢弃，不进入分发 | 实现前：衍生事件丢弃逻辑未实现，LEVITATE_IN_WINDOW 进入分发 |
| AC-4.2 | 单元测试 | service/module_loader/test/redispatch_derived_event_test.cpp | DiscardLevitateOutWindowOnRedispatch | 构造 POINTER_ACTION_LEVITATE_OUT_WINDOW 事件标记 EVENT_FLAG_REDISPATCH | 事件被丢弃，不进入分发 | 实现前：衍生事件丢弃逻辑未实现，LEVITATE_OUT_WINDOW 进入分发 |
| AC-5.1 | 单元测试 | service/event_dispatch/test/redispatch_visibility_test.cpp | MiddleListenerNotNotified | 注册中间监听回调，构造触控笔再分发事件 | 中间监听回调未收到事件 | 实现前：触控笔再分发事件可被中间监听感知 |
| AC-5.2 | 单元测试 | service/event_dispatch/test/redispatch_visibility_test.cpp | MiddleInterceptorNotNotified | 注册中间拦截回调，构造触控笔再分发事件 | 中间拦截回调未收到事件，目标窗口正常收到事件 | 实现前：触控笔再分发事件可被中间拦截感知 |
| AC-6.1 | 单元测试（仅 UT） | service/module_loader/test/redispatch_permission_test.cpp | PermissionDeniedWithoutInjectPermission | UT 模拟无权限调用方（mock 权限校验），调用 RedispatchInputEvent | 返回错误码 201 | 实现前：权限校验已存在但需验证覆盖触控笔场景 |
| AC-6.2 | 单元测试（仅 UT） | service/module_loader/test/redispatch_permission_test.cpp | PermissionGrantedWithInjectPermission | UT 模拟有权限调用方，归还触控笔事件 | 正常进入再分发 | 实现前：触控笔事件未被接受处理 |
| AC-7.1 | 回归测试 | service/window_manager/test/mouse_redispatch_regression_test.cpp | MouseGestureUnchanged | 鼠标形成手势事件 | 鼠标手势正常触发 | 实现前：无（现有行为，验证不受影响） |
| AC-7.2 | 回归测试 | service/window_manager/test/mouse_redispatch_regression_test.cpp | StylusGestureUnchanged | 触控笔形成手势事件 | 触控笔手势正常触发 | 实现前：无（现有行为，验证不受影响） |
| AC-7.3 | 回归测试 | service/window_manager/test/mouse_redispatch_regression_test.cpp | MouseRedispatchUnchanged | 鼠标未消费事件再分发 | 下层窗口收到完整鼠标序列 | 实现前：无（现有行为，验证不受影响） |
