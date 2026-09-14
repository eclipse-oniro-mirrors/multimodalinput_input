# RR-30094496 Implementation Plan

> Execute inline using superpowers:executing-plans. The user approved the implementation design on 2026-09-14.

**Goal:** Publish the education input-disable flag to `sys.input.edu_input_disabled`.

**Architecture:** Keep `InputDeviceManager::eduInputDisabled_` authoritative. Publish its current value after accepted control operations, matching-session recovery, and service initialization. Failed writes are logged without changing existing input control results; repeated requests retry publication.

**Tech Stack:** OpenHarmony C++, system parameters, GN, gtest/gmock.

- [x] Add parameter-write mock and behavioral tests in `service/device_manager/test/src/input_device_manager_test_with_mock.cpp`: true/false transitions, duplicate requests, ownership rejection, session recovery, write failure and retry, and initial-state publication. Mock only `OHOS::system::SetParameter`, with expectations on the exact key and string value; retain assertions on memory state, owner PID and device notifications.
- [x] Add `void PublishEduInputDisabledState() const` to `service/device_manager/include/input_device_manager.h`. Implement it in `service/device_manager/src/input_device_manager.cpp` using `OHOS::system::SetParameter(key, eduInputDisabled_ ? "true" : "false")`, INFO on success and ERROR on failure. Call after each accepted branch and matching-session recovery, outside device loops.
- [x] Call `INPUT_DEV_MGR->PublishEduInputDisabledState()` in `service/module_loader/src/mmi_service.cpp`, before `InitService()` exposes the service. Publish memory state rather than restoring policy from a parameter.
- [x] Review return codes, ownership checks, thread ordering and hot-path impact; run `git diff --check`.
- [ ] From the source root run `./build.sh --product-name rk3568 --build-target input --ccache` and `prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 InputDeviceManagerTestWithMock`. Run the resulting test binary on a test board. Record blockers accurately if unavailable.
- [ ] Verify on a board: service startup, disable, enable, repeated calls, owner disconnect, service restart, actual input behavior, writer/consumer parameter permissions and control-call latency. Do not claim board validation from mock tests.
- [ ] Commit with DCO and record verification status.

## Compatibility and verification limits

No SDK, IPC, error-code, permission-check or event-routing changes. The parameter is a best-effort mirror, not an atomic guarantee that every device has completed an enable/disable operation. Consumers must not infer publication success from the existing API return code.

The initial build attempt stopped before compilation: `prebuilts` is absent and the build script requires Node 14.21.1 but finds 20.20.0. Parameter DAC/SELinux configuration is not available in this partial checkout; writer and consumer access must be validated on the target product.

## Execution record

- Implemented the publication helper and all four device-manager call sites, plus startup publication before `InitService()`.
- Added six focused parameter tests and strengthened two existing device-control tests. The test executable substitutes `OHOS::system::SetParameter`; production dependency lists remain unchanged.
- `git diff --check`: passed.
- Source build after changes: exit 1 before compilation. Missing `prebuilts/python` and hvigor; Node version mismatch (expected 14.21.1, actual 20.20.0).
- Unit-test build: exit 127, `prebuilts/build-tools/linux-x86/bin/ninja: No such file or directory`.
- No red/green test execution or board validation was possible. Mock linking, target compilation, parameter permissions, service restart behavior and latency remain unverified on the target product.
- To finish validation, prepare the complete rk3568 source/dependency environment, run the two build commands above, then run `InputDeviceManagerTestWithMock` on the matching test image. Query `sys.input.edu_input_disabled` from the service consumer context after each lifecycle scenario and retain logs and actual input-behavior evidence.
