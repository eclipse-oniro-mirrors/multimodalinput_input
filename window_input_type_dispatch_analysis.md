# WindowInputType 各类型窗口在不同输入事件下的分发差异分析

> 基于当前代码：`foundation/multimodalinput/input`，分支 `feat_redispatch_pen`（HEAD a1bf70ed7）。
> 枚举定义：`interfaces/native/innerkits/proxy/include/window_info.h:206`。
> 本文只覆盖 mmi 仓库内的行为（本工作区无 window_manager 仓库代码，WMS/ArkUI 侧如何设置这些类型不在范围内）。

---

## 1. 结论速览

mmi 内 `windowInputType` 通过**两层机制**生效：

1. **服务端直检**：选窗时调用 `InputWindowsManager::HandleWindowInputType`（`service/window_manager/src/input_windows_manager.cpp:8640`）。返回 `true` 表示"特殊窗口"：该窗口被记入**多窗目标集合**并继续向下找窗口，最终由 `EventDispatchHandler::HandleMultiWindowPointerEvent`（`event_dispatch_handler.cpp:212`）把事件**拷贝分发给集合内所有窗口**（本文记为"双发"）；返回 `false` 则作为普通窗口，单窗收事件。
2. **客户端合成 Flag**：sceneboard 进程侧打包窗口信息时，`InputManagerImpl::WindowInputTypeToFlag`（`frameworks/proxy/event_handler/src/input_manager_impl.cpp:3587`）把部分枚举折算成 `WindowInputPolicy::FLAG_*` 合入 `flags` 字段，随 `WINDOW_INFO` 报文发给 mmi 服务（`InputManagerImpl::SendWindowInfo` `input_manager_impl.cpp:2063` → `server_msg_handler.cpp:1055` 解包）。服务端按 flag 在选窗/分发时另行过滤。

按当前代码，各枚举分类如下：

| 枚举 | 值 | HandleWindowInputType | 合成 Flag（WindowInputTypeToFlag） | mmi 内净效果 |
|---|---|---|---|---|
| NORMAL | 0 | `false`（普通窗） | 无 | 普通单窗分发 |
| TRANSMIT_ALL | 1 | `true`（恒定） | `EVENT_TRANSMIT_ALL`、`DRAG_DISABLED` | 所有指针动作双发（自身+下层第一个普通窗） |
| TRANSMIT_EXCEPT_MOVE | 2 | 仅 `MOVE`/`PULL_MOVE` 为 `true` | 无 | 仅鼠标悬停移动穿透；触摸侧实际等价 NORMAL |
| ANTI_MISTAKE_TOUCH | 3 | `false` | 无 | **当前 mmi 内与 NORMAL 完全一致** |
| TRANSMIT_AXIS_MOVE | 4 | `false` | 无 | **同上，等价 NORMAL** |
| TRANSMIT_MOUSE_MOVE | 5 | `false` | 无 | **同上，等价 NORMAL** |
| TRANSMIT_LEFT_RIGHT | 6 | `false` | 无 | **同上，等价 NORMAL** |
| TRANSMIT_BUTTOM | 7 | `false` | 无 | **同上，等价 NORMAL** |
| MIX_LEFT_RIGHT_ANTI_AXIS_MOVE | 18 | `false` | `MOUSE_LEFT_BUTTON_LOCK`、`STYLUS_ANTI_MISTAKE`、`DRAG_DISABLED` | 鼠标仅左键可命中；笔悬停穿透；笔防误触开关开时穿透；拖拽跳过；固定坐标豁免；锁屏补发 CANCEL |
| MIX_BUTTOM_ANTI_AXIS_MOVE | 19 | `false` | `MOUSE_LEFT_BUTTON_LOCK`、`STYLUS_ANTI_MISTAKE` | 同 18，但不跳过拖拽 |
| SLID_TOUCH_WINDOW | 40 | `false`（走 default） | 无 | 单手模式下滑动不锁定 DOWN 窗口，按位置重新选窗 |
| TRANSMIT_ANTI_AXIS_MOVE | 50 | `false`（走 default） | `MOUSE_UNHITTABLE`、`STYLUS_ANTI_MISTAKE`、`DRAG_DISABLED` | 鼠标完全不可命中（全部动作穿透）；笔防误触开关开时穿透；手指正常 |
| DUALTRIGGER_TOUCH | 60 | `true`（恒定） | `MOUSE_LEFT_BUTTON_LOCK`、`STYLUS_ANTI_MISTAKE`、`EVENT_TRANSMIT_ALL`、`DRAG_DISABLED` | 双触发（=TRANSMIT_ALL 的双发）+ 鼠标仅左键 + 笔防误触 |

> 关键差异一句话：**TRANSMIT_ALL / DUALTRIGGER_TOUCH 走"双发"，MIX_* / TRANSMIT_ANTI_AXIS_MOVE 走"Flag 过滤"（跳过=穿透），TRANSMIT_EXCEPT_MOVE 是唯一的"按动作区分"，其余 5 个（3/4/5/6/7）在 mmi 内不产生任何差异。**

---

## 2. 目标窗口选择与多窗分发机制

### 2.1 分发入口（按输入源分流）

`InputWindowsManager::UpdateTargetPointer`（`input_windows_manager.cpp:7673`）：

| 输入源 | 路径 | 选窗函数 |
|---|---|---|
| `SOURCE_TYPE_TOUCHSCREEN`（手指 + 手写笔/平板工具） | `OHOS_BUILD_ENABLE_TOUCH` | `UpdateTouchScreenTarget`（:6218） |
| `SOURCE_TYPE_MOUSE` | `OHOS_BUILD_ENABLE_POINTER` | `UpdateMouseTarget`（:5186）→ `SelectWindowInfo`（:4544） |
| `SOURCE_TYPE_TOUCHPAD` | `OHOS_BUILD_ENABLE_POINTER` | `UpdateTouchPadTarget`（:7499），复用鼠标路径 |

手写笔没有独立路径：以 `SOURCE_TYPE_TOUCHSCREEN` + `TOOL_TYPE_PEN/PENCIL` 走触摸路径，差异点见第 5.3 节。

### 2.2 HandleWindowInputType 与"特殊窗口"

```cpp
// input_windows_manager.cpp:8640（节选）
WindowInputType windowTypeTemp = window.windowInputType;
if (sourceType == PointerEvent::SOURCE_TYPE_TOUCHSCREEN) {
    GetActiveWindowTypeById(window.displayId, window.id, windowTypeTemp);   // 手势期类型快照
}
switch (windowTypeTemp) {
    case NORMAL: return false;
    case TRANSMIT_ALL: case DUALTRIGGER_TOUCH: return true;
    case TRANSMIT_EXCEPT_MOVE:
        return (action == MOVE || action == PULL_MOVE);
    // ANTI_MISTAKE_TOUCH / TRANSMIT_AXIS_MOVE / TRANSMIT_MOUSE_MOVE /
    // TRANSMIT_LEFT_RIGHT / TRANSMIT_BUTTOM / MIX_* 一律 return false
    default: return false;   // 含 SLID_TOUCH_WINDOW=40、TRANSMIT_ANTI_AXIS_MOVE=50
}
```

**触摸屏类型快照**：对触摸屏事件，判定用的类型先经 `GetActiveWindowTypeById`（:9894）用 `activeTouchWinTypes_` 覆盖。`AddActiveWindow`（:9904）在每次命中窗口时记录 `{displayId, windowId} → (当时的类型, pointerId 集合)`；`RemoveActiveWindow`（:9923）在 `UP/PULL_UP/CANCEL/PULL_THROW` 时移除指针、集合空则删除；单指 `DOWN` 时 `ClearActiveWindow`（:9949）。效果：**一次触摸手势期间，窗口参与判定的是手势开始时的类型快照**，WMS 中途变更类型不影响本次手势。

### 2.3 多窗目标集合与"双发"

两条选窗路径的套路一致（以 `UpdateTouchScreenTarget` :6390-6417 为例，鼠标对应 `SelectWindowInfo` :4648-4666）：

1. 命中窗口且 `HandleWindowInputType == true`（特殊窗）→ `AddTargetWindowIds`（:8735）记入目标集合，`isHotArea = true`，**继续向下找**；
2. 命中第一个普通窗 → 设为最终目标 `touchWindow` 并 break。触摸路径由 `CheckUIExtentionWindowDefaultHotArea`（:7128）末尾 `if (isHotArea)` 把最终窗口也补入集合（:7157）；鼠标路径由 `else if (isHotArea)` 分支补入（:4659）。

实际分发在 `EventDispatchHandler::HandlePointerEventInner`（`event_dispatch_handler.cpp:383`）：

- 目标集合非空 → `HandleMultiWindowPointerEvent`（:212）对集合内**每个窗口拷贝一份事件**逐个分发（`ReissueEvent` :140 保证 DOWN/UP 配对并维护 `cancelList`；对带 `FLAG_EVENT_TRANSMIT_ALL` 的窗口，多窗时 `PULL_UP` 降级为 `CANCEL`，:238）；
- 集合为空 → 单窗直发。

目标集合生命周期：触摸 `DOWN` 时 `ClearTargetWindowId` 后重建，`UP/PULL_UP/CANCEL/PULL_THROW` 后清理；鼠标在无按键状态分发后 `ClearMouseTargetWindowId`。拖拽过程中 `UpdateTargetTouchWinIds`（:7060）/`ClearMismatchTypeWinIds`（:7078）只保留带 `FLAG_EVENT_TRANSMIT_ALL` 的窗口并追加当前窗口。

### 2.4 先于 windowInputType 的"会话锁定"

| 会话 | 锁定时机 | 后续行为 |
|---|---|---|
| 鼠标按键会话 | `BUTTON_DOWN` → `mouseDownInfo_` | 按键拖动 `MOVE`/`BUTTON_UP` 沿用 down 窗口，不重新选窗 |
| 鼠标滚轮会话 | `AXIS_BEGIN` → `axisBeginWindowInfo_` | `AXIS_UPDATE/END` 只发给 axisBegin 锁定窗口（`SelectWindowInfo` :4695-4703，集合不参与） |
| 触摸落点会话 | `DOWN` 记录 `TouchItemDownInfos` 与 `TouchLockWindowInfo`（:6499） | `MOVE` 跟随 `pointerItem` 携带的 targetWindowId（:6373），不重新选窗 |

因此**"特殊窗判定"在触摸路径上实际只在 `DOWN`（及悬停进入）时发生**；鼠标路径在悬停移动、`BUTTON_DOWN`、`AXIS_BEGIN` 等重选时机发生。

两个例外会绕过落点会话按位置重选：

- 手写笔压力 = 0 的移动（悬停）：`targetWindowId >= 0 && action != DOWN && (toolType != PEN || pressure > 0)`（:6373-6375）不满足，按位置重选；
- 无障碍悬停跟踪：`NeedTouchTracking`（:9842）时 targetWindowId 取 `GLOBAL_WINDOW_ID`，窗口切换时由 `ProcessTouchTracking`（:9853）合成 `HOVER_CANCEL/HOVER_ENTER`。

### 2.5 服务端直接消费的策略 Flag

| Flag | 生效位置 | 行为 |
|---|---|---|
| `FLAG_UNTOUCHABLE` | 两条选窗路径 | 直接跳过（任何输入源） |
| `FLAG_MOUSE_UNHITTABLE` | `SelectWindowInfo` :4587 | 鼠标事件一律跳过该窗口（对鼠标不可见） |
| `FLAG_MOUSE_LEFT_BUTTON_LOCK` | `SelectWindowInfo` :4629 | 未锁窗时，悬停移动/滚轮 `AXIS_*`/`PULL_UP`/非左键 → 跳过；左键按下/拖动可命中。例外：触摸板滚动轴（`IsTouchPadScrollAxis` :10243，需窗口带 `FLAG_TOUCHPAD_AXIS_SCROLL_REDISPATCH`）不穿透 |
| `FLAG_STYLUS_ANTI_MISTAKE` | `SkipNavigationWindow` :5754 | 手写笔事件 + 防误触开关开启（settings `NAVIGATION_SWITCH_NAME`）→ 跳过 |
| `FLAG_EVENT_TRANSMIT_ALL` | `HandleMultiWindowPointerEvent` :238、`UpdateTargetTouchWinIds` :7060/:7092 | 多窗拖拽结束 `PULL_UP→CANCEL`；拖拽目标集合仅保留此类窗口 |
| `FLAG_DRAG_DISABLED` | 触摸拖拽选窗 :6337、`DispatchTouch` :7390 | 拖拽/PULL 选窗跳过 |
| `FLAG_HANDWRITING` | `SkipAnnotationWindow` :5748 | 手指事件跳过（批注/手写窗只收笔） |

另有与窗口 flag 无关但影响触摸路径的过滤：`SkipPrivacyProtectionWindow`（:5725，投播隐私保护）、`FLAG_FIRST_TOUCH_HIT`（首触锁定，`ProcessOtherTouchHit` :6897 把后续手指置 CANCEL）。

---

## 3. 逐类型详解

### 3.1 NORMAL (0)

所有输入源、所有动作：普通命中，单窗分发。无附加行为。

### 3.2 TRANSMIT_ALL (1)

- `HandleWindowInputType` 恒 `true`：**任何指针事件、任何源、任何动作**命中时都双发（自身 + 继续向下找到的第一个普通窗；若下方全是特殊窗/无窗口，则自身作为最终目标）。
- Flag 附加：
  - `EVENT_TRANSMIT_ALL`：多窗时拖拽结束 `PULL_UP→CANCEL`；拖拽集合仅保留此类窗口；
  - `DRAG_DISABLED`：拖拽/PULL 选窗跳过（:6337、:7390）。
- 鼠标悬停移动、滚轮、右键等**同样双发**（未合成左键锁/笔防误触 flag）。
- 滚轮序列：`AXIS_BEGIN` 双发，但 `AXIS_UPDATE/END` 受 axisBegin 会话锁定，只发 AXIS_BEGIN 时选中的下层普通窗。
- 调试：`PrintHighZorder`（:3401，`OHOS_BUILD_PC_PRIORITY`）将其排除在高 zOrder 覆盖告警之外。

### 3.3 TRANSMIT_EXCEPT_MOVE (2)

- 仅当 `pointerAction == MOVE || PULL_MOVE` 时 `HandleWindowInputType` 返回 `true`。
- **鼠标**：悬停移动（无按键 `MOVE`）每次重选窗 → 该窗口不拦截悬停移动，自身与下层窗口双发；`DOWN/UP/BUTTON_*/AXIS_*` 一律普通单窗；按住按键的移动受按键会话锁定不重选，无穿透效果。
- **触摸/笔**：`DOWN` 后的 `MOVE` 恒定跟随落点窗口（不重选、不走特殊窗分支），笔悬停用 `LEVITATE_MOVE` 动作也不满足条件 → **触摸路径上该类型与 NORMAL 无实际差异**。
- 无 flag 合成。

### 3.4 ANTI_MISTAKE_TOUCH (3) / TRANSMIT_AXIS_MOVE (4) / TRANSMIT_MOUSE_MOVE (5) / TRANSMIT_LEFT_RIGHT (6) / TRANSMIT_BUTTOM (7)

- `HandleWindowInputType` 中显式 `return false`；`WindowInputTypeToFlag` 落入 `default`（仅打日志 `"windowInputType not find"`）。
- **当前 mmi 代码内，所有输入源、所有动作下与 NORMAL 完全一致**，无任何特殊分支引用这些值。
- 从命名看应为由 WMS/应用侧消费的类型；本工作区不含 window_manager 仓库，无法验证其上层语义。

### 3.5 MIX_LEFT_RIGHT_ANTI_AXIS_MOVE (18)

直检点（不依赖 flag）：

- **笔悬停穿透**：`ShouldIgnoreLevitateEvent`（:7192）——`TOOL_TYPE_PEN` 且 `pressure <= 0` 时选窗直接跳过；
- 笔未按下的 `LEVITATE_IN_WINDOW / PULL_IN_WINDOW` 选窗跳过（`DispatchTouch` :7395）；
- `DOWN/MOVE` 缓存到 `TouchLastTouchEventOnBackGesture`（:6674-6679），锁屏时 `SendCancelEventWhenLock`（:8838）为进行中的手势补发 `CANCEL`（返回手势保护）；
- **固定坐标豁免**：`FixedMode::AUTO` 下不改写 DX/DY（`EventDispatchHandler::UpdateDisplayXY` :470、`DispatchTouch` :7432）；
- 调试/辅助：`PrintWindowNavbar`（:2348）dump、`PrintHighZorder` 排除、`IsValidNavigationWindow`（:5793）识别为导航窗。

Flag 效果（`MOUSE_LEFT_BUTTON_LOCK + STYLUS_ANTI_MISTAKE + DRAG_DISABLED`）：

| 输入 | 行为 |
|---|---|
| 鼠标悬停/滚轮/右中键 | 穿透（左键锁跳过；触摸板滚动轴例外，见 2.5） |
| 鼠标左键 DOWN/拖动 | 可命中；命中后 `HandleWindowInputType=false` → **单窗**（无双发，这是与 DUALTRIGGER 的核心差异） |
| 手写笔 | 防误触开关开 → 穿透；关 → 单窗命中 |
| 手指 | 单窗命中 |
| 拖拽/PULL | `DRAG_DISABLED` 跳过 |

### 3.6 MIX_BUTTOM_ANTI_AXIS_MOVE (19)

与 3.5 完全一致，唯一差异：**无 `FLAG_DRAG_DISABLED`**——拖拽/PULL 场景可正常命中。

### 3.7 SLID_TOUCH_WINDOW (40)

- `HandleWindowInputType` 走 `default → false`；无 flag 合成。
- 仅 `OHOS_BUILD_ENABLE_ONE_HAND_MODE` 下有效：`DOWN` 时 `TouchLockWindowInfo` 记录落点窗口（:6499）；后续单指滑动（`isSlidTouch`：FINGER + 触摸屏 + 单点 + `FixedMode::AUTO`，或 `PULL_UP`）且锁窗为该类型时，**不走 targetWindowId 跟随，而是按当前位置重新命中**（:6356-6366），并以 `UpdateTargetTouchWinIds` 维护集合；滑动数据走 `TouchEnterLeaveEvent` 发跨窗 `ENTER/LEAVE`（:6720 附近）。
- 其余动作、非单手模式：与 NORMAL 相同。

### 3.8 TRANSMIT_ANTI_AXIS_MOVE (50)

- `HandleWindowInputType` 走 `default → false`（**无双发**）。
- Flag：`MOUSE_UNHITTABLE + STYLUS_ANTI_MISTAKE + DRAG_DISABLED`：

| 输入 | 行为 |
|---|---|
| 鼠标（全部动作，含左键） | `SelectWindowInfo` :4587 一律跳过——该窗口对鼠标**完全不可见**，全部穿透 |
| 手写笔 | 防误触开关开 → 穿透；关 → 单窗命中 |
| 手指 | 单窗命中 |
| 拖拽/PULL | 跳过 |

### 3.9 DUALTRIGGER_TOUCH (60)

- `HandleWindowInputType` 恒 `true`：所有命中**双发**（自身 + 下层第一个普通窗）——即"双触发"。
- Flag：`MOUSE_LEFT_BUTTON_LOCK + STYLUS_ANTI_MISTAKE + EVENT_TRANSMIT_ALL + DRAG_DISABLED`：

| 输入 | 行为 |
|---|---|
| 鼠标悬停/滚轮/右中键 | 穿透（左键锁在 `HandleWindowInputType` 之前生效，根本不进目标集合） |
| 鼠标左键 DOWN/拖动 | 可命中且**双发**（导航条与应用同时响应） |
| 手写笔 | 防误触开关开 → 穿透；关 → 命中且双发 |
| 手指 | 命中且双发 |
| 拖拽/PULL | `DRAG_DISABLED` 跳过；多窗时 `PULL_UP→CANCEL` |

- 固定坐标豁免、`PrintWindowNavbar`、`PrintHighZorder`、`IsValidNavigationWindow` 与 MIX_* 相同。
- 附：命中特殊窗置位的 `isFirstSpecialWindow` 还参与 Anco 窗口接管判断（`IsShouldSendToAnco`，:6578）。

---

## 4. 输入源 × 动作 × 窗口类型 行为矩阵

图例：
- **单** = 该窗口作为唯一目标（普通分发）
- **双** = 该窗口与其下方第一个普通窗口同时收到（多窗分发）
- **穿** = 该窗口被跳过，只有下方窗口收到
- `→锁定` = 该动作本身不重选窗，沿用此前会话锁定窗口（与窗口类型无关）

### 4.1 鼠标（`SOURCE_TYPE_MOUSE` → `SelectWindowInfo`）

| 窗口类型 | 悬停 MOVE（无按键） | 左键 DOWN | 左键拖动 MOVE / UP | 右/中键 | 滚轮 AXIS_BEGIN | AXIS_UPDATE / END |
|---|---|---|---|---|---|---|
| NORMAL | 单 | 单 | →锁定 | 单 | 单 | →锁定 |
| TRANSMIT_ALL | 双 | 双 | 双 | 双 | 双 | →锁定 |
| TRANSMIT_EXCEPT_MOVE | **双** | 单 | →锁定 | 单 | 单 | →锁定 |
| MIX_18 / MIX_19 | 穿 | 单 | →锁定（单） | 穿 | 穿 | 穿 |
| TRANSMIT_ANTI_AXIS_MOVE | 穿 | 穿 | 穿 | 穿 | 穿 | 穿 |
| DUALTRIGGER_TOUCH | 穿 | **双** | **双** | 穿 | 穿 | 穿 |
| 3 / 4 / 5 / 6 / 7 / 40 | 单 | 单 | →锁定 | 单 | 单 | →锁定 |

> `AXIS_UPDATE/END` 恒定只发 AXIS_BEGIN 锁定的窗口，与类型无关；MIX_18/19 与 DUALTRIGGER 的区别就在左键命中后是否双发。

### 4.2 触摸屏手指（`SOURCE_TYPE_TOUCHSCREEN` + `TOOL_TYPE_FINGER`）

| 窗口类型 | DOWN | MOVE | UP / CANCEL | 拖拽 PULL（extraData） |
|---|---|---|---|---|
| NORMAL | 单 | →锁定 | 单 | 单 |
| TRANSMIT_ALL | **双** | **双** | **双** | 跳过；多窗时 PULL_UP→CANCEL |
| TRANSMIT_EXCEPT_MOVE | 单 | →锁定（无差异） | 单 | 单 |
| MIX_18 | 单 | →锁定 | 单 | 跳过 |
| MIX_19 | 单 | →锁定 | 单 | 单 |
| SLID_TOUCH_WINDOW | 单（记入锁窗） | 单手模式：按位置重选；其余：→锁定 | 单 | 单 |
| TRANSMIT_ANTI_AXIS_MOVE | 单 | →锁定 | 单 | 跳过 |
| DUALTRIGGER_TOUCH | **双** | **双** | **双** | 跳过；多窗时 PULL_UP→CANCEL |
| 3 / 4 / 5 / 6 / 7 | 单 | →锁定 | 单 | 单 |

> 手指通用过滤（与 windowInputType 无关）：`FLAG_UNTOUCHABLE`、投播隐私保护、`FLAG_HANDWRITING` 批注窗跳过手指、`FLAG_FIRST_TOUCH_HIT` 首触后其余手指置 CANCEL。

### 4.3 手写笔（`TOOL_TYPE_PEN/PENCIL`，走触摸路径）

| 窗口类型 | 落笔 DOWN（压力>0） | 移动（压力>0） | 悬停（压力=0，LEVITATE_* / PROXIMITY_*） | 防误触开关开启时 |
|---|---|---|---|---|
| NORMAL | 单 | →锁定 | 单（按位置重选） | 单 |
| TRANSMIT_ALL | 双 | 双 | 双 | 双 |
| TRANSMIT_EXCEPT_MOVE | 单 | →锁定 | 单（LEVITATE_MOVE 不满足条件） | 单 |
| MIX_18 / MIX_19 | 单 | →锁定 | **穿**（:7192） | **穿**（STYLUS_ANTI_MISTAKE） |
| TRANSMIT_ANTI_AXIS_MOVE | 单 | →锁定 | 单 | **穿** |
| DUALTRIGGER_TOUCH | 双 | 双 | 双 | **穿** |
| 3 / 4 / 5 / 6 / 7 / 40 | 单 | →锁定 | 单 | 单 |

> 笔的悬停进出窗口由 `HandleLevitateInOutEvent`（:7286）在窗口切换时合成 `LEVITATE_IN/OUT_WINDOW`；`EVENT_FLAG_REDISPATCH` 事件跳过该合成。笔悬停不跟随落点会话，每个事件按位置重选。

---

## 5. 补充说明

1. **等价 NORMAL 的 5 个类型**（3/4/5/6/7）：`HandleWindowInputType` 显式 `return false` + `WindowInputTypeToFlag` 落入 `default`，mmi 全链路（含测试外的生产代码）无任何分支引用。若设备上观察到差异，效果只能来自 window_manager/ArkUI 侧（不在本仓库）。
2. **编译宏条件**：`SelectWindowInfo`/`UpdateMouseTarget` 在 `OHOS_BUILD_ENABLE_POINTER` 内；`UpdateTouchScreenTarget` 在 `OHOS_BUILD_ENABLE_TOUCH` 内；`SLID_TOUCH_WINDOW` 与固定坐标 DX 豁免在 `OHOS_BUILD_ENABLE_ONE_HAND_MODE` 内；`PrintHighZorder` 在 `OHOS_BUILD_PC_PRIORITY` 内。
3. **与 redispatch 特性的关系**（本分支）：`EVENT_FLAG_REDISPATCH`/`EVENT_FLAG_SIMULATE` 事件在 `IsValidZorderWindow`（:8616）中按事件 zOrder 过滤高于目标的窗口；`HandleWindowInputType` 的特殊窗判定对 redispatch 事件同样生效，`mouseRedispatchStore_`/`touchRedispatchStore_` 会话（按 zOrder）独立于上述窗口类型逻辑。
4. 文中行号基于 `feat_redispatch_pen` 分支 HEAD（a1bf70ed7），后续改动可能漂移，以函数名定位为准。
