# 鼠标与手写笔事件再分发扩展 —— 设计评审：风险点与未明确决策点

> 评审对象：`20260806903969/` 下 design.md（为主）、proposal.md、spec.md、execution-plan.md、metadata_tracking.yaml
> 评审基线：master @ 458f28a4d（!10091）；草稿 PR !10062（da4ead8ef，分支 `wld/feat_redispatch_pen`）
> 评审日期：2026-10-28
> 结论摘要：设计整体可行，但存在 **1 个范围级矛盾（鼠标）**、**2 个语义级歧义（衍生事件丢弃、目标窗口选择）**、**1 个安全校验缺口（zOrder）** 及若干与代码现状脱节的执行隐患。建议先完成 D1~D10 决策闭环再进入 TASK 执行。

---

## 一、未明确的关键决策点

### D1.【最大】鼠标到底改不改 —— 需求与设计自相矛盾

- 需求名称与 US-2 要求"归还未消费的**鼠标**事件"；但 design 的 DD-1~DD-4、TASK-1~TASK-5 **全部只做触控笔**，鼠标被列为"行为不变"（设计约束 2、非目标）。
- **AC-7.3 的回归基线是假的**：宣称现状支持"鼠标按下/移动/抬起/滚轮完整序列再分发"，但代码现状**只有 AXIS 序列可再分发**：
  - 激活点仅有 `AXIS_BEGIN → SetWindowActive`（service/window_manager/src/input_windows_manager.cpp:5222-5227）
  - BUTTON_DOWN / MOVE / UP 无任何激活入口，首次调用即被 `AbandonMouseRedispatch` 拒绝（mouse_redispatch_store.cpp:91-103）
- TASK-5 的 `MouseRedispatchUnchanged` 断言一个**当前不存在的行为**，照写必失败。

**需决策**：
1. 鼠标 BUTTON/MOVE 会话纳入本需求 → 必须新增"鼠标按钮会话激活模型"设计决策，并解除"不可修改鼠标分支逻辑"约束；
2. 或明确本期鼠标仅支持滚轮（AXIS）→ US-2 / AC-7.3 必须改写，否则验收无解。

若全局手势业务方实际会归还点击事件，选 2 将直接导致交付不满足诉求。

### D2. 衍生事件丢弃集合不完整，且与草稿实现已分歧

- design/spec 只丢弃 `LEVITATE_IN_WINDOW / LEVITATE_OUT_WINDOW`；草稿 PR !10062 丢弃的是 `HOVER_ENTER / HOVER_EXIT`（mmi_service.cpp Inner 入口）。**两套口径未对齐**，也没有白名单说明取舍依据。
- **语义边界未写清**：服务端在 `UpdateTouchScreenTarget` 内部会为再分发会话**合法合成** LEVITATE_IN/OUT_WINDOW（PR 9527 的 `TouchLastTouchInfos` 路由、PR 9830 的 `DispatchTouch` zOrder 过滤都是为它服务的；spec AC-3.1 的日志表还将 `Levitate in by horizon` 列为预期路径）。因此：
  - 丢弃的只能是 **Inner 入口的客户端注入衍生事件**；
  - **不能**在 EventDispatchHandler 分发路径过滤——否则把服务端派生的也杀掉，悬停跨窗直接失效（回退 9830/9527 的行为）。
- design DD-2 的表述（"再分发时丢弃进入/离开窗口衍生事件"）没有把这个边界说死，执行 Agent 极易走偏。

**需决策**：给出完整的再分发 PointerAction 白/黑名单（LEVITATE_IN/OUT_WINDOW、HOVER_ENTER/EXIT、HOVER_CANCEL、ENTER/LEAVE_WINDOW、TOUCHPAD_ACTIVE、PULL_* 逐一表态），并写明"丢弃仅限 RedispatchInputEventInner 入口的客户端注入事件"。

### D3. toolType 隔离只覆盖了 Store，未覆盖真实路径与次级触点

DD-3 将 FingerMap key 扩为 `(deviceId, pointerId, toolType)` 方向正确，但同键冲突不止在 Store：

- 正向冲突检测读的**真实路径** `touchItemDownInfos_`（`GetRealFingerDownWindowId`，input_windows_manager.cpp:6147）仍是 `(deviceId, pointerId)` 键。spec EX-2 自己承认"同一 deviceId 下笔与手指同时活跃"，则真实路径同样撞键：笔的再分发 DOWN 可能被真实手指（或反向）误判同窗冲突而误杀。草稿 PR 也未改此处。
- 改 key 的连带触点未列入任务文件清单：
  | 触点 | 位置 | 说明 |
  |---|---|---|
  | `DispatchTouchRedispatch` 签名 | input_windows_manager.cpp:3270 | 按 key 取事件快照 |
  | 真实 DOWN 反向冲突遍历 FingerMap | input_windows_manager.cpp:6456-6471 | `fingerMap.find({deviceId, pointerId})` |
  | `SetFingerActive` 调用点 | input_windows_manager.cpp:6802-6805 | 激活记录 |
  | Inner 会话恢复/去激活 | mmi_service.cpp:6642-6673 | `IsFingerActive`/`GetFingerWindowId`/`DeactivateFinger` |
- TASK-1 的 Files 只含 store 两文件；上述触点分布在 TASK-2/TASK-3 的文件里但任务描述未提及。key 类型变更后 `find` 编译错可兜住大部分，但**字面量构造 key 的位置会静默错配**。

**需决策**：① 真实路径冲突键（`touchItemDownInfos_`/`GetRealFingerDownWindowId`）是否同步加 toolType；② 次级触点清单补进对应 TASK。

### D4. zOrder 输入校验完全缺失（安全口子）

`IsValidZorderWindow` 对 **zOrder ≤ 0 的事件直接全放行**（`MMI_LE(zOrder, 0.0f) → return true`，input_windows_manager.cpp:8562）。即持注入权限的调用方传 zOrder=0 时，重分发可命中**任意窗口（包括手势窗口自身之上的系统窗口）**。spec"输入验证完备 ✅"只列了 SourceType/ToolType/PointerAction，未覆盖 zOrder 合法性。

**需决策**：是否强制 zOrder > 0；进一步是否校验 zOrder ≤ 调用方自身窗口 zOrder（服务端可按调用方 pid 查窗口树）。这是本接口当前唯一未收紧的注入边界。

### D5. 错误码口径不一致

| 场景 | spec 声称 | 实际实现 |
|---|---|---|
| 无 INJECT_INPUT_EVENT 权限 | 201 | `ERROR_NOT_SYSAPI` = **202**（mmi_service.cpp:6719-6726，error_multimodal.h:28） |
| 非系统应用 | 未提及 | 同样返回 202（与无权限共用，不可区分） |
| sourceType 不支持 | 未提及 | RET_ERR(-1) |
| 会话未激活被 Abandon | 未提及 | RET_ERR(-1) |
| 服务未运行 | 未提及 | `MMISERVICE_NOT_RUNNING`（枚举） |

TASK-5 断言"返回错误码 201"，照写必红。

**需决策**：改实现对齐 201（行为变更，与"现有调用方无感知"矛盾），或改 spec 承认 202 并补完整错误码表（区分上述五类）。

### D6. "无目标窗口"时会话激活动作的策略未定义

EX-1 定义"无目标 → 丢弃 + 警告日志 + 返回成功"。但若**无目标的是 DOWN**：`SetFingerActive` 不会执行（激活发生在 UpdateTouchScreenTarget 的 DOWN/HOVER_ENTER 分支，input_windows_manager.cpp:6792-6805）→ 后续 MOVE/UP 全部因"未激活"被 Abandon 返回 **RET_ERR**。调用方看到"DOWN 成功、MOVE 报错"，错误语义未定义（spec 仅定义 201）。

**需决策**：无目标时是否记录会话使整条流返回统一口径；或在调用方契约中明确"DOWN 成功不保证后续事件成功"。

### D7. 工具类型覆盖边界

BR-1 只认 TOOL_TYPE_PEN/PENCIL。`TOOL_TYPE_RUBBER`（笔尾橡皮，手写笔标配）将按**手指规则**处理：UP 即去激活、无 PROXIMITY 自动激活、抬笔后悬停断流。BRUSH/AIRBRUSH 同理。

**需决策**：全量手写笔形态（含橡皮）是否统一按"笔规则"处理。实现上仅是 `isPen` 集合加成员（草稿已含 PEN/PENCIL），但这是行为决策而非代码细节。

### D8. 会话生命周期无兜底（design/proposal 均未提及）

- `TouchRedispatchStore::Reset()` 与 `MouseRedispatchStore::Reset()/ClearDeviceEvents()` 在生产代码**零调用点**，清理仅靠事件序列正常收尾。
- 会话 key 不含调用方标识（无法按调用方回收）。

组合故障：笔 PROXIMITY_OUT 丢失（拔笔 / 蓝牙断连 / **手势窗口进程崩溃**）→ 僵尸会话永不去激活 → 后续同 `(zOrder, deviceId, pointerId, toolType)` 的事件被放行到已失效窗口。

**需决策**：① 设备热插拔时清理 Store（挂 InputDeviceManager remove 回调）；② 会话是否记录调用方 tokenId 支持进程退出清理；③ 明确异常路径依赖 Inner RAII（Guard 析构）兜底的范围。

### D9. TASK-3 实现文字与 DD-4 自相矛盾（照做必错）

- DD-4："复用 UpdateTouchScreenTarget + IsValidZorderWindow，不新增独立目标选择"。
- TASK-3 Step 3："如需新增 `SelectRedispatchTarget(float zOrder)`：**遍历窗口列表，选择 Z 序小于指定 zOrder 的最大 Z 序可见窗口**"——这是**错误语义**：触摸目标 = **坐标命中** ∩ zOrder 过滤，而非"Z 序最大窗口"（会把手势投到坐标之外的窗口，破坏拖拽/手势连续性）。
- spec BR-5"目标窗口为指定 Z 序之下的**下一个可见窗口**"同样是 zOrder 遍历的表述。

**需决策/修订**：BR-5 与 TASK-3 统一改写为"指定 Z 序之下、**事件坐标命中**的窗口；无命中按 EX-1 处理"，并显式禁止新增独立寻窗函数。

### D10. TASK-4 的开放式修改授权应收敛

TASK-4 Files 允许"`event_dispatch_handler.cpp` 修改（按需，仅在验证发现缺口时）"。结构性跳过是现状（Inner 直接调用职责链末端节点），理论零改动；给 Agent 留"按需修改高频分发路径"的口子，一旦 UT 写法有误导致假 RED，Agent 可能在 Dispatch 链上加 flag 判断——恰好违反设计约束 6（不影响正常分发性能）。

**需决策**：TASK-4 收敛为"仅新增测试，禁止修改生产代码"。

---

## 二、风险点（按严重度排序）

| # | 风险 | 等级 | 说明 | 关联决策 |
|---|------|------|------|----------|
| R1 | AC-7.3 假基线导致验收僵局 | 高 | 回归测试断言不存在的鼠标行为，TASK-5 无法闭合；若为"让它过"而顺手实现鼠标按钮会话，则越出设计边界 | D1 |
| R2 | 衍生事件语义误读破坏悬停跨窗 | 高 | 在分发路径过滤（DD-2 备选方向）会杀掉服务端为再分会话合成的 LEVITATE_IN/OUT，回退 9830/9527 修复的行为 | D2 |
| R3 | zOrder ≤ 0 全放行的注入面 | 高 | 唯一未收紧的输入校验；当前仅靠 INJECT 权限 + 系统应用两道门 | D4 |
| R4 | 设计文档基线漂移 | 中 | design/execution-plan 引用行号（mmi_service.cpp:6322/6371、input_windows_manager.cpp:7072/5770/7934）对应旧基线（!10053 附近）；当前 master 为 6620/6701、7613/6300/8556，且中间合入了 9830。按图索骥找不到代码；TASK"只读上下文"同样过期 | — |
| R5 | 会话泄漏 → 僵尸会话 | 中 | 无生产代码清理路径、无调用方绑定 | D8 |
| R6 | 真实路径冲突检测 toolType 缺失 | 中 | 笔/指同 pointerId 时误判同窗冲突，表现为"再分发 DOWN 莫名失败" | D3 |
| R7 | 高频悬停流的同步 IPC 性能被低估 | 中 | LEVITATE_MOVE 每帧一次 `RedispatchInputEvent`：调用方**同步阻塞**等 PostSyncTask 在服务主线程完成寻窗+分发。design 风险表标"低"的依据（"无全量扫描"）未覆盖 IPC 往返本身；120Hz 悬停 ≈ 每帧一次同步 IPC。需板侧延迟压测，必要时评估 fire-and-forget 选项 | — |
| R8 | 新增测试文件改不了 BUILD.gn | 中 | TASK Files 白名单不含各 test/BUILD.gn，新增 6 个测试文件必须登记构建；"不得修改范围外文件"硬规则会使执行卡死。需把 window_manager/test、module_loader/test、event_dispatch/test 的 BUILD.gn 补进对应 TASK | D10 |
| R9 | UT 并发测试与线程模型前提冲突 | 中 | design Concurrency 节要求"单测验证并发 SetFingerActive/DeactivateFinger 交错不产生数据竞争"，但 Store 本身**非线程安全**（依赖 PostSyncTask 串行）；多线程 UT 直接打 Store 会真 race。并发验证应改为 IPC 层串行化语义验证，或 Store 测试外部加串行 | — |
| R10 | 文档内部矛盾降低可信度 | 低 | proposal"IPC/跨进程=否，同进程内部接口" vs design 安全节"跨进程 IPC"；DD-2 备选理由（"分发路径是高频路径"）不成立；design 时序图把 `SetFingerActive` 画成每事件执行（实际仅 DOWN/HOVER_ENTER/PROXIMITY_IN 激活）；proposal"代码增量 ≤500 行"未含测试量级 | — |

---

## 三、草稿 PR !10062（da4ead8ef）与设计的对齐清单

metadata_tracking.yaml 已将草稿挂在本需求下，但目前**草稿 ≠ 设计**，合并前需对齐：

1. **衍生事件集合分歧**：草稿丢 HOVER_ENTER/EXIT，design 丢 LEVITATE_IN/OUT_WINDOW → 按 D2 决策后统一（建议终态：两者都丢，且仅限 Inner 入口）。
2. **草稿基线落后**：基于 !10053（b4aa39b72），当前 master 已含 9830（DispatchTouch zOrder 过滤）等；rebase 后重点核对 `TouchLastTouchInfos` 路由与冲突检测区域。
3. **草稿未覆盖、design 也未覆盖**（D3/D4/D8 三项缺口两边都没有）：真实路径冲突键 toolType、zOrder 校验、设备级清理兜底。建议在设计中定案后一次性补进草稿，避免二次返工。
4. 草稿 6 个测试文件（store/derived/target/permission/regression/visibility）与 execution-plan 规划的测试**高度重合**，TASK 可直接以草稿测试为骨架，避免重复劳动。

---

## 四、建议处置顺序

| 顺序 | 动作 | 落点 |
|------|------|------|
| 1 | D1 鼠标范围拍板（需求方确认） | proposal/spec 修订 |
| 2 | D2 衍生事件白名单定案 + 语义边界写死 | design DD-2 / spec BR-2 修订 |
| 3 | D4 zOrder 校验方案定案（安全评审） | design 新增 DD / spec 输入校验项 |
| 4 | D9 目标选择语义修正、D10 TASK-4 授权收敛、R8 补 BUILD.gn、R4 刷新行号 | execution-plan 修订 |
| 5 | D3/D5/D6/D7/D8 逐项定案并补入设计 | design/spec 修订 |
| 6 | 草稿 rebase 至当前 master，按定案补齐缺口 | 代码（!10062） |

> 相关文档：整体接口机制分析见仓根 `redispatch_input_event_analysis.md`。
