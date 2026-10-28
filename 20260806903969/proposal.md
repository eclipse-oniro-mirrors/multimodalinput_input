---
target_release: "OpenHarmony-7.1-Release"
change_type: "enhancement"
issue: "20260806903969"
author: "AI-assisted"
date: "2026-09-08"
status: "Draft"
feature_id: "FEAT-30094739"
rr_id: "RR-30094499"
---

# 需求文档

## 需求输入

### 基本信息

| 字段 | 内容 |
|------|------|
| 需求ID | 20260806903969 |
| SR 编号 | 20260806903969 |
| 需求名称 | 支持鼠标和触控笔输入事件再分发 |
| RR 单号 | RR-30094499 |
| 关联 Feature | FEAT-30094739 |
| 来源 | 生态诉求（全局手势业务方） |
| 提出人 | 全局手势业务方 |
| 子系统&模块 | 多模输入 / 事件分发（service/module_loader、service/event_dispatch）、窗口管理（service/window_manager） |
| 目标发行版本 | OpenHarmony-7.1-Release（引用 proposal.target_release） |
| 优先级 | P1 |
| 复杂度 | 标准 |

## 背景与问题

### US-1: 全局手势窗口归还未消费的触控笔事件
- **角色**：全局手势窗口（ArkUI 手势业务调用方）
- **业务触发**：手势窗口接收触控笔输入事件后未形成手势，需将事件归还给下层窗口
- **业务价值**：下层窗口接收完整触控笔事件序列，交互不中断，用户体验完整

### US-2: 全局手势窗口归还未消费的鼠标事件
- **角色**：全局手势窗口（ArkUI 手势业务调用方）
- **业务触发**：手势窗口接收鼠标输入事件后未形成手势，需将事件归还给下层窗口
- **业务价值**：下层窗口接收完整鼠标事件序列，交互不中断

全局手势窗口处于 Z 序顶层，接收输入事件后若不形成手势，需将事件归还给下层窗口。现有 `RedispatchInputEvent` 内部接口已支持鼠标（`SOURCE_TYPE_MOUSE`）和触屏（`SOURCE_TYPE_TOUCHSCREEN`）事件再分发，但触控笔（`TOOL_TYPE_PEN` / `TOOL_TYPE_PENCIL`）的特有事件（接近、远离、悬停移动）再分发缺失，且衍生事件（进入窗口 `POINTER_ACTION_LEVITATE_IN_WINDOW` / 离开窗口 `POINTER_ACTION_LEVITATE_OUT_WINDOW`）丢弃逻辑未覆盖触控笔场景。当前 `TouchRedispatchStore` 以 `(deviceId, pointerId)` 为 key 管理活跃指针状态，未区分工具类型，触控笔与手指事件状态可能混淆，导致下层窗口收到错误的事件序列。

## 初始分级判断

| 判断项 | 结果 | 依据 |
|--------|------|------|
| 变更类型 | enhancement | 在现有再分发框架上扩展触控笔支持，非全新功能 |
| 复杂度 | 标准 | 单仓改动，6 个文件，3 人月，无跨仓依赖 |
| 涉及仓数量 | 1 | multimodalinput_input |
| 是否涉及 Public/System API | 否 | 使用现有内部接口 RedispatchInputEvent（InnerAPI） |
| 是否涉及安全/性能关键路径 | 否 | 再分发走独立标记路径（EVENT_FLAG_REDISPATCH），不经过高频分发主链路 |
| 是否跨 SIG | 否 | 单仓内完成，无跨 SIG 协作 |

## 目标

- 在 `RedispatchInputEventInner` 触屏分支中增加触控笔工具类型识别，支持触控笔接近、远离、按下、移动、抬起、悬停移动事件再分发
- `TouchRedispatchStore` 按工具类型隔离触控笔和手指的活跃状态，避免互相干扰
- 再分发时丢弃进入/离开窗口衍生事件（`POINTER_ACTION_LEVITATE_IN_WINDOW` / `POINTER_ACTION_LEVITATE_OUT_WINDOW`），不论工具类型
- 再分发事件从职责链末端分发节点（EventDispatchHandler）进入，中间监听/拦截环节不感知
- 鼠标再分发现有行为保持不变
- 触屏手指事件再分发现有行为保持不变

## 非目标

- 不改变手势窗口的事件消费逻辑（由 ArkUI 侧负责）
- 不新增公开 API 或 System API（使用现有内部接口）
- 不支持键盘事件再分发
- 不改变触屏手指事件再分发现有行为
- 不改变鼠标再分发现有行为

## Agent Scope Guard

| 维度 | 范围/限制 | 需人工确认的触发条件 |
|------|-----------|----------------------|
| 允许仓库 | multimodalinput_input | 需修改其他仓时 |
| 允许模块/目录 | service/module_loader/、service/window_manager/、service/event_dispatch/ | 需修改上述目录外文件时 |
| 禁止修改项 | ArkUI 手势消费逻辑、公开 API 声明文件（.d.ts）、鼠标再分发核心逻辑、触屏手指再分发核心逻辑 | 需修改鼠标/触屏手指再分发行为时 |
| 外部依赖/网络访问 | 无 | 需引入新依赖时 |

## 成功标准

| 标准 | 可观察指标 | 验证方式 |
|------|-----------|---------|
| 触控笔再分发事件完整 | 下层窗口收到完整的触控笔接近/按下/移动/抬起/远离/悬停移动事件序列 | 板侧功能测试 |
| 衍生事件丢弃 | 下层窗口不收到 LEVITATE_IN_WINDOW / LEVITATE_OUT_WINDOW 再分发事件 | 单元测试 |
| 中间环节不感知 | 监听/拦截回调未收到再分发事件 | 单元测试 + 板侧验证 |
| 现有行为不变 | 鼠标和触屏手指再分发行为无变化 | 回归测试 |
| 权限校验生效 | 无注入权限调用返回权限错误 | 单元测试 |

## 影响范围

| 子系统 | 仓库 | 模块/路径 | 影响类型 |
|--------|------|-----------|---------|
| 多模输入 | multimodalinput_input | service/module_loader/src/mmi_service.cpp | 修改：RedispatchInputEventInner 触屏分支增加触控笔识别与衍生事件丢弃 |
| 多模输入 | multimodalinput_input | service/window_manager/include/touch_redispatch_store.h | 修改：按工具类型隔离活跃状态 |
| 多模输入 | multimodalinput_input | service/window_manager/src/touch_redispatch_store.cpp | 修改：按工具类型隔离活跃状态实现 |
| 多模输入 | multimodalinput_input | service/window_manager/include/input_windows_manager.h | 修改：新增触控笔再分发目标选择方法声明 |
| 多模输入 | multimodalinput_input | service/window_manager/src/input_windows_manager.cpp | 修改：触控笔再分发目标选择与 Z 序传递 |
| 多模输入 | multimodalinput_input | service/event_dispatch/ | 修改：分发路径中对再分发衍生事件的过滤判断 |

## 假设与开放问题

### 假设
- 触控笔事件走 `SOURCE_TYPE_TOUCHSCREEN` 分支，复用触屏再分发框架
- 衍生事件 `POINTER_ACTION_LEVITATE_IN_WINDOW` / `POINTER_ACTION_LEVITATE_OUT_WINDOW`（进入/离开窗口）在再分发路径统一丢弃，不论工具类型
- `TouchRedispatchStore` 中触控笔活跃状态与手指隔离，以 `(deviceId, pointerId, toolType)` 为 key
- 再分发事件由 `RedispatchInputEventInner` 从职责链末端节点（EventDispatchHandler）进入分发，中间监听/拦截结构性不感知；`EVENT_FLAG_REDISPATCH` 标记用于 `InputWindowsManager` 目标选择与 Z 序过滤
- 权限校验使用现有注入权限 `ohos.permission.INJECT_INPUT_EVENT`，不新增权限

### 开放问题
- 触控笔与手指状态隔离的精确 key 设计待代码设计阶段确认（遗留-1，来自 03-arch-decision-record.md §6，已在 design.md DD-3 中确认）
- 衍生事件丢弃在 `RedispatchInputEventInner` 入口还是 `EventDispatchHandler` 分发路径中执行，待设计阶段确认（已在 design.md DD-2 中确认：入口处丢弃）

## 资源开销审视

| 维度 | 状态 | 信号/依据 | 确认人/理由/范围 |
|------|------|-----------|-----------------|
| 性能 | not-applicable | 再分发走独立标记路径，仅过滤检查，无全量扫描，不影响正常输入事件分发性能 | AI-assisted / 再分发仅在业务方调用时触发，非高频路径 |
| 内存(RAM) | not-applicable | TouchRedispatchStore 新增触控笔活跃状态条目，数量与活跃触控笔指针数成正比，单设备同时活跃触控笔数量极少（≤1） | AI-assisted / 增量可忽略 |
| 存储(ROM) | not-applicable | 无持久化数据变更，代码增量 ≤500 行 | AI-assisted / 仅运行时逻辑 |

## 不涉及项确认

| 维度 | 是否涉及 | 依据 |
|------|---------|------|
| 性能 | 否 | 再分发走独立标记路径，无高频路径影响（资源开销审视性能=not-applicable） |
| 安全/权限 | 是 | 注入权限校验（现有能力，本次不新增权限，仅验证现有校验覆盖触控笔场景） |
| 兼容性 | 否 | 使用运行时分支，不影响现有鼠标和触屏手指再分发行为 |
| API/SDK | 否 | 使用现有内部接口 RedispatchInputEvent，不新增公开 API |
| IPC/跨进程 | 否 | 不新增跨进程调用，RedispatchInputEvent 为同进程内部接口 |
| 构建/组件 | 否 | 不新增编译目标或组件依赖 |
| 国际化/无障碍 | 否 | 无 UI 变更，无文案变更 |
| 数据迁移 | 否 | 无持久化数据变更 |

## 1+8设备差异规格

本需求不涉及1+8设备差异规格。适用设备：平板、PC（主要目标，支持触控笔和鼠标输入）；手机兼容但不作为主要验证目标。

## DFX设计

本需求不涉及DFX设计。

## API 设计属性

### API 范围声明

| 属性 | 值 | 说明 |
|------|-----|------|
| **涉及 Kit** | 不涉及 | 使用现有内部接口 RedispatchInputEvent，不新增 Kit |
| **API 类型** | 不涉及 | 不新增 Public 或 System API |
| **编程语言** | 不涉及 | 不新增 API 声明文件 |
| **ArkTS 静态类型 API** | 不涉及 | 不新增 ArkTS API |

本次变更不涉及 API/SDK 设计。理由：使用现有内部接口 `RedispatchInputEvent`（InnerAPI），仅在 C++ 实现层扩展触控笔事件处理逻辑，不新增或修改任何公开 API 声明文件（.d.ts / .h 对外接口）。

### API 使用场景探索（可选）

**开发者场景**：
- 场景 1：全局手势业务方通过 `RedispatchInputEvent` 归还未消费的触控笔事件，指定 Z 序，系统按 Z 序重新选择目标窗口分发
- 场景 2：全局手势业务方通过 `RedispatchInputEvent` 归还未消费的鼠标事件，指定 Z 序，系统按 Z 序重新选择目标窗口分发

**用户画像**：
- 画像 1：系统应用开发者（全局手势业务方），使用 C++ 内部接口，需注入权限 `ohos.permission.INJECT_INPUT_EVENT`，关注事件归还的完整性和 Z 序正确性

## 依据与上下文引用

| 来源 | 内容 | 基线采用的结论 |
|------|------|----------------|
| 01-requirement.md | 原始诉求（RR-30094499，全局手势业务方） | 鼠标/触控笔未消费事件再分发、丢弃衍生事件、中间监听/拦截不感知 |
| 02-feasibility.md | 可行性分析 | 单仓可行，复用现有再分发框架，3 人月 |
| 03-arch-decision-record.md | 方案选型 | 方案A：在现有触屏分支扩展触控笔支持（status=Accepted） |
| 04-feature.md | Feature 基线 | Gate=Ready，单一 proposal 拆分 |
| 05-proposal-01.md / SR-01.md | GA 证据 | PROP-01 GA-Approved（2026-08-13），SR 编号 20260806903969 |
| spec-check-report.md | spec 规则检查 | 46 条规则全部通过或豁免（2026-09-08） |
