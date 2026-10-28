# `RedispatchInputEvent` 接口实现分析报告

> 分析基线：`feat_redispatch` 分支（与 master 同步，HEAD = 458f28a4d，!10091）
> 分析日期：2026-10-28
> 目的：为接口扩展开发（鼠标事件 + 全量手写笔事件）提供实现细节参考

---

## 1. 接口概览

`RedispatchInputEvent` 是 MMI 子系统（@since 24，system API）提供的**指针类事件重分发（再分发）接口**：调用方把一个**经正常 Dispatch 路径派发到自身窗口**的 `PointerEvent`（触摸屏事件或鼠标事件）交还给 MMI 服务，由服务端**重新做一次窗口命中（hit-test）并派发到目标窗口**——但整个寻窗与状态维护过程运行在一套与真实硬件输入**完全隔离的影子状态**里，且带 zOrder 层级约束和会话冲突消解。**不涉及拦截器（interceptor）和 Hook 机制**。

接口声明：`interfaces/native/innerkits/proxy/include/input_manager.h:1470`

```cpp
/**
 * @brief Redispatch a touchpad input event, touchscreen input event, or mouse device input event.
 * @param pointerEvent Indicates the touchpad input event, touchscreen input event,
 * or mouse device input event to redispatch.
 * @return Return <b>0</b> if successful; returns a non-0 value otherwise.
 * @since 24
 */
int32_t RedispatchInputEvent(std::shared_ptr<PointerEvent> pointerEvent);
```

**典型使用场景**（手势识别未达成的回落分发）：

1. 真实输入事件经正常 Dispatch 路径派发到应用窗口 W（其 zOrder 记为 Z_W）；
2. 应用在窗口内启动手势识别（如系统级手势、批注起笔判定等）；
3. 手势识别**未达成**（未形成有效手势）时，应用调用本接口，将事件 zOrder 指定为**低于自身窗口 Z_W** 的值，把事件"退还"给 MMI；
4. MMI 强制重新寻窗：`IsValidZorderWindow`（input_windows_manager.cpp:8556）过滤掉 zOrder ≥ 事件指定值的窗口，事件只能落到**低于指定 zOrder** 的窗口——W 自身及 W 之上的窗口均不可命中；
5. 该输入流后续的 MOVE/UP 依次重分发，并锁定在首次寻窗命中的窗口（会话模型见 §5.3）；期间若真实手指在命中窗口按下，冲突消解会取消重分发会话（"真实赢"）。

即：**事件的第一个消费者是应用自身（正常派发路径），应用保留"自己消化或下放"的决定权——识别不成手势才把整条输入流下放到更低层级窗口**。这与 Hook（服务端派发前截取）、拦截器（服务端订阅事件副本）是三类不同机制，本接口不涉及后两者。

⚠️ **文档与实现不一致**：头文件注释声称支持 touchpad，但服务端实现自始至终**拒绝 TOUCHPAD**（sourceType 白名单里没有，见 §3），扩展时需先决策 touchpad 是否纳入。

## 2. 完整调用链

```
应用进程 (system app / native)
 │  InputManager::RedispatchInputEvent                 frameworks/proxy/events/src/input_manager.cpp:1115
 ▼
InputManagerImpl::RedispatchInputEvent                frameworks/proxy/event_handler/src/input_manager_impl.cpp:3641
 │  · 编译期守卫 OHOS_BUILD_ENABLE_POINTER || TOUCH（否则直接 RET_ERR）
 │  · 判空 pointerEvent
 ▼
MultimodalInputConnectManager::RedispatchInputEvent   service/connect_manager/src/multimodal_input_connect_manager.cpp:1507
 │  · 全局 mutex lock_ 串行化
 │  · multimodalInputConnectService_ 为空 → 返回 INVALID_HANDLER_ID
 ▼
IPC 同步调用（IDL 生成代码）
    IMultimodalInputConnect.idl:214  void RedispatchInputEvent([in] PointerEvent pointerEvent);
    · PointerEvent 全量 Marshalling/Unmarshalling，zOrder_ 字段包含在内
      （pointer_event.cpp:1221 WRITEFLOAT / :1319 READFLOAT，默认 -1.0f）
 ▼
MMIService::RedispatchInputEvent                      service/module_loader/src/mmi_service.cpp:6701
 │  · 校验链 + delegateTasks_.PostSyncTask 投递到服务主线程
 ▼
MMIService::RedispatchInputEventInner                 mmi_service.cpp:6620
 │  · 打标 + 按 sourceType 分流 → UpdateTargetPointer 寻窗 → Abandon 校验 → 分发
 ▼
EventDispatchHandler::HandleTouchEvent / HandlePointerEvent   service/event_dispatch/src/event_dispatch_handler.cpp:89/79
 │  → HandlePointerEventInner (:383) → UDS 会话定位 → DispatchPointerEventInner (:512)
 ▼
目标应用窗口（UDS socket，事件携带 EVENT_FLAG_REDISPATCH 位）
```

## 3. 服务端入口：校验链与线程模型（mmi_service.cpp:6701-6735）

按顺序执行校验，全部通过后才分发：

| 步骤 | 内容 | 失败返回 |
|---|---|---|
| 深拷贝 | `std::make_shared<PointerEvent>(pointerEvent)`，拷贝构造保留 zOrder 等全部字段（pointer_event.cpp:687） | RET_ERR |
| **sourceType 白名单** | 只接受 `SOURCE_TYPE_MOUSE`、`SOURCE_TYPE_TOUCHSCREEN`（mmi_service.cpp:6709-6714） | RET_ERR |
| 服务运行态 | `IsRunning()` | `MMISERVICE_NOT_RUNNING` |
| 系统应用校验 | `VerifySystemApp()`：TOKEN_NATIVE/TOKEN_SHELL 直接通过，HAP 需 systemApp 全量 token（permission_helper.cpp:47-62） | `ERROR_NOT_SYSAPI`（202） |
| 注入权限 | `CheckInjectPermission()`：TOKEN_SHELL 通过，否则校验 `ohos.permission.INJECT_INPUT_EVENT`（permission_helper.cpp:64-82） | `ERROR_NOT_SYSAPI`（202） |

**线程模型**：通过 `delegateTasks_.PostSyncTask` 把 `RedispatchInputEventInner` 投递到 MMI 服务的主循环（UDS epoll）线程**同步执行**——IPC 调用阻塞到事件完成寻窗与分发才返回。这保证了重分发过程与真实硬件事件在时间上互斥，是后续所有"影子状态无需加锁"设计的前提。

## 4. RedispatchInputEventInner 分发细节（mmi_service.cpp:6620-6699）

### 4.1 公共前置处理

```cpp
pointerEvent->SetTargetWindowId(-1);                       // ① 无视调用方指定的目标窗口，强制重新寻窗
pointerEvent->AddFlag(EVENT_FLAG_REDISPATCH);              // ② 打标（0x00100000，input_event.h:167）
LogTracer lt(...)                                          // ③ 链路追踪 id（mmi_log.h:32）
```

①是重要的安全性质：调用方**不能**越过寻窗逻辑直接指派窗口，一切以服务端按当前窗口树 + zOrder 约束计算的结果为准。

### 4.2 触摸屏路径（6634-6679，TOUCHSCREEN 分支）

1. **逐 Item 清零目标窗口**（6636-6639）。
2. **构造 `TouchRedispatchStore::Guard`**（6641）——RAII 激活影子状态（见 §5.2），作用域到 case 块结束。
3. **会话窗口恢复**（6642-6658）：非 DOWN/HOVER_ENTER 事件，若该手指会话活跃则从 Store 取回 DOWN 时锁定的 `windowId` 直接作为目标——**保证 MOVE/UP 锁定在 DOWN 窗口**（PR 9518 修复项）。
4. **`WIN_MGR->UpdateTargetPointer`**（6659）——完整走一次触摸寻窗流程 `UpdateTouchScreenTarget`（input_windows_manager.cpp:7613 分发，TOUCHSCREEN → :7631），期间全部状态访问被路由到影子 Store。
5. **与真实手指的正向冲突检测**（6660-6673）：DOWN/HOVER_ENTER 时查 `GetRealFingerDownWindowId`（input_windows_manager.cpp:6147，读**真实路径**的 `touchItemDownInfos_`），若真实手指已在同一窗口按下 → `DeactivateFinger` + RET_ERR（PR 9518 第二个 fix 加了 `realWindowId == targetWindowId` 的窗口比对，避免误拦截）。
6. **`AbandonTouchRedispatch`**（6674）→ 会话准入校验（见 §5.3），拒绝则 RET_ERR。
7. **`HandleTouchEvent`**（6677）进入正常分发。

### 4.3 鼠标路径（6682-6690，MOUSE 分支）

```cpp
MouseRedispatchStore::Guard guard(pointerEvent);
WIN_MGR->UpdateTargetPointer(pointerEvent);    // UpdateMouseTarget
WIN_MGR->AbandonMouseRedispatch(pointerEvent); // 目标窗口必须在 activeWindows_ 集合中
eventDispatchHandler->HandlePointerEvent(pointerEvent);
```

鼠标会话的激活点在 `UpdateMouseTarget` 内部：**只有 AXIS_BEGIN**（且 Guard 激活中）才会 `SetAxisBeginWindow` + `SetWindowActive(touchWindow->id)`（input_windows_manager.cpp:5222-5227）；AXIS 序列全程锁定在该窗口（寻窗失败时回落到记录的 axisBeginWindow，5303-5307）；AXIS_END 时 `DeactivateWindow`（mouse_redispatch_store.cpp:99-101）。

**因此当前鼠标重分发只支持 AXIS（滚轮）事件序列**——BUTTON_DOWN/MOVE/UP 没有任何激活入口，首次调用即被 Abandon 拒绝。这是扩展鼠标能力时最核心的现状约束。

### 4.4 不支持的 sourceType

其余一律 `RET_ERR`（6692-6694）。

## 5. 三大核心机制

### 5.1 zOrder 层级隔离

- 调用方通过 `PointerEvent::SetZOrder(float)` 设置一个 **> 0 的层级值**（随 IPC 序列化携带，默认 -1.0f）。
- `IsValidZorderWindow`（input_windows_manager.cpp:8556-8578）：带 REDISPATCH/SIMULATE 标记且 zOrder>0 的事件，命中测试时**只允许命中 zOrder 严格小于事件 zOrder 的窗口**；MOVE/AXIS_UPDATE 命中失败时不打错误日志。
- 生效的命中过滤点：input_windows_manager.cpp 516 / 548 / 951 / 4595 / 6242 / 7323（7323 为 PR 9830 给派生 LEVITATE 事件补的）。
- zOrder 同时是并发会话的隔离键：`TouchRedispatchStore` 内所有状态都是 `map<float /*zOrder*/, ...>`，多个调用方（不同 zOrder）的会话互不可见。

### 5.2 Guard 影子状态隔离（RAII）

`Guard` 构造时置 `active_=true` 并记录 `zOrder_`，析构时复位（touch_redispatch_store.cpp:28-40 / mouse_redispatch_store.cpp:28-40）。激活期间，`InputWindowsManager` 中 **10 个状态访问器**把原来直读成员变量的路径切换为读写影子 Store 的按-zOrder 子表（input_windows_manager.cpp:6072-6125）：

| 访问器 | 影子化的真实状态 |
|---|---|
| `TouchTargetWinIds` (:6072) | targetTouchWinIds_（多窗触点分发） |
| `TouchFirstTouchInfos` (:6078) | firstTouchWindowInfos_ |
| `TouchLockWindowInfo` (:6084) | lockWindowInfo_（DOWN 锁窗） |
| `TouchLastTouchEventOnBackGesture` (:6090) | 返回手势事件缓存 |
| `TouchLastPointerEventForWindowChangeMap` (:6096) | 窗口切换缓存 |
| `TouchLastPointerEventForGesture` (:6102) | 手势缓存 |
| `TouchDispatchEventCache` (:6108) | dispatchEventCache_ |
| `TouchLastTouchInfos` (:6114) | LastTouchInfos_（LEVITATE 判定依据） |
| `GetCancelEventList` (:6120) | cancelEventList_（event_dispatch_handler.cpp:126,143 经接口路由） |

鼠标侧对应 `MouseRedispatchStore`：`lastEvent_`（事件快照）、`axisBeginWindowMap_`（按 zOrder）、`activeWindows_`。

真实输入路径上的 `lastPointerEvent_`、`mouseDownInfo_`、LEAVE/ENTER_WINDOW 合成等**完全不被重分发污染**——重分发的 MOVE 在 `UpdatePointerEvent` 里只 `CacheLastEvent` 进影子 Store 就返回（input_windows_manager.cpp:5014-5020）。

### 5.3 会话激活模型、双向冲突消解与派生事件合成

**准入（Abandon）**，touch_redispatch_store.cpp:161-202：

| 工具 | 激活动作 | 去激活动作 | UP 之后 |
|---|---|---|---|
| 手指 | DOWN / HOVER_ENTER（input_windows_manager.cpp:6802-6805 记录 windowId+事件快照） | UP / CANCEL / HOVER_CANCEL / HOVER_EXIT（touch_redispatch_store.cpp:192-199） | 会话结束 |
| 手写笔（PEN） | **PROXIMITY_IN 自动激活**（:173-179） | **仅 PROXIMITY_OUT**（:185-190） | **保持激活**，允许后续 LEVITATE_MOVE 悬停续传 |

未激活的事件一律 `Abandon → RET_ERR` 丢弃。

**双向冲突消解**（真实事件与重分发会话互相让位）：

1. 真实手指 DOWN 与重分发会话同窗 → 给会话窗口**合成 CANCEL**（`DispatchTouchRedispatch`，input_windows_manager.cpp:3270-3314：从 Store 里的事件快照重建事件、UpdateId、`SetCanceled(true)`）并去激活会话（6456-6471）——"真实赢"。
2. 重分发 DOWN 与真实手指同窗 → 会话直接去激活并 RET_ERR（mmi_service.cpp:6660-6673）。
3. 真实鼠标 AXIS_BEGIN 落在活跃重分发窗口 → 用 `lastEvent_` 快照**合成 AXIS_END**（`DispatchPointerRedispatch`，3218-3260，保留按键状态）并去激活（5230-5243）。

### 5.4 拖拽（Pull）事件在再分发寻窗中的命中隔离

**规则**：拖拽事件（`POINTER_ACTION_PULL_DOWN / PULL_MOVE / PULL_UP / PULL_CANCEL / PULL_IN_WINDOW / PULL_OUT_WINDOW` 系列）在再分发场景中，寻窗环节**不会命中具备再分发属性的窗口**（即再分发调用方——如全局手势窗口——自身及其上层窗口），避免拖拽流被回环派发回手势窗口造成二次劫持/重复消费。

该规则由两条机制共同保证：

1. **zOrder 命中上限对 Pull 系列同样生效**：Pull 事件与普通触点一样经过 `IsValidZorderWindow` 过滤——触摸寻窗遍历（input_windows_manager.cpp:6241-6242）、鼠标寻窗遍历（:4594-4595，含 PULL_UP 选窗热区匹配 :4601-4624）。带 `EVENT_FLAG_REDISPATCH` 且 zOrder>0 的事件只允许命中 zOrder 严格小于事件 zOrder 的窗口，调用方自身（zOrder 相等）及更高层窗口直接跳过（:8556-8578）。
2. **拖拽会话绑定**：PULL_UP / PULL_MOVE 的选窗依赖拖拽会话数据（`extraData_` + `IsSameDragUser`，触摸路径 :6229-6232、鼠标路径 :4604-4606），命中域被约束在拖拽发起会话关联的窗口/热区内，不会漂移到无关窗口；`PULL_IN_WINDOW / PULL_OUT_WINDOW` 由 `HandlePullEvent`（:7663，位于 `UpdateTargetPointer` 末尾，再分发路径同样执行）基于会话生成。

**扩展约束**：鼠标 BUTTON/MOVE 会话与全量手写笔扩展新增任何寻窗/会话逻辑时，不得破坏该隔离——不得为 Pull 系列事件提供绕过 zOrder 过滤或拖拽会话绑定的命中路径。

## 6. 手写笔支持现状

手写笔以 `SOURCE_TYPE_TOUCHSCREEN + TOOL_TYPE_PEN` 进入，与触摸屏共用路径；服务自身对 tablet-tool 还有**派生事件**：悬停跨窗时由 `HandleLevitateInOutEvent`（input_windows_manager.cpp:7231）合成 `LEVITATE_IN_WINDOW/LEVITATE_OUT_WINDOW`（`DispatchTouch` 派发，:7215/:7228）。相关实现与修复：

- `IsTabletToolEvent = PEN || PENCIL`（input_windows_manager.cpp:7147-7151），目前只用于窗口输入类型判定，**未用于重分发会话键**。
- **PR 9521**（db15fe86af）：笔悬停支持——PROXIMITY_IN 自动激活 / PROXIMITY_OUT 去激活 / UP 不去激活（允许抬笔后继续悬停续传）。
- **PR 9527**（db0e512438，合入 d9978e65c6，5月16日）：`IsEnterWindowTriggered/IsLeaveWindowTriggered` 改读 `TouchLastTouchInfos()`（Guard 路由后的影子表，7153-7182）——否则重分发 LEVITATE_MOVE 会拿真实路径的 LastTouch 状态，**凭空产生 LEVITATE_IN/OUT**。
- **PR 9830**（a9eddc524，合入 2a4ac54fb6，7月24日）：`DispatchTouch` 派发循环补 `IsValidZorderWindow` 过滤（6875-6879）——派生的 LEVITATE 事件同样遵守 zOrder 层级。

## 7. 交付演进时间线

| PR | 本地提交 | 内容 |
|---|---|---|
| （前史）!8263 | 8c12811309 | KeyEvent ReDispatch（安全口令输入），提出 REDISPATCH flag 概念 |
| （前史） | 3e9cb8af1f → 5ab36ef54c | 鼠标 RedispatchInputEvent 骨架（@since 23）→ 服务端落地（仅鼠标） |
| **9471** | 0613d0d3f6 + 221074b20a（合入 42353a2570，5月8日） | 触摸屏重分发：引入 Touch/MouseRedispatchStore + Guard + 10 访问器，19 文件 +1046/-277 |
| **9518** | 84e38b7676 + 879b2e3d + 2c68382f16（合入 d3fc17a725，5月19日） | ① MOVE/UP 锁定 DOWN 窗口 ② 反向冲突检测加窗口比对防误拦 ③ DOWN 被丢弃时清理 finger 激活记录防泄漏 |
| **9521** | 38f752a065 + db15fe86af（合入 d597f84006） | 手写笔悬停：PROXIMITY_IN 自动激活 / PROXIMITY_OUT 去激活 / UP 不去激活 |
| **9527** | db0e512438（合入 d9978e65c6，5月16日） | LEVITATE_IN/OUT 误触发修复（Guard 路由 LastTouchInfos） |
| **9830** | a9eddc524（合入 2a4ac54fb6，7月24日） | DispatchTouch 派发补 zOrder 校验 |

## 8. 扩展开发要点（鼠标 + 全量手写笔）

### 8.1 能力缺口矩阵

| 能力 | 现状 | 扩展需动的地方 |
|---|---|---|
| 鼠标 AXIS 序列 | ✅ 已支持 | — |
| 鼠标 BUTTON_DOWN/UP（点击/拖拽） | ❌ 无激活入口，Abandon 直接拒 | 需为按钮会话设计激活点（类似 `mouseDownInfo_` 的影子版）+ `DispatchPointerRedispatch` 增加 BUTTON_UP 合成路径 |
| 鼠标 MOVE（悬停移动） | ❌ 同上 | 同上，且要决定是否驱动真实光标绘制（当前无窗时仍会 `DrawMovePointer`，input_windows_manager.cpp:5256-5283） |
| 手写笔 DOWN/MOVE/UP | ✅（PEN，经 PROXIMITY_IN 或 DOWN 激活） | 键冲突问题见 8.2 |
| 手写笔悬停 PROXIMITY/LEVITATE | ✅ | — |
| TOOL_TYPE_PENCIL | ❌ 按手指规则处理（UP 即去激活、无 PROXIMITY 激活） | 会话键 + Abandon 工具判定 |
| TOOL_TYPE_RUBBER/BRUSH/AIRBRUSH | ❌ | 需决策：按笔（设备级悬停会话）还是按手指（触点会话）处理 |

ToolType 枚举全集（pointer_event.h:488-530）：FINGER=0, PEN=1, RUBBER=2, BRUSH=3, PENCIL=4, AIRBRUSH=5, MOUSE=6, ...

### 8.2 最关键的结构问题：会话键缺少 toolType 维度

`redispatchFingers_` 的键是 `{zOrder, deviceId, pointerId}`（touch_redispatch_store.h:111）。**同一触控设备上手写笔和手指通常共用 pointerId 序列**，两类会话会互相覆盖（`SetFingerActive` 同键写）、冲突检测（input_windows_manager.cpp:6459-6461 遍历 FingerMap 不分 toolType）会跨工具误伤、`DeactivateFinger` 会跨工具误删。

全量手写笔适配**必须**把键扩为 `{deviceId, pointerId, toolType}`，并同步修改：

- `TouchRedispatchStore::SetFingerActive / IsFingerActive / DeactivateFinger / GetFingerEvent / GetFingerWindowId`
- `DispatchTouchRedispatch` 签名（input_windows_manager.cpp:3270）
- 真实 DOWN 反向冲突检测（input_windows_manager.cpp:6456-6471）
- `SetFingerActive` 调用点（input_windows_manager.cpp:6802-6805）
- mmi_service.cpp Inner 中的恢复/去激活调用点（6642-6673）

> 草稿分支 `wld/feat_redispatch_pen`（da4ead8ef5，2026-10-14）已做：`RedispatchFingerKey{deviceId,pointerId,toolType}`、PEN/PENCIL 归一为 `isPen`、丢弃派生的 HOVER_ENTER/HOVER_EXIT（mmi_service.cpp Inner 前置直接 RET_OK）、6 个测试文件约 1100 行 UT。
> **注意**：该草稿基于 !10053 时期代码，落后当前 master（!10091），需先 rebase；9527/9830 修复的上下文（`TouchLastTouchInfos` 路由、`IsValidZorderWindow` in DispatchTouch）与草稿改动区域相邻，rebase 时重点核对。

### 8.3 风险清单

1. **会话泄漏**：`TouchRedispatchStore::Reset()` 与 `MouseRedispatchStore::Reset()/ClearDeviceEvents()` 在生产代码中**无任何调用点**，清理只靠事件序列正常收尾。设备中途拔出（收不到 PROXIMITY_OUT/AXIS_END）会留下僵尸会话——后续同 (zOrder, key) 的事件会被放行到已失效的窗口。扩展时建议补设备热插拔清理（可挂到 `InputDeviceManager` 的 remove 回调）。
2. **派生事件双投递**：LEVITATE_IN/OUT_WINDOW 是服务在 `UpdateTouchScreenTarget` 内部合成的；客户端若直接注入这两类 action 会与派生逻辑叠加。草稿对 HOVER_ENTER/EXIT 选择了"丢弃"，建议对 LEVITATE_IN/OUT_WINDOW 明确同样策略或放行策略，形成白名单 action 清单。
3. **IPC 直达坐标/窗口不可信**：虽然 targetWindowId 被强制重置，但坐标、zOrder、pointerId、deviceId 全部来自客户端——现有防线是 zOrder 命中过滤 + INJECT 权限 + 系统应用校验，扩展鼠标按钮会话时同样不要引入"客户端指定窗口"的捷径。
4. **PostSyncTask 同步阻塞**：分发在主线程完成才返回 IPC，若扩展路径新增耗时操作（如光标绘制回调）会拖慢所有输入主流程。
5. **多点并发会话**：Guard 是全局单例开关（`active_` 非按 zOrder 计数），依赖"主线程串行"前提；扩展若引入异步（如延迟分发）会破坏该前提。
6. **UP 后笔会话常驻**（设计使然）：只有 PROXIMITY_OUT 收尾，与风险 1 叠加后尤其需要设备清理兜底。
7. **拖拽（Pull）命中隔离需保持**：Pull 系列事件在再分发寻窗中不命中具备再分发属性的窗口（见 §5.4），扩展鼠标/手写笔路径时不得引入绕过 zOrder 过滤或拖拽会话绑定的命中逻辑。

### 8.4 建议的落地顺序

1. rebase 草稿到当前 master，恢复 9527/9830 修复的兼容性；
2. 会话键加 toolType（草稿已备）+ 冲突检测/派发签名同步（草稿已备）；
3. 手写笔工具集决策（PENCIL 覆盖、RUBBER/BRUSH/AIRBRUSH 归类）；
4. 补设备级清理兜底（风险 1，两套 Store 一起）；
5. 鼠标按钮/MOVE 会话单独设计（激活点 + 合成收尾事件 + 光标行为决策），草稿目前只有回归测试保护、无功能实现；
6. 按仓内 checklist 补 UT（草稿 6 个测试文件可作为骨架），门禁关注 dayu200 编译 + part_compile。
