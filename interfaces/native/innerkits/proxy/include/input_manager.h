/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "event_handler.h"

#include "error_multimodal.h"
#include "extra_data.h"
#include "i_anco_consumer.h"
#include "i_anr_observer.h"
#include "i_input_device_listener.h"
#include "i_input_event_consumer.h"
#include "i_input_event_filter.h"
#include "i_input_service_watcher.h"
#include "i_window_checker.h"
#include "infrared_frequency_info.h"
#include "input_device.h"
#include "key_option.h"
#include "long_press_event.h"
#include "mmi_event_observer.h"
#include "pointer_style.h"
#include "touchpad_control_display_gain.h"
#include "shift_info.h"

namespace OHOS {
namespace MMI {
class InputManager {
public:
    /**
     * @brief Obtains an <b>InputManager</b> instance.
     * @return Returns the pointer to the <b>InputManager</b> instance.
     * @since 9
     */
    static InputManager *GetInstance();
    virtual ~InputManager() = default;

    int32_t GetDisplayBindInfo(DisplayBindInfos &infos);
    int32_t SetDisplayBind(int32_t deviceId, int32_t displayId, std::string &msg);

    /**
     * @brief Updates the screen、display and window information array.
     * @param userScreenInfo Indicates the user screen、display and window information.
     * @since 20
     */
    int32_t UpdateDisplayInfo(const UserScreenInfo &userScreenInfo);

    /**
     * @brief Updates the screen and window information array.
     * @param displayGroupInfo Indicates the logical screen information array.
     * @since 9
     */
    int32_t UpdateDisplayInfo(const std::vector<DisplayGroupInfo> &displayGroupInfo);

    /**
     * @brief Updates the windows information.
     * @param windowGroupInfo Indicates the window group information.
     * @since 9
     */
    int32_t UpdateWindowInfo(const WindowGroupInfo &windowGroupInfo);

    int32_t AddInputEventFilter(std::shared_ptr<IInputEventFilter> filter, int32_t priority, uint32_t deviceTags);
    int32_t RemoveInputEventFilter(int32_t filterId);

    /**
     * @brief Updates the process info to other server.
     * @param observer Indicates the progess info.
     * @return the observer setting successed or not.
     * @since 10
     */
    int32_t AddInputEventObserver(std::shared_ptr<MMIEventObserver> observer);

    /**
     * @brief Callback interface of the remove module.
     * @param observer Indicates the progess info.
     * @return EC_OK if unsubscribe successfully, else return other errcodes.
     * @since 10
     */
    int32_t RemoveInputEventObserver(std::shared_ptr<MMIEventObserver> observer = nullptr);

    /**
     * @brief Set the process info to mmi server.
     * @param pid Indicates pid.
     * @param uid Indicates uid.
     * @param bundleName Indicates bundleName.
     * @param napStatus Indicates napStatus.
     * @since 10
     */
    void SetNapStatus(int32_t pid, int32_t uid, std::string bundleName, int32_t napStatus);

    /**
     * @brief Get the process info datas to other server.
     * @param callback Indicates the callback used to receive the reported data.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 10
     */
    int32_t GetAllMmiSubscribedEvents(std::map<std::tuple<int32_t, int32_t, std::string>, int32_t> &datas);

    /**
     * @brief Sets a consumer for the window input event of the current process.
     * @param inputEventConsumer Indicates the consumer to set. The window input event of the current process
     * will be called back to the consumer object for processing.
     * @since 9
     */
    int32_t SetWindowInputEventConsumer(std::shared_ptr<IInputEventConsumer> inputEventConsumer);

    /**
     * @brief Sets a window input event consumer that runs on the specified thread.
     * @param inputEventConsumer Indicates the consumer to set.
     * @param eventHandler Indicates the thread running the consumer.
     * @since 9
     */
    int32_t SetWindowInputEventConsumer(std::shared_ptr<IInputEventConsumer> inputEventConsumer,
        std::shared_ptr<AppExecFwk::EventHandler> eventHandler);

    /**
     * @brief Subscribes to the key input event that meets a specific condition. When such an event occurs,
     * the <b>callback</b> specified is invoked to process the event.
     * @param keyOption Indicates the condition of the key input event.
     * @param callback Indicates the callback.
     * @return Returns the subscription ID, which uniquely identifies a subscription in the process.
     * If the value is greater than or equal to <b>0</b>,
     * the subscription is successful. Otherwise, the subscription fails.
     * @since 9
     */
    int32_t SubscribeKeyEvent(std::shared_ptr<KeyOption> keyOption,
        std::function<void(std::shared_ptr<KeyEvent>)> callback);

    /**
     * @brief Unsubscribes from a key input event.
     * @param subscriberId Indicates the subscription ID, which is the return value of <b>SubscribeKeyEvent</b>.
     * @return void
     * @since 9
     */
    void UnsubscribeKeyEvent(int32_t subscriberId);

    int32_t SubscribeHotkey(std::shared_ptr<KeyOption> keyOption,
        std::function<void(std::shared_ptr<KeyEvent>)> callback);
    void UnsubscribeHotkey(int32_t subscriberId);

    int32_t SubscribeKeyMonitor(const KeyMonitorOption &keyOption,
        std::function<void(std::shared_ptr<KeyEvent>)> callback);
    int32_t UnsubscribeKeyMonitor(int32_t subscriberId);

    /**
     * @brief Subscribes to the switch input event that meets a specific condition. When such an event occurs,
     * the <b>callback</b> specified is invoked to process the event.
     * @param callback Indicates the callback.
     * @param switchType Indicates the type of switch input event.
     * @return Returns the subscription ID, which uniquely identifies a subscription in the process.
     * If the value is greater than or equal to <b>0</b>,
     * the subscription is successful. Otherwise, the subscription fails.
     * @since 9
     */
    int32_t SubscribeSwitchEvent(std::function<void(std::shared_ptr<SwitchEvent>)> callback,
        SwitchEvent::SwitchType switchType = SwitchEvent::SwitchType::SWITCH_DEFAULT);

    /**
     * @brief Unsubscribes from a switch input event.
     * @param subscriberId Indicates the subscription ID, which is the return value of <b>SubscribeKeyEvent</b>.
     * @return void
     * @since 9
     */
    void UnsubscribeSwitchEvent(int32_t subscriberId);

    /**
     * @brief Query the current status of switches that meet specific conditions.
     * @param switchType Indicates the type of switch to query.
     * @param state Indicates the state of switch with given type.
     * @return Returns <b>0<b/> if success; returns a non-0 value otherwise.
     * @since 18
     */
    int32_t QuerySwitchStatus(SwitchEvent::SwitchType switchType, SwitchEvent::SwitchState &state);

 /**
     * @brief Subscribes to the switch input event that meets a specific condition. When such an event occurs,
     * the <b>callback</b> specified is invoked to process the event.
     * @param callback Indicates the callback.
     * @return Returns the subscription ID, which uniquely identifies a subscription in the process.
     * If the value is greater than or equal to <b>0</b>,
     * the subscription is successful. Otherwise, the subscription fails.
     * @since 14
     */
    int32_t SubscribeTabletProximity(std::function<void(std::shared_ptr<PointerEvent>)> callback);

    /**
     * @brief Unsubscribes from a switch input event.
     * @param subscriberId Indicates the subscription ID, which is the return value of <b>SubscribeKeyEvent</b>.
     * @return void
     * @since 14
     */
    void UnsubscribetabletProximity(int32_t subscriberId);
    /**
     * @brief Subscribes to the long-press event that meets a specific condition.
     * @param LongPressRequest Long-press event.
     * @param callback Callback invoked to process the event.
     * @return Subscription ID, which uniquely identifies a subscription in the process.
     * If the value is greater than or equal to <b>0</b>,
     * the subscription is successful. Otherwise, the subscription fails.
     * @since 16
     */
    int32_t SubscribeLongPressEvent(const LongPressRequest &LongPressRequest,
        std::function<void(LongPressEvent)> callback);

    /**
     * @brief Unsubscribes from a long-press event.
     * @param subscriberId Subscription ID, which is the return value of <b>SubscribeKeyEvent</b>.
     * @return void
     * @since 16
     */
    void UnsubscribeLongPressEvent(int32_t subscriberId);
    
    /**
     * @brief Adds an input event monitor. After such a monitor is added,
     * an input event is copied and distributed to the monitor while being distributed to the original target.
     * @param monitor Indicates the input event monitor. After an input event is generated,
     * the functions of the monitor object will be called.
     * @return Returns the monitor ID, which uniquely identifies a monitor in the process.
     * If the value is greater than or equal to <b>0</b>, the monitor is successfully added. Otherwise,
     * the monitor fails to be added.
     * @since 9
     */
    int32_t AddMonitor(std::function<void(std::shared_ptr<KeyEvent>)> monitor);

    /**
     * @brief Adds a pre input event monitor. After such a monitor is added,
     * an input event is copied and distributed to the monitor while being distributed to the original target.
     * @param monitor Indicates the input event monitor. After an input event is generated,
     * the functions of the monitor object will be called.
     * @param eventType Indicates the eventType for monitor.
     * @param keys Event type, which is **key**.
     * @return Returns the monitor ID, which uniquely identifies a monitor in the process.
     * If the value is greater than or equal to <b>0</b>, the monitor is successfully added. Otherwise,
     * the monitor fails to be added.
     * @since 15
     */
    int32_t AddPreMonitor(std::shared_ptr<IInputEventConsumer> monitor,
        HandleEventType eventType, std::vector<int32_t> keys);

    /**
     * @brief Removes a pre monitor.
     * @param monitorId Indicates the monitor ID, which is the return value of <b>AddPreMonitor</b>.
     * @return void
     * @since 15
     */
    void RemovePreMonitor(int32_t monitorId);
 
    /**
     * @brief Adds an input event monitor. After such a monitor is added,
     * an input event is copied and distributed to the monitor while being distributed to the original target.
     * @param monitor Indicates the input event monitor. After an input event is generated,
     * the functions of the monitor object will be called.
     * @return Returns the monitor ID, which uniquely identifies a monitor in the process.
     * If the value is greater than or equal to <b>0</b>, the monitor is successfully added. Otherwise,
     * the monitor fails to be added.
     * @since 9
     */
    int32_t AddMonitor(std::function<void(std::shared_ptr<PointerEvent>)> monitor);

    /**
     * @brief Adds an input event monitor. After such a monitor is added,
     * an input event is copied and distributed to the monitor while being distributed to the original target.
     * @param monitor Indicates the input event monitor. After an input event is generated,
     * the functions of the monitor object will be called.
     * @param eventType Indicates the eventType for monitor.
     * @return Returns the monitor ID, which uniquely identifies a monitor in the process.
     * If the value is greater than or equal to <b>0</b>, the monitor is successfully added. Otherwise,
     * the monitor fails to be added.
     * @since 9
     */
    int32_t AddMonitor(std::shared_ptr<IInputEventConsumer> monitor, HandleEventType eventType = HANDLE_EVENT_TYPE_KP);
    /**
     * @brief Adds an input event monitor. After such a monitor is added,
     * an input event is copied and distributed to the monitor while being distributed to the original target.
     * @param monitor Indicates the input event monitor. After an input event is generated,
     * the functions of the monitor object will be called.
     * @param actionsType Indicates the actionsType for monitor.
     * @return Returns the monitor ID, which uniquely identifies a monitor in the process.
     * If the value is greater than or equal to <b>0</b>, the monitor is successfully added. Otherwise,
     * the monitor fails to be added.
     * @since 9
     */
    int32_t AddMonitor(std::shared_ptr<IInputEventConsumer> monitor, std::vector<int32_t> actionsType);

    /**
     * @brief Removes a monitor.
     * @param monitorId Indicates the monitor ID, which is the return value of <b>AddMonitor</b>.
     * @return void
     * @since 9
     */
    void RemoveMonitor(int32_t monitorId);

    /**
     * @brief Marks that a monitor has consumed a touchscreen input event. After being consumed,
     * the touchscreen input event will not be distributed to the original target.
     * @param monitorId Indicates the monitor ID.
     * @param eventId Indicates the ID of the consumed touchscreen input event.
     * @return void
     * @since 9
     */
    void MarkConsumed(int32_t monitorId, int32_t eventId);

    /**
     * @brief Moves the cursor to the specified position.
     * @param offsetX Indicates the offset on the X axis.
     * @param offsetY Indicates the offset on the Y axis.
     * @return void
     * @since 9
     */
    void MoveMouse(int32_t offsetX, int32_t offsetY);

    /**
     * @brief Adds an input event interceptor. After such an interceptor is added,
     * an input event will be distributed to the interceptor instead of the original target and monitor.
     * @param interceptor Indicates the input event interceptor. After an input event is generated,
     * the functions of the interceptor object will be called.
     * @return Returns the interceptor ID, which uniquely identifies an interceptor in the process.
     * If the value is greater than or equal to <b>0</b>,the interceptor is successfully added. Otherwise,
     * the interceptor fails to be added.
     * @since 9
     */
    int32_t AddInterceptor(std::shared_ptr<IInputEventConsumer> interceptor);
    int32_t AddInterceptor(std::function<void(std::shared_ptr<KeyEvent>)> interceptor);
    int32_t AddInterceptor(std::shared_ptr<IInputEventConsumer> interceptor, int32_t priority, uint32_t deviceTags);

    /**
     * @brief Removes an interceptor.
     * @param interceptorId Indicates the interceptor ID, which is the return value of <b>AddInterceptor</b>.
     * @return void
     * @since 9
     */
    void RemoveInterceptor(int32_t interceptorId);

    /**
     * @brief Simulates a key input event. This event will be distributed and
     * processed in the same way as the event reported by the input device.
     * @param keyEvent Indicates the key input event to simulate.
     * @return void
     * @since 9
     */
    void SimulateInputEvent(std::shared_ptr<KeyEvent> keyEvent);

    /**
     * @brief Simulates a touchpad input event, touchscreen input event, or mouse device input event.
     * This event will be distributed and processed in the same way as the event reported by the input device.
     * @param pointerEvent Indicates the touchpad input event, touchscreen input event,
     * or mouse device input event to simulate.
     * @param isAutoToVirtualScreen In one-handed mode, true indicates that the data is automatically injected to
     * the virtual screen, and false indicates that the data is not automatically injected to the virtual screen.
     * @param useCoordinate Which coordinates to use for injecting events.
     * @return void
     * @since 9
     */
    void SimulateInputEvent(std::shared_ptr<PointerEvent> pointerEvent, bool isAutoToVirtualScreen = true,
        int32_t useCoordinate = PointerEvent::DISPLAY_COORDINATE);

    /**
     * @brief Simulates a touchpad input event, touchscreen input event, or mouse device input event.
     * This event will be distributed and processed in the same way as the event reported by the input device.
     * @param pointerEvent Indicates the touchpad input event, touchscreen input event,
     * or mouse device input event to simulate.
     * @param zOrder Indicates the point event will inject to the window whose index value is less than the zOrder
     * @param isAutoToVirtualScreen In one-handed mode, true indicates that the data is automatically injected to
     * the virtual screen, and false indicates that the data is not automatically injected to the virtual screen.
     * @param useCoordinate Which coordinates to use for injecting events.
     * @return void
     * @since 9
     */
    void SimulateInputEvent(std::shared_ptr<PointerEvent> pointerEvent, float zOrder,
        bool isAutoToVirtualScreen = true, int32_t useCoordinate = PointerEvent::DISPLAY_COORDINATE);
    void SimulateTouchPadInputEvent(std::shared_ptr<PointerEvent> pointerEvent, const TouchpadCDG &touchpadCDG);

    /**
     * @brief Simulates a touchpad input event.
     * because some event for touchpad is very different from other input,
     *  especially pointer.id must be same as actual data 0, 1, 2.
     * @param pointerEvent Indicates the touchpad input event.
     * @return void
     * @since 12
     */
    void SimulateTouchPadEvent(std::shared_ptr<PointerEvent> pointerEvent);

    /**
     * @brief Convert mouse events to touch events.
     * @param pointerEvent PointerEvent object.
     * @return bool
     * @since 20
     */
    bool TransformMouseEventToTouchEvent(std::shared_ptr<PointerEvent> pointerEvent);

    /**
     * @brief Convert touch events to mouse events.
     * @param pointerEvent PointerEvent object.
     * @return bool
     * @since 20
     */
    bool TransformTouchEventToMouseEvent(std::shared_ptr<PointerEvent> pointerEvent);

    /**
     * @brief Starts listening for an input device event.
     * @param type Indicates the type of the input device event, which is <b>change</b>.
     * @param listener Indicates the listener for the input device event.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t RegisterDevListener(std::string type, std::shared_ptr<IInputDeviceListener> listener);

    /**
     * @brief Stops listening for an input device event.
     * @param type Indicates the type of the input device event, which is <b>change</b>.
     * @param listener Indicates the listener for the input device event.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t UnregisterDevListener(std::string type, std::shared_ptr<IInputDeviceListener> listener = nullptr);

    /**
     * @brief Obtains the information about an input device.
     * @param callback Indicates the callback used to receive the reported data.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t GetDeviceIds(std::function<void(std::vector<int32_t>&)> callback);

    /**
     * @brief Obtains the information about an input device.
     * @param deviceId Indicates the ID of the input device whose information is to be obtained.
     * @param callback Indicates the callback used to receive the reported data.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t GetDevice(int32_t deviceId, std::function<void(std::shared_ptr<InputDevice>)> callback);

    /**
     * @brief Checks whether the specified key codes of an input device are supported.
     * @param deviceId Indicates the ID of the input device.
     * @param keyCodes Indicates the key codes of the input device.
     * @param callback Indicates the callback used to receive the reported data.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SupportKeys(int32_t deviceId, std::vector<int32_t> keyCodes,
        std::function<void(std::vector<bool>&)> callback);

    /**
     * @brief Sets the number of the mouse scrolling rows.
     * @param rows Indicates the number of the mouse scrolling rows.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SetMouseScrollRows(int32_t rows);

    /**
     * @brief Set pixelMap to override ohos mouse icon resouce.
     * @param windowId Indicates the windowId of the window
     * @param pixelMap Indicates the image resouce for this mouse icon. which realtype must be OHOS::Media::PixelMap*
     * @param focusX Indicates focus x
     * @param focusY Indicates focus y
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SetCustomCursor(int32_t windowId, void* pixelMap, int32_t focusX = 0, int32_t focusY = 0);

    /**
     * @brief Set pixelMap to override ohos mouse icon resouce.
     * @param windowId Indicates the windowId of the window
     * @param pixelMap Indicates the image resouce for this mouse icon. which realtype must be OHOS::Media::PixelMap*
     * @return vint32_t
     * @since 10
     */
    int32_t SetMouseIcon(int32_t windowId, void* pixelMap);

    /**
     * @brief Set mouse icon hot spot.
     * @param windowId Indicates the windowId of the window
     * @param hotSpotX Indicates the hot spot x for this mouse icon.
     * @param hotSpotY Indicates the hot spot y for this mouse icon.
     * @return vint32_t
     * @since 10
     */
    int32_t SetMouseHotSpot(int32_t windowId, int32_t hotSpotX, int32_t hotSpotY);

    /**
     * @brief Gets the number of the mouse scrolling rows.
     * @param rows Indicates the number of the mouse scrolling rows.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t GetMouseScrollRows(int32_t &rows);

    /**
     * @brief Sets pointer size.
     * @param size Indicates pointer size.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SetPointerSize(int32_t size);

    /**
     * @brief Gets pointer size.
     * @param size Indicates pointer size.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t GetPointerSize(int32_t &size);

    /**
     * @brief Gets surface ID of the cursor.
     * @param size Indicates surface ID of the cursor.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 13
     */
    int32_t GetCursorSurfaceId(uint64_t &surfaceId);

    /**
     * @brief Enable combine key
     * @param enable Indicates whether the combine key is enabled. The value true indicates that the combine key
     * is enabled, and the value false indicates the opposite.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 11
     */
    int32_t EnableCombineKey(bool enable);

    /**
     * @brief Sets mouse primary button.
     * @param primaryButton Indicates the ID of the mouse primary button.The value 0 indicates that
     * the primary button is left button.The value 1 indicates that the primary button is right button.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SetMousePrimaryButton(int32_t primaryButton);

    /**
     * @brief Gets mouse primary button.
     * @param primaryButton Indicates the ID of the mouse primary button.The value 0 indicates that
     * the primary button is left button.The value 1 indicates that the primary button is right button.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t GetMousePrimaryButton(int32_t &primaryButton);

    /**
     * @brief Sets whether the mouse hover scroll is enabled in inactive window.
     * @param state Indicates whether the mouse hover scroll is enabled in inactive window. The value true
     * indicates that the mouse hover scroll is enabled, and the value false indicates the opposite.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SetHoverScrollState(bool state);

    /**
     * @brief Gets a status whether the mouse hover scroll is enabled in inactive window.
     * @param state Indicates whether the mouse hover scroll is enabled in inactive window. The value true
     * indicates that the mouse hover scroll is enabled, and the value false indicates the opposite.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t GetHoverScrollState(bool &state);

    /**
     * @brief Sets whether the pointer icon is visible.
     * @param visible Indicates whether the pointer icon is visible. The value <b>true</b> indicates that
     * the pointer icon is visible, and the value <b>false</b> indicates the opposite.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SetPointerVisible(bool visible, int32_t priority = 0);

    /**
     * @brief Checks whether the pointer icon is visible.
     * @return Returns <b>true</b> if the pointer icon is visible; returns <b>false</b> otherwise.
     * @since 9
     */
    bool IsPointerVisible();

    /**
     * @brief Sets the mouse pointer style.
     * @param windowId Indicates the ID of the window for which the mouse pointer style is set.
     * @param pointerStyle Indicates the ID of the mouse pointer style.
     * @return Returns <b>0</b> if the operation is successful; returns an error code otherwise.
     * @since 9
     */
    int32_t SetPointerStyle(int32_t windowId, PointerStyle pointerStyle, bool isUiExtension = false);

    /**
     * @brief Obtains the mouse pointer style.
     * @param windowId Indicates the ID of the window for which the mouse pointer style is obtained.
     * @param pointerStyle Indicates the ID of the mouse pointer style.
     * @return Returns <b>0</b> if the operation is successful; returns an error code otherwise.
     * @since 9
     */
    int32_t GetPointerStyle(int32_t windowId, PointerStyle &pointerStyle, bool isUiExtension = false);

    /**
     * @brief Sets pointer color.
     * @param color Indicates pointer color.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SetPointerColor(int32_t color);

    /**
     * @brief Gets pointer color.
     * @param color Indicates pointer color.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t GetPointerColor(int32_t &color);

    /**
     * @brief Sets the mouse pointer speed, which ranges from 1 to 11.
     * @param speed Indicates the mouse pointer speed to set.
     * @return Returns <b>RET_OK</b> if success; returns <b>RET_ERR</b> otherwise.
     * @since 9
     */
    int32_t SetPointerSpeed(int32_t speed);

    /**
     * @brief Obtains the mouse pointer speed.
     * @param speed Indicates the mouse pointer speed to get.
     * @return Returns the mouse pointer speed if the operation is successful; returns <b>RET_ERR</b> otherwise.
     * @since 9
     */
    int32_t GetPointerSpeed(int32_t &speed);

    /**
     * @brief Queries the keyboard type.
     * @param deviceId Indicates the keyboard device ID.
     * @param callback Callback used to return the keyboard type.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t GetKeyboardType(int32_t deviceId, std::function<void(int32_t)> callback);

    /**
     * @brief Sets the observer for events indicating that the application does not respond.
     * @param observer Indicates the observer for events indicating that the application does not respond.
     * @return void
     * @since 9
     */
    void SetAnrObserver(std::shared_ptr<IAnrObserver> observer);

    /**
     * @brief Obtains the enablement status of the specified function key on the keyboard.
     * @param funcKey Indicates the function key. Currently, the following function keys are supported:
     * NUM_LOCK_FUNCTION_KEY
     * CAPS_LOCK_FUNCTION_KEY
     * SCROLL_LOCK_FUNCTION_KEY
     * @return Returns <b>true</b> if the function key is enabled;
     * returns <b>false</b> otherwise.
     */
    int32_t GetFunctionKeyState(int32_t funcKey, bool &state);

    /**
     * @brief Sets the enablement status of the specified function key on the keyboard.
     * @param funcKey Indicates the function key. Currently, the following function keys are supported:
     * NUM_LOCK_FUNCTION_KEY
     * CAPS_LOCK_FUNCTION_KEY
     * SCROLL_LOCK_FUNCTION_KEY
     * @param isEnable Indicates the enablement status to set.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     */
    int32_t SetFunctionKeyState(int32_t funcKey, bool enable);

    /**
     * @brief Sets the absolute coordinate of mouse.
     * @param x Specifies the x coordinate of the mouse to be set.
     * @param y Specifies the y coordinate of the mouse to be set.
     * @param displayId Specifies the id of the physical screen to be set.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SetPointerLocation(int32_t x, int32_t y, int32_t displayId = -1);

    /**
     * @brief Query pointer location.
     * @param displayId The displayId for the pointer location.
     * @param displayX The displayX for the pointer location.
     * @param displayY The displayY for the pointer location.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 20
     */
    int32_t GetPointerLocation(int32_t &displayId, double &displayX, double &displayY);

    /**
     * @brief 进入捕获模式
     * @param windowId 窗口id.
     * @return 进入捕获模式成功或失败.
     * @since 9
     */
    int32_t EnterCaptureMode(int32_t windowId);

    /**
     * @brief 退出捕获模式
     * @param windowId 窗口id.
     * @return 退出捕获模式成功或失败.
     * @since 9
     */
    int32_t LeaveCaptureMode(int32_t windowId);

    int32_t GetWindowPid(int32_t windowId);

    /**
     * @brief pointer event添加辅助信息
     * @param extraData 添加的信息.
     * @return 设置拖拽数据成功或失败.
     * @since 9
     */
    int32_t AppendExtraData(const ExtraData& extraData);

    /**
     * @brief 使能或者禁用输入设备
     * @param enable 输入设备的使能状态
     * @return 返回0表示接口调用成功，否则，表示接口调用失败。
     * @since 9
     */
    int32_t EnableInputDevice(bool enable);

     /**
     * @brief 自定义设置快捷键拉起ability延迟时间
     * @param businessId 应用在ability_launch_config.json中注册的唯一标识符.
     * @param delay 延迟时间 0-4000ms
     * @return 设置快捷键拉起ability延迟时间成功或失败
     * @since 10
     */
    int32_t SetKeyDownDuration(const std::string &businessId, int32_t delay);

    /**
     * @brief Sets the keyboard repeat delay, which ranges from 300 to 1000.
     * @param delay Indicates the keyboard repeat delay to set.
     * @return Returns <b>RET_OK</b> if success; returns <b>RET_ERR</b> otherwise.
     * @since 10
     */
    int32_t SetKeyboardRepeatDelay(int32_t delay);

    /**
     * @brief Sets the keyboard repeat rate, which ranges from 36 to 100.
     * @param rate Indicates the keyboard repeat rate to set.
     * @return Returns <b>RET_OK</b> if success; returns <b>RET_ERR</b> otherwise.
     * @since 10
     */
    int32_t SetKeyboardRepeatRate(int32_t rate);

    /**
     * @brief Gets the keyboard repeat delay.
     * @param callback Callback used to return the keyboard repeat delay.
     * @return Returns <b>RET_OK</b> if success; returns <b>RET_ERR</b> otherwise.
     * @since 10
     */
    int32_t GetKeyboardRepeatDelay(std::function<void(int32_t)> callback);

    /**
     * @brief Gets the keyboard repeat rate.
     * @param callback Callback used to return the keyboard repeat rate.
     * @return Returns <b>RET_OK</b> if success; returns <b>RET_ERR</b> otherwise.
     * @since 10
     */
    int32_t GetKeyboardRepeatRate(std::function<void(int32_t)> callback);

    /**
     * @brief Set the switch of touchpad scroll.
     * @param switchFlag Indicates the touchpad scroll switch state.
     * @return if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SetTouchpadScrollSwitch(bool switchFlag);

    /**
     * @brief Get the switch of touchpad scroll.
     * @param switchFlag Indicates the touchpad scroll switch state.
     * @return if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t GetTouchpadScrollSwitch(bool &switchFlag);

    /**
     * @brief Set the switch of touchpad scroll direction.
     * @param state Indicates the touchpad scroll switch direction state.
     * @return if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SetTouchpadScrollDirection(bool state);

    /**
     * @brief Get the switch of touchpad scroll direction.
     * @param state Indicates the touchpad scroll switch direction state.
     * @return if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t GetTouchpadScrollDirection(bool &state);

    /**
     * @brief Set the switch of touchpad tap.
     * @param switchFlag Indicates the touchpad tap switch state.
     * @return if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SetTouchpadTapSwitch(bool switchFlag);

    /**
     * @brief Get the switch of touchpad tap.
     * @param switchFlag Indicates the touchpad tap switch state.
     * @return if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t GetTouchpadTapSwitch(bool &switchFlag);

    /**
     * @brief Set the touchpad poniter speed.
     * @param speed Indicates the touchpad pointer speed.
     * @return if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SetTouchpadPointerSpeed(int32_t speed);

    /**
     * @brief Get the touchpad poniter speed.
     * @param speed Indicates the touchpad pointer speed.
     * @return if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t GetTouchpadPointerSpeed(int32_t &speed);
    int32_t GetTouchpadCDG(TouchpadCDG &touchpadCDG);

    /**
     * @brief Set the switch of touchpad pinch.
     * @param switchFlag Indicates the touchpad pinch switch state.
     * @return if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SetTouchpadPinchSwitch(bool switchFlag);

    /**
     * @brief Get the switch of touchpad pinch.
     * @param switchFlag Indicates the touchpad pinch switch state.
     * @return if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t GetTouchpadPinchSwitch(bool &switchFlag);

    /**
     * @brief Set the switch of touchpad swipe.
     * @param switchFlag Indicates the touchpad swipe switch state.
     * @return if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SetTouchpadSwipeSwitch(bool switchFlag);

    /**
     * @brief Get the switch of touchpad swipe.
     * @param switchFlag Indicates the touchpad swipe switch state.
     * @return if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t GetTouchpadSwipeSwitch(bool &switchFlag);

    /**
     * @brief Set the touchpad right click type.
     * @param type Indicates the touchpad right menu type.
     * @return if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SetTouchpadRightClickType(int32_t type);

    /**
     * @brief Get the touchpad right click type.
     * @param type Indicates the touchpad right menu type.
     * @return if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t GetTouchpadRightClickType(int32_t &type);

     /**
     * @brief Turn on or off hard cursor statistics.
     * @param frameCount Counting the frame rate of continuous mouse movement.
     * @param frameCount Statistics of mouse continuous movement synchronization frame rate.
     * @return if success; returns a non-0 value otherwise.
     * @since 12
     */
    int32_t EnableHardwareCursorStats(bool enable);
    /**
     * @brief Get the mouse hard cursor information.
     * @param frameCount Counting the frame rate of continuous mouse movement.
     * @param frameCount Statistics of mouse continuous movement synchronization frame rate.
     * @return if success; returns a non-0 value otherwise.
     * @since 12
     */
    int32_t GetHardwareCursorStats(uint32_t &frameCount, uint32_t &vsyncCount);

    /**
     * @brief Get the pointer snapshot.
     * @param pixelMapPtr Indicates the image resource for this mouse icon. which realtype must be
     * std::shared_ptr<OHOS::Media::PixelMap>*.
     * @return if success; returns a non-0 value otherwise.
     * @since 12
     */
    int32_t GetPointerSnapshot(void *pixelMapPtr);

    /**
     * @brief Sets the number of the touchpad scrolling rows.
     * @param rows Indicates the number of the touchpad scrolling rows.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 12
     */
    int32_t SetTouchpadScrollRows(int32_t rows);

    /**
     * @brief Gets the number of the touchpad scrolling rows.
     * @param rows Indicates the number of the touchpad scrolling rows.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 12
     */
    int32_t GetTouchpadScrollRows(int32_t &rows);

    /**
     * @brief ClearWindowPointerStyle.
     * @param pid Indicates pid.
     * @param windowId Indicates windowId.
     * @return void.
     * @since 9
     */
    void ClearWindowPointerStyle(int32_t pid, int32_t windowId);

    /**
     * @brief Sets whether shield key event interception, only support shield key event.
     * @param shieldMode Indicates shield mode.
     * @param isShield Indicates whether key event handler chain is shield. The value <b>true</b> indicates that
     * the key event build chain is shield, all key events derictly dispatch to window,
     * if the value <b>false</b> indicates not shield key event interception, handle by the chain.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 9
     */
    int32_t SetShieldStatus(int32_t shieldMode, bool isShield);

    /**
    * Gets shield event interception status corresponding to shield mode
    *
    * @param shieldMode - Accroding the shield mode select shield status.
    * @param isShield - shield status of shield mode param.
    * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
    * @since 9
    */
    int32_t GetShieldStatus(int32_t shieldMode, bool &isShield);

    int32_t MarkProcessed(int32_t eventId, int64_t actionTime, bool enable = true);

    int32_t GetKeyState(std::vector<int32_t> &pressedKeys, std::map<int32_t, int32_t> &specialKeysState);

    void Authorize(bool isAuthorize);
#ifdef OHOS_BUILD_ENABLE_SECURITY_COMPONENT
    /**
     * @brief Sets the enhance config of the security component.
     * @param cfg Indicates the security component enhance config.
     * @param cfgLen Indicates the security component enhance config len.
     * @return void.
     * @since 9
     */
    void SetEnhanceConfig(uint8_t *cfg, uint32_t cfgLen);
#endif // OHOS_BUILD_ENABLE_SECURITY_COMPONENT

#ifdef OHOS_BUILD_ENABLE_ANCO
    void SimulateInputEventExt(std::shared_ptr<KeyEvent> keyEvent);
    void SimulateInputEventExt(std::shared_ptr<PointerEvent> pointerEvent);
#endif // OHOS_BUILD_ENABLE_ANCO

    void AddServiceWatcher(std::shared_ptr<IInputServiceWatcher> watcher);
    void RemoveServiceWatcher(std::shared_ptr<IInputServiceWatcher> watcher);

    /**
     * @brief Set the switch of touchpad rotate.
     * @param rotateSwitch Indicates the touchpad rotate switch state.
     * @return 0 if success; returns a non-0 value otherwise.
     * @since 11
     */
    int32_t SetTouchpadRotateSwitch(bool rotateSwitch);

    /**
     * @brief Get the switch of touchpad rotate.
     * @param rotateSwitch Indicates the touchpad rotate switch state.
     * @return 0 if success; returns a non-0 value otherwise.
     * @since 11
     */
    int32_t GetTouchpadRotateSwitch(bool &rotateSwitch);

    /**
     * @brief Set the switch of touchpad double tap and drag.
     * @param switchFlag Indicates the touchpad double tap and drag switch state.
     *  true: user can use double tap and drag function. otherwise can't use
     * @return if success; returns a non-0 value otherwise.
     * @since 12
     */
    int32_t SetTouchpadDoubleTapAndDragState(bool switchFlag);

    /**
     * @brief Get the switch of touchpad double tap and drag.
     * @param switchFlag Indicates the touchpad double tap and drag switch state.
     * true: user can use double tap and drag function. otherwise can't use
     * @return if success; returns a non-0 value otherwise.
     * @since 12
     */
    int32_t GetTouchpadDoubleTapAndDragState(bool &switchFlag);

    /**
     * @brief Set touch move event filters.
     * @param flag if set move event filters or not.
     * @return if success; returns a non-0 value otherwise.
     * @since 12
     */
    int32_t SetMoveEventFilters(bool flag);

    /**
     * @brief Get whether System has IrEmitter.
     * @param hasIrEmitter the para takes the value which Indicates the device has IrEmitter or not.
     * @return 0 if success; returns a non-0 value otherwise.
     * @since 12
     */
    int32_t HasIrEmitter(bool &hasIrEmitter);

    /**
     * @brief Get InfraredFrequency of the IrEmitter in device.
     * @param requencys take out the IrEmitter's Frequency.
     * @return 0 if success; returns a non-0 value otherwise.
     * @since 12
     */
    int32_t GetInfraredFrequencies(std::vector<InfraredFrequency>& requencys);

    /**
     * @brief user IrEmitter with parameter number and pattern.
     * @param number Frequency of IrEmitter works .
     * @param pattern Pattern of signal transmission in alternate on/off mode, in microseconds.
     * @return 0 if success; returns a non-0 value otherwise.
     * @since 12
     */
    int32_t TransmitInfrared(int64_t number, std::vector<int64_t>& pattern);

#ifdef OHOS_BUILD_ENABLE_VKEYBOARD
    int32_t CreateVKeyboardDevice(sptr<IRemoteObject> &vkeyboardDevice);
#endif // OHOS_BUILD_ENABLE_VKEYBOARD

    int32_t SetCurrentUser(int32_t userId);
    
    /**
     * @brief Set the switch of touchpad three finger tap.
     * @param switchFlag Indicates the touchpad three finger tap switch state.
     *  true: user can use three finger function. otherwise can't use
     * @return if success; returns a non-0 value otherwise.
     * @since 12
     */
    int32_t SetTouchpadThreeFingersTapSwitch(bool switchFlag);

    /**
     * @brief Get the switch of touchpad three finger tap.
     * @param switchFlag Indicates the touchpad three finger tap switch state.
     * true: user can use three finger function. otherwise can't use
     * @return if success; returns a non-0 value otherwise.
     * @since 12
     */
    int32_t GetTouchpadThreeFingersTapSwitch(bool &switchFlag);
    
    int32_t GetWinSyncBatchSize(int32_t maxAreasCount, int32_t displayCount);
    
    /**
     * @brief 添加虚拟输入设备
     * @param device 输入设备信息
     * @param deviceId 出参，所创建的虚拟输入设备对应的设备Id
     * @return 返回0表示接口调用成功，否则，表示接口调用失败。
     * @since 12
     */
    int32_t AddVirtualInputDevice(std::shared_ptr<InputDevice> device, int32_t &deviceId);

    /**
     * @brief 移除虚拟输入设备
     * @param deviceId 要移除的虚拟输入设备对应的设备Id
     * @return 返回0表示接口调用成功，否则，表示接口调用失败。
     * @since 12
     */
    int32_t RemoveVirtualInputDevice(int32_t deviceId);

    int32_t AncoAddConsumer(std::shared_ptr<IAncoConsumer> consumer);
    int32_t AncoRemoveConsumer(std::shared_ptr<IAncoConsumer> consumer);

    int32_t SkipPointerLayer(bool isSkip);

    int32_t RegisterWindowStateErrorCallback(std::function<void(int32_t, int32_t)> callback);
    /**
     * @brief Get Interval Since Last Input.
     * @param timeInterval the value which Indicates the time interval since last input.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 13
     */
    int32_t GetIntervalSinceLastInput(int64_t &timeInterval);

    int32_t GetAllSystemHotkeys(std::vector<std::unique_ptr<KeyOption>> &keyOptions, int32_t &count);

    /**
     * @brief Converted to a Capi-defined key action value.
     * @param keyAction The key action value of the return value of inner api.
     * @return Returns Capi-defined key action value if success; returns a negative number value otherwise.
     * @since 13
     */
    int32_t ConvertToCapiKeyAction(int32_t keyAction);

    /**
     * @brief 添加手势事件监视器。添加这样的监视器后，手势事件会被分发到到监视器。
     * @param consumer 表示手势事件监视器。手势事件产生后，将调用监视器对象的函数。
     * @param type 表示监视的手势类型。
     * @param fingers 表示监视的手势达成的手指数量。
     * @return 返回监视器ID，该ID唯一标识进程中的监视器。如果value的值大于等于0，表示添加成功。否则，添加监视器失败。
     * @since 13
     */
    int32_t AddGestureMonitor(std::shared_ptr<IInputEventConsumer> consumer,
        TouchGestureType type, int32_t fingers = 0);

    /**
     * @brief 删除一个手势监听器。
     * @param monitorId 表示手势事件监视器ID，即AddGestureMonitor的返回值。
     * @return 如果成功，则返回0；否则返回非0值。
     * @since 13
     */
    int32_t RemoveGestureMonitor(int32_t monitorId);

    /**
     * @brief Turn on/off the device input data update.
     * @param deviceID input device id.
     * @param enable Indicates the device input value.
     * @return if success; returns a non-0 value otherwise.
     * @since 14
     */
    int32_t SetInputDeviceEnabled(int32_t deviceId, bool enable, std::function<void(int32_t)> callback);

    /**
     * @brief shift AppPointerEvent from source window to target window
     * @param param - param for shift pointer event.
     * @param autoGenDown - send down event if true.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 13
     */
    int32_t ShiftAppPointerEvent(const ShiftWindowParam &param, bool autoGenDown = true);

    /**
     * @brief Sets the custom cursor. You can set whether to adjust the cursor size based on the system settings.
     * @param windowId Indicates the windowId of the window
     * @param cursor Custom cursor, including the custom cursor resource and focus position.
     * @param options Custom cursor option
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 14
     */
    int32_t SetCustomCursor(int32_t windowId, CustomCursor cursor, CursorOptions options);

    /**
     * @brief Check whether the touch position is a knuckle event.
     * @param pointX X coordinate of the touch position
     * @param pointY Y coordinate of the touch position
     * @param isKnuckleType true is kunckle type
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 14
     */
    int32_t CheckKnuckleEvent(float pointX, float pointY, bool &isKnuckleType);

    void SetMultiWindowScreenId(uint64_t screenId, uint64_t displayNodeScreenId);

    /**
     * @brief Enables or disables the knuckle switch.
     * @param knuckleSwitch Set to true to enable, false to disable.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 18
    */
    int32_t SetKnuckleSwitch(bool knuckleSwitch);

    /**
     * @brief 拉起小艺识屏
     * @return 如果成功，则返回0；否则返回非0值。
     * @since 14
     */
    int32_t LaunchAiScreenAbility();

    /**
     * @brief Get max Multi-Touch Points Num Supported.
     * @param pointNum the value which Indicates the max Multi-Touch point num supported.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 18
    */
    int32_t GetMaxMultiTouchPointNum(int32_t &pointNum);

    /**
     * @brief Allows the service party to specify the input event of the specified touchscreen input device.
     * @param deviceName List of supported device names.
     * @param consumer Indicates the input event consumer. After an input event is generated,
     * the functions of the consumer object will be called.
     * @return Returns <b>0</b> if success; returns a non-0 value otherwise.
     * @since 20
    */
    int32_t SetInputDeviceConsumer(const std::vector<std::string>& deviceName,
        std::shared_ptr<IInputEventConsumer> consumer);

    /**
     * @brief Subscribes to the input active that meets a specific condition. When such an event occurs,
     * the <b>callback</b> specified is invoked to process the event.
     * A process can only support a maximum of one interface call.
     * @param inputEventConsumer Indicates the callback.
     * @param interval Indicate the interval time.
     * When the interval value is less than 0, the value is invalid.
     * When it is equal to 0, no filtering is performed.
     * The effective range of filtering values is greater than or equal to 500ms and less than or equal to 2000ms.
     * If the filtering value is greater than 0 and less than 500ms, the filtering interval is 500ms.
     * If the filtering value is greater than 2000ms, the filtering interval is 2000ms.
     * @return Returns the subscribe ID, which uniquely identifies a subscribe id in the process.
     * If the value is greater than or equal to <b>0</b>
     * the subscription is successful. Otherwise, the subscription fails.
     * @since 20
    */
    int32_t SubscribeInputActive(std::shared_ptr<IInputEventConsumer> inputEventConsumer, int64_t interval);

    /**
     * @brief Unsubscribes from input active.
     * @param subscribeId. Indicates the subscription ID, which is the return value of <b>SubscribeInputActive</b>.
     * @return void.
     * @since 20
    */
    void UnsubscribeInputActive(int32_t subscribeId);

    /**
    * @brief Enables or disables the mouse acceleration feature.
    * @note Only users with specific shell permissions are allowed to call this interface.
    * @param deviceId The identifier of the input device.
    * @param enable A boolean value; true to enable acceleration, false to disable it.
    * @return Returns 0 on success, or a non-zero error code on failure.
    * @since 20
    */
    int32_t SetMouseAccelerateMotionSwitch(int32_t deviceId, bool enable);

    /**
    * @brief Temporarily enables or disables screen capture and screen recording permissions.
    * @param permissionType The type of permission to switch (e.g., SCREENSHOT or SCREENRECORD).
    * @param enable Set to true to enable the permission, false to disable it.
    * @return Returns <b>0</b> if successful; returns a non-0 value otherwise.
    * @since 20
    */
    int32_t SwitchScreenCapturePermission(uint32_t permissionType, bool enable);
    int32_t RequestInjection(int32_t &status, int32_t &reqId);
    int32_t QueryAuthorizedStatus(int32_t &status);
    void RequestInjectionCallback(int32_t reqId, int32_t status);
    void InsertRequestInjectionCallback(int32_t reqId, std::function<void(int32_t)> fun);

    /**
    * @brief When the collaborative application is disconnected, clear the mouse hide flag.
    * @param eventId The collaborative application event id.
    * @return Returns <b>0</b> if successful; returns a non-0 value otherwise.
    * @since 20
    */
    int32_t ClearMouseHideFlag(int32_t eventId);

    /**
    * @brief Retrieves a specified number of pointer event records.
    * @param count The number of pointer event records to retrieve.
    * @param pointerList A reference to a vector that will store the retriebed pointer events.
    * @return Returns <b>0</b> if successful; returns a non-0 value otherwise.
    * @since 20
    */
    int32_t QueryPointerRecord(int32_t count, std::vector<std::shared_ptr<PointerEvent>> &pointerList);
private:
    InputManager() = default;
    DISALLOW_COPY_AND_MOVE(InputManager);
    static InputManager *instance_;
    std::mutex mutexMapCallBack_;
    std::map<int32_t, std::function<void(int32_t)>> mapCallBack_;
};
} // namespace MMI
} // namespace OHOS
#endif // INPUT_MANAGER_H
