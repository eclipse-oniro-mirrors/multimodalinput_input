# ohos-input

## 概述

面向 AI Agent 应用和自动化测试，提供鼠标点击、双击、滚动、移动与拖动，键盘按键输入，以及触屏点击与滑动等模拟能力。仅部署于 PC（2in1）设备形态。

## 功能列表

- **模拟鼠标单击**：在指定坐标模拟一次鼠标点击，支持按键选择与修饰键组合
- **模拟鼠标双击**：两次完整点击，按下间隔可配置
- **模拟鼠标滚轮垂直滚动**：按齿数滚动，1 齿 = 15 度，正数向上、负数向下
- **移动鼠标光标**：将光标移动到指定显示器的指定位置
- **模拟鼠标拖动**：按住鼠标按键，将指针从起点拖动到终点；支持跨显示器拖动和设置拖动时长
- **模拟键盘按键**：按 OHOS 键码注入按键，支持 ctrl/alt/shift/meta 修饰键组合
- **模拟触屏点击**：在指定坐标模拟一次触屏点击，支持指定按压时长
- **模拟触屏滑动**：在指定显示器上模拟从起始坐标到目标坐标的手指滑动操作，支持设置滑动时长

## 依赖

### 系统能力

- `InputManager` - Controller 创建入口（libmmi-client 内建接口）
- `MouseController` - 鼠标事件注入（移动、按键、滚轮轴会话）
- `KeyboardController` - 键盘事件注入（按键按下与抬起）
- `TouchControllerImpl` - 触屏事件注入（按下、移动、抬起）

### 权限

| 权限 | 命令 | 说明 |
|-----------|----------|-------------|
| `ohos.permission.CONTROL_DEVICE` | 全部注入命令 | 注入输入事件时需要该权限 |

## 基本用法

```bash
ohos-input <subcommand> [options]
ohos-input --help
ohos-input --version
ohos-input <subcommand> --help
```

## 命令列表

| 命令 | 说明 | 参数 | 权限 | 前置依赖 |
|---------|-------------|------------|-------------|--------------|
| mouse-click | 模拟鼠标单击 | `--x --y`（必填）；`--displayId --button --holdDuration --modifier`（可选） | `ohos.permission.CONTROL_DEVICE` | PC（2in1），启用 Controller 注入能力 |
| mouse-double-click | 模拟鼠标双击 | `--x --y`（必填）；`--displayId --button --holdDuration --modifier --clickInterval`（可选） | `ohos.permission.CONTROL_DEVICE` | PC（2in1），启用 Controller 注入能力 |
| mouse-scroll | 模拟鼠标滚轮垂直滚动 | `--clicks`（必填）；`--modifier`（可选） | `ohos.permission.CONTROL_DEVICE` | PC（2in1），启用 Controller 注入能力 |
| mouse-move | 移动鼠标光标到指定位置 | `--x --y`（必填）；`--displayId --modifier`（可选） | `ohos.permission.CONTROL_DEVICE` | PC（2in1），启用 Controller 注入能力 |
| mouse-drag | 模拟鼠标拖动（支持跨显示器） | `--srcX --srcY --dstX --dstY`（必填）；`--srcDisplayId --dstDisplayId --button --duration --modifier`（可选） | `ohos.permission.CONTROL_DEVICE` | PC（2in1），启用 Controller 注入能力 |
| key-press | 模拟键盘按键 | `--key`（必填）；`--holdDuration --modifier`（可选） | `ohos.permission.CONTROL_DEVICE` | PC（2in1），启用 Controller 注入能力 |
| touch-tap | 模拟触屏点击 | `--x --y`（必填）；`--displayId --duration`（可选） | `ohos.permission.CONTROL_DEVICE` | PC（2in1），启用 Controller 注入能力 |
| touch-swipe | 模拟触屏滑动 | `--srcX --srcY --dstX --dstY`（必填）；`--displayId --duration`（可选） | `ohos.permission.CONTROL_DEVICE` | PC（2in1），启用 Controller 注入能力 |

**前置依赖说明**：
- 所有命令均适用于 PC（2in1）产品，且需启用 Controller 注入能力。构建时设置 `input_controller_inject_enable=true`，以启用 `OHOS_BUILD_ENABLE_CONTROLLER_INJECT` 实现。
- CLI 会根据命令自动创建所需 Controller；使用 `--modifier` 时还会创建 `KeyboardController`。

## 参数说明

### mouse-click

```
ohos-input mouse-click [options]
  --displayId <number>    显示器 ID（可选，>=0，默认 0）
  --x <integer>           目标 X 坐标（必填整数，>=0）
  --y <integer>           目标 Y 坐标（必填整数，>=0）
  --button <key>          鼠标按键（可选，left/right/middle，默认 left）
  --holdDuration <number> 按下持续时间 ms（可选，[50,200]，默认 100）
  --modifier <keys>       修饰键（可选，ctrl/alt/shift/meta，多个用 | 分隔，不可重复）
```

### mouse-double-click

```
ohos-input mouse-double-click [options]
  --displayId <number>    显示器 ID（可选，>=0，默认 0）
  --x <integer>           目标 X 坐标（必填整数，>=0）
  --y <integer>           目标 Y 坐标（必填整数，>=0）
  --button <key>          鼠标按键（可选，left/right/middle，默认 left）
  --holdDuration <number> 按下持续时间 ms（可选，[50,200]，默认 100）
  --modifier <keys>       修饰键（可选，ctrl/alt/shift/meta，多个用 | 分隔，不可重复）
  --clickInterval <number> 第一次按下到第二次按下的间隔 ms（可选，[100,400]，须大于 holdDuration，默认 250）
```

### mouse-scroll

```
ohos-input mouse-scroll [options]
  --clicks <number>       滚动齿数（必填，[-100,-1] 或 [1,100]，正数向上、负数向下，1 齿 = 15 度）
  --modifier <keys>       修饰键（可选，ctrl/alt/shift/meta，多个用 | 分隔，不可重复）
```

### mouse-move

```
ohos-input mouse-move [options]
  --displayId <number>    显示器 ID（可选，>=0，默认 0）
  --x <integer>           目标 X 坐标（必填整数，>=0）
  --y <integer>           目标 Y 坐标（必填整数，>=0）
  --modifier <keys>       修饰键（可选，ctrl/alt/shift/meta，多个用 | 分隔，不可重复）
```

### mouse-drag

```
ohos-input mouse-drag [options]
  --srcDisplayId <number> 源显示器 ID（可选，>=0，默认 0）
  --srcX/--srcY <integer> 起点 X/Y 坐标（必填整数，>=0）
  --dstDisplayId <number> 目标显示器 ID（可选，>=0，默认 0）
  --dstX/--dstY <integer> 终点 X/Y 坐标（必填整数，>=0）
  --button <key>          拖动按住的按键（可选，left/right/middle，默认 left）
  --duration <number>     总时长 ms（可选，[0,10000]，0 表示瞬间完成，默认 0；>0 时按 16ms 步进插值）
  --modifier <keys>       修饰键（可选，ctrl/alt/shift/meta，多个用 | 分隔，不可重复）
```

### key-press

```
ohos-input key-press [options]
  --key <number>          OHOS 键码（必填，>=1，如 2017=A、2054=回车）
  --holdDuration <number> 按下持续时间 ms（可选，[0,10000]，0 表示瞬间抬起，默认 100）
  --modifier <keys>       修饰键（可选，ctrl/alt/shift/meta，多个用 | 分隔，不可重复；修饰键+目标键总数不超过 5）
```

修饰键仅支持 `ctrl` / `alt` / `shift` / `meta`（映射 LEFT 变体键码 2072/2045/2047/2076），按下顺序为书写顺序，抬起顺序为逆序；`--key` 键码不得与已选修饰键键码重复。

### touch-tap

```
ohos-input touch-tap [options]
  --displayId <number>  显示器 ID（可选，>=0，默认 0）
  --x <integer>         目标 X 坐标（必填，>=0）
  --y <integer>         目标 Y 坐标（必填，>=0）
  --duration <number>   按下持续时间 ms（可选，[0,5000]，默认 100；duration为0时立即抬起）
```

### touch-swipe

```
ohos-input touch-swipe [options]
  --displayId <number>  显示器 ID（可选，>=0，默认 0）
  --srcX <integer>      起点 X 坐标（必填，>=0）
  --srcY <integer>      起点 Y 坐标（必填，>=0）
  --dstX <integer>      终点 X 坐标（必填，>=0）
  --dstY <integer>      终点 Y 坐标（必填，>=0）
  --duration <number>   总时长 ms（可选，[0,5000]，0 表示瞬间完成，默认 500；>0 时按约 16ms 步进插值移动）
```

## 输出格式

所有命令以 JSON 格式输出到 stdout，结构如下：

### 成功响应

```json
{
  "type": "result",
  "status": "success",
  "data": {
    // 命令特定数据
  }
}
```

### 失败响应

```json
{
  "type": "result",
  "status": "failed",
  "errCode": "ERR_XXX",
  "errMsg": "错误描述",
  "suggestion": "建议的下一步操作"
}
```

### 错误码

| 错误码 | 退出码 | 说明 |
|------------|---------|-------------|
| `ERR_INPUT_SERVICE_EXCEPTION` (3800001) | 1 | 输入服务异常（服务未运行、注入失败、运行期状态错误） |
| `ERR_PARAMETER_ERROR` (401) | 2 | 参数校验失败（前置拦截，不发起 IPC） |
| `ERR_PERMISSION_DENIED` (201) | 3 | 权限不足（未持有 ohos.permission.CONTROL_DEVICE） |

## 示例

### mouse-click

```bash
# 单击 (100, 200)
ohos-input mouse-click --x 100 --y 200

# Ctrl+单击 (300, 400)，按住 100ms
ohos-input mouse-click --x 300 --y 400 --holdDuration 100 --modifier ctrl

# 输出示例：
{"type":"result","status":"success","data":{"action":"mouse-click","displayId":0,"x":100,"y":200}}
```

### mouse-double-click

```bash
# 右键双击，自定义间隔
ohos-input mouse-double-click --x 100 --y 200 --button right --holdDuration 100 --clickInterval 200

# 输出示例：
{"type":"result","status":"success","data":{"action":"mouse-double-click","displayId":0,"x":100,"y":200}}
```

### mouse-scroll

```bash
# 向下滚动 3 齿（1 齿 = 15 度）
ohos-input mouse-scroll --clicks -3

# Ctrl+向上滚动 5 齿
ohos-input mouse-scroll --clicks 5 --modifier ctrl

# 输出示例：
{"type":"result","status":"success","data":{"action":"mouse-scroll","clicks":-3}}
```

### mouse-move

```bash
# 移动光标到 (100, 200)
ohos-input mouse-move --x 100 --y 200

# 移动光标到 1 号显示器 (100, 200)
ohos-input mouse-move --displayId 1 --x 100 --y 200

# 输出示例：
{"type":"result","status":"success","data":{"action":"mouse-move","displayId":1,"x":100,"y":200}}
```

### mouse-drag

```bash
# 左键从 (100,100) 拖动到 (300,300)，历时 500ms
ohos-input mouse-drag --srcX 100 --srcY 100 --dstX 300 --dstY 300 --duration 500

# 跨显示器拖动
ohos-input mouse-drag --srcDisplayId 0 --srcX 100 --srcY 100 --dstDisplayId 1 --dstX 200 --dstY 200 --duration 500

# 输出示例：
{"type":"result","status":"success","data":{"action":"mouse-drag","srcDisplayId":0,"srcX":100,"srcY":100,"dstDisplayId":0,"dstX":300,"dstY":300}}
```

### key-press

```bash
# 按下回车键（默认按住 100ms）
ohos-input key-press --key 2054

# Ctrl+A（全选）
ohos-input key-press --key 2017 --modifier ctrl

# Ctrl+Shift+S（按下顺序 ctrl、shift；抬起顺序 shift、ctrl）
ohos-input key-press --key 2035 --modifier ctrl|shift

# 输出示例：
{"type":"result","status":"success","data":{"action":"key-press","key":2017}}
```

### touch-tap

```bash
# 使用默认显示器和默认按下时长
ohos-input touch-tap --x 500 --y 800

# 在指定显示器按下 200ms
ohos-input touch-tap --displayId 0 --x 500 --y 800 --duration 200

# 输出示例：
{"type":"result","status":"success","data":{"action":"touch-tap","displayId":0,"x":500,"y":800,"duration":200}}
```

### touch-swipe

```bash
# 使用默认显示器和默认滑动时长
ohos-input touch-swipe --srcX 100 --srcY 500 --dstX 900 --dstY 500

# 在指定显示器滑动 500ms
ohos-input touch-swipe --displayId 0 --srcX 100 --srcY 500 --dstX 900 --dstY 500 --duration 500

# 输出示例：
{"type":"result","status":"success","data":{"action":"touch-swipe","displayId":0,"srcX":100,"srcY":500,"dstX":900,"dstY":500,"duration":500}}
```

## 典型工作流

### 鼠标与键盘操作

```bash
# 1. 移动光标到目标位置
ohos-input mouse-move --x 100 --y 200

# 2. 双击打开目标
ohos-input mouse-double-click --x 100 --y 200

# 3. 单击选中目标区域
ohos-input mouse-click --x 300 --y 400

# 4. 滚动浏览内容
ohos-input mouse-scroll --clicks -3

# 5. 输入按键（示例：按下回车键）
ohos-input key-press --key 2054
```

### 触屏操作

```bash
# 1. 手指滑动到指定位置
ohos-input touch-swipe --displayId 0 --srcX 100 --srcY 500 --dstX 900 --dstY 500 --duration 500

# 2. 点击目标位置
ohos-input touch-tap --displayId 0 --x 900 --y 500 --duration 100
```

## 安装

- 可执行文件：`/system/bin/cli_tool/executable/ohos-input`
- 描述文件：`/system/bin/cli_tool/configs/ohos-input.json`（由构建系统从本仓 ohos-input.json 自动部署）

## 构建配置

- **构建目标**：`ohos-input`
- **子系统**：`multimodalinput`
- **部件**：`input`
- **门控开关**：gn 变量 `input_controller_inject_enable`（默认 false），PC 产品配置中开启后编译
