/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "mmi_service.h"

#ifdef OHOS_BUILD_PC_PRIORITY
#include <sched.h>
#endif // OHOS_BUILD_PC_PRIORITY
#include <sys/signalfd.h>
#include <csignal>
#ifdef OHOS_RSS_CLIENT
#include <unordered_map>
#endif // OHOS_RSS_CLIENT

#include "ability_manager_client.h"
#include "anr_manager.h"
#include "app_state_observer.h"
#include "device_event_monitor.h"
#include "dfx_dump_catcher.h"
#include "dfx_hisysevent.h"

#ifdef OHOS_BUILD_ENABLE_KEYBOARD
#include "display_event_monitor.h"
#endif // OHOS_BUILD_ENABLE_KEYBOARD
#include "event_dump.h"
#include "event_statistic.h"
#include "event_log_helper.h"
#include "ffrt.h"
#ifdef OHOS_BUILD_ENABLE_FINGERSENSE_WRAPPER
#include "fingersense_wrapper.h"
#endif // OHOS_BUILD_ENABLE_FINGERSENSE_WRAPPER
#ifdef OHOS_BUILD_ENABLE_GESTURESENSE_WRAPPER
#include "gesturesense_wrapper.h"
#endif // OHOS_BUILD_ENABLE_GESTURESENSE_WRAPPER
#ifndef OHOS_BUILD_ENABLE_WATCH
#include "infrared_emitter_controller.h"
#endif // OHOS_BUILD_ENABLE_WATCH
#include "ipc_skeleton.h"
#include "i_pointer_drawing_manager.h"
#include "i_preference_manager.h"
#include "key_auto_repeat.h"
#ifdef SHORTCUT_KEY_MANAGER_ENABLED
#include "key_shortcut_manager.h"
#endif // SHORTCUT_KEY_MANAGER_ENABLED
#include "permission_helper.h"
#include "touch_event_normalize.h"
#if defined(OHOS_BUILD_ENABLE_TOUCH) && defined(OHOS_BUILD_ENABLE_MONITOR)
#include "touch_gesture_manager.h"
#endif // defined(OHOS_BUILD_ENABLE_TOUCH) && defined(OHOS_BUILD_ENABLE_MONITOR)
#include "util_ex.h"
#include "xcollie/xcollie.h"

#ifdef OHOS_RSS_CLIENT
#include "res_sched_client.h"
#include "res_type.h"
#include "system_ability_definition.h"
#endif // OHOS_RSS_CLIENT
#include "setting_datashare.h"
#include "touch_drawing_manager.h"
#ifdef OHOS_BUILD_ENABLE_ANCO
#include "app_mgr_client.h"
#include "running_process_info.h"
#endif // OHOS_BUILD_ENABLE_ANCO

#ifdef PLAYER_FRAMEWORK_EXISTS
#include "input_screen_capture_agent.h"
#endif // PLAYER_FRAMEWORK_EXISTS
#include "tablet_subscriber_handler.h"
#undef MMI_LOG_TAG
#define MMI_LOG_TAG "MMIService"
#undef MMI_LOG_DOMAIN
#define MMI_LOG_DOMAIN MMI_LOG_SERVER

namespace OHOS {
namespace MMI {
namespace {
std::mutex g_instanceMutex;
MMIService* g_MMIService;
const std::string DEF_INPUT_SEAT { "seat0" };
const char* THREAD_NAME { "mmi_service" };
constexpr int32_t WATCHDOG_INTERVAL_TIME { 30000 };
[[ maybe_unused ]] constexpr int32_t WATCHDOG_DELAY_TIME { 40000 };
constexpr int32_t RELOAD_DEVICE_TIME { 2000 };
[[ maybe_unused ]] constexpr int32_t WATCHDOG_WARNTIME { 6000 };
[[ maybe_unused ]] constexpr int32_t WATCHDOG_BLOCKTIME { 3000 };
constexpr int32_t REMOVE_OBSERVER { -2 };
constexpr int32_t REPEAT_COUNT { 2 };
constexpr int32_t UNSUBSCRIBED { -1 };
constexpr int32_t UNOBSERVED { -1 };
constexpr int32_t SUBSCRIBED { 1 };
[[ maybe_unused ]] constexpr int32_t DISTRIBUTE_TIME { 1000 }; // 1000ms
constexpr int32_t COMMON_PARAMETER_ERROR { 401 };
constexpr size_t MAX_FRAME_NUMS { 100 };
constexpr int32_t THREAD_BLOCK_TIMER_SPAN_S { 3 };
constexpr int32_t PRINT_INTERVAL_TIME { 30000 };
constexpr int32_t RETRY_CHECK_TIMES { 5 };
constexpr int32_t CHECK_EEVENT_INTERVAL_TIME { 4000 };
constexpr int32_t MAX_MULTI_TOUCH_POINT_NUM { 10 };
const std::string PRODUCT_DEVICE_TYPE = system::GetParameter("const.product.devicetype", "unknown");
const std::string PRODUCT_TYPE_PC = "2in1";
const int32_t ERROR_WINDOW_ID_PERMISSION_DENIED = 26500001;
const std::set<int32_t> g_keyCodeValueSet = {
#ifndef OHOS_BUILD_ENABLE_WATCH
    KeyEvent::KEYCODE_FN, KeyEvent::KEYCODE_DPAD_UP, KeyEvent::KEYCODE_DPAD_DOWN, KeyEvent::KEYCODE_DPAD_LEFT,
    KeyEvent::KEYCODE_DPAD_RIGHT, KeyEvent::KEYCODE_ALT_LEFT, KeyEvent::KEYCODE_ALT_RIGHT,
    KeyEvent::KEYCODE_SHIFT_LEFT, KeyEvent::KEYCODE_SHIFT_RIGHT, KeyEvent::KEYCODE_TAB, KeyEvent::KEYCODE_ENTER,
    KeyEvent::KEYCODE_DEL, KeyEvent::KEYCODE_MENU, KeyEvent::KEYCODE_PAGE_UP, KeyEvent::KEYCODE_PAGE_DOWN,
    KeyEvent::KEYCODE_ESCAPE, KeyEvent::KEYCODE_FORWARD_DEL, KeyEvent::KEYCODE_CTRL_LEFT, KeyEvent::KEYCODE_CTRL_RIGHT,
    KeyEvent::KEYCODE_CAPS_LOCK, KeyEvent::KEYCODE_SCROLL_LOCK, KeyEvent::KEYCODE_META_LEFT,
    KeyEvent::KEYCODE_META_RIGHT, KeyEvent::KEYCODE_SYSRQ, KeyEvent::KEYCODE_BREAK, KeyEvent::KEYCODE_MOVE_HOME,
    KeyEvent::KEYCODE_MOVE_END, KeyEvent::KEYCODE_INSERT, KeyEvent::KEYCODE_F1, KeyEvent::KEYCODE_F2,
    KeyEvent::KEYCODE_F3, KeyEvent::KEYCODE_F4, KeyEvent::KEYCODE_F5, KeyEvent::KEYCODE_F6, KeyEvent::KEYCODE_F7,
    KeyEvent::KEYCODE_F8, KeyEvent::KEYCODE_F9, KeyEvent::KEYCODE_F10, KeyEvent::KEYCODE_F11, KeyEvent::KEYCODE_F12,
    KeyEvent::KEYCODE_NUM_LOCK
#endif // OHOS_BUILD_ENABLE_WATCH
};
#ifdef OHOS_BUILD_ENABLE_ANCO
constexpr int32_t DEFAULT_USER_ID { 100 };
#endif // OHOS_BUILD_ENABLE_ANCO
#ifdef OHOS_BUILD_ENABLE_VKEYBOARD
const std::string DEVICE_TYPE_HPR { "HPR" };
const std::string PRODUCT_TYPE = OHOS::system::GetParameter("const.build.product", "HYM");
// Define vkeyboard functions from vendor
const std::string VKEYBOARD_PATH { "libvkeyboard_device.z.so" };
void* g_VKeyboardHandle = nullptr;
typedef int32_t (*HANDLE_TOUCHPOINT_TYPE)(
    double screenX, double screenY, int touchId, int32_t eventType, double touchPressure,
    int32_t longAxis, int32_t shortAxis);
HANDLE_TOUCHPOINT_TYPE handleTouchPoint_ = nullptr;
typedef int32_t (*STATEMACINEMESSAGQUEUE_GETLIBINPUTMESSAGE_TYPE)(
    int& delayMs, int& toggleCodeSecond, int& keyCode);
STATEMACINEMESSAGQUEUE_GETLIBINPUTMESSAGE_TYPE statemachineMessageQueue_getLibinputMessage_ = nullptr;
typedef void (*TRACKPADENGINE_GETALLTOUCHMESSAGE_TYPE)(
    std::vector<std::vector<int32_t>>& retMsgList);
TRACKPADENGINE_GETALLTOUCHMESSAGE_TYPE trackPadEngine_getAllTouchMessage_ = nullptr;
typedef void (*TRACKPADENGINE_CLEARTOUCHMESSAGE_TYPE)();
TRACKPADENGINE_CLEARTOUCHMESSAGE_TYPE trackPadEngine_clearTouchMessage_ = nullptr;
typedef void (*TRACKPADENGINE_GETALLKEYMESSAGE_TYPE)(
    std::vector<std::vector<int32_t>>& retMsgList);
TRACKPADENGINE_GETALLKEYMESSAGE_TYPE trackPadEngine_getAllKeyMessage_ = nullptr;
typedef void (*TRACKPADENGINE_CLEARKEYMESSAGE_TYPE)();
TRACKPADENGINE_CLEARKEYMESSAGE_TYPE trackPadEngine_clearKeyMessage_ = nullptr;
typedef int32_t (*VKEYBOARD_CREATEVKEYBOARDDEVICE_TYPE)(IRemoteObject* &vkeyboardDevice);
VKEYBOARD_CREATEVKEYBOARDDEVICE_TYPE vkeyboard_createVKeyboardDevice_ = nullptr;
typedef int32_t (*VKEYBOARD_ONFUNCKEYEVENT_TYPE)(std::shared_ptr<KeyEvent> funcKeyEvent);
VKEYBOARD_ONFUNCKEYEVENT_TYPE vkeyboard_onFuncKeyEvent_ = nullptr;
typedef void (*VKEYBOARD_HARDWAREKEYEVENTDETECTED_TYPE)();
VKEYBOARD_HARDWAREKEYEVENTDETECTED_TYPE vkeyboard_hardwareKeyEventDetected_ = nullptr;
typedef int32_t (*VKEYBOARD_GETKEYBOARDACTIVATIONSTATE_TYPE)();
VKEYBOARD_GETKEYBOARDACTIVATIONSTATE_TYPE vkeyboard_getKeyboardActivationState_ = nullptr;

#endif // OHOS_BUILD_ENABLE_VKEYBOARD
#ifdef OHOS_BUILD_PC_PRIORITY
constexpr int32_t PC_PRIORITY { 2 };
#endif // OHOS_BUILD_PC_PRIORITY
} // namespace

const bool REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(MMIService::GetInstance());

template <class... Ts> void CheckDefineOutput(const char *fmt, Ts... args)
{
    CHKPV(fmt);
    char buf[MAX_PACKET_BUF_SIZE] = {};
    int32_t ret = snprintf_s(buf, MAX_PACKET_BUF_SIZE, MAX_PACKET_BUF_SIZE - 1, fmt, args...);
    if (ret == -1) {
        KMSG_LOGE("Call snprintf_s failed.ret = %d", ret);
        return;
    }
    KMSG_LOGI("%s", buf);
    MMI_HILOGI("%{public}s", buf);
}

static void CheckDefine()
{
    CheckDefineOutput("ChkDefs:");
#ifdef OHOS_BUILD_ENABLE_POINTER_DRAWING
    CheckDefineOutput("%-40s", "OHOS_BUILD_ENABLE_POINTER_DRAWING");
#endif // OHOS_BUILD_ENABLE_POINTER_DRAWING
#ifdef OHOS_BUILD_ENABLE_INTERCEPTOR
    CheckDefineOutput("%-40s", "OHOS_BUILD_ENABLE_INTERCEPTOR");
#endif // OHOS_BUILD_ENABLE_INTERCEPTOR
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    CheckDefineOutput("%-40s", "OHOS_BUILD_ENABLE_KEYBOARD");
#endif // OHOS_BUILD_ENABLE_KEYBOARD
#ifdef OHOS_BUILD_ENABLE_POINTER
    CheckDefineOutput("%-40s", "OHOS_BUILD_ENABLE_POINTER");
#endif // OHOS_BUILD_ENABLE_POINTER
#ifdef OHOS_BUILD_ENABLE_TOUCH
    CheckDefineOutput("%-40s", "OHOS_BUILD_ENABLE_TOUCH");
#endif // OHOS_BUILD_ENABLE_TOUCH
#ifdef OHOS_BUILD_ENABLE_MONITOR
    CheckDefineOutput("%-40s", "OHOS_BUILD_ENABLE_MONITOR");
#endif // OHOS_BUILD_ENABLE_MONITOR
}

MMIService::MMIService() : SystemAbility(MULTIMODAL_INPUT_CONNECT_SERVICE_ID, true) {}

MMIService::~MMIService()
{
    if (g_MMIService != nullptr) {
        g_MMIService = nullptr;
    }
    MMI_HILOGI("~MMIService");
}

MMIService* MMIService::GetInstance()
{
    if (g_MMIService == nullptr) {
        std::lock_guard<std::mutex> lock(g_instanceMutex);
        if (g_MMIService == nullptr) {
            MMI_HILOGI("New MMIService");
            g_MMIService = new MMIService();
        }
    }
    return g_MMIService;
}

int32_t MMIService::AddEpoll(EpollEventType type, int32_t fd, bool readOnly)
{
    if (type < EPOLL_EVENT_BEGIN || type >= EPOLL_EVENT_END) {
        MMI_HILOGE("Invalid param type");
        return RET_ERR;
    }
    if (fd < 0) {
        MMI_HILOGE("Invalid param fd_");
        return RET_ERR;
    }
    if (mmiFd_ < 0) {
        MMI_HILOGE("Invalid param mmiFd_");
        return RET_ERR;
    }
    auto eventData = std::make_shared<mmi_epoll_event>();
    eventData->fd = fd;
    eventData->event_type = type;
    MMI_HILOGI("The userdata:[fd:%{public}d, type:%{public}d]", eventData->fd, eventData->event_type);

    struct epoll_event ev = {};
    if (readOnly) {
        ev.events = 0;
    } else {
        ev.events = EPOLLIN;
    }
    ev.data.fd = fd;
    auto ret = EpollCtl(fd, EPOLL_CTL_ADD, ev, mmiFd_);
    if (ret < 0) {
        eventData = nullptr;
        ev.data.ptr = nullptr;
        return ret;
    }
    AddEpollEvent(fd, eventData);
    return RET_OK;
}

int32_t MMIService::DelEpoll(EpollEventType type, int32_t fd)
{
    if (type < EPOLL_EVENT_BEGIN || type >= EPOLL_EVENT_END) {
        MMI_HILOGE("Invalid param type");
        return RET_ERR;
    }
    if (fd < 0) {
        MMI_HILOGE("Invalid param fd_");
        return RET_ERR;
    }
    if (mmiFd_ < 0) {
        MMI_HILOGE("Invalid param mmiFd_");
        return RET_ERR;
    }
    struct epoll_event ev = {};
    auto ret = EpollCtl(fd, EPOLL_CTL_DEL, ev, mmiFd_);
    if (ret < 0) {
        MMI_HILOGE("DelEpoll failed");
        return ret;
    }
    return RET_OK;
}

bool MMIService::IsRunning() const
{
    return (state_ == ServiceRunningState::STATE_RUNNING);
}

bool MMIService::InitLibinputService()
{
    if (!(libinputAdapter_.Init([](void *event, int64_t frameTime) { InputHandler->OnEvent(event, frameTime); }))) {
        MMI_HILOGE("Libinput init, bind failed");
        return false;
    }
    auto inputFds = libinputAdapter_.GetInputFds();
    for (auto fd : inputFds) {
        auto ret = AddEpoll(EPOLL_EVENT_INPUT, fd);
        if (ret < 0) {
            MMI_HILOGE("AddEpoll error ret:%{public}d", ret);
            EpollClose();
            return false;
        }
        MMI_HILOGD("AddEpoll, epollfd:%{public}d, fd:%{public}d", mmiFd_, fd);
    }
    return true;
}

bool MMIService::InitService()
{
    MMI_HILOGD("Server msg handler Init");
    sMsgHandler_.Init(*this);
    if (state_ != ServiceRunningState::STATE_NOT_START) {
        MMI_HILOGE("Service running status is not enabled");
        return false;
    }
    if (EpollCreate(MAX_EVENT_SIZE) < 0) {
        MMI_HILOGE("Create epoll failed");
        return false;
    }
    auto ret = AddEpoll(EPOLL_EVENT_SOCKET, epollFd_);
    if (ret < 0) {
        MMI_HILOGE("AddEpoll error ret:%{public}d", ret);
        EpollClose();
        return false;
    }
    state_ = ServiceRunningState::STATE_RUNNING;
    if (!(Publish(this))) {
        state_ = ServiceRunningState::STATE_NOT_START;
        MMI_HILOGE("Service initialization failed");
        EpollClose();
        return false;
    }
    MMI_HILOGI("AddEpoll, epollfd:%{public}d, fd:%{public}d", mmiFd_, epollFd_);
    return true;
}

bool MMIService::InitDelegateTasks()
{
    CALL_DEBUG_ENTER;
    if (!delegateTasks_.Init()) {
        MMI_HILOGE("The delegate task init failed");
        return false;
    }
    auto ret = AddEpoll(EPOLL_EVENT_ETASK, delegateTasks_.GetReadFd());
    if (ret < 0) {
        MMI_HILOGE("AddEpoll error ret:%{public}d", ret);
        EpollClose();
        return false;
    }
    std::function<int32_t(DTaskCallback)> fun = [this](DTaskCallback cb) -> int32_t {
        return delegateTasks_.PostSyncTask(cb);
    };
    std::function<int32_t(DTaskCallback)> asyncFun = [this](DTaskCallback cb) -> int32_t {
        return delegateTasks_.PostAsyncTask(cb);
    };
    delegateInterface_ = std::make_shared<DelegateInterface>(fun, asyncFun);
    delegateInterface_->Init();
    MMI_HILOGI("AddEpoll, epollfd:%{public}d, fd:%{public}d", mmiFd_, delegateTasks_.GetReadFd());
    return true;
}
__attribute__((no_sanitize("cfi")))
int32_t MMIService::Init()
{
    CheckDefine();
    MMI_HILOGD("WindowsManager Init");
    WIN_MGR->Init(*this);
    MMI_HILOGD("NapProcess Init");
    NapProcess::GetInstance()->Init(*this);
    MMI_HILOGD("ANRManager Init");
    ANRMgr->Init(*this);
    MMI_HILOGI("PointerDrawingManager Init");
#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
    if (!IPointerDrawingManager::GetInstance()->Init()) {
        MMI_HILOGE("Pointer draw init failed");
        return POINTER_DRAW_INIT_FAIL;
    }
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING
    mmiFd_ = EpollCreate(MAX_EVENT_SIZE);
    if (mmiFd_ < 0) {
        MMI_HILOGE("Create epoll failed");
        return EPOLL_CREATE_FAIL;
    }
    MMI_HILOGD("Input msg handler init");
    InputHandler->Init(*this);
    MMI_HILOGD("Init DelegateTasks init");
    if (!InitDelegateTasks()) {
        MMI_HILOGE("Delegate tasks init failed");
        return ETASKS_INIT_FAIL;
    }
    MMI_HILOGD("Libinput service init");
    if (!InitLibinputService()) {
        MMI_HILOGE("Libinput init failed");
        return LIBINPUT_INIT_FAIL;
    }
    SetRecvFun([this] (SessionPtr sess, NetPacket& pkt) {sMsgHandler_.OnMsgHandler(sess, pkt);});
    KeyMapMgr->GetConfigKeyValue("default_keymap", KeyMapMgr->GetDefaultKeyId());
    OHOS::system::SetParameter(INPUT_POINTER_DEVICES, "false");
    if (!InitService()) {
        MMI_HILOGE("Saservice init failed");
        return SASERVICE_INIT_FAIL;
    }
    MMI_HILOGI("Set para input.pointer.device false");
    return RET_OK;
}

void MMIService::OnStart()
{
    CHK_PID_AND_TID();
    int32_t ret = Init();
    CHKNOKRV(ret, "Init mmi_service failed");
    MMI_HILOGD("Started successfully");
    AddReloadDeviceTimer();
    auto keyHandler = InputHandler->GetKeyCommandHandler();
    if (keyHandler != nullptr) {
        keyHandler->PreHandleEvent();
    }
    t_ = std::thread([this] {this->OnThread();});
    pthread_setname_np(t_.native_handle(), THREAD_NAME);
    eventMonitorThread_ = std::thread(&EventStatistic::WriteEventFile);
    pthread_setname_np(eventMonitorThread_.native_handle(), "event-monitor");
#ifdef OHOS_RSS_CLIENT
    MMI_HILOGI("Add system ability listener start");
    AddSystemAbilityListener(RES_SCHED_SYS_ABILITY_ID);
    MMI_HILOGI("Add system ability listener success");
#endif // OHOS_RSS_CLIENT
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    MMI_HILOGI("Add SA listener COMMON_EVENT_SERVICE_ID start");
    AddSystemAbilityListener(COMMON_EVENT_SERVICE_ID);
    MMI_HILOGI("Add SA listener COMMON_EVENT_SERVICE_ID success");
#endif // OHOS_BUILD_ENABLE_KEYBOARD
#if defined(OHOS_BUILD_ENABLE_FINGERSENSE_WRAPPER) && defined(OHOS_BUILD_ENABLE_KEYBOARD)
    FINGERSENSE_WRAPPER->InitFingerSenseWrapper();
#endif // OHOS_BUILD_ENABLE_FINGERSENSE_WRAPPER && OHOS_BUILD_ENABLE_KEYBOARD
#ifdef OHOS_BUILD_ENABLE_GESTURESENSE_WRAPPER
    GESTURESENSE_WRAPPER->InitGestureSenseWrapper();
#endif // OHOS_BUILD_ENABLE_GESTURESENSE_WRAPPER
    MMI_HILOGI("Add app manager service listener start");
    AddSystemAbilityListener(APP_MGR_SERVICE_ID);
    APP_OBSERVER_MGR->InitAppStateObserver();
    MMI_HILOGI("Add app manager service listener end");
    AddSystemAbilityListener(RENDER_SERVICE);
    AddAppDebugListener();
    AddSystemAbilityListener(DISPLAY_MANAGER_SERVICE_SA_ID);
#ifdef OHOS_BUILD_ENABLE_ANCO
    InitAncoUds();
#endif // OHOS_BUILD_ENABLE_ANCO
#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
    IPointerDrawingManager::GetInstance()->InitPointerObserver();
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING
    InitPreferences();
#if OHOS_BUILD_ENABLE_POINTER
    bool switchFlag = false;
    TOUCH_EVENT_HDR->GetTouchpadDoubleTapAndDragState(switchFlag);
    TOUCH_EVENT_HDR->SetTouchpadDoubleTapAndDragState(switchFlag);
#endif
    TimerMgr->AddTimer(WATCHDOG_INTERVAL_TIME, -1, [this]() {
        MMI_HILOGI("Set thread status flag to true");
        threadStatusFlag_ = true;
    });
    [[ maybe_unused ]] auto taskFunc = [this]() {
        if (threadStatusFlag_) {
            MMI_HILOGI("Set thread status flag to false");
            threadStatusFlag_ = false;
        } else {
            MMI_HILOGI("Mmi-server Timeout");
        }
    };
    MMI_HILOGI("Run periodical task success");
    InitPrintClientInfo();
}

void MMIService::OnStop()
{
    CHK_PID_AND_TID();
    UdsStop();
    libinputAdapter_.Stop();
    state_ = ServiceRunningState::STATE_NOT_START;
#ifdef OHOS_RSS_CLIENT
    MMI_HILOGI("Remove system ability listener start");
    RemoveSystemAbilityListener(RES_SCHED_SYS_ABILITY_ID);
    MMI_HILOGI("Remove system ability listener success");
#endif // OHOS_RSS_CLIENT
#ifdef OHOS_BUILD_ENABLE_FINGERSENSE_WRAPPER
    RemoveSystemAbilityListener(COMMON_EVENT_SERVICE_ID);
#endif // OHOS_BUILD_ENABLE_FINGERSENSE_WRAPPER
    RemoveSystemAbilityListener(APP_MGR_SERVICE_ID);
    RemoveSystemAbilityListener(RENDER_SERVICE);
    RemoveAppDebugListener();
    RemoveSystemAbilityListener(DISPLAY_MANAGER_SERVICE_SA_ID);
#ifdef OHOS_BUILD_ENABLE_ANCO
    StopAncoUds();
#endif // OHOS_BUILD_ENABLE_ANCO
}

void MMIService::AddAppDebugListener()
{
    CALL_DEBUG_ENTER;
    appDebugListener_ = AppDebugListener::GetInstance();
    auto begin = std::chrono::high_resolution_clock::now();
    auto errCode =
        AAFwk::AbilityManagerClient::GetInstance()->RegisterAppDebugListener(appDebugListener_);
    auto durationMS = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - begin).count();
#ifdef OHOS_BUILD_ENABLE_DFX_RADAR
    DfxHisysevent::ReportApiCallTimes(ApiDurationStatistics::Api::REGISTER_APP_DEBUG_LISTENER, durationMS);
#endif // OHOS_BUILD_ENABLE_DFX_RADAR
    if (errCode != RET_OK) {
        MMI_HILOGE("Call RegisterAppDebugListener failed, errCode:%{public}d", errCode);
    }
}

void MMIService::RemoveAppDebugListener()
{
    CALL_DEBUG_ENTER;
    CHKPV(appDebugListener_);
    auto begin = std::chrono::high_resolution_clock::now();
    auto errCode = AAFwk::AbilityManagerClient::GetInstance()->UnregisterAppDebugListener(appDebugListener_);
    auto durationMS = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - begin).count();
#ifdef OHOS_BUILD_ENABLE_DFX_RADAR
    DfxHisysevent::ReportApiCallTimes(ApiDurationStatistics::Api::REGISTER_APP_DEBUG_LISTENER, durationMS);
#endif // OHOS_BUILD_ENABLE_DFX_RADAR
    if (errCode != RET_OK) {
        MMI_HILOGE("Call UnregisterAppDebugListener failed, errCode:%{public}d", errCode);
    }
}

int32_t MMIService::AllocSocketFd(const std::string &programName, const int32_t moduleType, int32_t &toReturnClientFd,
    int32_t &tokenType)
{
    toReturnClientFd = IMultimodalInputConnect::INVALID_SOCKET_FD;
    int32_t serverFd = IMultimodalInputConnect::INVALID_SOCKET_FD;
    int32_t pid = GetCallingPid();
    int32_t uid = GetCallingUid();
    MMI_HILOGI("Enter, programName:%{public}s, moduleType:%{public}d, pid:%{public}d",
        programName.c_str(), moduleType, pid);
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &programName, moduleType, uid, pid, &serverFd, &toReturnClientFd, &tokenType] {
            return this->AddSocketPairInfo(programName, moduleType, uid, pid, serverFd, toReturnClientFd, tokenType);
        }
        );
    DfxHisysevent::ClientConnectData data = {
        .pid = pid,
        .uid = uid,
        .moduleType = moduleType,
        .programName = programName,
        .serverFd = serverFd
    };
    if (ret != RET_OK) {
        MMI_HILOGE("Call AddSocketPairInfo failed, return:%{public}d", ret);
        DfxHisysevent::OnClientConnect(data, OHOS::HiviewDFX::HiSysEvent::EventType::FAULT);
        return ret;
    }
    MMI_HILOGIK("Leave, programName:%{public}s, moduleType:%{public}d, alloc success", programName.c_str(),
                moduleType);
    DfxHisysevent::OnClientConnect(data, OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR);
    return RET_OK;
}

int32_t MMIService::AddInputEventFilter(sptr<IEventFilter> filter, int32_t filterId, int32_t priority,
    uint32_t deviceTags)
{
    CALL_INFO_TRACE;
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH) || defined(OHOS_BUILD_ENABLE_KEYBOARD)
    CHKPR(filter, ERROR_NULL_POINTER);
    int32_t clientPid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, filter, filterId, priority, deviceTags, clientPid] {
            return sMsgHandler_.AddInputEventFilter(filter, filterId, priority, deviceTags, clientPid);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Add event filter failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH || OHOS_BUILD_ENABLE_KEYBOARD
    return RET_OK;
}

int32_t MMIService::RemoveInputEventFilter(int32_t filterId)
{
    CALL_INFO_TRACE;
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH) || defined(OHOS_BUILD_ENABLE_KEYBOARD)
    int32_t clientPid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, filterId, clientPid] {
            return sMsgHandler_.RemoveInputEventFilter(filterId, clientPid);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Remove event filter failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH || OHOS_BUILD_ENABLE_KEYBOARD
    return RET_OK;
}

void MMIService::OnConnected(SessionPtr s)
{
    CHKPV(s);
    MMI_HILOGI("Get fd:%{public}d", s->GetFd());
#ifdef OHOS_BUILD_ENABLE_ANCO
    if (s->GetProgramName() != SHELL_ASSISTANT) {
        return;
    }
    auto appMgrClient = DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance();
    if (appMgrClient == nullptr) {
        return;
    }
    int32_t userid = WIN_MGR->GetCurrentUserId();
    if (userid < 0) {
        userid = DEFAULT_USER_ID;
    }
    std::vector<AppExecFwk::RunningProcessInfo> info;
    auto begin = std::chrono::high_resolution_clock::now();
    appMgrClient->GetProcessRunningInfosByUserId(info, userid);
    auto durationMS = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - begin).count();
#ifdef OHOS_BUILD_ENABLE_DFX_RADAR
    DfxHisysevent::ReportApiCallTimes(ApiDurationStatistics::Api::GET_PROC_RUNNING_INFOS_BY_UID, durationMS);
#endif // OHOS_BUILD_ENABLE_DFX_RADAR
    for (auto &item : info) {
        if (item.bundleNames.empty()) {
            continue;
        }
        if (SHELL_ASSISTANT == item.bundleNames[0].c_str()) {
            MMI_HILOGW("Record client processes pid %{public}d", item.pid_);
            shellAssitentPid_ = item.pid_;
        }
    }
#endif // OHOS_BUILD_ENABLE_ANCO
}

void MMIService::OnDisconnected(SessionPtr s)
{
    CHKPV(s);
    MMI_HILOGW("Enter, session desc:%{public}s, fd:%{public}d", s->GetDescript().c_str(), s->GetFd());
    auto ret = RemoveInputEventFilter(-1);
    if (ret != RET_OK) {
        MMI_HILOGF("Remove all filter failed, ret:%{public}d", ret);
    }
#ifdef OHOS_BUILD_ENABLE_ANCO
    if (s->GetProgramName() == SHELL_ASSISTANT && shellAssitentPid_ == s->GetPid()) {
        MMI_HILOGW("Clean all shell windows pid:%{public}d", s->GetPid());
        shellAssitentPid_ = -1;
        IInputWindowsManager::GetInstance()->CleanShellWindowIds();
    }
#endif // OHOS_BUILD_ENABLE_ANCO
#ifdef OHOS_BUILD_ENABLE_POINTER
    IPointerDrawingManager::GetInstance()->DeletePointerVisible(s->GetPid());
#endif // OHOS_BUILD_ENABLE_POINTER
}

int32_t MMIService::SetMouseScrollRows(int32_t rows)
{
    CALL_INFO_TRACE;
#if defined OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [rows] {
            return ::OHOS::DelayedSingleton<MouseEventNormalize>::GetInstance()->SetMouseScrollRows(rows);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set the number of mouse scrolling rows failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::SetCustomCursor(int32_t windowId, int32_t focusX, int32_t focusY, void* pixelMap)
{
    CALL_INFO_TRACE;
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(std::bind(
        [pid, windowId] {
            return WIN_MGR->CheckWindowIdPermissionByPid(windowId, pid);
        })
        );
    if (windowId > 0 && ret != RET_OK) {
        MMI_HILOGE("Set the custom cursor failed, ret:%{public}d", ret);
        return ERROR_WINDOW_ID_PERMISSION_DENIED;
    }
#if defined OHOS_BUILD_ENABLE_POINTER
    auto type = PER_HELPER->GetTokenType();
    if (windowId < 0 && (type == OHOS::Security::AccessToken::TOKEN_HAP ||
        type == OHOS::Security::AccessToken::TOKEN_NATIVE)) {
        // The windowID of the application must be greater than 0
        MMI_HILOGE("Set the custom cursor failed, ret:%{public}d", RET_ERR);
        return RET_ERR;
    }
    if (windowId >= 0) {
        int32_t ret = CheckPidPermission(pid);
        if (ret != RET_OK) {
            MMI_HILOGE("Check pid permission failed");
            return ret;
        }
    }
    ret = delegateTasks_.PostSyncTask(std::bind(
        [pixelMap, pid, windowId, focusX, focusY] {
            return IPointerDrawingManager::GetInstance()->SetCustomCursor(pixelMap, pid, windowId, focusX, focusY);
        }
        ));
    if (ret != RET_OK) {
        MMI_HILOGE("Set the custom cursor failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::SetMouseIcon(int32_t windowId, void* pixelMap)
{
    CALL_INFO_TRACE;
#if defined OHOS_BUILD_ENABLE_POINTER
    int32_t pid = GetCallingPid();
    int32_t ret = CheckPidPermission(pid);
    if (ret != RET_OK) {
        MMI_HILOGE("Check pid permission failed");
        return ret;
    }
    ret = delegateTasks_.PostSyncTask(std::bind(
        [pid, windowId, pixelMap] {
            return IPointerDrawingManager::GetInstance()->SetMouseIcon(pid, windowId, pixelMap);
        }
        ));
    if (ret != RET_OK) {
        MMI_HILOGE("Set the mouse icon failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::SetMouseHotSpot(int32_t pid, int32_t windowId, int32_t hotSpotX, int32_t hotSpotY)
{
    CALL_INFO_TRACE;
#if defined OHOS_BUILD_ENABLE_POINTER
    int32_t ret = CheckPidPermission(pid);
    if (ret != RET_OK) {
        MMI_HILOGE("Check pid permission failed");
        return ret;
    }
    ret = delegateTasks_.PostSyncTask(
        [pid, windowId, hotSpotX, hotSpotY] {
            return IPointerDrawingManager::GetInstance()->SetMouseHotSpot(pid, windowId, hotSpotX, hotSpotY);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set the mouse hot spot failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::SetNapStatus(int32_t pid, int32_t uid, std::string bundleName, int32_t napStatus)
{
    CALL_INFO_TRACE;
    int32_t ret = CheckPidPermission(pid);
    if (ret != RET_OK) {
        MMI_HILOGE("Check pid permission failed");
        return ret;
    }
    NapProcess::GetInstance()->SetNapStatus(pid, uid, bundleName, napStatus);
    return RET_OK;
}

#ifdef OHOS_BUILD_ENABLE_POINTER
int32_t MMIService::ReadMouseScrollRows(int32_t &rows)
{
    rows = MouseEventHdr->GetMouseScrollRows();
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_POINTER

int32_t MMIService::GetMouseScrollRows(int32_t &rows)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &rows] {
            return this->ReadMouseScrollRows(rows);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get the number of mouse scrolling rows failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::SetPointerSize(int32_t size)
{
    CALL_INFO_TRACE;
#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
    int32_t ret = delegateTasks_.PostSyncTask(
        [size] {
            return IPointerDrawingManager::GetInstance()->SetPointerSize(size);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set pointer size failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING
    return RET_OK;
}

#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
int32_t MMIService::ReadPointerSize(int32_t &size)
{
    size = IPointerDrawingManager::GetInstance()->GetPointerSize();
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING

int32_t MMIService::GetPointerSize(int32_t &size)
{
    CALL_INFO_TRACE;
#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &size] {
            return this->ReadPointerSize(size);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get pointer size failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING
    return RET_OK;
}

int32_t MMIService::GetCursorSurfaceId(uint64_t &surfaceId)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    auto ret = delegateTasks_.PostSyncTask(
        [&surfaceId] {
            return IPointerDrawingManager::GetInstance()->GetCursorSurfaceId(surfaceId);
        });
    if (ret != RET_OK) {
        MMI_HILOGE("GetCursorSurfaceId fail, error:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::SetMousePrimaryButton(int32_t primaryButton)
{
    CALL_INFO_TRACE;
#if defined OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [primaryButton] {
            return ::OHOS::DelayedSingleton<MouseEventNormalize>::GetInstance()->SetMousePrimaryButton(primaryButton);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set mouse primary button failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

#ifdef OHOS_BUILD_ENABLE_POINTER
int32_t MMIService::ReadMousePrimaryButton(int32_t &primaryButton)
{
    primaryButton = MouseEventHdr->GetMousePrimaryButton();
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_POINTER

int32_t MMIService::GetMousePrimaryButton(int32_t &primaryButton)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &primaryButton] {
            return this->ReadMousePrimaryButton(primaryButton);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get mouse primary button failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::SetPointerVisible(bool visible, int32_t priority)
{
    CALL_INFO_TRACE;
#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto tokenType = OHOS::Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    bool isHap = false;
    if (tokenType == OHOS::Security::AccessToken::TOKEN_HAP) {
        isHap = true;
    }
    int32_t clientPid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [clientPid, visible, priority, isHap] {
            return IPointerDrawingManager::GetInstance()->SetPointerVisible(clientPid, visible, priority, isHap);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set pointer visible failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING
    return RET_OK;
}

#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
int32_t MMIService::CheckPointerVisible(bool &visible)
{
    WIN_MGR->UpdatePointerDrawingManagerWindowInfo();
    visible = IPointerDrawingManager::GetInstance()->IsPointerVisible();
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING

int32_t MMIService::IsPointerVisible(bool &visible)
{
    CALL_DEBUG_ENTER;
#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &visible] {
            return this->CheckPointerVisible(visible);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Is pointer visible failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING
    return RET_OK;
}

int32_t MMIService::MarkProcessed(int32_t eventType, int32_t eventId)
{
    CALL_DEBUG_ENTER;
    CHKPR(ANRMgr, RET_ERR);
    int32_t clientPid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [clientPid, eventType, eventId] {
            return ::OHOS::DelayedSingleton<ANRManager>::GetInstance()->MarkProcessed(clientPid, eventType, eventId);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGD("Mark event processed failed, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::SetPointerColor(int32_t color)
{
    CALL_INFO_TRACE;
#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
    int32_t ret = delegateTasks_.PostSyncTask(
        [color] {
            return IPointerDrawingManager::GetInstance()->SetPointerColor(color);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set pointer color failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING
    return RET_OK;
}

#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
int32_t MMIService::ReadPointerColor(int32_t &color)
{
    color = IPointerDrawingManager::GetInstance()->GetPointerColor();
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING

int32_t MMIService::GetPointerColor(int32_t &color)
{
    CALL_INFO_TRACE;
#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &color] {
            return this->ReadPointerColor(color);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get pointer color failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING
    return RET_OK;
}

int32_t MMIService::SetPointerSpeed(int32_t speed)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [speed] {
            return ::OHOS::DelayedSingleton<MouseEventNormalize>::GetInstance()->SetPointerSpeed(speed);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set pointer speed failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

#ifdef OHOS_BUILD_ENABLE_POINTER
int32_t MMIService::ReadPointerSpeed(int32_t &speed)
{
    speed = MouseEventHdr->GetPointerSpeed();
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_POINTER

int32_t MMIService::GetPointerSpeed(int32_t &speed)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &speed] {
            return this->ReadPointerSpeed(speed);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get pointer speed failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::NotifyNapOnline()
{
    CALL_DEBUG_ENTER;
    NapProcess::GetInstance()->NotifyNapOnline();
    return RET_OK;
}

int32_t MMIService::RemoveInputEventObserver()
{
    CALL_DEBUG_ENTER;
    NapProcess::GetInstance()->RemoveInputEventObserver();
    return RET_OK;
}

int32_t MMIService::SetPointerStyle(int32_t windowId, PointerStyle pointerStyle, bool isUiExtension)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t clientPid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [clientPid, windowId, pointerStyle, isUiExtension] {
            return IPointerDrawingManager::GetInstance()->SetPointerStyle(
                clientPid, windowId, pointerStyle, isUiExtension);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set pointer style failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::ClearWindowPointerStyle(int32_t pid, int32_t windowId)
{
    CALL_DEBUG_ENTER;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = CheckPidPermission(pid);
    if (ret != RET_OK) {
        MMI_HILOGE("Check pid permission failed");
        return ret;
    }
    ret = delegateTasks_.PostSyncTask(
        [pid, windowId] {
            return IPointerDrawingManager::GetInstance()->ClearWindowPointerStyle(pid, windowId);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set pointer style failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::GetPointerStyle(int32_t windowId, PointerStyle &pointerStyle, bool isUiExtension)
{
    CALL_DEBUG_ENTER;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t clientPid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [clientPid, windowId, &pointerStyle, isUiExtension] {
            return IPointerDrawingManager::GetInstance()->GetPointerStyle(
                clientPid, windowId, pointerStyle, isUiExtension);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get pointer style failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::SetHoverScrollState(bool state)
{
    CALL_INFO_TRACE;
#if defined OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [state] {
            return ::OHOS::MMI::IInputWindowsManager::GetInstance()->SetHoverScrollState(state);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set mouse hover scroll state failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

#ifdef OHOS_BUILD_ENABLE_POINTER
int32_t MMIService::ReadHoverScrollState(bool &state)
{
    state = WIN_MGR->GetHoverScrollState();
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_POINTER

int32_t MMIService::GetHoverScrollState(bool &state)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &state] {
            return this->ReadHoverScrollState(state);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get mouse hover scroll state, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::OnSupportKeys(int32_t deviceId, std::vector<int32_t> &keys, std::vector<bool> &keystroke)
{
    CALL_DEBUG_ENTER;
    int32_t ret = INPUT_DEV_MGR->SupportKeys(deviceId, keys, keystroke);
    if (ret != RET_OK) {
        MMI_HILOGE("Device id not support");
        return ret;
    }
    if (keystroke.size() > MAX_SUPPORT_KEY) {
        MMI_HILOGE("Device exceeds the max range");
        return RET_ERR;
    }
    return RET_OK;
}

int32_t MMIService::SupportKeys(int32_t deviceId, std::vector<int32_t> &keys, std::vector<bool> &keystroke)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, deviceId, &keys, &keystroke] {
            return this->OnSupportKeys(deviceId, keys, keystroke);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Support keys info process failed, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::OnGetDeviceIds(std::vector<int32_t> &ids)
{
    CALL_DEBUG_ENTER;
    ids = INPUT_DEV_MGR->GetInputDeviceIds();
    return RET_OK;
}

int32_t MMIService::GetDeviceIds(std::vector<int32_t> &ids)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &ids] {
            return this->OnGetDeviceIds(ids);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get deviceids failed, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::OnGetDevice(int32_t deviceId, std::shared_ptr<InputDevice> inputDevice)
{
    CALL_DEBUG_ENTER;
    if (INPUT_DEV_MGR->GetInputDevice(deviceId) == nullptr) {
        MMI_HILOGE("Input device not found");
        return COMMON_PARAMETER_ERROR;
    }
    auto tmpDevice = INPUT_DEV_MGR->GetInputDevice(deviceId);
    inputDevice->SetId(tmpDevice->GetId());
    inputDevice->SetType(tmpDevice->GetType());
    inputDevice->SetName(tmpDevice->GetName());
    inputDevice->SetBus(tmpDevice->GetBus());
    inputDevice->SetVersion(tmpDevice->GetVersion());
    inputDevice->SetProduct(tmpDevice->GetProduct());
    inputDevice->SetVendor(tmpDevice->GetVendor());
    inputDevice->SetPhys(tmpDevice->GetPhys());
    inputDevice->SetUniq(tmpDevice->GetUniq());
    inputDevice->SetCapabilities(tmpDevice->GetCapabilities());
    inputDevice->SetAxisInfo(tmpDevice->GetAxisInfo());

    return RET_OK;
}

int32_t MMIService::GetDevice(int32_t deviceId, std::shared_ptr<InputDevice> &inputDevice)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, deviceId, inputDevice] {
            return this->OnGetDevice(deviceId, inputDevice);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get input device info failed, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::OnRegisterDevListener(int32_t pid)
{
    auto sess = GetSession(GetClientFd(pid));
    CHKPR(sess, RET_ERR);
    INPUT_DEV_MGR->AddDevListener(sess);
    return RET_OK;
}

int32_t MMIService::RegisterDevListener()
{
    CALL_DEBUG_ENTER;
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid] {
            return this->OnRegisterDevListener(pid);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Register device listener failed, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::OnUnregisterDevListener(int32_t pid)
{
    auto sess = GetSession(GetClientFd(pid));
    INPUT_DEV_MGR->RemoveDevListener(sess);
    return RET_OK;
}

int32_t MMIService::UnregisterDevListener()
{
    CALL_DEBUG_ENTER;
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid] {
            return this->OnUnregisterDevListener(pid);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Unregister device listener failed failed, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::OnGetKeyboardType(int32_t deviceId, int32_t &keyboardType)
{
    CALL_DEBUG_ENTER;
    int32_t ret = INPUT_DEV_MGR->GetKeyboardType(deviceId, keyboardType);
    if (ret != RET_OK) {
        MMI_HILOGD("GetKeyboardType call failed");
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::GetKeyboardType(int32_t deviceId, int32_t &keyboardType)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, deviceId, &keyboardType] {
            return this->OnGetKeyboardType(deviceId, keyboardType);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGD("Get keyboard type failed, ret:%{public}d", ret);
        return ret;
    }
    return ret;
}

int32_t MMIService::SetKeyboardRepeatDelay(int32_t delay)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    int32_t ret = delegateTasks_.PostSyncTask(
        [delay] {
            return ::OHOS::DelayedSingleton<KeyAutoRepeat>::GetInstance()->SetKeyboardRepeatDelay(delay);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set keyboard repeat delay failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    return RET_OK;
}

int32_t MMIService::SetKeyboardRepeatRate(int32_t rate)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    int32_t ret = delegateTasks_.PostSyncTask(
        [rate] {
            return ::OHOS::DelayedSingleton<KeyAutoRepeat>::GetInstance()->SetKeyboardRepeatRate(rate);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set keyboard repeat rate failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    return RET_OK;
}

int32_t MMIService::GetKeyboardRepeatDelay(int32_t &delay)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    int32_t ret = delegateTasks_.PostSyncTask(
        [&delay] {
            return ::OHOS::DelayedSingleton<KeyAutoRepeat>::GetInstance()->GetKeyboardRepeatDelay(delay);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get keyboard repeat delay failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    return RET_OK;
}

int32_t MMIService::GetKeyboardRepeatRate(int32_t &rate)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    int32_t ret = delegateTasks_.PostSyncTask(
        [&rate] {
            return ::OHOS::DelayedSingleton<KeyAutoRepeat>::GetInstance()->GetKeyboardRepeatRate(rate);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get keyboard repeat rate failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    return RET_OK;
}

#if defined(OHOS_BUILD_ENABLE_INTERCEPTOR) || defined(OHOS_BUILD_ENABLE_MONITOR)
int32_t MMIService::CheckAddInput(int32_t pid, InputHandlerType handlerType, HandleEventType eventType,
    int32_t priority, uint32_t deviceTags)
{
    auto sess = GetSessionByPid(pid);
    CHKPR(sess, ERROR_NULL_POINTER);
    return sMsgHandler_.OnAddInputHandler(sess, handlerType, eventType, priority, deviceTags);
}

#endif // OHOS_BUILD_ENABLE_INTERCEPTOR || OHOS_BUILD_ENABLE_MONITOR

int32_t MMIService::AddInputHandler(InputHandlerType handlerType, HandleEventType eventType, int32_t priority,
    uint32_t deviceTags, std::vector<int32_t> actionsType)
{
    CALL_INFO_TRACE;
    bool isRegisterCaptureCb = false;
#if defined(OHOS_BUILD_ENABLE_MONITOR) && defined(PLAYER_FRAMEWORK_EXISTS)
    if (!PER_HELPER->VerifySystemApp() && handlerType == InputHandlerType::MONITOR) {
        isRegisterCaptureCb = true;
    }
#endif // OHOS_BUILD_ENABLE_MONITOR && PLAYER_FRAMEWORK_EXISTS
#if defined(OHOS_BUILD_ENABLE_INTERCEPTOR) || defined(OHOS_BUILD_ENABLE_MONITOR)
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, handlerType, eventType, priority, deviceTags, isRegisterCaptureCb] {
            if (isRegisterCaptureCb) {
#if defined(OHOS_BUILD_ENABLE_MONITOR) && defined(PLAYER_FRAMEWORK_EXISTS)
                RegisterScreenCaptureCallback();
#endif // OHOS_BUILD_ENABLE_MONITOR && PLAYER_FRAMEWORK_EXISTS
            }
            return this->CheckAddInput(pid, handlerType, eventType, priority, deviceTags);
        });
    if (ret != RET_OK) {
        MMI_HILOGE("Add input handler failed, ret:%{public}d", ret);
        return ret;
    }
    ret = ObserverAddInputHandler(pid);
    if (ret != RET_OK) {
        MMI_HILOGE("AddInputHandler info to observer failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_INTERCEPTOR || OHOS_BUILD_ENABLE_MONITOR
    return RET_OK;
}

int32_t MMIService::AddPreInputHandler(int32_t handlerId, HandleEventType eventType, std::vector<int32_t> keys)
{
    CALL_DEBUG_ENTER;
#ifdef OHOS_BUILD_ENABLE_MONITOR
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask([this, pid, handlerId, eventType, keys] () -> int32_t {
        auto sess = GetSessionByPid(pid);
        CHKPR(sess, ERROR_NULL_POINTER);
        auto preMonitorHandler = InputHandler->GetEventPreMonitorHandler();
        CHKPR(preMonitorHandler, RET_ERR);
        return preMonitorHandler->AddInputHandler(sess, handlerId, eventType, keys);
    });
    if (ret != RET_OK) {
        MMI_HILOGE("The AddPreInputHandler key event processed failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_MONITOR
    return RET_OK;
}

int32_t MMIService::RemovePreInputHandler(int32_t handlerId)
{
    CALL_DEBUG_ENTER;
#ifdef OHOS_BUILD_ENABLE_MONITOR
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask([this, pid, handlerId] () -> int32_t {
        auto sess = GetSessionByPid(pid);
        CHKPR(sess, ERROR_NULL_POINTER);
        auto preMonitorHandler = InputHandler->GetEventPreMonitorHandler();
        CHKPR(preMonitorHandler, RET_ERR);
        preMonitorHandler->RemoveInputHandler(sess, handlerId);
        return RET_OK;
    });
    if (ret != RET_OK) {
        MMI_HILOGE("Remove pre input handler failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_MONITOR
    return RET_OK;
}

#if defined(OHOS_BUILD_ENABLE_INTERCEPTOR) || defined(OHOS_BUILD_ENABLE_MONITOR)
int32_t MMIService::CheckRemoveInput(int32_t pid, InputHandlerType handlerType, HandleEventType eventType,
    int32_t priority, uint32_t deviceTags)
{
    auto sess = GetSessionByPid(pid);
    CHKPR(sess, ERROR_NULL_POINTER);
    return sMsgHandler_.OnRemoveInputHandler(sess, handlerType, eventType, priority, deviceTags);
}

int32_t MMIService::ObserverAddInputHandler(int32_t pid)
{
    if (NapProcess::GetInstance()->GetNapClientPid() != REMOVE_OBSERVER) {
        OHOS::MMI::NapProcess::NapStatusData napData;
        napData.pid = GetCallingPid();
        napData.uid = GetCallingUid();
        auto sess = GetSessionByPid(pid);
        CHKPR(sess, ERROR_NULL_POINTER);
        napData.bundleName = sess->GetProgramName();
        int32_t syncState = SUBSCRIBED;
        MMI_HILOGD("AddInputHandler info to observer : pid:%{public}d, uid:%d, bundleName:%{public}s",
            napData.pid, napData.uid, napData.bundleName.c_str());
        NapProcess::GetInstance()->AddMmiSubscribedEventData(napData, syncState);
        if (NapProcess::GetInstance()->GetNapClientPid() != UNOBSERVED) {
            NapProcess::GetInstance()->NotifyBundleName(napData, syncState);
        }
    }
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_INTERCEPTOR || OHOS_BUILD_ENABLE_MONITOR

int32_t MMIService::RemoveInputHandler(InputHandlerType handlerType, HandleEventType eventType, int32_t priority,
    uint32_t deviceTags, std::vector<int32_t> actionsType)
{
    CALL_INFO_TRACE;
#if defined(OHOS_BUILD_ENABLE_INTERCEPTOR) || defined(OHOS_BUILD_ENABLE_MONITOR)
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, handlerType, eventType, priority, deviceTags] {
            return this->CheckRemoveInput(pid, handlerType, eventType, priority, deviceTags);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Remove input handler failed, ret:%{public}d", ret);
        return ret;
    }
    if (NapProcess::GetInstance()->GetNapClientPid() != REMOVE_OBSERVER) {
        OHOS::MMI::NapProcess::NapStatusData napData;
        napData.pid = GetCallingPid();
        napData.uid = GetCallingUid();
        auto sess = GetSessionByPid(pid);
        CHKPR(sess, ERROR_NULL_POINTER);
        napData.bundleName = sess->GetProgramName();
        int32_t syncState = UNSUBSCRIBED;
        MMI_HILOGD("RemoveInputHandler info to observer : pid:%{public}d, uid:%{public}d, bundleName:%{public}s",
            napData.pid, napData.uid, napData.bundleName.c_str());
        NapProcess::GetInstance()->AddMmiSubscribedEventData(napData, syncState);
        if (NapProcess::GetInstance()->GetNapClientPid() != UNOBSERVED) {
            NapProcess::GetInstance()->NotifyBundleName(napData, syncState);
        }
    }
#endif // OHOS_BUILD_ENABLE_INTERCEPTOR || OHOS_BUILD_ENABLE_MONITOR
    return RET_OK;
}

int32_t MMIService::AddGestureMonitor(InputHandlerType handlerType,
    HandleEventType eventType, TouchGestureType gestureType, int32_t fingers)
{
    CALL_INFO_TRACE;
#if defined(OHOS_BUILD_ENABLE_TOUCH) && defined(OHOS_BUILD_ENABLE_MONITOR)
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, handlerType, eventType, gestureType, fingers]() -> int32_t {
            if (((eventType & HANDLE_EVENT_TYPE_TOUCH_GESTURE) != HANDLE_EVENT_TYPE_TOUCH_GESTURE)) {
                MMI_HILOGE("Illegal type:%{public}d", eventType);
                return RET_ERR;
            }
            if (!GestureMonitorHandler::CheckMonitorValid(gestureType, fingers)) {
                MMI_HILOGE("Wrong number of fingers:%{public}d", fingers);
                return RET_ERR;
            }
            if (touchGestureMgr_ == nullptr) {
                touchGestureMgr_ = std::make_shared<TouchGestureManager>(delegateInterface_);
            }
            touchGestureMgr_->AddHandler(pid, gestureType, fingers);

            auto sess = GetSessionByPid(pid);
            CHKPR(sess, ERROR_NULL_POINTER);
            return sMsgHandler_.OnAddGestureMonitor(sess, handlerType, eventType, gestureType, fingers);
        });
    if (ret != RET_OK) {
        MMI_HILOGE("Add gesture handler failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_TOUCH && OHOS_BUILD_ENABLE_MONITOR
    return RET_OK;
}

int32_t MMIService::RemoveGestureMonitor(InputHandlerType handlerType,
    HandleEventType eventType, TouchGestureType gestureType, int32_t fingers)
{
    CALL_INFO_TRACE;
#if defined(OHOS_BUILD_ENABLE_TOUCH) && defined(OHOS_BUILD_ENABLE_MONITOR)
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, handlerType, eventType, gestureType, fingers]() -> int32_t {
            auto sess = GetSessionByPid(pid);
            CHKPR(sess, ERROR_NULL_POINTER);
            int32_t ret = sMsgHandler_.OnRemoveGestureMonitor(sess, handlerType, eventType, gestureType, fingers);
            if (ret != RET_OK) {
                MMI_HILOGE("Failed to remove gesture recognizer, ret:%{public}d", ret);
                return ret;
            }
            if (touchGestureMgr_ != nullptr) {
                touchGestureMgr_->RemoveHandler(pid, gestureType, fingers);
            }
            return RET_OK;
        });
    if (ret != RET_OK) {
        MMI_HILOGE("Remove gesture handler failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_TOUCH && OHOS_BUILD_ENABLE_MONITOR
    return RET_OK;
}

#ifdef OHOS_BUILD_ENABLE_MONITOR
int32_t MMIService::CheckMarkConsumed(int32_t pid, int32_t eventId)
{
    auto sess = GetSessionByPid(pid);
    CHKPR(sess, ERROR_NULL_POINTER);
    return sMsgHandler_.OnMarkConsumed(sess, eventId);
}
#endif // OHOS_BUILD_ENABLE_MONITOR

int32_t MMIService::MarkEventConsumed(int32_t eventId)
{
    CALL_DEBUG_ENTER;
#ifdef OHOS_BUILD_ENABLE_MONITOR
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, eventId] {
            return this->CheckMarkConsumed(pid, eventId);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Mark event consumed failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_MONITOR
    return RET_OK;
}

int32_t MMIService::MoveMouseEvent(int32_t offsetX, int32_t offsetY)
{
    CALL_DEBUG_ENTER;
#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
    int32_t ret =
        delegateTasks_.PostSyncTask(
            [this, offsetX, offsetY] {
                return sMsgHandler_.OnMoveMouse(offsetX, offsetY);
            }
            );
    if (ret != RET_OK) {
        MMI_HILOGE("The movemouse event processed failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING
    return RET_OK;
}

int32_t MMIService::InjectKeyEvent(const std::shared_ptr<KeyEvent> keyEvent, bool isNativeInject)
{
    CALL_DEBUG_ENTER;
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    int32_t ret;
    int32_t pid = GetCallingPid();
#ifdef OHOS_BUILD_ENABLE_ANCO
    ret = InjectKeyEventExt(keyEvent, pid, isNativeInject);
#else
    ret = delegateTasks_.PostSyncTask(
        [this, keyEvent, pid, isNativeInject] {
            return this->CheckInjectKeyEvent(keyEvent, pid, isNativeInject);
        }
        );
#endif // OHOS_BUILD_ENABLE_ANCO
    if (ret != RET_OK) {
        MMI_HILOGE("Inject key event failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    return RET_OK;
}

int32_t MMIService::CheckInjectKeyEvent(const std::shared_ptr<KeyEvent> keyEvent, int32_t pid, bool isNativeInject)
{
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    CHKPR(keyEvent, ERROR_NULL_POINTER);
    LogTracer lt(keyEvent->GetId(), keyEvent->GetEventType(), keyEvent->GetKeyAction());
    return sMsgHandler_.OnInjectKeyEvent(keyEvent, pid, isNativeInject);
#else
    return RET_OK;
#endif // OHOS_BUILD_ENABLE_KEYBOARD
}

#ifdef OHOS_BUILD_ENABLE_KEYBOARD
int32_t MMIService::OnGetKeyState(std::vector<int32_t> &pressedKeys, std::map<int32_t, int32_t> &specialKeysState)
{
    auto keyEvent = KeyEventHdr->GetKeyEvent();
    CHKPR(keyEvent, ERROR_NULL_POINTER);
    pressedKeys = keyEvent->GetPressedKeys();
    for (auto iter = pressedKeys.begin(); iter != pressedKeys.end();) {
        if (g_keyCodeValueSet.find(*iter) == g_keyCodeValueSet.end()) {
            iter = pressedKeys.erase(iter);
            continue;
        }
        ++iter;
    }
    specialKeysState[KeyEvent::KEYCODE_CAPS_LOCK] =
        static_cast<int32_t>(keyEvent->GetFunctionKey(KeyEvent::CAPS_LOCK_FUNCTION_KEY));
    specialKeysState[KeyEvent::KEYCODE_SCROLL_LOCK] =
        static_cast<int32_t>(keyEvent->GetFunctionKey(KeyEvent::SCROLL_LOCK_FUNCTION_KEY));
    specialKeysState[KeyEvent::KEYCODE_NUM_LOCK] =
        static_cast<int32_t>(keyEvent->GetFunctionKey(KeyEvent::NUM_LOCK_FUNCTION_KEY));
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_KEYBOARD

int32_t MMIService::CheckInjectPointerEvent(const std::shared_ptr<PointerEvent> pointerEvent,
    int32_t pid, bool isNativeInject, bool isShell)
{
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    LogTracer lt(pointerEvent->GetId(), pointerEvent->GetEventType(), pointerEvent->GetPointerAction());
    return sMsgHandler_.OnInjectPointerEvent(pointerEvent, pid, isNativeInject, isShell);
#else
    return RET_OK;
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
}

int32_t MMIService::CheckTouchPadEvent(const std::shared_ptr<PointerEvent> pointerEvent,
    int32_t pid, const TouchpadCDG &touchpadCDG, bool isNativeInject, bool isShell)
{
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    CHKPR(pointerEvent, ERROR_NULL_POINTER);
    LogTracer lt(pointerEvent->GetId(), pointerEvent->GetEventType(), pointerEvent->GetPointerAction());
    return sMsgHandler_.OnInjectTouchPadEvent(pointerEvent, pid, touchpadCDG, isNativeInject, isShell);
#else
    return RET_OK;
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
}

int32_t MMIService::InjectPointerEvent(const std::shared_ptr<PointerEvent> pointerEvent, bool isNativeInject)
{
    CALL_DEBUG_ENTER;
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    int32_t ret;
    int32_t pid = GetCallingPid();
    bool isShell = PER_HELPER->RequestFromShell();
#ifdef OHOS_BUILD_ENABLE_ANCO
    ret = InjectPointerEventExt(pointerEvent, pid, isNativeInject, isShell);
#else
    ret = delegateTasks_.PostSyncTask(
        [this, pointerEvent, pid, isNativeInject, isShell] {
            return this->CheckInjectPointerEvent(pointerEvent, pid, isNativeInject, isShell);
        }
        );
#endif // OHOS_BUILD_ENABLE_ANCO
    if (ret != RET_OK) {
        MMI_HILOGE("Inject pointer event failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
    return RET_OK;
}

int32_t MMIService::InjectTouchPadEvent(const std::shared_ptr<PointerEvent> pointerEvent,
    const TouchpadCDG &touchpadCDG, bool isNativeInject)
{
    CALL_DEBUG_ENTER;
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    int32_t ret;
    int32_t pid = GetCallingPid();
    bool isShell = PER_HELPER->RequestFromShell();
    ret = delegateTasks_.PostSyncTask(
        [this, pointerEvent, pid, touchpadCDG, isNativeInject, isShell] {
            return sMsgHandler_.OnInjectTouchPadEvent(pointerEvent, pid, touchpadCDG, isNativeInject, isShell);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Inject touchpad event failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
    return RET_OK;
}

#if defined(OHOS_RSS_CLIENT) && !defined(OHOS_BUILD_PC_PRIORITY)
void MMIService::OnAddResSchedSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    int sleepSeconds = 1;
    sleep(sleepSeconds);
    uint64_t tid = tid_.load();
    int32_t userInteraction = 2;
    std::unordered_map<std::string, std::string> payload;
    payload["uid"] = std::to_string(getuid());
    payload["pid"] = std::to_string(getpid());
    payload["extType"] = "10002";
    payload["tid"] = std::to_string(tid);
    payload["isSa"] = "1";
    payload["cgroupPrio"] = "1";
    payload["threadName"] = "mmi_service";
    auto begin = std::chrono::high_resolution_clock::now();
    ResourceSchedule::ResSchedClient::GetInstance().ReportData(
        ResourceSchedule::ResType::RES_TYPE_KEY_PERF_SCENE, userInteraction, payload);
    auto durationMS = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - begin).count();
#ifdef OHOS_BUILD_ENABLE_DFX_RADAR
    DfxHisysevent::ReportApiCallTimes(ApiDurationStatistics::Api::RESOURCE_SCHEDULE_REPORT_DATA, durationMS);
#endif // OHOS_BUILD_ENABLE_DFX_RADAR
}
#endif // defined(OHOS_RSS_CLIENT) && !defined(OHOS_BUILD_PC_PRIORITY)

void MMIService::OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    CALL_INFO_TRACE;
    MMI_HILOGI("The systemAbilityId is %{public}d", systemAbilityId);
#if defined(OHOS_RSS_CLIENT) && !defined(OHOS_BUILD_PC_PRIORITY)
    if (systemAbilityId == RES_SCHED_SYS_ABILITY_ID) {
        OnAddResSchedSystemAbility(systemAbilityId, deviceId);
    }
#endif // defined(OHOS_RSS_CLIENT) && !defined(OHOS_BUILD_PC_PRIORITY)
#ifdef OHOS_BUILD_ENABLE_FINGERSENSE_WRAPPER
    if (systemAbilityId == COMMON_EVENT_SERVICE_ID) {
        isCesStart_ = true;
    }
#endif // OHOS_BUILD_ENABLE_FINGERSENSE_WRAPPER
    if (systemAbilityId == APP_MGR_SERVICE_ID) {
        APP_OBSERVER_MGR->InitAppStateObserver();
    }
    if (systemAbilityId == COMMON_EVENT_SERVICE_ID) {
        DEVICE_MONITOR->InitCommonEventSubscriber();
#if defined(OHOS_BUILD_ENABLE_KEYBOARD) && defined(OHOS_BUILD_ENABLE_FINGERSENSE_WRAPPER)
        DISPLAY_MONITOR->InitCommonEventSubscriber();
#endif // OHOS_BUILD_ENABLE_KEYBOARD && OHOS_BUILD_ENABLE_FINGERSENSE_WRAPPER
#ifdef OHOS_BUILD_ENABLE_VKEYBOARD
        libinputAdapter_.RegisterBootStatusReceiver();
#endif // OHOS_BUILD_ENABLE_VKEYBOARD
    }
#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
    if (systemAbilityId == RENDER_SERVICE) {
        IPointerDrawingManager::GetInstance()->InitPointerCallback();
    }
    if (systemAbilityId == DISPLAY_MANAGER_SERVICE_SA_ID) {
        IPointerDrawingManager::GetInstance()->InitScreenInfo();
    }
    if (systemAbilityId == DISPLAY_MANAGER_SERVICE_SA_ID) {
        IPointerDrawingManager::GetInstance()->SubscribeScreenModeChange();
    }
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING
    if (systemAbilityId == DISPLAY_MANAGER_SERVICE_SA_ID) {
        WIN_MGR->SetFoldState();
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
        KeyEventHdr->Init();
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    }
#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
    if (systemAbilityId == DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID) {
        if (SettingDataShare::GetInstance(DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID).CheckIfSettingsDataReady()) {
            IPointerDrawingManager::GetInstance()->InitPointerObserver();
        }
    }
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING
}

#if defined(OHOS_BUILD_ENABLE_MONITOR) && defined(PLAYER_FRAMEWORK_EXISTS)
void MMIService::ScreenCaptureCallback(int32_t pid, bool isStart)
{
    auto service = MMIService::GetInstance();
    CHKPV(service);
    int32_t ret = service->delegateTasks_.PostSyncTask(
        [pid, isStart] {
            auto monitorHandler = InputHandler->GetMonitorHandler();
            CHKPR(monitorHandler, RET_ERR);
            monitorHandler->ProcessScreenCapture(pid, isStart);
            return RET_OK;
        });
}

void MMIService::RegisterScreenCaptureCallback()
{
    if (hasRegisterListener_) {
        return;
    }
    InputScreenCaptureAgent::GetInstance().RegisterListener(ScreenCaptureCallback);
    hasRegisterListener_ = true;
}
#endif // OHOS_BUILD_ENABLE_MONITOR && PLAYER_FRAMEWORK_EXISTS

int32_t MMIService::SubscribeKeyEvent(int32_t subscribeId, const std::shared_ptr<KeyOption> option)
{
    CALL_DEBUG_ENTER;
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, subscribeId, option] {
            return sMsgHandler_.OnSubscribeKeyEvent(this, pid, subscribeId, option);
        });
    if (ret != RET_OK) {
        MMI_HILOGE("The subscribe key event processed failed, ret:%{public}d", ret);
        return ret;
    }
    if (NapProcess::GetInstance()->GetNapClientPid() != REMOVE_OBSERVER) {
        OHOS::MMI::NapProcess::NapStatusData napData;
        napData.pid = GetCallingPid();
        napData.uid = GetCallingUid();
        auto sess = GetSessionByPid(pid);
        CHKPR(sess, ERROR_NULL_POINTER);
        napData.bundleName = sess->GetProgramName();
        int32_t syncState = SUBSCRIBED;
        MMI_HILOGD("SubscribeKeyEvent info to observer : pid:%{public}d, uid:%{public}d, bundleName:%{public}s",
            napData.pid, napData.uid, napData.bundleName.c_str());
        NapProcess::GetInstance()->AddMmiSubscribedEventData(napData, syncState);
        if (NapProcess::GetInstance()->GetNapClientPid() != UNOBSERVED) {
            NapProcess::GetInstance()->NotifyBundleName(napData, syncState);
        }
    }
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    return RET_OK;
}

int32_t MMIService::UnsubscribeKeyEvent(int32_t subscribeId)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, subscribeId] {
            return sMsgHandler_.OnUnsubscribeKeyEvent(this, pid, subscribeId);
        });
    if (ret != RET_OK) {
        MMI_HILOGE("The unsubscribe key event processed failed, ret:%{public}d", ret);
        return ret;
    }
    if (NapProcess::GetInstance()->GetNapClientPid() != REMOVE_OBSERVER) {
        OHOS::MMI::NapProcess::NapStatusData napData;
        napData.pid = GetCallingPid();
        napData.uid = GetCallingUid();
        auto sess = GetSessionByPid(pid);
        CHKPR(sess, ERROR_NULL_POINTER);
        napData.bundleName = sess->GetProgramName();
        int32_t syncState = UNSUBSCRIBED;
        MMI_HILOGD("UnsubscribeKeyEvent info to observer : pid:%{public}d, uid:%{public}d, bundleName:%{public}s",
            napData.pid, napData.uid, napData.bundleName.c_str());
        NapProcess::GetInstance()->AddMmiSubscribedEventData(napData, syncState);
        if (NapProcess::GetInstance()->GetNapClientPid() != UNOBSERVED) {
            NapProcess::GetInstance()->NotifyBundleName(napData, syncState);
        }
    }
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    return RET_OK;
}

int32_t MMIService::SubscribeHotkey(int32_t subscribeId, const std::shared_ptr<KeyOption> option)
{
    CALL_DEBUG_ENTER;
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, subscribeId, option] {
            return sMsgHandler_.OnSubscribeHotkey(this, pid, subscribeId, option);
        });
    if (ret != RET_OK) {
        MMI_HILOGE("ServerMsgHandler::OnSubscribeHotkey fail, error:%{public}d", ret);
        return ret;
    }
    if (NapProcess::GetInstance()->GetNapClientPid() != REMOVE_OBSERVER) {
        OHOS::MMI::NapProcess::NapStatusData napData;
        napData.pid = GetCallingPid();
        napData.uid = GetCallingUid();
        auto sess = GetSessionByPid(pid);
        CHKPR(sess, ERROR_NULL_POINTER);
        napData.bundleName = sess->GetProgramName();
        int32_t syncState = SUBSCRIBED;
        MMI_HILOGD("SubscribeHotkey info to observer : pid:%{public}d, bundleName:%{public}s",
            napData.pid, napData.bundleName.c_str());
        NapProcess::GetInstance()->AddMmiSubscribedEventData(napData, syncState);
        if (NapProcess::GetInstance()->GetNapClientPid() != UNOBSERVED) {
            NapProcess::GetInstance()->NotifyBundleName(napData, syncState);
        }
    }
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    return RET_OK;
}

int32_t MMIService::UnsubscribeHotkey(int32_t subscribeId)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, subscribeId] {
            return sMsgHandler_.OnUnsubscribeHotkey(this, pid, subscribeId);
        });
    if (ret != RET_OK) {
        MMI_HILOGE("ServerMsgHandler::OnUnsubscribeHotkey fail, error:%{public}d", ret);
        return ret;
    }
    if (NapProcess::GetInstance()->GetNapClientPid() != REMOVE_OBSERVER) {
        OHOS::MMI::NapProcess::NapStatusData napData;
        napData.pid = GetCallingPid();
        napData.uid = GetCallingUid();
        auto sess = GetSessionByPid(pid);
        CHKPR(sess, ERROR_NULL_POINTER);
        napData.bundleName = sess->GetProgramName();
        int32_t syncState = UNSUBSCRIBED;
        MMI_HILOGD("UnsubscribeHotkey info to observer : pid:%{public}d, bundleName:%{public}s",
            napData.pid, napData.bundleName.c_str());
        NapProcess::GetInstance()->AddMmiSubscribedEventData(napData, syncState);
        if (NapProcess::GetInstance()->GetNapClientPid() != UNOBSERVED) {
            NapProcess::GetInstance()->NotifyBundleName(napData, syncState);
        }
    }
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    return RET_OK;
}

#ifdef OHOS_BUILD_ENABLE_KEY_PRESSED_HANDLER
int32_t MMIService::SubscribeKeyMonitor(const KeyMonitorOption &keyOption)
{
    CALL_INFO_TRACE;
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, keyOption] {
            return sMsgHandler_.SubscribeKeyMonitor(pid, keyOption);
        });
    if (ret != RET_OK) {
        MMI_HILOGE("ServerMsgHandler::SubscribeKeyMonitor fail, error:%{public}d", ret);
    }
    return ret;
}

int32_t MMIService::UnsubscribeKeyMonitor(const KeyMonitorOption &keyOption)
{
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, keyOption] {
            return sMsgHandler_.UnsubscribeKeyMonitor(pid, keyOption);
        });
    if (ret != RET_OK) {
        MMI_HILOGE("ServerMsgHandler::UnsubscribeKeyMonitor fail, error:%{public}d", ret);
    }
    return ret;
}
#endif // OHOS_BUILD_ENABLE_KEY_PRESSED_HANDLER

int32_t MMIService::SubscribeSwitchEvent(int32_t subscribeId, int32_t switchType)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_SWITCH
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, subscribeId, switchType] {
            return sMsgHandler_.OnSubscribeSwitchEvent(this, pid, subscribeId, switchType);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("The subscribe switch event processed failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_SWITCH
    return RET_OK;
}

int32_t MMIService::UnsubscribeSwitchEvent(int32_t subscribeId)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_SWITCH
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, subscribeId] {
            return sMsgHandler_.OnUnsubscribeSwitchEvent(this, pid, subscribeId);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("The unsubscribe switch event processed failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_SWITCH
    return RET_OK;
}

int32_t MMIService::SubscribeTabletProximity(int32_t subscribeId)
{
    CALL_INFO_TRACE;
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, subscribeId] {
            auto sess = this->GetSessionByPid(pid);
            CHKPR(sess, RET_ERR);
            return TABLET_SCRIBER_HANDLER->SubscribeTabletProximity(sess, subscribeId);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("The subscribe tablet event processed failed, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::UnsubscribetabletProximity(int32_t subscribeId)
{
    CALL_INFO_TRACE;
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, subscribeId] {
            auto sess = this->GetSessionByPid(pid);
            CHKPR(sess, RET_ERR);
            return TABLET_SCRIBER_HANDLER->UnsubscribetabletProximity(sess, subscribeId);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("The unsubscribe tablet event processed failed, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::SubscribeLongPressEvent(int32_t subscribeId, const LongPressRequest &longPressRequest)
{
    CALL_INFO_TRACE;
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, subscribeId, longPressRequest] {
            return sMsgHandler_.OnSubscribeLongPressEvent(this, pid, subscribeId, longPressRequest);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("The subscribe long press event processed failed, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}
 
int32_t MMIService::UnsubscribeLongPressEvent(int32_t subscribeId)
{
    CALL_INFO_TRACE;
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, subscribeId] {
            return sMsgHandler_.OnUnsubscribeLongPressEvent(this, pid, subscribeId);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("The unsubscribe long press event processed failed, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::SetAnrObserver()
{
    CALL_INFO_TRACE;
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [pid] {
            return ::OHOS::DelayedSingleton<ANRManager>::GetInstance()->SetANRNoticedPid(pid);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set ANRNoticed pid failed, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::GetDisplayBindInfo(DisplayBindInfos &infos)
{
    CALL_INFO_TRACE;
    int32_t ret = delegateTasks_.PostSyncTask(
        [&infos] {
            return ::OHOS::MMI::IInputWindowsManager::GetInstance()->GetDisplayBindInfo(infos);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("GetDisplayBindInfo pid failed, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::GetAllMmiSubscribedEvents(std::map<std::tuple<int32_t, int32_t, std::string>, int32_t> &datas)
{
    CALL_INFO_TRACE;
    NapProcess::GetInstance()->GetAllMmiSubscribedEvents(datas);
    return RET_OK;
}

int32_t MMIService::SetDisplayBind(int32_t deviceId, int32_t displayId, std::string &msg)
{
    CALL_INFO_TRACE;
    int32_t ret = delegateTasks_.PostSyncTask(
        [deviceId, displayId, &msg] {
            return ::OHOS::MMI::IInputWindowsManager::GetInstance()->SetDisplayBind(deviceId, displayId, msg);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("SetDisplayBind pid failed, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::GetFunctionKeyState(int32_t funcKey, bool &state)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, funcKey, &state] {
            return sMsgHandler_.OnGetFunctionKeyState(funcKey, state);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Failed to get the keyboard status, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    return RET_OK;
}

int32_t MMIService::SetFunctionKeyState(int32_t funcKey, bool enable)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    int32_t clientPid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, clientPid, funcKey, enable] {
            return sMsgHandler_.OnSetFunctionKeyState(clientPid, funcKey, enable);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Failed to update the keyboard status, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    return RET_OK;
}

int32_t MMIService::SetPointerLocation(int32_t x, int32_t y, int32_t displayId)
{
    CALL_INFO_TRACE;
#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
    int32_t ret = delegateTasks_.PostSyncTask(
        [x, y, displayId] {
            return ::OHOS::DelayedSingleton<MouseEventNormalize>::GetInstance()->SetPointerLocation(x, y, displayId);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set pointer location failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING
    return RET_OK;
}

void MMIService::OnDelegateTask(epoll_event &ev)
{
    if ((ev.events & EPOLLIN) == 0) {
        MMI_HILOGW("Not epollin");
        return;
    }
    DelegateTasks::TaskData data = {};
    auto res = read(delegateTasks_.GetReadFd(), &data, sizeof(data));
    if (res == -1) {
        MMI_HILOGW("Read failed erron:%{public}d", errno);
    }
    MMI_HILOGD("RemoteRequest notify td:%{public}" PRId64 ",std:%{public}" PRId64 ""
        ",taskId:%{public}d",
        GetThisThreadId(), data.tid, data.taskId);
    delegateTasks_.ProcessTasks();
}

void MMIService::OnThread()
{
    SetThreadName(std::string("mmi_service"));
    uint64_t tid = GetThisThreadId();
    delegateTasks_.SetWorkerThreadId(tid);
    MMI_HILOGI("Main worker thread start. tid:%{public}" PRId64 "", tid);
#ifdef OHOS_BUILD_PC_PRIORITY
    SetMmiServicePriority();
#endif // OHOS_BUILD_PC_PRIORITY
#ifdef OHOS_RSS_CLIENT
    tid_.store(tid);
#endif // OHOS_RSS_CLIENT
    PreEventLoop();

    while (state_ == ServiceRunningState::STATE_RUNNING) {
#if defined(OHOS_BUILD_ENABLE_FINGERSENSE_WRAPPER) && defined(OHOS_BUILD_ENABLE_KEYBOARD)
        if (isCesStart_ && !DISPLAY_MONITOR->IsCommonEventSubscriberInit()) {
            DISPLAY_MONITOR->InitCommonEventSubscriber();
        }
#endif // OHOS_BUILD_ENABLE_FINGERSENSE_WRAPPER && OHOS_BUILD_ENABLE_KEYBOARD
        epoll_event ev[MAX_EVENT_SIZE] = {};
        int32_t timeout = TimerMgr->CalcNextDelay();
        MMI_HILOGD("timeout:%{public}d", timeout);
        int32_t count = EpollWait(ev[0], MAX_EVENT_SIZE, timeout, mmiFd_);
        for (int32_t i = 0; i < count && state_ == ServiceRunningState::STATE_RUNNING; i++) {
            auto mmiEdIter = epollEventMap_.find(ev[i].data.fd);
            if (mmiEdIter == epollEventMap_.end()) {
                MMI_HILOGW("Invalid event %{public}d %{public}d", ev[i].data.fd, count);
                continue;
            }
            std::shared_ptr<mmi_epoll_event> mmiEd = mmiEdIter->second;
            CHKPC(mmiEd);
            epoll_event event = ev[i];
            if (mmiEd->event_type == EPOLL_EVENT_INPUT) {
                CalculateFuntionRunningTime([this, &mmiEd] () { libinputAdapter_.EventDispatch(mmiEd->fd); },
                    "EPOLL_EVENT_INPUT");
            } else if (mmiEd->event_type == EPOLL_EVENT_SOCKET) {
                CalculateFuntionRunningTime([this, &event]() { this->OnEpollEvent(event); }, "MMI:EPOLL_EVENT_SOCKET");
            } else if (mmiEd->event_type == EPOLL_EVENT_SIGNAL) {
                OnSignalEvent(mmiEd->fd);
            } else if (mmiEd->event_type == EPOLL_EVENT_ETASK) {
                CalculateFuntionRunningTime([this, &event]() { this->OnDelegateTask(event); }, "MMI:EPOLL_EVENT_ETASK");
            } else {
                MMI_HILOGW("Unknown epoll event type:%{public}d", mmiEd->event_type);
            }
        }
        TimerMgr->ProcessTimers();
        if (state_ != ServiceRunningState::STATE_RUNNING) {
            break;
        }
    }
    MMI_HILOGI("Main worker thread stop. tid:%{public}" PRId64 "", tid);
}

#ifdef OHOS_BUILD_PC_PRIORITY
void MMIService::SetMmiServicePriority()
{
    struct sched_param param = {0};
    param.sched_priority = PC_PRIORITY;
    int32_t schRet = sched_setscheduler(0, SCHED_FIFO, &param);
    if (schRet != 0) {
        MMI_HILOGE("mmi_service Couldn't set SCHED_FIFO, schRet:%{public}d", schRet);
    } else {
        MMI_HILOGI("The mmi_service set SCHED_FIFO succeed, schRet:%{public}d", schRet);
    }
}
#endif // OHOS_BUILD_PC_PRIORITY

void MMIService::PreEventLoop()
{
#if defined(OHOS_BUILD_ENABLE_TOUCH) && defined(OHOS_BUILD_ENABLE_MONITOR)
    SetupTouchGestureHandler();
#endif // defined(OHOS_BUILD_ENABLE_TOUCH) && defined(OHOS_BUILD_ENABLE_MONITOR)
    libinputAdapter_.ProcessPendingEvents();
}

bool MMIService::InitSignalHandler()
{
    CALL_DEBUG_ENTER;
    sigset_t mask = { 0 };
    int32_t retCode = sigfillset(&mask);
    if (retCode < 0) {
        MMI_HILOGE("Fill signal set failed:%{public}d", errno);
        return false;
    }

    retCode = sigprocmask(SIG_SETMASK, &mask, nullptr);
    if (retCode < 0) {
        MMI_HILOGE("Sigprocmask failed:%{public}d", errno);
        return false;
    }

    int32_t fdSignal = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (fdSignal < 0) {
        MMI_HILOGE("Signal fd failed:%{public}d", errno);
        return false;
    }

    retCode = AddEpoll(EPOLL_EVENT_SIGNAL, fdSignal);
    if (retCode < 0) {
        MMI_HILOGE("AddEpoll signalFd failed:%{public}d", retCode);
        close(fdSignal);
        return false;
    }
    return true;
}

void MMIService::OnSignalEvent(int32_t signalFd)
{
    CALL_DEBUG_ENTER;
    signalfd_siginfo sigInfo;
    int32_t size = ::read(signalFd, &sigInfo, sizeof(signalfd_siginfo));
    if (size != static_cast<int32_t>(sizeof(signalfd_siginfo))) {
        MMI_HILOGE("Read signal info failed, invalid size:%{public}d, errno:%{public}d", size, errno);
        return;
    }
    int32_t signo = static_cast<int32_t>(sigInfo.ssi_signo);
    MMI_HILOGD("Receive signal:%{public}d", signo);
    switch (signo) {
        case SIGINT:
        case SIGQUIT:
        case SIGILL:
        case SIGABRT:
        case SIGBUS:
        case SIGFPE:
        case SIGKILL:
        case SIGSEGV:
        case SIGTERM: {
            state_ = ServiceRunningState::STATE_EXIT;
            break;
        }
        default: {
            break;
        }
    }
}

void MMIService::AddReloadDeviceTimer()
{
    CALL_DEBUG_ENTER;
    TimerMgr->AddTimer(RELOAD_DEVICE_TIME, REPEAT_COUNT, [this]() {
        auto deviceIds = INPUT_DEV_MGR->GetInputDeviceIds();
        if (deviceIds.empty()) {
            libinputAdapter_.ReloadDevice();
        }
    });
}

int32_t MMIService::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    CALL_DEBUG_ENTER;
    if (fd < 0) {
        MMI_HILOGE("The fd is invalid");
        return DUMP_PARAM_ERR;
    }
    if (args.empty()) {
        MMI_HILOGE("The args cannot be empty");
        mprintf(fd, "args cannot be empty\n");
        MMIEventDump->DumpHelp(fd);
        return DUMP_PARAM_ERR;
    }
    std::vector<std::string> argList = { "" };
    std::transform(args.begin(), args.end(), std::back_inserter(argList),
        [](const std::u16string &arg) { return Str16ToStr8(arg); });
    MMIEventDump->ParseCommand(fd, argList);
    return RET_OK;
}

int32_t MMIService::SetMouseCaptureMode(int32_t windowId, bool isCaptureMode)
{
    CALL_INFO_TRACE;
    int32_t ret = delegateTasks_.PostSyncTask(
        [windowId, isCaptureMode] {
            return ::OHOS::MMI::IInputWindowsManager::GetInstance()->SetMouseCaptureMode(windowId, isCaptureMode);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set capture failed, return:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::OnGetWindowPid(int32_t windowId, int32_t &windowPid)
{
    CALL_DEBUG_ENTER;
    windowPid = WIN_MGR->GetWindowPid(windowId);
    if (windowPid == RET_ERR) {
        MMI_HILOGE("Get window pid failed");
        return RET_ERR;
    }
    MMI_HILOGD("The windowpid is:%{public}d", windowPid);
    return RET_OK;
}

int32_t MMIService::GetWindowPid(int32_t windowId)
{
    CALL_INFO_TRACE;
    int32_t windowPid = INVALID_PID;
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, windowId, &windowPid] {
            return this->OnGetWindowPid(windowId, windowPid);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("OnGetWindowPid failed, ret:%{public}d", ret);
        return ret;
    }
    MMI_HILOGD("The windowpid is:%{public}d", windowPid);
    return windowPid;
}

int32_t MMIService::AppendExtraData(const ExtraData &extraData)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask(
        [extraData] {
            return ::OHOS::MMI::IInputWindowsManager::GetInstance()->AppendExtraData(extraData);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Append extra data failed:%{public}d", ret);
    }
    return ret;
}

int32_t MMIService::EnableInputDevice(bool enable)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask(
        [enable] {
            return ::OHOS::MMI::InputDeviceManager::GetInstance()->OnEnableInputDevice(enable);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("OnEnableInputDevice failed:%{public}d", ret);
    }
    return ret;
}

#if defined(OHOS_BUILD_ENABLE_KEYBOARD) && defined(OHOS_BUILD_ENABLE_COMBINATION_KEY)
int32_t MMIService::UpdateCombineKeyState(bool enable)
{
    auto eventSubscriberHandler = InputHandler->GetSubscriberHandler();
    CHKPR(eventSubscriberHandler, RET_ERR);
    int32_t ret = eventSubscriberHandler->EnableCombineKey(enable);
    if (ret != RET_OK) {
        MMI_HILOGE("EnableCombineKey is failed in key command:%{public}d", ret);
    }

    auto eventKeyCommandHandler = InputHandler->GetKeyCommandHandler();
    CHKPR(eventKeyCommandHandler, RET_ERR);
    ret = eventKeyCommandHandler->EnableCombineKey(enable);
    if (ret != RET_OK) {
        MMI_HILOGE("EnableCombineKey is failed in key command:%{public}d", ret);
    }
    return ret;
}
#endif // OHOS_BUILD_ENABLE_KEYBOARD && OHOS_BUILD_ENABLE_COMBINATION_KEY

int32_t MMIService::CheckPidPermission(int32_t pid)
{
    CALL_DEBUG_ENTER;
    int32_t checkingPid = GetCallingPid();
    if (checkingPid != pid) {
        MMI_HILOGE("Check pid failed, input pid:%{public}d, but checking pid:%{public}d", pid, checkingPid);
        return RET_ERR;
    }
    return RET_OK;
}

int32_t MMIService::EnableCombineKey(bool enable)
{
    CALL_DEBUG_ENTER;
#if defined(OHOS_BUILD_ENABLE_KEYBOARD) && defined(OHOS_BUILD_ENABLE_COMBINATION_KEY)
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, enable] {
            return this->UpdateCombineKeyState(enable);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set key down duration failed:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_KEYBOARD && OHOS_BUILD_ENABLE_COMBINATION_KEY
    return RET_OK;
}

#if defined(OHOS_BUILD_ENABLE_KEYBOARD) && defined(OHOS_BUILD_ENABLE_COMBINATION_KEY)
int32_t MMIService::UpdateSettingsXml(const std::string &businessId, int32_t delay)
{
    std::shared_ptr<KeyCommandHandler> eventKeyCommandHandler = InputHandler->GetKeyCommandHandler();
    CHKPR(eventKeyCommandHandler, RET_ERR);
    return eventKeyCommandHandler->UpdateSettingsXml(businessId, delay);
}
#endif // OHOS_BUILD_ENABLE_KEYBOARD && OHOS_BUILD_ENABLE_COMBINATION_KEY

int32_t MMIService::SetKeyDownDuration(const std::string &businessId, int32_t delay)
{
    CALL_INFO_TRACE;
#if defined(OHOS_BUILD_ENABLE_KEYBOARD) && defined(OHOS_BUILD_ENABLE_COMBINATION_KEY)
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, businessId, delay] {
            return this->UpdateSettingsXml(businessId, delay);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set key down duration failed:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_KEYBOARD && OHOS_BUILD_ENABLE_COMBINATION_KEY
    return RET_OK;
}

#ifdef OHOS_BUILD_ENABLE_POINTER
int32_t MMIService::ReadTouchpadScrollSwich(bool &switchFlag)
{
    MouseEventHdr->GetTouchpadScrollSwitch(switchFlag);
    return RET_OK;
}

int32_t MMIService::ReadTouchpadScrollDirection(bool &switchFlag)
{
    MouseEventHdr->GetTouchpadScrollDirection(switchFlag);
    return RET_OK;
}

int32_t MMIService::ReadTouchpadTapSwitch(bool &switchFlag)
{
    MouseEventHdr->GetTouchpadTapSwitch(switchFlag);
    return RET_OK;
}

int32_t MMIService::ReadTouchpadPointerSpeed(int32_t &speed)
{
    MouseEventHdr->GetTouchpadPointerSpeed(speed);
    return RET_OK;
}

int32_t MMIService::ReadTouchpadCDG(TouchpadCDG &touchpadCDG)
{
    MouseEventHdr->GetTouchpadCDG(touchpadCDG);
    return RET_OK;
}

int32_t MMIService::ReadTouchpadPinchSwitch(bool &switchFlag)
{
    TOUCH_EVENT_HDR->GetTouchpadPinchSwitch(switchFlag);
    return RET_OK;
}

int32_t MMIService::ReadTouchpadSwipeSwitch(bool &switchFlag)
{
    TOUCH_EVENT_HDR->GetTouchpadSwipeSwitch(switchFlag);
    return RET_OK;
}

int32_t MMIService::ReadTouchpadRightMenuType(int32_t &type)
{
    MouseEventHdr->GetTouchpadRightClickType(type);
    return RET_OK;
}

int32_t MMIService::ReadTouchpadRotateSwitch(bool &rotateSwitch)
{
    TOUCH_EVENT_HDR->GetTouchpadRotateSwitch(rotateSwitch);
    return RET_OK;
}

int32_t MMIService::ReadTouchpadDoubleTapAndDragState(bool &switchFlag)
{
    TOUCH_EVENT_HDR->GetTouchpadDoubleTapAndDragState(switchFlag);
    return RET_OK;
}

#endif // OHOS_BUILD_ENABLE_POINTER

int32_t MMIService::SetTouchpadScrollSwitch(bool switchFlag)
{
    CALL_INFO_TRACE;
#if defined OHOS_BUILD_ENABLE_POINTER
    int32_t clientPid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [clientPid, switchFlag] {
            return ::OHOS::DelayedSingleton<MouseEventNormalize>::GetInstance()->SetTouchpadScrollSwitch(clientPid,
                switchFlag);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set touchpad scroll switch failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::GetTouchpadScrollSwitch(bool &switchFlag)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &switchFlag] {
            return this->ReadTouchpadScrollSwich(switchFlag);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get touchpad scroll switch failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::SetTouchpadScrollDirection(bool state)
{
    CALL_INFO_TRACE;
#if defined OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [state] {
            return ::OHOS::DelayedSingleton<MouseEventNormalize>::GetInstance()->SetTouchpadScrollDirection(state);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set touchpad scroll direction switch failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::GetTouchpadScrollDirection(bool &state)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &state] {
            return this->ReadTouchpadScrollDirection(state);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get touchpad scroll direction switch failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::SetTouchpadTapSwitch(bool switchFlag)
{
    CALL_INFO_TRACE;
#if defined OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [switchFlag] {
            return ::OHOS::DelayedSingleton<MouseEventNormalize>::GetInstance()->SetTouchpadTapSwitch(switchFlag);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set touchpad tap switch failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::GetTouchpadTapSwitch(bool &switchFlag)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &switchFlag] {
            return this->ReadTouchpadTapSwitch(switchFlag);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get touchpad tap switch failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::SetTouchpadPointerSpeed(int32_t speed)
{
    CALL_INFO_TRACE;
#if defined OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [speed] {
            return ::OHOS::DelayedSingleton<MouseEventNormalize>::GetInstance()->SetTouchpadPointerSpeed(speed);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set touchpad speed failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::GetTouchpadPointerSpeed(int32_t &speed)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &speed] {
            return this->ReadTouchpadPointerSpeed(speed);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get touchpad speed failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::GetTouchpadCDG(TouchpadCDG &touchpadCDG)
{
    CALL_INFO_TRACE;
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &touchpadCDG] {
            return this->ReadTouchpadCDG(touchpadCDG);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get touchpad option failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
    return RET_OK;
}

int32_t MMIService::SetTouchpadPinchSwitch(bool switchFlag)
{
    CALL_INFO_TRACE;
#if defined OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [switchFlag] {
            return ::OHOS::DelayedSingleton<TouchEventNormalize>::GetInstance()->SetTouchpadPinchSwitch(switchFlag);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set touch pad pinch switch failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::GetTouchpadPinchSwitch(bool &switchFlag)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &switchFlag] {
            return this->ReadTouchpadPinchSwitch(switchFlag);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get touch pad pinch switch failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::SetTouchpadSwipeSwitch(bool switchFlag)
{
    CALL_INFO_TRACE;
#if defined OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [switchFlag] {
            return ::OHOS::DelayedSingleton<TouchEventNormalize>::GetInstance()->SetTouchpadSwipeSwitch(switchFlag);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set touchpad swipe switch failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::GetTouchpadSwipeSwitch(bool &switchFlag)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &switchFlag] {
            return this->ReadTouchpadSwipeSwitch(switchFlag);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get touchpad swipe switch failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::SetTouchpadRightClickType(int32_t type)
{
    CALL_INFO_TRACE;
#if defined OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [type] {
            return ::OHOS::DelayedSingleton<MouseEventNormalize>::GetInstance()->SetTouchpadRightClickType(type);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set touchpad right button menu type failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::GetTouchpadRightClickType(int32_t &type)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &type] {
            return this->ReadTouchpadRightMenuType(type);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get touchpad right button menu type failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::SetTouchpadRotateSwitch(bool rotateSwitch)
{
    CALL_INFO_TRACE;
#if defined OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [rotateSwitch] {
            return ::OHOS::DelayedSingleton<TouchEventNormalize>::GetInstance()->SetTouchpadRotateSwitch(rotateSwitch);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set touchpad rotate switch failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::GetTouchpadRotateSwitch(bool &rotateSwitch)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &rotateSwitch] {
            return this->ReadTouchpadRotateSwitch(rotateSwitch);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get touchpad rotate switch failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::SetTouchpadDoubleTapAndDragState(bool switchFlag)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [switchFlag] {
            return ::OHOS::DelayedSingleton<TouchEventNormalize>::GetInstance()->SetTouchpadDoubleTapAndDragState(
                switchFlag);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Failed to SetTouchpadDoubleTapAndDragState status, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::GetTouchpadDoubleTapAndDragState(bool &switchFlag)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &switchFlag] {
            return this->ReadTouchpadDoubleTapAndDragState(switchFlag);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Failed to GetTouchpadDoubleTapAndDragState status, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::SetShieldStatus(int32_t shieldMode, bool isShield)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, shieldMode, isShield] {
            return sMsgHandler_.SetShieldStatus(shieldMode, isShield);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set shield event interception state failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    return RET_OK;
}

int32_t MMIService::GetShieldStatus(int32_t shieldMode, bool &isShield)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, shieldMode, &isShield] {
            return sMsgHandler_.GetShieldStatus(shieldMode, isShield);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Failed to set shield event interception status, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    return RET_OK;
}

int32_t MMIService::GetKeyState(std::vector<int32_t> &pressedKeys, std::map<int32_t, int32_t> &specialKeysState)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_KEYBOARD
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &pressedKeys, &specialKeysState] {
            return this->OnGetKeyState(pressedKeys, specialKeysState);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get pressed keys failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_KEYBOARD
    return RET_OK;
}

int32_t MMIService::Authorize(bool isAuthorize)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, isAuthorize] {
            return this->OnAuthorize(isAuthorize);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("OnAuthorize failed, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::OnAuthorize(bool isAuthorize)
{
    return sMsgHandler_.OnAuthorize(isAuthorize);
}

int32_t MMIService::CancelInjection()
{
    CALL_DEBUG_ENTER;
    int32_t callPid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, callPid] {
            return this->OnCancelInjection(callPid);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("OnCancelInjection failed, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::OnCancelInjection(int32_t callPid)
{
    return sMsgHandler_.OnCancelInjection(callPid);
}

int32_t MMIService::HasIrEmitter(bool &hasIrEmitter)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &hasIrEmitter] {
            return this->OnHasIrEmitter(hasIrEmitter);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("OnHasIrEmitter failed, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::GetInfraredFrequencies(std::vector<InfraredFrequency>& frequencies)
{
    CALL_DEBUG_ENTER;
#ifndef OHOS_BUILD_ENABLE_WATCH
    MMI_HILOGI("Start get infrared frequency");
    std::vector<InfraredFrequencyInfo> infos;
    if (!InfraredEmitterController::GetInstance()->GetFrequencies(infos)) {
        MMI_HILOGE("Failed to get frequencies");
        return RET_ERR;
    }
    for (auto &item : infos) {
        InfraredFrequency info;
        info.min_ = item.min_;
        info.max_ = item.max_;
        frequencies.push_back(info);
    }
    std::string context = "";
    int32_t size = static_cast<int32_t>(frequencies.size());
    for (int32_t i = 0; i < size; i++) {
        context = context + "frequencies[" + std::to_string(i) + "]. max=" + std::to_string(frequencies[i].max_) +
        ",min=" + std::to_string(frequencies[i].min_) + ";";
    }
    MMI_HILOGD("Data from hdf context:%{public}s", context.c_str());
#endif // OHOS_BUILD_ENABLE_WATCH
    return RET_OK;
}

int32_t MMIService::TransmitInfrared(int64_t number, std::vector<int64_t>& pattern)
{
    CALL_DEBUG_ENTER;
#ifndef OHOS_BUILD_ENABLE_WATCH
    std::string context = "infraredFrequency:" + std::to_string(number) + ";";
    int32_t size = static_cast<int32_t>(pattern.size());
    for (int32_t i = 0; i < size; i++) {
        context = context + "index:" + std::to_string(i) + ": pattern:" + std::to_string(pattern[i]) + ";";
    }
    MMI_HILOGI("TransmitInfrared para context:%{public}s", context.c_str());
    if (!InfraredEmitterController::GetInstance()->Transmit(number, pattern)) {
        MMI_HILOGE("Failed to transmit");
        return RET_ERR;
    }
#endif // OHOS_BUILD_ENABLE_WATCH
    return RET_OK;
}

#ifdef OHOS_BUILD_ENABLE_VKEYBOARD
int32_t MMIService::CreateVKeyboardDevice(sptr<IRemoteObject> &vkeyboardDevice)
{
    CALL_INFO_TRACE;
    isHPR_ = PRODUCT_TYPE == DEVICE_TYPE_HPR;
    if (!isHPR_) {
        MMI_HILOGE("Failed to create vkeyboard device, feature not support");
        return RET_ERR;
    }
    int32_t ret = RET_OK;
    // init keyboard handler
    if (g_VKeyboardHandle == nullptr) {
        InitVKeyboardFuncHandler();
    }
    if (g_VKeyboardHandle == nullptr) {
        MMI_HILOGE("VKeyboard handler is nullptr");
        return RET_ERR;
    } else {
        ret = delegateTasks_.PostSyncTask(
            [this, &vkeyboardDevice] {
                return this->OnCreateVKeyboardDevice(vkeyboardDevice);
            }
            );
        if (ret != RET_OK) {
            MMI_HILOGE("Failed to create vkeyboard device, ret:%{public}d", ret);
        }
    }
    return ret;
}

int32_t MMIService::OnCreateVKeyboardDevice(sptr<IRemoteObject> &vkeyboardDevice)
{
    if (g_VKeyboardHandle == nullptr) {
        MMI_HILOGE("VKeyboard handler is nullptr");
        return RET_ERR;
    }
    vkeyboard_createVKeyboardDevice_ = (VKEYBOARD_CREATEVKEYBOARDDEVICE_TYPE)dlsym(
        g_VKeyboardHandle, "CreateVKeyboardDevice");
    IRemoteObject* vkbDevice = nullptr;
    int32_t ret = vkeyboard_createVKeyboardDevice_(vkbDevice);
    if (ret != RET_OK) {
        MMI_HILOGE("Create vkeyboard device failed");
        return ret;
    }
    if (vkbDevice == nullptr) {
        MMI_HILOGE("VKeyboard device pointer is nullptr");
        return RET_ERR;
    }
    vkeyboardDevice = sptr(vkbDevice);

    vkeyboard_onFuncKeyEvent_ = (VKEYBOARD_ONFUNCKEYEVENT_TYPE)dlsym(
        g_VKeyboardHandle, "OnFuncKeyEvent");
    
    auto keyEvent = KeyEventHdr->GetKeyEvent();
    CHKPR(keyEvent, ERROR_NULL_POINTER);
    if (vkeyboard_onFuncKeyEvent_ != nullptr) {
        vkeyboard_onFuncKeyEvent_(keyEvent);
    }
    return RET_OK;
}

void MMIService::InitVKeyboardFuncHandler()
{
    if (isHPR_) {
        // Initialize vkeyboard handler
        g_VKeyboardHandle = dlopen(VKEYBOARD_PATH.c_str(), RTLD_NOW);
        if (g_VKeyboardHandle != nullptr) {
            handleTouchPoint_ = (HANDLE_TOUCHPOINT_TYPE)dlsym(g_VKeyboardHandle, "HandleTouchPoint");
            statemachineMessageQueue_getLibinputMessage_ = (STATEMACINEMESSAGQUEUE_GETLIBINPUTMESSAGE_TYPE)dlsym(
                g_VKeyboardHandle, "StateMachineMessageQueueGetLibinputMessage");
            trackPadEngine_getAllTouchMessage_ = (TRACKPADENGINE_GETALLTOUCHMESSAGE_TYPE)dlsym(
                g_VKeyboardHandle, "TrackPadEngineGetAllTouchMessage");
            trackPadEngine_clearTouchMessage_ = (TRACKPADENGINE_CLEARTOUCHMESSAGE_TYPE)dlsym(
                g_VKeyboardHandle, "TrackPadEngineClearTouchMessage");
            trackPadEngine_getAllKeyMessage_ = (TRACKPADENGINE_GETALLKEYMESSAGE_TYPE)dlsym(
                g_VKeyboardHandle, "TrackPadEngineGetAllKeyMessage");
            trackPadEngine_clearKeyMessage_ = (TRACKPADENGINE_CLEARKEYMESSAGE_TYPE)dlsym(
                g_VKeyboardHandle, "TrackPadEngineClearKeyMessage");
            vkeyboard_hardwareKeyEventDetected_ = (VKEYBOARD_HARDWAREKEYEVENTDETECTED_TYPE)dlsym(
                g_VKeyboardHandle, "HardwareKeyEventDetected");
            vkeyboard_getKeyboardActivationState_ = (VKEYBOARD_GETKEYBOARDACTIVATIONSTATE_TYPE)dlsym(
                g_VKeyboardHandle, "GetKeyboardActivationState");

            libinputAdapter_.InitVKeyboard(handleTouchPoint_,
                statemachineMessageQueue_getLibinputMessage_,
                trackPadEngine_getAllTouchMessage_,
                trackPadEngine_clearTouchMessage_,
                trackPadEngine_getAllKeyMessage_,
                trackPadEngine_clearKeyMessage_,
                vkeyboard_hardwareKeyEventDetected_,
                vkeyboard_getKeyboardActivationState_);
        }
    }
}
#endif // OHOS_BUILD_ENABLE_VKEYBOARD

int32_t MMIService::OnHasIrEmitter(bool &hasIrEmitter)
{
    hasIrEmitter = false;
    return RET_OK;
}

int32_t MMIService::SetPixelMapData(int32_t infoId, void* pixelMap)
{
    CALL_DEBUG_ENTER;
    CHKPR(pixelMap, ERROR_NULL_POINTER);
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, infoId, pixelMap] {
            return sMsgHandler_.SetPixelMapData(infoId, pixelMap);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Failed to set pixelmap, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

void MMIService::InitPreferences()
{
    PREFERENCES_MGR->InitPreferences();
#ifdef OHOS_BUILD_ENABLE_MOVE_EVENT_FILTERS
    int32_t ret = SetMoveEventFilters(PREFERENCES_MGR->GetBoolValue("moveEventFilterFlag", false));
    if (ret != RET_OK) {
        MMI_HILOGE("Failed to read moveEventFilterFlag, ret:%{public}d", ret);
    }
#endif // OHOS_BUILD_ENABLE_MOVE_EVENT_FILTERS
}

int32_t MMIService::SetMoveEventFilters(bool flag)
{
    CALL_DEBUG_ENTER;
#ifdef OHOS_BUILD_ENABLE_MOVE_EVENT_FILTERS
    int32_t ret = delegateTasks_.PostSyncTask(
        std::bind(&InputEventHandler::SetMoveEventFilters, InputHandler, flag));
    if (ret != RET_OK) {
        MMI_HILOGE("Failed to set move event filter flag, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_MOVE_EVENT_FILTERS
    return RET_OK;
}

int32_t MMIService::SetCurrentUser(int32_t userId)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask(
        [userId] {
            return ::OHOS::MMI::IInputWindowsManager::GetInstance()->SetCurrentUser(userId);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Failed to set current user, ret:%{public}d", ret);
        return ret;
    }
    auto eventKeyCommandHandler = InputHandler->GetKeyCommandHandler();
    CHKPR(eventKeyCommandHandler, RET_ERR);
    ret = delegateTasks_.PostSyncTask(
        [userId, eventKeyCommandHandler] {
            return eventKeyCommandHandler->RegisterKnuckleSwitchByUserId(userId);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Failed to set current user, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::SetTouchpadThreeFingersTapSwitch(bool switchFlag)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [switchFlag] {
            return ::OHOS::DelayedSingleton<TouchEventNormalize>::GetInstance()->SetTouchpadThreeFingersTapSwitch(
                switchFlag);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Failed to SetTouchpadThreeFingersTapSwitch status, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::GetTouchpadThreeFingersTapSwitch(bool &switchFlag)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [&switchFlag] {
            return ::OHOS::DelayedSingleton<TouchEventNormalize>::GetInstance()->GetTouchpadThreeFingersTapSwitch(
                switchFlag);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Failed to GetTouchpadThreeFingersTapSwitch status, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

int32_t MMIService::AddVirtualInputDevice(std::shared_ptr<InputDevice> device, int32_t &deviceId)
{
    CALL_DEBUG_ENTER;
    CHKPR(device, ERROR_NULL_POINTER);
    int32_t ret = delegateTasks_.PostSyncTask(
        [device, &deviceId] {
            return ::OHOS::MMI::InputDeviceManager::GetInstance()->AddVirtualInputDevice(device, deviceId);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("AddVirtualInputDevice failed:%{public}d", ret);
    }
    return ret;
}

int32_t MMIService::RemoveVirtualInputDevice(int32_t deviceId)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask(
        [deviceId] {
            return ::OHOS::MMI::InputDeviceManager::GetInstance()->RemoveVirtualInputDevice(deviceId);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("RemoveVirtualInputDevice failed:%{public}d", ret);
    }
    return ret;
}

int32_t MMIService::EnableHardwareCursorStats(bool enable)
{
    CALL_DEBUG_ENTER;
#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [pid, enable] {
            return IPointerDrawingManager::GetInstance()->EnableHardwareCursorStats(pid, enable);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Enable hardware cursor stats failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING
    return RET_OK;
}

int32_t MMIService::GetHardwareCursorStats(uint32_t &frameCount, uint32_t &vsyncCount)
{
    CALL_DEBUG_ENTER;
#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [pid, &frameCount, &vsyncCount] {
            return IPointerDrawingManager::GetInstance()->GetHardwareCursorStats(pid, frameCount, vsyncCount);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get hardware cursor stats failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING
    return RET_OK;
}

#ifdef OHOS_BUILD_ENABLE_MAGICCURSOR
int32_t MMIService::GetPointerSnapshot(void *pixelMapPtr)
{
    CALL_INFO_TRACE;
#if defined OHOS_BUILD_ENABLE_POINTER
    MMI_HILOGI("Get pointer snapshot from process(%{public}d)", GetCallingPid());
    int32_t ret = delegateTasks_.PostSyncTask(std::bind(
        std::bind(&IPointerDrawingManager::GetPointerSnapshot, IPointerDrawingManager::GetInstance(), pixelMapPtr)));
    if (ret != RET_OK) {
        MMI_HILOGE("Get the pointer snapshot failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_MAGICCURSOR

int32_t MMIService::SetTouchpadScrollRows(int32_t rows)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [rows] {
            return ::OHOS::DelayedSingleton<TouchEventNormalize>::GetInstance()->SetTouchpadScrollRows(rows);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set the number of touchpad scrolling rows failed, return %{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

#ifdef OHOS_BUILD_ENABLE_POINTER
int32_t MMIService::ReadTouchpadScrollRows(int32_t &rows)
{
    rows = TOUCH_EVENT_HDR->GetTouchpadScrollRows();
    return RET_OK;
}
#endif // OHOS_BUILD_ENABLE_POINTER

int32_t MMIService::GetTouchpadScrollRows(int32_t &rows)
{
    CALL_INFO_TRACE;
#ifdef OHOS_BUILD_ENABLE_POINTER
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &rows] {
            return this->ReadTouchpadScrollRows(rows);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Get the number of touchpad scrolling rows failed, return %{public}d, pid:%{public}d", ret,
            GetCallingPid());
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

#ifdef OHOS_BUILD_ENABLE_ANCO
int32_t MMIService::AncoAddChannel(sptr<IAncoChannel> channel)
{
    int32_t ret = delegateTasks_.PostSyncTask([channel]() {
        return WIN_MGR->AncoAddChannel(channel);
    });
    if (ret != RET_OK) {
        MMI_HILOGE("AncoAddChannel fail, error:%{public}d", ret);
    }
    SyncKnuckleStatus();
    return ret;
}

int32_t MMIService::AncoRemoveChannel(sptr<IAncoChannel> channel)
{
    int32_t ret = delegateTasks_.PostSyncTask([channel]() {
        return WIN_MGR->AncoRemoveChannel(channel);
    });
    if (ret != RET_OK) {
        MMI_HILOGE("AncoRemoveChannel fail, error:%{public}d", ret);
    }
    return ret;
}
#endif // OHOS_BUILD_ENABLE_ANCO

int32_t MMIService::TransferBinderClientSrv(const sptr<IRemoteObject> &binderClientObject)
{
    CALL_DEBUG_ENTER;
    int32_t pid = GetCallingPid();
    int32_t ret =
        delegateTasks_.PostSyncTask(
            [this, pid, binderClientObject] {
                return sMsgHandler_.OnTransferBinderClientSrv(binderClientObject, pid);
            }
        );
    MMI_HILOGI("TransferBinderClientSrv result:%{public}d", ret);
    return ret;
}

void MMIService::CalculateFuntionRunningTime(std::function<void()> func, const std::string &flag)
{
    std::function<void (void *)> printLog = std::bind(&MMIService::PrintLog, this, flag, THREAD_BLOCK_TIMER_SPAN_S,
        getpid(), gettid());
    int32_t id = HiviewDFX::XCollie::GetInstance().SetTimer(flag, THREAD_BLOCK_TIMER_SPAN_S, printLog, nullptr,
        HiviewDFX::XCOLLIE_FLAG_NOOP);
    func();
    HiviewDFX::XCollie::GetInstance().CancelTimer(id);
}

void MMIService::PrintLog(const std::string &flag, int32_t duration, int32_t pid, int32_t tid)
{
    std::string dfxThreadBlockMsg { "MMIBlockTask name:" };
    dfxThreadBlockMsg += flag;
    dfxThreadBlockMsg += ", duration time:";
    dfxThreadBlockMsg += std::to_string(duration);
    dfxThreadBlockMsg += ", pid:";
    dfxThreadBlockMsg += std::to_string(pid);
    dfxThreadBlockMsg += ", tid:";
    dfxThreadBlockMsg += std::to_string(tid);
    MMI_HILOGW("DfxThreadBlockMsg:%{public}s", dfxThreadBlockMsg.c_str());
    OHOS::HiviewDFX::DfxDumpCatcher dumpCatcher;
    dumpCatcher.DumpCatch(pid, tid, dfxThreadBlockMsg, MAX_FRAME_NUMS, false);
    MMI_HILOGW("BlockMsg:%{public}s", dfxThreadBlockMsg.c_str());
}

int32_t MMIService::SkipPointerLayer(bool isSkip)
{
    CALL_INFO_TRACE;
#if defined(OHOS_BUILD_ENABLE_POINTER) && defined(OHOS_BUILD_ENABLE_POINTER_DRAWING)
    int32_t ret = delegateTasks_.PostSyncTask(
        [isSkip] {
            return IPointerDrawingManager::GetInstance()->SkipPointerLayer(isSkip);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Skip pointer layerfailed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER && OHOS_BUILD_ENABLE_POINTER_DRAWING
    return RET_OK;
}

void MMIService::OnSessionDelete(SessionPtr session)
{
    CALL_DEBUG_ENTER;
    CHKPV(session);
    std::string programName = session->GetProgramName();
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = clientInfos_.find(programName);
    if (it != clientInfos_.end()) {
        clientInfos_.erase(it);
        MMI_HILOGD("Clear the client info, programName:%{public}s", programName.c_str());
    }
}

int32_t MMIService::SetClientInfo(int32_t pid, uint64_t readThreadId)
{
    CALL_DEBUG_ENTER;
    auto sess = GetSessionByPid(pid);
    CHKPR(sess, ERROR_NULL_POINTER);
    std::string programName = sess->GetProgramName();
    std::lock_guard<std::mutex> guard(mutex_);
    if (clientInfos_.find(programName) != clientInfos_.end()) {
        clientInfos_[programName].pid = pid;
        clientInfos_[programName].readThreadId = readThreadId;
        return RET_OK;
    }
    ClientInfo clientInfo {
        .pid = pid,
        .readThreadId = readThreadId
    };
    clientInfos_[programName] = clientInfo;
    return RET_OK;
}

void MMIService::InitPrintClientInfo()
{
    CALL_DEBUG_ENTER;
    TimerMgr->AddLongTimer(PRINT_INTERVAL_TIME, -1, [this]() {
        ffrt::submit([this] {
            std::lock_guard<std::mutex> guard(mutex_);
            for (const auto &info : clientInfos_) {
                if (static_cast<uint64_t>(info.second.pid) == info.second.readThreadId) {
                    MMI_HILOGW("The application main thread and event reading thread are combined, such as:"
                    "programName:%{public}s, pid:%{public}d, mainThreadId:%{public}d, readThreadId:%{public}" PRIu64,
                    info.first.c_str(), info.second.pid, info.second.pid, info.second.readThreadId);
                    return;
                }
            }
            if (!clientInfos_.empty()) {
                auto it = clientInfos_.begin();
                MMI_HILOGI("The application main thread and event reading thread are separated, such as:"
                "programName:%{public}s, pid:%{public}d, mainThreadId:%{public}d, readThreadId:%{public}" PRIu64,
                it->first.c_str(), it->second.pid, it->second.pid, it->second.readThreadId);
            }
        });
    });
    std::function<void(SessionPtr)> callback = [this](SessionPtr sess) {
        return this->OnSessionDelete(sess);
    };
    AddSessionDeletedCallback(callback);
}

int32_t MMIService::GetIntervalSinceLastInput(int64_t &timeInterval)
{
    CALL_INFO_TRACE;
    int32_t ret = delegateTasks_.PostSyncTask(std::bind(&InputEventHandler::GetIntervalSinceLastInput,
        InputHandler, std::ref(timeInterval)));
    MMI_HILOGD("timeInterval:%{public}" PRId64, timeInterval);
    if (ret != RET_OK) {
        MMI_HILOGE("Failed to GetIntervalSinceLastInput, ret:%{public}d", ret);
    }
    return ret;
}

int32_t MMIService::GetAllSystemHotkeys(std::vector<std::unique_ptr<KeyOption>> &keyOptions)
{
    CALL_DEBUG_ENTER;
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &keyOptions] {
            return this->OnGetAllSystemHotkey(keyOptions);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGD("Get all system hot key, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::OnGetAllSystemHotkey(std::vector<std::unique_ptr<KeyOption>> &keyOptions)
{
    CALL_DEBUG_ENTER;
    #ifdef SHORTCUT_KEY_MANAGER_ENABLED
    return KEY_SHORTCUT_MGR->GetAllSystemHotkeys(keyOptions);
    #endif // SHORTCUT_KEY_MANAGER_ENABLED
    MMI_HILOGI("OnGetAllSystemHotkey function does not support");
    return ERROR_UNSUPPORT;
}

#if defined(OHOS_BUILD_ENABLE_TOUCH) && defined(OHOS_BUILD_ENABLE_MONITOR)
void MMIService::SetupTouchGestureHandler()
{
    touchGestureMgr_ = std::make_shared<TouchGestureManager>(delegateInterface_);
    WIN_MGR->AttachTouchGestureMgr(touchGestureMgr_);
}
#endif // defined(OHOS_BUILD_ENABLE_TOUCH) && defined(OHOS_BUILD_ENABLE_MONITOR)

int32_t MMIService::SetInputDeviceEnable(int32_t deviceId, bool enable, int32_t index, int32_t pid, SessionPtr sess)
{
    CALL_INFO_TRACE;
    CHKPR(sess, RET_ERR);
    int32_t ret = INPUT_DEV_MGR->SetInputDeviceEnabled(deviceId, enable, index, pid, sess);
    if (RET_OK != ret) {
        MMI_HILOGE("Set inputdevice enabled failed, return:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::SetInputDeviceEnabled(int32_t deviceId, bool enable, int32_t index)
{
    CALL_INFO_TRACE;
    int32_t pid = GetCallingPid();
    auto sess = GetSessionByPid(pid);
    int32_t ret = delegateTasks_.PostAsyncTask(
        [this, deviceId, enable, index, pid, sess] {
            return this->SetInputDeviceEnable(deviceId, enable, index, pid, sess);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Set inputdevice enable failed, return:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::ShiftAppPointerEvent(const ShiftWindowParam &param, bool autoGenDown)
{
    CALL_DEBUG_ENTER;
#if defined(OHOS_BUILD_ENABLE_POINTER) || defined(OHOS_BUILD_ENABLE_TOUCH)
    int32_t ret = delegateTasks_.PostSyncTask(
        [param, autoGenDown]() {
            return WIN_MGR->ShiftAppPointerEvent(param, autoGenDown);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("Shift AppPointerEvent failed, return:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER || OHOS_BUILD_ENABLE_TOUCH
    return RET_OK;
}

int32_t MMIService::SetCustomCursor(int32_t windowId, CustomCursor cursor, CursorOptions options)
{
    CALL_INFO_TRACE;
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(std::bind(
        [pid, windowId] {
            return WIN_MGR->CheckWindowIdPermissionByPid(windowId, pid);
        })
        );
    if (windowId > 0 && ret != RET_OK) {
        MMI_HILOGE("Set the custom cursor failed, ret:%{public}d", ret);
        return ERROR_WINDOW_ID_PERMISSION_DENIED;
    }
#if defined OHOS_BUILD_ENABLE_POINTER
    ret = delegateTasks_.PostSyncTask(std::bind(
        [pid, windowId, cursor, options] {
            return IPointerDrawingManager::GetInstance()->SetCustomCursor(pid, windowId, cursor, options);
        }
        ));
    if (ret != RET_OK) {
        MMI_HILOGE("Set the custom cursor failed, ret:%{public}d", ret);
        return ret;
    }
#endif // OHOS_BUILD_ENABLE_POINTER
    return RET_OK;
}

#ifdef OHOS_BUILD_ENABLE_ANCO
int32_t MMIService::CheckKnuckleEvent(float pointX, float pointY, bool &isKnuckleType)
{
    CALL_INFO_TRACE;
#if defined(OHOS_BUILD_ENABLE_FINGERSENSE_WRAPPER)
    int tryTimes = RETRY_CHECK_TIMES;
    int32_t ret = RET_ERR;
    for (int count = 0; count < tryTimes; ++count) {
        ret = FINGERSENSE_WRAPPER->CheckKnuckleEvent(pointX, pointY, isKnuckleType);
        if (ret == RET_OK) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(CHECK_EEVENT_INTERVAL_TIME));
    }
    return ret;
#endif // OHOS_BUILD_ENABLE_FINGERSENSE_WRAPPER
    return RET_OK;
}

int32_t MMIService::SyncKnuckleStatus()
{
    CALL_DEBUG_ENTER;
    int ret = delegateTasks_.PostSyncTask([] {
        auto keyHandler = InputHandler->GetKeyCommandHandler();
        if (keyHandler == nullptr) {
            return RET_ERR;
        }
        bool isKnuckleEnable = !keyHandler->SkipKnuckleDetect();
        return WIN_MGR->SyncKnuckleStatus(isKnuckleEnable);
    });
    if (ret != RET_OK) {
        MMI_HILOGE("post sync knuckle status fail, ret:%{public}d", ret);
    }
    return ret;
}
#endif

int32_t MMIService::SetMultiWindowScreenIdInner(uint64_t screenId, uint64_t displayNodeScreenId)
{
    TOUCH_DRAWING_MGR->SetMultiWindowScreenId(screenId, displayNodeScreenId);
    return RET_OK;
}

int32_t MMIService::SetMultiWindowScreenId(uint64_t screenId, uint64_t displayNodeScreenId)
{
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &screenId, &displayNodeScreenId] {
            return this->SetMultiWindowScreenIdInner(screenId, displayNodeScreenId);
        }
    );
    if (ret != RET_OK) {
        MMI_HILOGE("SetMultiWindowScreenId failed, return:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::SubscribeInputActive(int32_t subscribeId, int64_t interval)
{
    CALL_INFO_TRACE;
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, subscribeId, interval] {
            auto sess = this->GetSessionByPid(pid);
            CHKPR(sess, RET_ERR);
            auto subscriberHandler = InputHandler->GetInputActiveSubscriberHandler();
            CHKPR(subscriberHandler, RET_ERR);
            return subscriberHandler->SubscribeInputActive(sess, subscribeId, interval);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("The subscribe input active processed failed, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::UnsubscribeInputActive(int32_t subscribeId)
{
    CALL_INFO_TRACE;
    int32_t pid = GetCallingPid();
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, pid, subscribeId] {
            auto sess = this->GetSessionByPid(pid);
            CHKPR(sess, RET_ERR);
            auto subscriberHandler = InputHandler->GetInputActiveSubscriberHandler();
            CHKPR(subscriberHandler, RET_ERR);
            return subscriberHandler->UnsubscribeInputActive(sess, subscribeId);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("The unsubscribe input active processed failed, ret:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::SetInputDeviceConsumer(const std::vector<std::string>& deviceNames)
{
    CALL_INFO_TRACE;
    int32_t pid = GetCallingPid();
    auto sess = GetSessionByPid(pid);
    CHKPR(sess, ERROR_NULL_POINTER);
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &deviceNames, &sess] {
            return DEVICEHANDLER->SetDeviceConsumerHandler(deviceNames, sess);
        }
    );
    if (ret != RET_OK) {
        MMI_HILOGE("SetDeviceConsumerHandler failed, return:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::ClearInputDeviceConsumer(const std::vector<std::string>& deviceNames)
{
    CALL_INFO_TRACE;
    int32_t pid = GetCallingPid();
    auto sess = GetSessionByPid(pid);
    int32_t ret = delegateTasks_.PostSyncTask(
        [this, &deviceNames, &sess] {
            return DEVICEHANDLER->ClearDeviceConsumerHandler(deviceNames, sess);
        }
    );
    if (ret != RET_OK) {
        MMI_HILOGE("ClearInputDeviceConsumer failed, return:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}

int32_t MMIService::GetMaxMultiTouchPointNum(int32_t &pointNum)
{
    int ret = delegateTasks_.PostSyncTask(
        [&pointNum] () {
            auto productDeviceType = PRODUCT_DEVICE_TYPE;
            MMI_HILOGI("ProductDeviceType:%{public}s", productDeviceType.c_str());
            pointNum = MAX_MULTI_TOUCH_POINT_NUM;
            return RET_OK;
        }
    );
    if (ret != RET_OK) {
        MMI_HILOGE("GetMaxMultiTouchPointNum failed, return:%{public}d", ret);
    }
    return ret;
}

int32_t MMIService::SwitchScreenCapturePermission(uint32_t permissionType, bool enable)
{
    CALL_INFO_TRACE;
    int32_t pid = GetCallingPid();
    auto sess = GetSessionByPid(pid);
    auto eventKeyCommandHandler = InputHandler->GetKeyCommandHandler();
    CHKPR(eventKeyCommandHandler, RET_ERR);
    int32_t ret = delegateTasks_.PostSyncTask(
        [permissionType, enable, eventKeyCommandHandler] {
            return eventKeyCommandHandler->SwitchScreenCapturePermission(permissionType, enable);
        }
        );
    if (ret != RET_OK) {
        MMI_HILOGE("SwitchScreenCapturePermission failed, return:%{public}d", ret);
        return ret;
    }
    return RET_OK;
}
} // namespace MMI
} // namespace OHOS
