# ohos-input unit tests

Tests use the OpenHarmony gtest framework (`HWTEST_F`) and follow the layout of
`ohos-pasteboard/tests`: one file per responsibility.

| File | Responsibility |
| --- | --- |
| `command_runner.h` | shared helpers: run `ExecuteCommand` capturing stdout, parse JSON, compare recorded controller calls |
| `mock_controller_factory.h/.cpp` | link-time stubs for `InputManager`, `MouseControllerImpl` and `KeyboardControllerImpl` |
| `modifier_test.cpp` | `ParseModifiers` ordering, single keys, malformed and duplicate input |
| `option_parser_test.cpp` | `ParseOptionPairs` whitelist/duplicates, `ParseNumber` defaults, ranges and messages |
| `command_table_test.cpp` | read-only command lookup, declaration order, stable repeated lookup |
| `printer_test.cpp` | JSON shape, help printing, parameter, unknown-command and controller-error mappings |
| `executor_test.cpp` | help routing (global/command, `--help` anywhere), `--version`, unknown command JSON, command availability |
| `mouse_support_test.cpp` | `ValidateScrollClicks` boundaries and `ParseButton` names/defaults |
| `mouse_drag_command_test.cpp` | `BuildDragSteps` step splitting, stationary and cross-display paths, no overflow |
| `integration_test.cpp` | full command execution against mock controllers: call order, validation before controller creation, failure paths and controller destruction |
| `input_touch_command_test.cpp` | touch CLI tests: invalid parameters rejected before controller creation; mock factory call count, tap/swipe DOWN/MOVE/UP order, zero-duration behavior, permission/service mapping and MOVE failure cleanup |

## Host mock link boundary

`mock_controller_factory.cpp` supplies test-only definitions of `InputManager` creation/coordinate methods
and the platform Controller constructors, destructors and input methods. The test target uses the real
platform headers and production CLI sources; no mock subclasses or replacement headers
are used. `OhosInputCommandTest` includes every CLI production source except `src/main.cpp`.
Do not add the real platform implementation sources to these test targets: their method symbols
are intentionally replaced to record command ordering and inject failures. These tests do not verify
platform injection or destructor cleanup; those require platform/board tests.

## Device execution matrix

| Command example | Coverage | Permission | Prerequisite |
| --- | --- | --- | --- |
| `ohos-input --help` / `--version` | top-level help and version | None | None |
| `ohos-input mouse-click --x 0 --y 0 --displayId 0 --button left --holdDuration 50 --modifier ctrl\|shift` | defaults, ranges, modifiers | CONTROL_DEVICE | PC |
| `ohos-input mouse-double-click --x 100 --y 200 --button right --holdDuration 100 --clickInterval 250 --modifier alt` | all options and interval | CONTROL_DEVICE | PC |
| `ohos-input mouse-scroll --clicks 1` / `--clicks -100 --modifier ctrl` | positive/negative boundaries, modifiers | CONTROL_DEVICE | PC |
| `ohos-input mouse-move --displayId 1 --x 100 --y 200 --modifier meta` | all options | CONTROL_DEVICE | PC |
| `ohos-input mouse-drag --srcDisplayId 0 --srcX 0 --srcY 0 --dstDisplayId 1 --dstX 100 --dstY 200 --button middle --duration 16 --modifier shift` | all options | CONTROL_DEVICE | PC |
| `ohos-input key-press --key 2049 --holdDuration 0 --modifier ctrl\|shift` | all options and modifier order | CONTROL_DEVICE | PC |
| `ohos-input touch-tap --x 500 --y 800` | required arguments, default display 0 and duration 100ms | CONTROL_DEVICE | PC + controller injection feature |
| `ohos-input touch-tap --displayId 0 --x 500 --y 800 --duration 200` | explicit display and duration; DOWN, hold, UP | CONTROL_DEVICE | PC + controller injection feature |
| `ohos-input touch-tap --x 0 --y 0 --duration 0` | minimum coordinate/duration; immediate DOWN then UP | CONTROL_DEVICE | PC + controller injection feature |
| `ohos-input touch-tap --x 1 --y 2 --duration 5000` | maximum accepted duration | CONTROL_DEVICE | PC + controller injection feature |
| `ohos-input touch-swipe --srcX 100 --srcY 500 --dstX 900 --dstY 500` | required arguments, default display 0 and duration 500ms | CONTROL_DEVICE | PC + controller injection feature |
| `ohos-input touch-swipe --displayId 0 --srcX 100 --srcY 500 --dstX 900 --dstY 500 --duration 500` | explicit display and duration; DOWN, linear MOVE, UP | CONTROL_DEVICE | PC + controller injection feature |
| `ohos-input touch-swipe --srcX 0 --srcY 0 --dstX 10 --dstY 10 --duration 0` | zero duration; DOWN then UP without MOVE | CONTROL_DEVICE | PC + controller injection feature |
| `ohos-input touch-swipe --srcX 1 --srcY 2 --dstX 3 --dstY 4 --duration 5000` | maximum accepted duration | CONTROL_DEVICE | PC + controller injection feature |
| mocked `touch-tap --x 15 --y 25 --duration 0` | `CreateTouchController`, DOWN(15,25), UP(15,25); success JSON | Mocked controller | Host unit test |
| mocked `touch-tap --displayId 0 --x 500 --y 800 --duration 200` | spec AC-1.1 coordinates/display, DOWN then UP, JSON data and at least 180ms elapsed | Mocked controller | Host unit test |
| mocked `touch-swipe --displayId 2 --srcX 10 --srcY 20 --dstX 30 --dstY 40 --duration 0` | DOWN then UP at destination, with no MOVE | Mocked controller | Host unit test |
| mocked `touch-swipe --srcX 0 --srcY 20 --dstX 100 --dstY 40 --duration 256` | ordered DOWN, one or more MOVE, UP at endpoint | Mocked controller | Host unit test (uses short real timer interval) |
| mocked `touch-swipe --displayId 0 --srcX 100 --srcY 500 --dstX 900 --dstY 500 --duration 500` | spec AC-2.1 arguments, >=2 MOVE calls with strictly increasing X below 900, endpoint UP at 900, JSON data and at least 480ms elapsed | Mocked controller | Host unit test |
| mocked permission denial from touch controller factory | `ERR_PERMISSION_DENIED`, exit 3, and no DOWN/MOVE/UP | Mocked controller | Host unit test; actual SA identity enforcement also needs board test |
| mocked swipe MOVE failure | service error; best-effort UP at last successful coordinate | Mocked controller | Host unit test |
| mocked touch DOWN/UP failure | service error; no UP after failed DOWN, no duplicate UP after failed UP | Mocked controller | Host unit test |
| invalid/missing options; malformed or duplicate modifiers; `clicks=0`; primary key duplicated in modifiers | parameter error and exit 2 before controller creation | None | None |
| touch command missing/unknown/duplicate options, negative coordinates or display ID, non-integer, int32 overflow, duration outside [0,5000] | Standard failed JSON with `ERR_PARAMETER_ERROR` on stdout, empty stderr, exit 2 before IPC | None | None |

The host mock tests verify command call ordering, zero-duration behavior, short swipe MOVE sequence, cleanup attempts, and exit-code mapping for injected permission/service errors. PC product/feature-gated board verification is still required to confirm actual SA event delivery, timing, and permission enforcement for authorized and unauthorized caller identities.
