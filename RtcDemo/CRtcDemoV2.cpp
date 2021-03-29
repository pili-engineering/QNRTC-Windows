// CRtcDemoV2.cpp : implementation file
//
#include "stdafx.h"
#include "CRtcDemoV2.h"
#include "afxdialogex.h"
#include "resource.h"
#include <fstream>
#include "Global.h"
#include "QNErrorCode.h"

#define CHECK_CALLBACK_TIMER 10001       // SDK 事件驱动定时器
#define UPDATE_TIME_DURATION_TIMER 10002 // 定时更新连麦时长

UINT TrackInfoUI::WINDOW_ID = 1000000;

// 服务器合流配置的画布大小，后台配置
#define CANVAS_WIDTH 480
#define CANVAS_HEIGHT 848

#define CAMERA_TAG "camera"
#define MICROPHONE_TAG "microphone"
#define SCREENCASTS_TAG "screen"
#define EXTERNAL_TAG "external"


// CRtcDemoV2 dialog

IMPLEMENT_DYNAMIC(CRtcDemoV2, CDialogEx)

#define VOLUMEMAX   32767
#define VOLUMEMIN	-32768

#ifndef core_min
#define core_min(a, b)		((a) < (b) ? (a) : (b))
#endif

// 获取roomtoken,JoinRoom的第一个输入参数是此方法返回的token_。
// @param app_id_ 获取token所需打点appid
// @param room_name_  房间名
// @param user_id_  房间ID
// @param time_out_ 获取超时时间
// @param token_ 输出的roomtoken
// @return 返回0时表示成功，-1表示失败，-3表示获取超时
extern "C" QINIU_EXPORT_DLL int GetRoomToken_s(
    const std::string& app_id_,
    const std::string& room_name_,
    const std::string& user_id_,
    const std::string& host_name_,
    const int time_out_,
    std::string& token_);

// 获取音频分贝值
static uint32_t ProcessAudioLevel(const int16_t* data, const int32_t& data_size)
{
    uint32_t ret = 0;

    if (data_size > 0) {
        int32_t sum = 0;
        int16_t* pos = (int16_t *)data;
        for (int i = 0; i < data_size; i++) {
            sum += abs(*pos);
            pos++;
        }

        ret = sum * 500.0 / (data_size * VOLUMEMAX);
        ret = core_min(ret, 100);
    }

    return ret;
}

CRtcDemoV2::CRtcDemoV2(CWnd* main_dlg, CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_DIALOG_V2, pParent)
    , _main_dlg_ptr(main_dlg)
{
}

CRtcDemoV2::~CRtcDemoV2()
{
}

void CRtcDemoV2::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_PLAYER, _user_list_ctrl);
    DDX_Control(pDX, IDC_RICHEDIT_MSG, _msg_rich_edit_ctrl);
}

BOOL CRtcDemoV2::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    
    ReadConfigFile();

    std::string ver;
    qiniu_v2::QNRoomInterface::GetVersion(ver);
    TRACE("Sdk version: %s", ver.c_str());
    qiniu_v2::QNRoomInterface::SetLogParams(qiniu_v2::LOG_INFO, "rtc_log", "rtc.log");
    
    _rtc_room_interface = qiniu_v2::QNRoomInterface::ObtainRoomInterface();
    _rtc_room_interface->SetRoomListener(this);
    _rtc_room_interface->EnableStatistics(1000);

    _rtc_video_interface = _rtc_room_interface->ObtainVideoInterface();
    _rtc_video_interface->SetVideoListener(this);
    _rtc_audio_interface = _rtc_room_interface->ObtainAudioInterface();
    _rtc_audio_interface->SetAudioListener(this);
        
    // 定义一个定时器，在主线程定时执行 QNRoomInterface::Loop() 方法，以触发各种回调
    // 目的是为了让 SDK 的回调在主线程中执行，以方便 UI 操作
    SetTimer(CHECK_CALLBACK_TIMER, 10, nullptr);

    InitUI();

    return TRUE;
}

BEGIN_MESSAGE_MAP(CRtcDemoV2, CDialogEx)
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDCANCEL, &CRtcDemoV2::OnBnClickedCancel)
    ON_WM_CREATE()
    ON_WM_TIMER()
    ON_MESSAGE(MERGE_MESSAGE_ID, &CRtcDemoV2::OnHandleMessage)
    ON_MESSAGE(SEND_MESSAGE_ID, &CRtcDemoV2::OnSendMessage)
    ON_BN_CLICKED(IDC_BUTTON_LOGIN, &CRtcDemoV2::OnBnClickedButtonLogin)
    ON_BN_CLICKED(IDC_BUTTON_PREVIEW_VIDEO, &CRtcDemoV2::OnBnClickedButtonPreviewVideo)
    ON_BN_CLICKED(IDC_BTN_FLUSH, &CRtcDemoV2::OnBnClickedBtnFlush)
    ON_BN_CLICKED(IDC_BUTTON_PREVIEW_SCREEN, &CRtcDemoV2::OnBnClickedButtonPreviewScreen)
    ON_BN_CLICKED(IDC_CHECK_CAMERA, &CRtcDemoV2::OnBnClickedCheckCamera)
    ON_BN_CLICKED(IDC_CHECK_SCREEN, &CRtcDemoV2::OnBnClickedCheckScreen)
    ON_BN_CLICKED(IDC_CHECK_AUDIO, &CRtcDemoV2::OnBnClickedCheckAudio)
    ON_BN_CLICKED(IDC_CHECK_IMPORT_RAW_DATA, &CRtcDemoV2::OnBnClickedCheckImportRawData)
    ON_BN_CLICKED(IDC_CHECK_DESKTOP_AUDIO, &CRtcDemoV2::OnBnClickedCheckDesktopAudio)
    ON_BN_CLICKED(IDC_BTN_KICKOUT, &CRtcDemoV2::OnBnClickedBtnKickout)
    ON_BN_CLICKED(IDC_CHECK_MUTE_AUDIO, &CRtcDemoV2::OnBnClickedCheckMuteAudio)
    ON_BN_CLICKED(IDC_CHECK_MUTE_VIDEO, &CRtcDemoV2::OnBnClickedCheckMuteVideo)
    ON_WM_HSCROLL()
    ON_CBN_SELCHANGE(IDC_COMBO_MICROPHONE, &CRtcDemoV2::OnCbnSelchangeComboMicrophone)
    ON_CBN_SELCHANGE(IDC_COMBO_PLAYOUT, &CRtcDemoV2::OnCbnSelchangeComboPlayout)
    ON_BN_CLICKED(IDC_BUTTON_MERGE, &CRtcDemoV2::OnBnClickedButtonMerge)
    ON_BN_CLICKED(IDC_BUTTON_SEND_MSG, &CRtcDemoV2::OnBnClickedButtonSendMsg)
    ON_CBN_SELCHANGE(IDC_COMBO_LOCAL_ROTATION, &CRtcDemoV2::OnCbnSelchangeComboLocalRotate)
    ON_CBN_SELCHANGE(IDC_COMBO_REMOTE_ROTATION, &CRtcDemoV2::OnCbnSelchangeComboRemoteRotate)
    ON_CBN_SELCHANGE(IDC_COMBO_SUBSCRIBE_PROFILE, &CRtcDemoV2::OnCbnSelchangeComboSubscribeProfile)
    ON_BN_CLICKED(IDC_BUTTON_SIMULCAST, &CRtcDemoV2::OnBnClickedButtonSimulcast)
    ON_BN_CLICKED(IDC_BUTTON_FORWARD, &CRtcDemoV2::OnBnClickedButtonForward)
    ON_BN_CLICKED(IDC_BUTTON_EXTRA_DATA, &CRtcDemoV2::OnBnClickedButtonExtraData)
    ON_BN_CLICKED(IDC_CHECK_CAMERA_IMAGE, &CRtcDemoV2::OnBnClickedCheckCameraImage)
    ON_BN_CLICKED(IDC_CHECK_CAMERA_MIRROR, &CRtcDemoV2::OnBnClickedCheckCameraMirror)
    ON_BN_CLICKED(IDC_BTN_SEI, &CRtcDemoV2::OnBnClickedBtnSei)
END_MESSAGE_MAP()


// CRtcDemoV2 message handlers

void CRtcDemoV2::OnDestroy()
{
    // 结束外部数据导入的线程
    _stop_external_flag = true;
    if (_fake_audio_thread.joinable()) {
        _fake_audio_thread.join();
    }
    if (_fake_video_thread.joinable()) {
        _fake_video_thread.join();
    }
    // 释放 RTC SDK 资源
    if (_rtc_room_interface) {
        StopPublish();
        // 注意在调用 LeaveRoom 之前，用户需确保sdk的回调接口已处理并返回,
        // 否则 LeaveRoom 容易阻塞。
        _rtc_room_interface->LeaveRoom();
        qiniu_v2::QNRoomInterface::DestroyRoomInterface(_rtc_room_interface);
        _rtc_room_interface = nullptr;
    }
    CDialogEx::OnDestroy();
}

void CRtcDemoV2::OnBnClickedCancel()
{
    // TODO: Add your control notification handler code here
    CDialogEx::OnCancel();
    
    // 通知 V1 主界面显示
    //::PostMessage(_main_dlg_ptr->m_hWnd, WM_SHOWWINDOW, 0, 0);
}

void CRtcDemoV2::OnTimer(UINT_PTR nIDEvent)
{
    if (CHECK_CALLBACK_TIMER == nIDEvent) {
        // SDK 事件驱动器
        if (_rtc_room_interface) {
            _rtc_room_interface->Loop();
        }
    } else if (UPDATE_TIME_DURATION_TIMER == nIDEvent) {
        // 更新连麦时间
        chrono::seconds df_time
            = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - _start_tp);
        int hour = df_time.count() / 3600;
        int minute = df_time.count() % 3600 / 60;
        int sec = df_time.count() % 3600 % 60;
        wchar_t time_buf[128] = { 0 };
        wsprintf(time_buf,
            _T("连麦时长：%02d:%02d:%02d"),
            hour,
            minute,
            sec
        );
        _wnd_status_bar.SetText(time_buf, 0, 0);
    }
}

void CRtcDemoV2::ReadConfigFile()
{
    ifstream is("config");
    if (is.bad()) {
        return;
    }
    char appId_buf[128] = { 0 };
    char room_buf[128] = { 0 };
    char user_buf[128] = { 0 };
    if (!is.getline(appId_buf, 128)) {
        // 默认值
        SetDlgItemText(IDC_EDIT_APPID, utf2unicode("d8lk7l4ed").c_str());
        return;
    }
    if (!is.getline(room_buf, 128)) {
        return;
    }
    if (!is.getline(user_buf, 128)) {
        return;
    }
    SetDlgItemText(IDC_EDIT_APPID, utf2unicode(appId_buf).c_str());
    SetDlgItemText(IDC_EDIT_ROOM_ID, utf2unicode(room_buf).c_str());
    SetDlgItemText(IDC_EDIT_PLAYER_ID, utf2unicode(user_buf).c_str());
    is.close();
}

void CRtcDemoV2::WriteConfigFile()
{
    ofstream os("config");
    if (os.bad()) {
        return;
    }
    os.clear();
    string app_id = unicode2utf(_app_id.GetBuffer());
    string room_name = unicode2utf(_room_name.GetBuffer());
    string user_id = unicode2utf(_user_id.GetBuffer());
    os.write(app_id.c_str(), app_id.size());
    os.write("\n", 1);
    os.write(room_name.c_str(), room_name.size());
    os.write("\n", 1);
    os.write(user_id.c_str(), user_id.size());
    os.close();
}

void CRtcDemoV2::InitUI()
{
    GetDlgItem(IDC_BUTTON_SEND_MSG)->EnableWindow(FALSE);
    GetDlgItem(IDC_COMBO_LOCAL_ROTATION)->EnableWindow(FALSE);
    GetDlgItem(IDC_COMBO_REMOTE_ROTATION)->EnableWindow(FALSE);
    GetDlgItem(IDC_BUTTON_FORWARD)->EnableWindow(FALSE);
    GetDlgItem(IDC_BUTTON_EXTRA_DATA)->EnableWindow(TRUE);
    ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_ROTATION))->InsertString(-1, utf2unicode("0").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_ROTATION))->InsertString(-1, utf2unicode("90").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_ROTATION))->InsertString(-1, utf2unicode("180").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_ROTATION))->InsertString(-1, utf2unicode("270").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_ROTATION))->SetCurSel(0);

    ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_ROTATION))->InsertString(-1, utf2unicode("0").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_ROTATION))->InsertString(-1, utf2unicode("90").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_ROTATION))->InsertString(-1, utf2unicode("180").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_ROTATION))->InsertString(-1, utf2unicode("270").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_ROTATION))->SetCurSel(0);

    ((CComboBox *)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))->InsertString(-1, utf2unicode("HIGH").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))->InsertString(-1, utf2unicode("MEDIUM").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))->InsertString(-1, utf2unicode("LOW").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))->SetCurSel(0);

    ((CComboBox *)GetDlgItem(IDC_COMBO_CLIP_CROP))->InsertString(-1, utf2unicode("camera").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_CLIP_CROP))->InsertString(-1, utf2unicode("video external").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_CLIP_CROP))->SetCurSel(0);

    if (_enable_simulcast) {
        SetDlgItemText(IDC_BUTTON_SIMULCAST, _T("关闭多流"));
    } else {
        SetDlgItemText(IDC_BUTTON_SIMULCAST, _T("开启多流"));
    }
    SetDlgItemText(IDC_IPADDRESS1, _T("223.5.5.5"));
    _wnd_status_bar.Create(WS_CHILD | WS_VISIBLE | SBT_OWNERDRAW, CRect(0, 0, 0, 0), this, 0);
    RECT rc;
    GetWindowRect(&rc);
    int strPartDim[3] = { rc.right / 5, rc.right / 5 * 3, -1 };
    _wnd_status_bar.SetParts(3, strPartDim);
    // 设置状态栏文本 
    _wnd_status_bar.SetText(_T("通话时长：00:00::00"), 0, 0);
    _wnd_status_bar.SetText(_T("连麦状态"), 1, 0);
    _wnd_status_bar.SetText(utf2unicode(GetAppVersion(__DATE__, __TIME__)).c_str(), 2, 0);

    // 初始化音量控制条配置
    ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_RECORD))->SetRange(0, 100);
    ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_RECORD))->SetPos(100);
    ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_PLAYOUT))->SetRange(0, 100);
    ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_PLAYOUT))->SetPos(100);

    // 初始化用户列表控件
    _user_list_ctrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    _user_list_ctrl.InsertColumn(0, _T("用户 ID"), LVCFMT_LEFT, 100, 0);    // 设置列
    _user_list_ctrl.InsertColumn(1, _T("用户发布流状态"), LVCFMT_LEFT, 350, 1);

    // 初始化视频采集设备 combobox
    int camera_count = _rtc_video_interface->GetCameraCount();
    for (int i(0); i < camera_count; ++i) {
        qiniu_v2::CameraDeviceInfo ci = _rtc_video_interface->GetCameraInfo(i);
        _camera_dev_map[ci.device_id] = ci;
        ((CComboBox *)GetDlgItem(IDC_COMBO_CAMERA))->InsertString(-1, utf2unicode(ci.device_name).c_str());
    }
    ((CComboBox *)GetDlgItem(IDC_COMBO_CAMERA))->SetCurSel(0);

    // 初始化屏幕窗口列表
    int screen_count = _rtc_video_interface->GetScreenWindowCount();
    for (int i(0); i < screen_count; ++i) {
        qiniu_v2::ScreenWindowInfo sw = _rtc_video_interface->GetScreenWindowInfo(i);
        _screen_info_map[sw.id] = sw;
        ((CComboBox *)GetDlgItem(IDC_COMBO_SCREEN))->InsertString(-1, utf2unicode(sw.title).c_str());
    }
    ((CComboBox *)GetDlgItem(IDC_COMBO_SCREEN))->SetCurSel(0);

    // 初始化音频采集设备列表
    int audio_rec_count = _rtc_audio_interface->GetAudioDeviceCount(qiniu_v2::AudioDeviceInfo::adt_record);
    for (int i(0); i < audio_rec_count; ++i) {
        qiniu_v2::AudioDeviceInfo audio_info;
        if (_rtc_audio_interface->GetAudioDeviceInfo(qiniu_v2::AudioDeviceInfo::adt_record, i, audio_info) == 0) {
            ((CComboBox *)GetDlgItem(IDC_COMBO_MICROPHONE))->InsertString(
                -1,
                utf2unicode(audio_info.device_name).c_str()
            );
            if (audio_info.is_default) {
                qiniu_v2::AudioDeviceSetting ads;
                ads.device_index = audio_info.device_index;
                ads.device_type = qiniu_v2::AudioDeviceSetting::wdt_DefaultDevice;
                _rtc_audio_interface->SetRecordingDevice(ads);
            }
            _microphone_dev_map[i] = audio_info;
        }
    }
    ((CComboBox *)GetDlgItem(IDC_COMBO_MICROPHONE))->SetCurSel(0);

    // 初始化音频播放设备列表
    int audio_play_count = _rtc_audio_interface->GetAudioDeviceCount(qiniu_v2::AudioDeviceInfo::adt_playout);
    for (int i(0); i < audio_play_count; ++i) {
        qiniu_v2::AudioDeviceInfo audio_info;
        if (_rtc_audio_interface->GetAudioDeviceInfo(qiniu_v2::AudioDeviceInfo::adt_playout, i, audio_info) == 0) {
            ((CComboBox *)GetDlgItem(IDC_COMBO_PLAYOUT))->InsertString(
                -1,
                utf2unicode(audio_info.device_name).c_str()
            );
            if (audio_info.is_default) {
                qiniu_v2::AudioDeviceSetting ads;
                ads.device_index = audio_info.device_index;
                ads.device_type = qiniu_v2::AudioDeviceSetting::wdt_DefaultDevice;
                _rtc_audio_interface->SetPlayoutDevice(ads);
            }
            _playout_dev_map[i] = audio_info;
        }
    }
    ((CComboBox *)GetDlgItem(IDC_COMBO_PLAYOUT))->SetCurSel(0);

    ((CButton*)GetDlgItem(IDC_CHECK_CAMERA))->SetCheck(1);
    ((CButton*)GetDlgItem(IDC_CHECK_SCREEN))->SetCheck(0);
    ((CButton*)GetDlgItem(IDC_CHECK_AUDIO))->SetCheck(1);
    ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->SetCheck(0);
}

std::tuple<int, int> CRtcDemoV2::FindBestVideoSize(const qiniu_v2::CameraCapabilityVec& camera_cap_vec_)
{
    if (camera_cap_vec_.empty()) {
        return{ 0,0 };
    }
    // 高宽比例
    float wh_ratio = 1.0f * 9 / 16;
    int dest_width(0), dest_height(0);
    for (auto itor : camera_cap_vec_) {
        if ((1.0f * itor.height / itor.width) == wh_ratio) {
            if (itor.width >= 480) {
                dest_width = itor.width;
                dest_height = itor.height;
            }
        }
    }
    if (dest_width == 0 || dest_height == 0) {
        dest_width = camera_cap_vec_.back().width;
        dest_height = camera_cap_vec_.back().height;
    }
    return std::make_tuple(dest_width, dest_height);
}

/**
 * 关于错误异常的相关处理，都应在相应的回调中完成; 需要处理的错误码及建议处理逻辑如下:
 *
 *【TOKEN相关】
 * 1. Err_Token_Error 表示您提供的房间 token 不符合七牛 token 签算规则,
 *    详情请参考【服务端开发说明 RoomToken 签发服务】 https://doc.qnsdk.com/rtn/docs/server_overview#1，
 *    在 OnJoinResult 回调中暴露；
 * 2. Err_Token_Expired 表示您的房间 token 过期, 需要重新生成 token 再加入，在 OnJoinResult 回调中暴露；
 * 3. Err_ReconnToken_Error 表示你重新进入房间 token 错误，需要注意网络质量，请务必调用 LeaveRoom 后重新进入，
 *    在 OnJoinResult 回调中暴露；
 *
 *【房间设置相关】以下情况可以与您的业务服务开发确认具体设置
 * 1. Err_Room_Full 当房间已加入人数超过每个房间的人数限制触发，请确认后台服务的设置，在 OnJoinResult 回调中暴露；
 * 2. Err_User_Already_Exist 后台如果配置为开启【禁止自动踢人】,则同一用户重复加入/未
 *    正常退出再加入会触发此错误,您的业务可根据实际情况选择配置，在 OnJoinResult 回调中暴露；
 * 3. Err_No_Permission 用户对于特定操作，如合流需要配置权限，禁止出现未授权的用户操作，在 OnKickoutResult 回调中暴露；
 * 4. Err_Room_Closed 房间已被管理员关闭，在 OnJoinResult 回调中暴露；
 * 5. Err_User_Not_Exist 用户不存在，这里要注意被踢用户是否在房间内，在 OnKickoutResult 回调中暴露；
 *
 *【其他错误】
 * 1. Err_Invalid_Parameter 服务交互参数错误，请注意信令参数设置，在 OnJoinResult、OnPublishTracksResult、OnKickoutResult 回调中暴露；
 * 2. Err_Multi_Master_AV 请确定对于音频/视频 Track，分别最多只能有一路为 master，在 OnPublishTracksResult 回调中暴露；
 */

void CRtcDemoV2::OnJoinResult(
    int error_code_,
    const string& error_str_,
    const qiniu_v2::UserInfoList& user_vec_,
    const qiniu_v2::TrackInfoList& tracks_vec_,
    bool reconnect_flag_)
{
    wchar_t buff[1024] = { 0 };
    GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(TRUE);

    _start_tp = chrono::steady_clock::now();
    if (0 == error_code_) {
        _wnd_status_bar.SetText(_T("登录成功！"), 1, 0);
        SetDlgItemText(IDC_BUTTON_LOGIN, _T("离开"));
        GetDlgItem(IDC_BUTTON_SIMULCAST)->EnableWindow(FALSE);
        GetDlgItem(IDC_BUTTON_FORWARD)->EnableWindow(TRUE);
        GetDlgItem(IDC_BUTTON_EXTRA_DATA)->EnableWindow(FALSE);
        lock_guard<recursive_mutex> lck(_mutex);
        _user_list.clear();
        _user_list_ctrl.DeleteAllItems();
        for each (qiniu_v2::UserInfo itor in user_vec_)
        {
            _user_list_ctrl.InsertItem(0, utf2unicode(itor.user_id).c_str());
            _user_list_ctrl.SetItemText(0, 0, utf2unicode(itor.user_id).c_str());

            _user_list.push_back(itor.user_id);
        }

        _local_tracks_list.clear();
        _remote_tracks_map.clear();
        qiniu_v2::TrackInfoList sub_tracks_list;

        for (auto&& itor : tracks_vec_) {
            if (itor->GetUserId().compare(unicode2utf(_user_id.GetBuffer())) == 0) {
                continue;
            }
            TRACE("%s", itor->GetKind());
            auto tmp_track_ptr = qiniu_v2::QNTrackInfo::Copy(itor);
            // 自动订阅
            shared_ptr<TrackInfoUI> tiu(new TrackInfoUI(this, tmp_track_ptr));
            if (tiu->render_wnd_ptr) {
                tmp_track_ptr->SetRenderHwnd((void*)tiu->render_wnd_ptr->m_hWnd);
            }
            // 默认订阅的 profile 为 HIGH 
            if (tmp_track_ptr->GetKind().compare("video") == 0) {
                for (auto&& layeritor : tmp_track_ptr->GetLayerInfo())
                {
                    if (layeritor.mProfile == qiniu_v2::HIGH) {
                        layeritor.mChooseToSub = true;
                    }
                }
            }
            _remote_tracks_map.insert_or_assign(tmp_track_ptr->GetTrackId(), tiu);
            sub_tracks_list.emplace_back(tmp_track_ptr);
            itor->Release();
        }
        if (!sub_tracks_list.empty()) {
            _rtc_room_interface->SubscribeTracks(sub_tracks_list);
            // 调整订阅窗口的布局
            AdjustSubscribeLayouts();
        }

        SetTimer(UPDATE_TIME_DURATION_TIMER, 100, nullptr);

        // 主动配置当前音频设备，也可以不调用，则 SDK 使用系统默认设备
        OnCbnSelchangeComboMicrophone();
        OnCbnSelchangeComboPlayout();

        if (!reconnect_flag_) {
            // 自动开始发布
            StartPublish();
        }
        _msg_rich_edit_ctrl.SetWindowTextW(_T(""));
        _msg_rich_edit_ctrl.UpdateData();
        _msg_rich_edit_ctrl.Invalidate();

        if (1 == ((CButton*)GetDlgItem(IDC_CHECK_MERGE))->GetCheck()) {
            CreateCustomMergeJob();
        }
        GetDlgItem(IDC_BUTTON_SEND_MSG)->EnableWindow(TRUE);
        GetDlgItem(IDC_COMBO_LOCAL_ROTATION)->EnableWindow(TRUE);

    } else {
        switch (error_code_) {
        case Err_Token_Error:
            _snwprintf(buff, 1024, _T("登录失败，error code:%d, error msg:%s"),
                error_code_, utf2unicode("RoomToken 错误,请检查后从新生成再加入房间").c_str());
            break;
        case Err_Token_Expired:
            _snwprintf(buff, 1024, _T("登录失败，error code:%d, error msg:%s"),
                error_code_, utf2unicode("roomToken 过期").c_str());
            break;
        case Err_Room_Closed:
            _snwprintf(buff, 1024, _T("登录失败，error code:%d, error msg:%s"),
                error_code_, utf2unicode("房间已被管理员关闭").c_str());
            break;
        case Err_Room_Full:
            _snwprintf(buff, 1024, _T("登录失败，error code:%d, error msg:%s"),
                error_code_, utf2unicode("房间人数已满").c_str());
            break;
        case Err_User_Already_Exist:
            _snwprintf(buff, 1024, _T("登录失败，error code:%d, error msg:%s"),
                error_code_, utf2unicode("不允许同一用户重复进入").c_str());
            break;
        case Err_ReconnToken_Error:
            _snwprintf(buff, 1024, _T("登录失败，error code:%d, error msg:%s"),
                error_code_, utf2unicode("重新进入房间超时，请重新获取roomtoken后进入房间").c_str());
            break;
        case Err_Room_Not_Exist:
            _snwprintf(buff, 1024, _T("登录失败，error code:%d, error msg:%s"),
                error_code_, utf2unicode("房间不存在，请先确认房间是否已创建").c_str());
            break;
        case Err_Invalid_Parameter:
            _snwprintf(buff, 1024, _T("登录失败，error code:%d, error msg:%s"),
                error_code_, utf2unicode("服务交互参数错误").c_str());
            break;
        default:
            break;
        }
        GetDlgItem(IDC_BUTTON_FORWARD)->EnableWindow(FALSE);
        _wnd_status_bar.SetText(buff, 1, 0);
        SetDlgItemText(IDC_BUTTON_LOGIN, _T("登录"));
        // ReJoin，demo 这边针对异常做了重新连接功能，
        // 客户可以根据自己需要做相应的异常处理或者重新连接功能。
        PostMessage(WM_COMMAND, MAKEWPARAM(IDC_BUTTON_LOGIN, BN_CLICKED), NULL);
    }
}

void CRtcDemoV2::OnLeave(int error_code_, const string& error_str_, const string& user_id_)
{
    // 用户被踢、同一用户名登录同一房间或者 closeroom 会触发此消息，
    // client 收到这个消息需主动断开连接，且不要去重连
    wchar_t buff[1024] = { 0 };
    _snwprintf(
        buff, 
        1024, 
        _T("您以非正常方式离开了房间：error code:%d, error msg:%s, kickout user:%s"), 
        error_code_, 
        utf2unicode(error_str_).c_str(),
        utf2unicode(user_id_).c_str()
    );
    _wnd_status_bar.SetText(buff, 1, 0);
    CString msg_str(buff);

    thread([&, msg_str] {
        MessageBox(msg_str);
    }).detach();
    
    KillTimer(UPDATE_TIME_DURATION_TIMER);
    Invalidate();

    // 发送消息离开房间，并释放 SDK 资源
    PostMessage(WM_COMMAND, MAKEWPARAM(IDC_BUTTON_LOGIN, BN_CLICKED), NULL);
}

void CRtcDemoV2::OnRoomStateChange(qiniu_v2::RoomState state_)
{
    if (state_ == qiniu_v2::RoomState::rs_reconnecting) {
        _wnd_status_bar.SetText(_T("网络断开，自动重连中，请检查您的网络！"), 1, 0);
    }
}

void CRtcDemoV2::OnPublishTracksResult(int error_code_, 
    const string& error_str_, const qiniu_v2::TrackInfoList& track_info_list_)
{
    lock_guard<recursive_mutex> lck(_mutex);
    wchar_t buff[1024] = { 0 };
    for (auto&& itor : track_info_list_) {
        // 还原控件状态 
        if (itor->GetTag().compare(CAMERA_TAG) == 0) {
            GetDlgItem(IDC_CHECK_CAMERA)->EnableWindow(TRUE);
        } else if (itor->GetTag().compare(MICROPHONE_TAG) == 0) {
            GetDlgItem(IDC_CHECK_AUDIO)->EnableWindow(TRUE);
        } else if (itor->GetTag().compare(SCREENCASTS_TAG) == 0) {
            GetDlgItem(IDC_CHECK_SCREEN)->EnableWindow(TRUE);
        } else if (itor->GetTag().compare(EXTERNAL_TAG) == 0) {
            GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA)->EnableWindow(TRUE);
        }

        if (itor->GetTrackId().empty()) {
            // 此 Track 发布失败
            _snwprintf(buff, 1024, _T("本地流发布失败，Tag：%d"), utf2unicode(itor->GetTag()));
            _wnd_status_bar.SetText(buff, 1, 0);
            continue;
        }
        _local_tracks_list.emplace_back(qiniu_v2::QNTrackInfo::Copy(itor));
        if (itor->GetSourceType() == qiniu_v2::tst_ExternalYUV) {
            ImportExternalRawFrame(itor->GetTrackId());
        }
    }
    if (0 == error_code_) {
        _snwprintf(buff, 1024, _T("本地流发布成功，流数量：%d"), track_info_list_.size());
    } else {
        switch (error_code_) {
        case Err_Invalid_Parameter:
            _snwprintf(buff, 1024, _T("本地流发布失败，error code:%d, error msg:%s"),
                error_code_, utf2unicode("服务交互参数错误").c_str());
            break;
        case Err_Multi_Master_AV:
            _snwprintf(buff, 1024, _T("本地流发布失败，error code:%d, error msg:%s"),
                error_code_, utf2unicode("请查看是否加入房间，并确定对于音频/视频 Track，分别只能有一路为 master").c_str());
            break;
        default:
            break;
        }
    }
    // 释放 SDK 内部资源
    for (auto&& itor : track_info_list_)
    {
        itor->Release();
    }
    _wnd_status_bar.SetText(buff, 1, 0);
    AdjustMergeStreamLayouts();
}

void CRtcDemoV2::OnUnPublishTracksResult(const qiniu_v2::TrackInfoList& track_list_)
{
    // 释放 SDK 内部资源
    for (auto&& itor : track_list_){
        itor->Release();
    }
}

void CRtcDemoV2::OnRemoteStatisticsUpdated(const qiniu_v2::StatisticsReportList& statistics_list_)
{
    // 远端上行网络质量
    wchar_t dest_buf[1024] = { 0 };
    for (auto&& itor : statistics_list_){
        if (itor.is_video) {
            _snwprintf(dest_buf,
                sizeof(dest_buf),
                _T("用户:%s, Track Id:%s, 视频: rtt:%I64d, 丢包率:%0.3f"),
                utf2unicode(itor.user_id).c_str(),
                utf2unicode(itor.track_id).c_str(),
                itor.out_rtt,
                itor.video_packet_lost_rate
            );
        } else {
            _snwprintf(dest_buf,
                sizeof(dest_buf),
                _T("用户:%s, Track Id:%s, 音频: rtt:%I64d, 丢包率:%0.3f"),
                utf2unicode(itor.user_id).c_str(),
                utf2unicode(itor.track_id).c_str(),
                itor.out_rtt,
                itor.audio_packet_lost_rate
            );
        }

        TRACE(dest_buf);

        int line_count = _msg_rich_edit_ctrl.GetLineCount();
        if (line_count >= 1000) {
            // 此控件可存储数据量有限，为避免卡顿，及时清除
            _msg_rich_edit_ctrl.SetWindowTextW(_T(""));
            _msg_rich_edit_ctrl.UpdateData();
            _msg_rich_edit_ctrl.Invalidate();
        }
        _msg_rich_edit_ctrl.SetSel(-1, -1);
        _msg_rich_edit_ctrl.ReplaceSel(_T("\n"));
        _msg_rich_edit_ctrl.ReplaceSel(dest_buf);
        _msg_rich_edit_ctrl.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
    }
}


void CRtcDemoV2::OnCreateMergeResult(
    const std::string& job_id_,
    int error_code_,
    const std::string& error_str_)
{
    wchar_t buff[1024] = { 0 };
    if (error_code_ != 0) {
        switch (error_code_) {
        case Err_No_Permission:
            _snwprintf(buff, 1024, _T("合流失败，error code:%d, error msg:%s"),
                error_code_, utf2unicode("请检查用户是否有合流权限").c_str());
            break;
        case Err_Invalid_Parameter:
            _snwprintf(buff, 1024, _T("合流失败，error code:%d, error msg:%s"),
                error_code_, utf2unicode("服务交互参数错误").c_str());
            break;
        default:
            break;
        }
        _wnd_status_bar.SetText(buff, 1, 0);
    }
    else {
        _snwprintf(buff, 1024, _T("合流成功"));
        if (_contain_forward_flag) {
            // 如果是单路转推切合流任务，在合流成功之后停止单路转推 
            _rtc_room_interface->StopForwardJob(_custom_forward_id, 0);
        }
    }
}

void CRtcDemoV2::OnStopMergeResult(
    const std::string& job_id_,
    const std::string& job_iid_,
    int error_code_,
    const std::string& error_str_)
{
    wchar_t buff[1024] = { 0 };
    if (error_code_ != 0) {
        switch (error_code_) {
        case Err_No_Permission:
            _snwprintf(buff, 1024, _T("合流停止失败，error code:%d, error msg:%s"),
                error_code_, utf2unicode("请检查用户是否有合流权限").c_str());
            break;
        case Err_Invalid_Parameter:
            _snwprintf(buff, 1024, _T("合流停止失败，error code:%d, error msg:%s"),
                error_code_, utf2unicode("服务交互参数错误").c_str());
            break;
        default:
            break;
        }
        _wnd_status_bar.SetText(buff, 1, 0);
    }
    else {
        _snwprintf(buff, 1024, _T("合流停止成功"));
    }
}

void CRtcDemoV2::OnSetSubscribeTracksProfileResult(
    int error_code_,
    const string& error_str_,
    const qiniu_v2::TrackInfoList& track_list_
)
{
    // 切换 profile 后，将对应的生效状态 mActive 置为 true。
    for (auto&& itor : track_list_)
    {
        auto tmp_itor = _remote_tracks_map.find(itor->GetTrackId());
        if (tmp_itor == _remote_tracks_map.end()) {
            continue;
        }

        if (tmp_itor->second->track_info_ptr->GetKind().compare(VIDEO_KIND_TYPE) == 0) {
            for (auto&& layer_itor : itor->GetLayerInfo())
            {
                if (layer_itor.mActive) {
                    for (auto&& cur_itor : tmp_itor->second->track_info_ptr->GetLayerInfo()) {
                        if (layer_itor.mProfile == cur_itor.mProfile) {
                            cur_itor.mActive = true;
                            if (cur_itor.mProfile == qiniu_v2::HIGH) {
                                ((CComboBox*)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))->SetCurSel(0);
                            } else if (cur_itor.mProfile == qiniu_v2::MEDIUM) {
                                ((CComboBox*)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))->SetCurSel(1);
                            } else if (cur_itor.mProfile == qiniu_v2::LOW) {
                                ((CComboBox*)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))->SetCurSel(2);
                            }
                        } else {
                            cur_itor.mActive = false;
                        }
                        cur_itor.mChooseToSub = false;
                    }
                }

            }
        }
    }

    for (auto&& itor : track_list_) {
        itor->Release();
    }
}

void CRtcDemoV2::OnCreateForwardResult(
    const std::string& job_id_,
    int error_code_,
    const std::string& error_str_)
{
    wchar_t buff[1024] = { 0 };
    if (error_code_ != 0) {
        switch (error_code_) {
        case Err_No_Permission:
            _snwprintf(buff, 1024, _T("单路转推失败，error code:%d, error msg:%s"),
                error_code_, utf2unicode("请检查用户是否有转推权限").c_str());
            break;
        case Err_Invalid_Parameter:
            _snwprintf(buff, 1024, _T("单路转推失败，error code:%d, error msg:%s"),
                error_code_, utf2unicode("服务交互参数错误").c_str());
            break;
        default:
            break;
        }
        _wnd_status_bar.SetText(buff, 1, 0);
    } else {
        _snwprintf(buff, 1024, _T("单路转推成功"));
        // 如果是合流切单路转推任务，在单路转推成功之后停止单路转推 
        if (_contain_admin_flag) {
            _rtc_room_interface->StopMergeStream(string(), 0);
        }
        if (1 == ((CButton*)GetDlgItem(IDC_CHECK_MERGE))->GetCheck()) {
            _rtc_room_interface->StopMergeStream(_custom_merge_id, 0);
        }
    }
}

void CRtcDemoV2::OnStopForwardResult(
    const std::string& job_id_,
    const std::string& job_iid_,
    int error_code_,
    const std::string& error_str_)
{
    wchar_t buff[1024] = { 0 };
    if (error_code_ != 0) {
        switch (error_code_) {
        case Err_No_Permission:
            _snwprintf(buff, 1024, _T("单路转推停止失败，error code:%d, error msg:%s"),
                error_code_, utf2unicode("请检查用户是否有转推权限").c_str());
            break;
        case Err_Invalid_Parameter:
            _snwprintf(buff, 1024, _T("单路转推停止失败，error code:%d, error msg:%s"),
                error_code_, utf2unicode("服务交互参数错误").c_str());
            break;
        default:
            break;
        }
        _wnd_status_bar.SetText(buff, 1, 0);
    } else {
        _snwprintf(buff, 1024, _T("单路转推停止成功"));
    }
}

void CRtcDemoV2::OnRemoteUserReconnecting(const std::string& remote_user_id_)
{
    thread(
        [&, remote_user_id_] {
        wchar_t buf[512] = { 0 };

        _snwprintf(buf,
            sizeof(buf),
            _T("%s 用户正在重连"),
            utf2unicode(remote_user_id_).c_str()
        );
        AfxMessageBox(buf, MB_OK);
    }
    ).detach();
}

void CRtcDemoV2::OnRemoteUserReconnected(const std::string& remote_user_id_)
{
    thread(
        [&, remote_user_id_] {
        wchar_t buf[512] = { 0 };

        _snwprintf(buf,
            sizeof(buf),
            _T("%s 用户重连成功"),
            utf2unicode(remote_user_id_).c_str()
        );
        AfxMessageBox(buf, MB_OK);
    }
    ).detach();
}

void CRtcDemoV2::OnSubscribeTracksResult(int error_code_, 
    const std::string &error_str_, const qiniu_v2::TrackInfoList &track_info_list_)
{
    wchar_t buff[1024] = { 0 };
    int succ_num(0), failed_num(0);
    // 依次判断订阅结果
    // 将订阅的 profile 所对应的生效状态 mActive 置为 true。
    for (auto&& itor : track_info_list_)
    {
        if (itor->IsConnected()) {
            _rtc_video_interface->SetStretchMode(itor->GetTrackId(), qiniu_v2::ASPECT_FILL);
            auto tmp_itor = _remote_tracks_map.find(itor->GetTrackId());
            if (tmp_itor == _remote_tracks_map.end()) {
                continue;
            }

            if (tmp_itor->second->track_info_ptr->GetKind().compare(VIDEO_KIND_TYPE) == 0) {
                for (auto&& layer_itor : itor->GetLayerInfo())
                {
                    if (layer_itor.mActive) {
                        for (auto&& cur_itor : tmp_itor->second->track_info_ptr->GetLayerInfo()) {
                            if (layer_itor.mProfile == cur_itor.mProfile) {
                                cur_itor.mActive = true;  //当前订阅的远端 track 的 profile 对应生效状态置为 true 
                            } else {
                                cur_itor.mActive = false;
                            }
                            cur_itor.mChooseToSub = false;
                        }
                    }

                }
            }

            ++succ_num;
            TRACE(
                _T("订阅成功， User Id:%s, track Id：%s, Kind:%s, tag:%s\n"),
                utf2unicode(itor->GetUserId()).c_str(),
                utf2unicode(itor->GetTrackId()).c_str(),
                utf2unicode(itor->GetKind()).c_str(),
                utf2unicode(itor->GetTag()).c_str()
            );
        } else {
            ++failed_num;
            // 释放已分配的资源（渲染窗口等）
            auto itor_ui = _remote_tracks_map.find(itor->GetTrackId());
            if (itor_ui != _remote_tracks_map.end()) {
                _remote_tracks_map.erase(itor_ui);
            }
            TRACE(
                _T("订阅失败， User Id:%s, track Id：%s, Kind:%s, tag:%s\n"), 
                utf2unicode(itor->GetUserId()).c_str(),
                utf2unicode(itor->GetTrackId()).c_str(),
                utf2unicode(itor->GetKind()).c_str(),
                utf2unicode(itor->GetTag()).c_str()
                );
        }
    }
    if (0 == error_code_) {
        GetDlgItem(IDC_COMBO_REMOTE_ROTATION)->EnableWindow(TRUE);
        _snwprintf(
            buff, 
            1024, 
            _T("数据流订阅成功数量：%d，失败数量：%d"), 
            succ_num,
            failed_num
        );
    } else {
        _snwprintf(
            buff,
            1024,
            _T("数据流订阅失败, error code：%d，error msg:%s"),
            error_code_,
            utf2unicode(error_str_).c_str()
        );
    }
    _wnd_status_bar.SetText(buff, 1, 0);
    
    AdjustMergeStreamLayouts();

    for (auto&& itor : track_info_list_) {
        itor->Release();
    }
}

void CRtcDemoV2::OnRemoteAddTracks(const qiniu_v2::TrackInfoList& track_list_)
{
    ASSERT(!track_list_.empty());
    wchar_t buff[1024] = { 0 };
    _snwprintf(buff, 1024, _T("%s 发布了 %d 路媒体流。"), 
        utf2unicode(track_list_.front()->GetUserId()).c_str(), track_list_.size());
    _wnd_status_bar.SetText(buff, 1, 0);

    lock_guard<recursive_mutex> lck(_mutex);
    qiniu_v2::TrackInfoList sub_tracks_list;
    for (auto&& itor : track_list_) {
        TRACE("%s", itor->GetKind());
        auto tmp_track_ptr = qiniu_v2::QNTrackInfo::Copy(itor);
        shared_ptr<TrackInfoUI> tiu(new TrackInfoUI(this, tmp_track_ptr));
        if (tiu->render_wnd_ptr) {
            tmp_track_ptr->SetRenderHwnd((void*)tiu->render_wnd_ptr->m_hWnd);
        }
        // 默认订阅的 profile 为 HIGH 
        if (tmp_track_ptr->GetKind().compare("video") == 0) {
            for (auto&& layeritor : tmp_track_ptr->GetLayerInfo())
            {
                if (layeritor.mProfile == qiniu_v2::HIGH) {
                    layeritor.mChooseToSub = true;
                }
            }
        }
        
        if (tmp_track_ptr->GetKind().compare("audio") == 0) {
            _show_extra = true;
        }
        _remote_tracks_map.insert_or_assign(tmp_track_ptr->GetTrackId(), tiu);

        sub_tracks_list.emplace_back(tmp_track_ptr);
    }
    _rtc_room_interface->SubscribeTracks(sub_tracks_list);
    // 调整订阅窗口的布局
    AdjustSubscribeLayouts();

    for (auto&& itor : track_list_) {
        itor->Release();
    }
}

void CRtcDemoV2::OnRemoteDeleteTracks(const list<string>& track_list_)
{
    wchar_t buff[1024] = { 0 };
    _snwprintf(buff, 1024, _T("远端用户取消发布了 %d 路媒体流。"), track_list_.size());
    _wnd_status_bar.SetText(buff, 1, 0);

    // 释放本地资源
    if (_remote_tracks_map.empty()) {
        return;
    }
    lock_guard<recursive_mutex> lck(_mutex);
    for (auto&& itor : track_list_) {
        if (_remote_tracks_map.empty()) {
            break;
        }
        auto tmp_ptr = _remote_tracks_map.begin();
        while (tmp_ptr != _remote_tracks_map.end()) {
            if (tmp_ptr->second->track_info_ptr->GetTrackId().compare(itor) == 0) {
                _remote_tracks_map.erase(tmp_ptr);
                break;
            }
            ++tmp_ptr;
        }
    }
    AdjustSubscribeLayouts();
}

void CRtcDemoV2::OnRemoteUserJoin(const string& user_id_, const string& user_data_)
{
    lock_guard<recursive_mutex> lck(_mutex);

    _user_list.push_back(user_id_);

    CString str;
    for (int i = 0; i < _user_list_ctrl.GetItemCount(); i++) {
        str = _user_list_ctrl.GetItemText(i, 0);
        if (str.CompareNoCase(utf2unicode(user_id_).c_str()) == 0) {
            _user_list_ctrl.DeleteItem(i);
            break;
        }
    }
    _user_list_ctrl.InsertItem(0, utf2unicode(user_id_).c_str());
    _user_list_ctrl.SetItemText(0, 1, _T(""));

    wchar_t buff[1024] = { 0 };
    _snwprintf(
        buff,
        1024,
        _T("%s 加入了房间！"),
        utf2unicode(user_id_).c_str()
    );
    _wnd_status_bar.SetText(buff, 1, 0);
}

void CRtcDemoV2::OnRemoteUserLeave(const string& user_id_, int error_code_)
{
    lock_guard<recursive_mutex> lck(_mutex);
    CString str;
    for (int i = 0; i < _user_list_ctrl.GetItemCount(); i++) {
        str = _user_list_ctrl.GetItemText(i, 0);
        if (str.CompareNoCase(utf2unicode(user_id_).c_str()) == 0) {
            _user_list_ctrl.DeleteItem(i);
            break;
        }
    }

    auto itor = std::find(_user_list.begin(), _user_list.end(), user_id_);
    if (itor != _user_list.end()) {
        _user_list.erase(itor);
    }

    wchar_t buff[1024] = { 0 };
    _snwprintf(
        buff,
        1024,
        _T("%s 离开了房间！"),
        utf2unicode(user_id_).c_str()
    );
    _wnd_status_bar.SetText(buff, 1, 0);
    ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_ROTATION))->SetCurSel(0);
    GetDlgItem(IDC_COMBO_REMOTE_ROTATION)->EnableWindow(FALSE);
}

// 踢人结果通知
void CRtcDemoV2::OnKickoutResult(const std::string& kicked_out_user_id_, 
    int error_code_, const std::string& error_str_)
{
    lock_guard<recursive_mutex> lck(_mutex);
    
    auto itor = std::find(_user_list.begin(), _user_list.end(), kicked_out_user_id_);
    if (itor != _user_list.end()) {
        _user_list.erase(itor);
    }

    wchar_t buff[1024] = { 0 };
    if (0 == error_code_) {
        _snwprintf(
            buff,
            1024,
            _T("踢出用户：%s 成功！"),
            utf2unicode(kicked_out_user_id_).c_str()
        );
    } else {
        switch (error_code_) {
        case Err_User_Not_Exist:
            _snwprintf(
                buff,
                1024,
                _T("踢出用户：%s 失败！error code:%d, error msg:%s"),
                utf2unicode(kicked_out_user_id_).c_str(),
                error_code_,
                utf2unicode("被踢用户不存在").c_str()
            );
            break;
        case Err_No_Permission:
            _snwprintf(
                buff,
                1024,
                _T("踢出用户：%s 失败！error code:%d, error msg:%s"),
                utf2unicode(kicked_out_user_id_).c_str(),
                error_code_,
                utf2unicode("请检查用户是否有踢人权限").c_str()
            );
            break;
        case Err_Invalid_Parameter:
            _snwprintf(
                buff,
                1024,
                _T("踢出用户：%s 失败！error code:%d, error msg:%s"),
                utf2unicode(kicked_out_user_id_).c_str(),
                error_code_,
                utf2unicode("服务交互参数错误").c_str()
            );
            break;
        default:
            break;
        }
    }
    _wnd_status_bar.SetText(buff, 1, 0);
}

void CRtcDemoV2::OnRemoteTrackMuted(const string& track_id_, bool mute_flag_)
{
    wchar_t buff[1024] = { 0 };
    _snwprintf(
        buff,
        1024,
        mute_flag_ ? _T("远端用户静默了 Track Id : %s") : _T("远端用户取消了静默 Track Id : %s"),
        utf2unicode(track_id_).c_str()
    );
    _wnd_status_bar.SetText(buff, 1, 0);
}

void CRtcDemoV2::OnStatisticsUpdated(const qiniu_v2::StatisticsReport& statistics_)
{
    wchar_t dest_buf[1024] = { 0 };
    if (statistics_.is_video) {
        _snwprintf(dest_buf,
            sizeof(dest_buf),
            _T("用户:%s, Track Id:%s 视频: 分辨率:%d*%d, 帧率:%d, 码率:%d kbps, 丢包率:%0.3f"),
            utf2unicode(statistics_.user_id).c_str(),
            utf2unicode(statistics_.track_id).c_str(),
            statistics_.video_width,
            statistics_.video_height,
            statistics_.video_frame_rate,
            statistics_.video_bitrate / 1024,
            statistics_.video_packet_lost_rate
        );
    } else {
        _snwprintf(dest_buf,
            sizeof(dest_buf),
            _T("用户:%s, Track Id:%s, 音频：码率:%d kbps, 丢包率:%0.3f"),
            utf2unicode(statistics_.user_id).c_str(),
            utf2unicode(statistics_.track_id).c_str(),
            statistics_.audio_bitrate / 1024,
            statistics_.audio_packet_lost_rate
        );
    }

    TRACE(dest_buf);

    int line_count = _msg_rich_edit_ctrl.GetLineCount();
    if (line_count >= 1000) {
        // 此控件可存储数据量有限，为避免卡顿，及时清除
        _msg_rich_edit_ctrl.SetWindowTextW(_T(""));
        _msg_rich_edit_ctrl.UpdateData();
        _msg_rich_edit_ctrl.Invalidate();
    }
    _msg_rich_edit_ctrl.SetSel(-1, -1);
    _msg_rich_edit_ctrl.ReplaceSel(_T("\n"));
    _msg_rich_edit_ctrl.ReplaceSel(dest_buf);
    _msg_rich_edit_ctrl.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
}

void CRtcDemoV2::OnReceiveMessage(const qiniu_v2::CustomMessageList& custom_message_)
{
    // 接收远端自定义消息
    for (auto&& itor : custom_message_){
        _dlg_msg.OnReceiveMessage(itor.msg_sendid, utf8_to_string(itor.msg_text));
    }
}

void CRtcDemoV2::OnAudioPCMFrame(const unsigned char* audio_data_, 
    int bits_per_sample_, int sample_rate_, size_t number_of_channels_, 
    size_t number_of_frames_, const std::string& user_id_)
{
    if (bits_per_sample_ / 8 == sizeof(int16_t)) {
        // ASSERT(bits_per_sample_ / 8 == sizeof(int16_t));
        // 可以借助以下代码计算音量的实时分贝值
        auto level = ProcessAudioLevel(
            (int16_t*)audio_data_,
            bits_per_sample_ / 8 * number_of_channels_ * number_of_frames_ / sizeof(int16_t)
        );
    }
}

void CRtcDemoV2::OnAudioDeviceStateChanged(
    qiniu_v2::AudioDeviceState device_state_, const std::string& device_guid_)
{
    thread(
        [&, device_state_, device_guid_] {
        wchar_t buf[512] = { 0 };
        if (qiniu_v2::ads_active != device_state_) {
            _snwprintf(
                buf,
                512,
                _T("音频设备：%s 被拔出！"),
                utf2unicode(device_guid_).c_str()
            );
            AfxMessageBox(buf, MB_OK);
        }
    }
    ).detach();
}

int CRtcDemoV2::OnPutExtraData(
    unsigned char* extra_data_,
    int extra_data_max_size_,
    const std::string& track_id_)
{
    const unsigned char tmp[32] = "QNTEST";
    memcpy(extra_data_, tmp, strlen((char*)tmp));
    return strlen((char*)tmp);
}

int CRtcDemoV2::OnSetMaxEncryptSize(
    int frame_size_,
    const std::string& track_id_)
{
    return frame_size_ + 32;
}

int CRtcDemoV2::OnEncrypt(
    const unsigned char* frame_,
    int frame_size_,
    unsigned char* encrypted_frame_,
    const std::string& track_id_)
{
    // 这里加密演示是每个字节异或 0xAA，解密时也相应异或 0xAA。
    for (size_t i = 0; i < frame_size_; i++) {
        encrypted_frame_[i] = frame_[i] ^ 0xAA;
    }
    return frame_size_;
}

void CRtcDemoV2::OnGetExtraData(
    const unsigned char* extra_data_,
    int extra_data_size_,
    const std::string& track_id_)
{
    if (_show_extra) {
        thread(
            [&, extra_data_] {
            wchar_t buf[512] = { 0 };

            _snwprintf(buf,
                sizeof(buf),
                _T("extra data:%s"),
                utf2unicode((LPCSTR)extra_data_).c_str()
            );
            AfxMessageBox(buf, MB_OK);
        }
        ).detach();
        _show_extra = false;
    }
}

int CRtcDemoV2::OnSetMaxDecryptSize(
    int encrypted_frame_size_,
    const std::string& track_id_)
{
    return encrypted_frame_size_ + 32;
}

int CRtcDemoV2::OnDecrypt(
    const unsigned char* encrypted_frame_,
    int encrypted_size_,
    unsigned char* frame_,
    const std::string& track_id_)
{
    for (size_t i = 0; i < encrypted_size_; i++) {
        frame_[i] = encrypted_frame_[i] ^ 0xAA;
    }
    return encrypted_size_;
}

void CRtcDemoV2::OnVideoDeviceStateChanged(
    qiniu_v2::VideoDeviceState device_state_, const std::string& device_name_)
{
    thread(
        [&, device_state_, device_name_] {
        wchar_t buf[512] = { 0 };
        if (qiniu_v2::vds_lost == device_state_) {
            _snwprintf(
                buf,
                512,
                _T("视频设备：%s 被拔出！"),
                utf2unicode(device_name_).c_str()
            );
            AfxMessageBox(buf, MB_OK);
        }
    }
    ).detach();
}

void CRtcDemoV2::OnVideoFrame(const unsigned char* raw_data_, int data_len_, 
    int width_, int height_, qiniu_v2::VideoCaptureType video_type_, 
    const std::string& track_id_, const std::string& user_id_)
{

}

void CRtcDemoV2::OnVideoFramePreview(const unsigned char* raw_data_, int data_len_,
    int width_, int height_, qiniu_v2::VideoCaptureType video_type_)
{

}

void CRtcDemoV2::OnBnClickedButtonLogin()
{
    // TODO: Add your control notification handler code here
    // 设置画面裁剪和缩放参数，在 JoinRoom 之前设置。
    int clip_crop_source = ((CComboBox*)GetDlgItem(IDC_COMBO_CLIP_CROP))->GetCurSel();
    if (clip_crop_source == 0) {
        _src_capturer_source = qiniu_v2::tst_Camera;
    } else {
        _src_capturer_source = qiniu_v2::tst_ExternalYUV;
    }

    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_CLIP))->GetCheck()) {
        int cropX = GetDlgItemInt(IDC_EDIT_CLIP_X, NULL, 0);
        int cropY = GetDlgItemInt(IDC_EDIT_CLIP_Y, NULL, 0);
        int width = GetDlgItemInt(IDC_EDIT_CLIP_WIDTH, NULL, 0);
        int height = GetDlgItemInt(IDC_EDIT_CLIP_HEIGHT, NULL, 0);
        _rtc_video_interface->SetCropAndScale(_src_capturer_source, qiniu_v2::p_Crop, true, cropX, cropY, width, height);
    } else {
        _rtc_video_interface->SetCropAndScale(_src_capturer_source, qiniu_v2::p_Crop, false, 0, 0, 0, 0);
    }

    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_SCALE))->GetCheck()) {
        int width = GetDlgItemInt(IDC_EDIT_SCALE_WIDTH, NULL, 0);
        int height = GetDlgItemInt(IDC_EDIT_SCALE_HEIGHT, NULL, 0);
        _rtc_video_interface->SetCropAndScale(_src_capturer_source, qiniu_v2::p_Scale, true, 0, 0, width, height);
    } else {
        _rtc_video_interface->SetCropAndScale(_src_capturer_source, qiniu_v2::p_Scale, false, 0, 0, 0, 0);
    }

    CString btn_str;
    GetDlgItemText(IDC_BUTTON_LOGIN, btn_str);
    // 登录房间
    if (btn_str.CompareNoCase(_T("登录")) == 0) {
        _show_extra = true;
        if (_enable_encryptor_decryptor) {
            _rtc_audio_interface->EnableLocalAudioPacketCallBack(true);
            _rtc_audio_interface->EnableRemoteAudioPacketCallBack(true);
            SetDlgItemText(IDC_BUTTON_EXTRA_DATA, _T("关闭数据加解密"));
        }
        else {
            SetDlgItemText(IDC_BUTTON_EXTRA_DATA, _T("开启数据加解密"));
        }
        GetDlgItemText(IDC_EDIT_APPID, _app_id);
        GetDlgItemText(IDC_EDIT_ROOM_ID, _room_name);
        GetDlgItemText(IDC_EDIT_PLAYER_ID, _user_id);
        if (_room_name.IsEmpty() || _user_id.IsEmpty()) {
            MessageBox(_T("Room ID and Player ID can't be NULL!"));
            return;
        }
        WriteConfigFile();

        GetDlgItem(IDC_BUTTON_LOGIN)->SetWindowText(_T("登录中"));
        GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(FALSE);

        // 向 AppServer 获取 token 
        _room_token.clear();
        int ret = GetRoomToken_s(
            unicode2utf(_app_id.GetBuffer()),
            unicode2utf(_room_name.GetBuffer()),
            unicode2utf(_user_id.GetBuffer()),
            "api-demo.qnsdk.com", 5, _room_token);
        if (ret != 0) {
            CString msg_str;
            msg_str.Format(_T("获取房间 token 失败，请检查您的网络是否正常！Err:%d"), ret);
            _wnd_status_bar.SetText(msg_str.GetBuffer(), 1, 0);
            // MessageBox(_T("获取房间 token 失败，请检查您的网络是否正常！"));
            GetDlgItem(IDC_BUTTON_LOGIN)->SetWindowText(_T("登录"));
            GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(TRUE);

            thread([=] {
                  Sleep(2000);
                  PostMessage(WM_COMMAND, MAKEWPARAM(IDC_BUTTON_LOGIN, BN_CLICKED), NULL);
              }).detach();
            return;
        }
        if (_strnicmp(
            const_cast<char*>(unicode2utf(_user_id.GetBuffer()).c_str()),
            "admin",
            unicode2utf(_user_id.GetBuffer()).length()) == 0) {
            _contain_admin_flag = true;
        } else {
            _contain_admin_flag = false;
        }

        _wnd_status_bar.SetText(_T("获取房间 token 成功！"), 1, 0);
        CString dns_ip;
        GetDlgItemText(IDC_IPADDRESS1, dns_ip);
        _rtc_room_interface->SetDnsServerUrl(unicode2utf(dns_ip.GetBuffer()));
        _rtc_room_interface->JoinRoom(_room_token);
    } else {
        _rtc_audio_interface->EnableLocalAudioPacketCallBack(false);
        _rtc_audio_interface->EnableRemoteAudioPacketCallBack(false);
        // 退出房间前，发布停止合流的命令
        if (_contain_admin_flag) {
            _rtc_room_interface->StopMergeStream(string(), 0);
        }
        if (1 == ((CButton*)GetDlgItem(IDC_CHECK_MERGE))->GetCheck()) {
            _rtc_room_interface->StopMergeStream(_custom_merge_id, 0);
        }
        _rtc_room_interface->LeaveRoom();

        //退出时，远端画面旋转重置
        for (auto&& itor : _remote_tracks_map) {
            if (itor.second->track_info_ptr->GetKind().compare("video") == 0) {
                if (_rtc_video_interface) {
                    _rtc_video_interface->SetVideoRotation(itor.first, qiniu_v2::VideoRotation::kVideoRotation_0);
                }
            }
        }

        SetDlgItemText(IDC_BUTTON_LOGIN, _T("登录"));
        _wnd_status_bar.SetText(_T("当前未登录房间！"), 1, 0);
        _user_list_ctrl.DeleteAllItems();
        ((CButton*)GetDlgItem(IDC_CHECK_MUTE_AUDIO))->SetCheck(0);
        ((CButton*)GetDlgItem(IDC_CHECK_MUTE_VIDEO))->SetCheck(0);
        GetDlgItem(IDC_BUTTON_SEND_MSG)->EnableWindow(FALSE);
        GetDlgItem(IDC_COMBO_LOCAL_ROTATION)->EnableWindow(FALSE);
        GetDlgItem(IDC_COMBO_REMOTE_ROTATION)->EnableWindow(FALSE);
        ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_ROTATION))->SetCurSel(0);
        ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_ROTATION))->SetCurSel(0);
        GetDlgItem(IDC_BUTTON_SIMULCAST)->EnableWindow(TRUE);
        GetDlgItem(IDC_BUTTON_FORWARD)->EnableWindow(FALSE);
        GetDlgItem(IDC_BUTTON_EXTRA_DATA)->EnableWindow(TRUE);
        _remote_tracks_map.clear();

        KillTimer(UPDATE_TIME_DURATION_TIMER);
        Invalidate();
    }
}

void CRtcDemoV2::OnBnClickedButtonPreviewVideo()
{
    // TODO: Add your control notification handler code here
    if (!_rtc_video_interface) {
        return;
    }
    // 首先获取选中设备 name 和 id
    CString cur_dev_name;
    string cur_dev_id;
    GetDlgItem(IDC_COMBO_CAMERA)->GetWindowTextW(cur_dev_name);
    if (cur_dev_name.IsEmpty()) {
        MessageBox(_T("您当前没有任何视频设备！"));
        return;
    }
    // 获取 device id
    auto itor = _camera_dev_map.begin();
    while (itor != _camera_dev_map.end()) {
        if (itor->second.device_name.compare(unicode2utf(cur_dev_name.GetBuffer())) == 0) {
            cur_dev_id = itor->first;
            break;
        }
        ++itor;
    }

    CString btn_text;
    GetDlgItemText(IDC_BUTTON_PREVIEW_VIDEO, btn_text);
    if (btn_text.CompareNoCase(_T("取消预览")) == 0) {
        _rtc_video_interface->UnPreviewCamera(cur_dev_id);
        GetDlgItem(IDC_COMBO_CAMERA)->EnableWindow(TRUE);
        SetDlgItemText(IDC_BUTTON_PREVIEW_VIDEO, _T("预览"));
        GetDlgItem(IDC_STATIC_VIDEO_PREVIEW3)->Invalidate();
        return;
    }

    qiniu_v2::CameraSetting camera_setting;
    camera_setting.device_name = unicode2utf(cur_dev_name.GetBuffer());
    camera_setting.device_id   = cur_dev_id;
    camera_setting.width       = 640;
    camera_setting.height      = 480;
    camera_setting.max_fps     = 15;
    camera_setting.render_hwnd = GetDlgItem(IDC_STATIC_VIDEO_PREVIEW3)->m_hWnd;

    if (0 != _rtc_video_interface->PreviewCamera(camera_setting)) {
        MessageBox(_T("预览失败！"));
    } else {
        SetDlgItemText(IDC_BUTTON_PREVIEW_VIDEO, _T("取消预览"));
        GetDlgItem(IDC_COMBO_CAMERA)->EnableWindow(FALSE);
    }
}

void CRtcDemoV2::OnBnClickedBtnFlush()
{
    // TODO: Add your control notification handler code here
    if (!_rtc_video_interface) {
        return;
    }
    ((CComboBox *)GetDlgItem(IDC_COMBO_SCREEN))->ResetContent();
    int screen_count = _rtc_video_interface->GetScreenWindowCount();
    for (int i(0); i < screen_count; ++i) {
        qiniu_v2::ScreenWindowInfo sw = _rtc_video_interface->GetScreenWindowInfo(i);
        _screen_info_map[sw.id] = sw;
        ((CComboBox *)GetDlgItem(IDC_COMBO_SCREEN))->InsertString(-1, utf2unicode(sw.title).c_str());
    }
    ((CComboBox *)GetDlgItem(IDC_COMBO_SCREEN))->SetCurSel(0);
}

void CRtcDemoV2::OnBnClickedButtonPreviewScreen()
{
    // TODO: Add your control notification handler code here
    if (!_rtc_video_interface) {
        return;
    }
    // 首先获取选中设备 name 和 id
    CString cur_screen_title;
    GetDlgItem(IDC_COMBO_SCREEN)->GetWindowTextW(cur_screen_title);

    // 获取 source id
    int source_id(-1);
    auto itor = _screen_info_map.begin();
    while (itor != _screen_info_map.end()) {
        if (itor->second.title.compare(unicode2utf(cur_screen_title.GetBuffer())) == 0) {
            source_id = itor->first;
            break;
        }
        ++itor;
    }

    CString btn_text;
    GetDlgItemText(IDC_BUTTON_PREVIEW_SCREEN, btn_text);
    if (btn_text.CompareNoCase(_T("取消预览")) == 0) {
        _rtc_video_interface->UnPreviewScreenSource(source_id);
        GetDlgItem(IDC_COMBO_SCREEN)->EnableWindow(TRUE);
        GetDlgItem(IDC_BTN_FLUSH)->EnableWindow(TRUE);
        SetDlgItemText(IDC_BUTTON_PREVIEW_SCREEN, _T("预览屏幕"));
        GetDlgItem(IDC_STATIC_VIDEO_PREVIEW)->Invalidate();
        return;
    }

    if (0 != _rtc_video_interface->PreviewScreenSource(
        source_id, 
        GetDlgItem(IDC_STATIC_VIDEO_PREVIEW4)->m_hWnd, 
        true) ) {
        MessageBox(_T("预览失败！"));
    } else {
        SetDlgItemText(IDC_BUTTON_PREVIEW_SCREEN, _T("取消预览"));
        GetDlgItem(IDC_COMBO_SCREEN)->EnableWindow(FALSE);
        GetDlgItem(IDC_BTN_FLUSH)->EnableWindow(FALSE);
    }
}

void CRtcDemoV2::StartPublish()
{
    // TODO: Add your control notification handler code here
    if (!_rtc_room_interface) {
        return;
    }
    qiniu_v2::TrackInfoList track_list;
CHECK_CAMERA:
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_CAMERA))->GetCheck()) {
        CString video_dev_name;
        string video_dev_id;
        int audio_recorder_device_index(-1);

        GetDlgItem(IDC_COMBO_CAMERA)->GetWindowTextW(video_dev_name);
        if (video_dev_name.IsEmpty()) {
            thread([] {
                AfxMessageBox(_T("您当前没有任何视频设备！"));
            }).detach();
            goto CHECK_SCREEN;
        }
        auto itor = _camera_dev_map.begin();
        while (itor != _camera_dev_map.end()) {
            if (itor->second.device_name.compare(unicode2utf(video_dev_name.GetBuffer())) == 0) {
                video_dev_id = itor->first;
                break;
            }
            ++itor;
        }
        // 用户需根据摄像头采集能力集，设置适合自己的图像分辨率、帧率和码率。
        auto camera_size = FindBestVideoSize(_camera_dev_map[video_dev_id].capability_vec);
        int width = std::get<0>(camera_size);
        int height = std::get<1>(camera_size);
        auto video_track_ptr = qiniu_v2::QNTrackInfo::CreateVideoTrackInfo(
            video_dev_id,
            CAMERA_TAG,
            GetDlgItem(IDC_STATIC_VIDEO_PREVIEW)->m_hWnd,
            width,
            height,
            30,
            2000000,
            qiniu_v2::tst_Camera,
            false,
            _enable_simulcast     // 是否支持多流发送，只有分辨率大于等于 1280*720 的视频 track 支持多流发送,否则设置为false 
                                  // 请根据自己需要是否开启多流功能，开启此功能需注意发送端带宽是否支持 
        );
        track_list.emplace_back(video_track_ptr);
    }

CHECK_SCREEN:
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_SCREEN))->GetCheck()) {
        CString wnd_title;
        int source_id(-1);
        GetDlgItem(IDC_COMBO_SCREEN)->GetWindowTextW(wnd_title);
        for (auto&& itor : _screen_info_map) {
            if (itor.second.title.compare(unicode2utf(wnd_title.GetBuffer())) == 0) {
                source_id = itor.first;
                break;
            }
        }
        if (-1 == source_id) {
            goto CHECK_AUDIO;
        }
        auto video_track_ptr = qiniu_v2::QNTrackInfo::CreateVideoTrackInfo(
            std::to_string(source_id),
            SCREENCASTS_TAG,
            GetDlgItem(IDC_STATIC_VIDEO_PREVIEW2)->m_hWnd,
            640,
            480,
            30,
            500000,
            qiniu_v2::tst_ScreenCasts,
            false,
            false
        );
        track_list.emplace_back(video_track_ptr);
    }
CHECK_AUDIO:
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_AUDIO))->GetCheck()) {
        auto audio_track = qiniu_v2::QNTrackInfo::CreateAudioTrackInfo(
            MICROPHONE_TAG,
            32000,
            false
        );
        track_list.emplace_back(audio_track);
    }
CHECK_EXTERNAL:
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->GetCheck()) {
        // 这里需要设置的是外部视频源对应的分辨率和帧率
        auto video_track_ptr = qiniu_v2::QNTrackInfo::CreateVideoTrackInfo(
            "",
            EXTERNAL_TAG,
            GetDlgItem(IDC_STATIC_VIDEO_PREVIEW3)->m_hWnd,
            426,
            240,
            30,
            300000,
            qiniu_v2::tst_ExternalYUV,
            false,
            false
        );
        track_list.emplace_back(video_track_ptr);
    }

    if (!track_list.empty()) {
        auto ret = _rtc_room_interface->PublishTracks(track_list);
        if (ret == Err_Tracks_Publish_All_Failed) {
            thread([]() {
                AfxMessageBox(_T("全部发布失败，请检查您的网络状态！"));
            }).detach();
        } else if (ret == Err_Tracks_Publish_Partial_Failed) {
            thread([]() {
                AfxMessageBox(_T("部分发布失败，请检查您的设备状态是否可用？"));
            }).detach();
        }
        qiniu_v2::QNTrackInfo::ReleaseList(track_list);
    }
}

void CRtcDemoV2::StopPublish()
{
    if (!_rtc_room_interface) {
        return;
    }
    lock_guard<recursive_mutex> lck(_mutex);
    list<string> track_list;
    for (auto&& itor : _local_tracks_list) {
        track_list.emplace_back(itor->GetTrackId());
    }
    _rtc_room_interface->UnPublishTracks(track_list);

    qiniu_v2::QNTrackInfo::ReleaseList(_local_tracks_list);
}

void CRtcDemoV2::OnBnClickedCheckCamera()
{
    if (!_rtc_room_interface) {
        return;
    }
    lock_guard<recursive_mutex> lck(_mutex);
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_CAMERA))->GetCheck()) {
        if (!_rtc_room_interface->IsJoined()) {
            return;
        }
        CString video_dev_name;
        string video_dev_id;
        int audio_recorder_device_index(-1);

        GetDlgItem(IDC_COMBO_CAMERA)->GetWindowTextW(video_dev_name);
        if (video_dev_name.IsEmpty()) {
            thread([] {
                AfxMessageBox(_T("您当前没有任何视频设备！"));
            }).detach();
            return;
        }
        auto itor = _camera_dev_map.begin();
        while (itor != _camera_dev_map.end()) {
            if (itor->second.device_name.compare(unicode2utf(video_dev_name.GetBuffer())) == 0) {
                video_dev_id.assign(itor->first.c_str(), itor->first.length());
                TRACE("video dev id ptr:%x, ptr2:%x ;", itor->first.c_str(), video_dev_id.c_str());
                break;
            }
            ++itor;
        }

        auto camera_size = FindBestVideoSize(_camera_dev_map[video_dev_id].capability_vec);
        auto video_track_ptr = qiniu_v2::QNTrackInfo::CreateVideoTrackInfo(
            video_dev_id,
            CAMERA_TAG,
            GetDlgItem(IDC_STATIC_VIDEO_PREVIEW)->m_hWnd,
            std::get<0>(camera_size),
            std::get<1>(camera_size),
            15,
            500000,
            qiniu_v2::tst_Camera,
            false,
            false
        );
        qiniu_v2::TrackInfoList track_list;
        track_list.push_back(video_track_ptr);
        auto ret = _rtc_room_interface->PublishTracks(track_list);
        if (ret != 0) {
            ((CButton*)GetDlgItem(IDC_CHECK_CAMERA))->SetCheck(0);
            MessageBox(_T("发布失败，请检查设备状态是否可用？或者不要操作太快！"));
        } else {
            // 临时禁用，待发布结果通知后再激活控件，避免频繁操作
            ((CButton*)GetDlgItem(IDC_CHECK_CAMERA))->EnableWindow(FALSE);
        }
        video_track_ptr->Release();
    } else {
        list<string> track_list;
        if (_local_tracks_list.empty()) {
            if (_rtc_room_interface->IsJoined()) {
                ((CButton*)GetDlgItem(IDC_CHECK_CAMERA))->SetCheck(1);
            }
            return;
        }
        auto itor = _local_tracks_list.begin();
        while (itor != _local_tracks_list.end()) {
            if ((*itor)->GetTag().compare(CAMERA_TAG) == 0) {
                track_list.emplace_back((*itor)->GetTrackId());
                if (Err_Pre_Publish_Not_Complete == _rtc_room_interface->UnPublishTracks(track_list)) {
                    ((CButton*)GetDlgItem(IDC_CHECK_CAMERA))->SetCheck(1);
                }
                (*itor)->Release();
                _local_tracks_list.erase(itor);
                break;
            }
            ++itor;
        }
        GetDlgItem(IDC_STATIC_VIDEO_PREVIEW)->Invalidate();
        _wnd_status_bar.SetText(_T("取消发布摄像头"), 1, 0);
    }
}

void CRtcDemoV2::OnBnClickedCheckScreen()
{
    // TODO: Add your control notification handler code here
    if (!_rtc_room_interface) {
        return;
    }
    lock_guard<recursive_mutex> lck(_mutex);
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_SCREEN))->GetCheck()) {
        if (!_rtc_room_interface->IsJoined()) {
            return;
        }
        CString wnd_title;
        int source_id(-1);
        GetDlgItem(IDC_COMBO_SCREEN)->GetWindowTextW(wnd_title);
        for (auto&& itor : _screen_info_map) {
            if (itor.second.title.compare(unicode2utf(wnd_title.GetBuffer())) == 0) {
                source_id = itor.first;
                break;
            }
        }
        if (-1 == source_id) {
            ((CButton*)GetDlgItem(IDC_CHECK_SCREEN))->SetCheck(0);
            return;
        }

        auto video_track_ptr2 = qiniu_v2::QNTrackInfo::CreateVideoTrackInfo(
            std::to_string(source_id),
            SCREENCASTS_TAG,
            GetDlgItem(IDC_STATIC_VIDEO_PREVIEW2)->m_hWnd,
            640,
            480,
            30,
            300000,
            qiniu_v2::tst_ScreenCasts,
            false,
            false
        );
        qiniu_v2::TrackInfoList track_list;
        track_list.emplace_back(video_track_ptr2);
        auto ret = _rtc_room_interface->PublishTracks(track_list);
        if (ret != 0) {
            ((CButton*)GetDlgItem(IDC_CHECK_SCREEN))->SetCheck(0);
            MessageBox(_T("发布失败，请检查设备状态是否可用？或者不要操作太快！"));
        } else {
            // 临时禁用，待发布结果通知后再激活控件，避免频繁操作
            ((CButton*)GetDlgItem(IDC_CHECK_SCREEN))->EnableWindow(FALSE);
        }
        video_track_ptr2->Release();
    } else {
        list<string> track_list;
        if (_local_tracks_list.empty()) {
            if (_rtc_room_interface->IsJoined()) {
                ((CButton*)GetDlgItem(IDC_CHECK_SCREEN))->SetCheck(1);
            }
            return;
        }
        auto itor = _local_tracks_list.begin();
        while (itor != _local_tracks_list.end())
        {
            if ((*itor)->GetTag().compare(SCREENCASTS_TAG) == 0) {
                track_list.emplace_back((*itor)->GetTrackId());
                if (Err_Pre_Publish_Not_Complete == _rtc_room_interface->UnPublishTracks(track_list)) {
                    ((CButton*)GetDlgItem(IDC_CHECK_SCREEN))->SetCheck(1);
                }
                (*itor)->Release();
                _local_tracks_list.erase(itor);
                break;
            }
            ++itor;
        }
        GetDlgItem(IDC_STATIC_VIDEO_PREVIEW2)->Invalidate();
        _wnd_status_bar.SetText(_T("取消发布屏幕分享"), 1, 0);
    }
}


void CRtcDemoV2::OnBnClickedCheckAudio()
{
    // TODO: Add your control notification handler code here
    if (!_rtc_room_interface) {
        return;
    }
    ((CButton*)GetDlgItem(IDC_CHECK_MUTE_AUDIO))->SetCheck(0);

    lock_guard<recursive_mutex> lck(_mutex);
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_AUDIO))->GetCheck()) {
        if (!_rtc_room_interface->IsJoined()) {
            return;
        }

        OnCbnSelchangeComboMicrophone();

        auto audio_track = qiniu_v2::QNTrackInfo::CreateAudioTrackInfo(
            MICROPHONE_TAG,
            32000,
            false
        );
        qiniu_v2::TrackInfoList track_list;
        track_list.emplace_back(audio_track);
        auto ret = _rtc_room_interface->PublishTracks(track_list);
        if (ret != 0) {
            ((CButton*)GetDlgItem(IDC_CHECK_AUDIO))->SetCheck(0);
            MessageBox(_T("发布失败，请检查设备状态是否可用？或者不要操作太快！"));
        } else {
            // 临时禁用，待发布结果通知后再激活控件，避免频繁操作
            ((CButton*)GetDlgItem(IDC_CHECK_AUDIO))->EnableWindow(FALSE);
        }
        audio_track->Release();
    } else {
        list<string> track_list;
        if (_local_tracks_list.empty()) {
            if (_rtc_room_interface->IsJoined()) {
                ((CButton*)GetDlgItem(IDC_CHECK_AUDIO))->SetCheck(1);
            }
            return;
        }
        auto itor = _local_tracks_list.begin();
        while (itor != _local_tracks_list.end()) {
            if ((*itor)->GetTag().compare(MICROPHONE_TAG) == 0) {
                track_list.emplace_back((*itor)->GetTrackId());
                if (Err_Pre_Publish_Not_Complete == _rtc_room_interface->UnPublishTracks(track_list)) {
                    ((CButton*)GetDlgItem(IDC_CHECK_AUDIO))->SetCheck(1);
                }

                (*itor)->Release();
                _local_tracks_list.erase(itor);
                break;
            }
            ++itor;
        }
        _wnd_status_bar.SetText(_T("取消发布音频"), 1, 0);
    }
}

void CRtcDemoV2::OnBnClickedCheckImportRawData()
{
    // TODO: Add your control notification handler code here
    if (!_rtc_room_interface) {
        return;
    }
    lock_guard<recursive_mutex> lck(_mutex);
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->GetCheck()) {
        if (!_rtc_room_interface->IsJoined()) {
            return;
        }
        _rtc_audio_interface->EnableAudioFakeInput(true);
        auto video_track_ptr = qiniu_v2::QNTrackInfo::CreateVideoTrackInfo(
            "",
            EXTERNAL_TAG,
            GetDlgItem(IDC_STATIC_VIDEO_PREVIEW3)->m_hWnd,
            426,
            240,
            30,
            300000,
            qiniu_v2::tst_ExternalYUV,
            false,
            false
        );
        qiniu_v2::TrackInfoList track_list;
        track_list.emplace_back(video_track_ptr);
        auto ret = _rtc_room_interface->PublishTracks(track_list);
        if (ret != 0) {
            ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->SetCheck(0);
            MessageBox(_T("发布失败，请检查设备状态是否可用？或者不要操作太快！"));
        } else {
            // 临时禁用，待发布结果通知后再激活控件，避免频繁操作
            ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->EnableWindow(FALSE);
        }
        video_track_ptr->Release();
    } else {
        _rtc_audio_interface->EnableAudioFakeInput(false);

        list<string> track_list;
        if (_local_tracks_list.empty()) {
            if (_rtc_room_interface->IsJoined()) {
                ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->SetCheck(1);
            }
            return;
        }
        auto itor = _local_tracks_list.begin();
        while (itor != _local_tracks_list.end()) {
            if ((*itor)->GetTag().compare(EXTERNAL_TAG) == 0) {
                track_list.emplace_back((*itor)->GetTrackId());
                if (Err_Pre_Publish_Not_Complete == _rtc_room_interface->UnPublishTracks(track_list)) {
                    ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->SetCheck(1);
                }
                (*itor)->Release();
                _local_tracks_list.erase(itor);
                break;
            }
            ++itor;
        }
        GetDlgItem(IDC_STATIC_VIDEO_PREVIEW3)->Invalidate();

        _stop_external_flag = true;
        if (_fake_audio_thread.joinable()) {
            _fake_audio_thread.join();
        }
        if (_fake_video_thread.joinable()) {
            _fake_video_thread.join();
        }
        _wnd_status_bar.SetText(_T("取消发布外部导入"), 1, 0);
        AdjustMergeStreamLayouts();
    }
}

void CRtcDemoV2::ImportExternalRawFrame(const string& track_id_)
{
    // 模拟导入视频数据,当前使用当前目录下指定的音视频文件
    _stop_external_flag = true;
    if (_fake_video_thread.joinable()) {
        _fake_video_thread.join();
    }
    if (_fake_audio_thread.joinable()) {
        _fake_audio_thread.join();
    }

    string track_id(track_id_.c_str());
    _fake_video_thread = thread([&, track_id] {
        FILE* fp = nullptr;
        fopen_s(&fp, "426x240.yuv", "rb");
        uint8_t *buf = (uint8_t*)malloc(426 * 240 * 3 / 2);
        if (!fp || !buf) {
            MessageBox(_T("foreman_320x240.yuv 文件打开失败，请确认此文件件是否存在!"));
            return;
        }
        size_t ret(0);
        _stop_external_flag = false;
        chrono::system_clock::time_point start_tp = chrono::system_clock::now();
        while (!_stop_external_flag) {
            ret = fread_s(buf, 426 * 240 * 3 / 2, 1, 426 * 240 * 3 / 2, fp);
            if (ret > 0) {
                _rtc_video_interface->InputVideoFrame(
                    track_id,
                    buf,
                    426 * 240 * 3 / 2,
                    426,
                    240,
                    chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - start_tp).count(),
                    qiniu_v2::VideoCaptureType::kI420,
                    qiniu_v2::kVideoRotation_0);
            } else {
                fseek(fp, 0, SEEK_SET);
                continue;
            }
            Sleep(1000 / 30);
        }
        free(buf);
        fclose(fp);
    });

    // 模拟导入音频数据
    _rtc_audio_interface->EnableAudioFakeInput(true);
    _fake_audio_thread = thread([&] {
        FILE* fp = nullptr;
        fopen_s(&fp, "44100_16bits_2channels.pcm", "rb");
        if (!fp) {
            MessageBox(_T("PCM 文件:44100_16bits_2channels.pcm 打开失败，请确认此文件件是否存在!"));
            return;
        }
        // 每次导入 20 ms 的数据，即 441 * 2 个 samples
        uint8_t *buf = (uint8_t*)malloc(441 * 2 * 2 * 2);

        size_t ret(0);
        chrono::system_clock::time_point start_tp = chrono::system_clock::now();
        int64_t audio_frame_count(0);
        while (!_stop_external_flag) {
            if (chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - start_tp).count() >= audio_frame_count * 20000) {
            } else {
                Sleep(10);
                continue;
            }

            ret = fread_s(buf, 441 * 2 * 4, 1, 441 * 2 * 4, fp);
            if (ret >= 441 * 8) {
                _rtc_audio_interface->InputAudioFrame(
                    buf,
                    441 * 8,
                    16,
                    44100,
                    2,
                    441 * 2
                );
                ++audio_frame_count;
            } else {
                fseek(fp, 0, SEEK_SET);
                continue;
            }
        }
        free(buf);
        fclose(fp);
    });
}

void CRtcDemoV2::CreateCustomMergeJob()
{
    qiniu_v2::MergeJob job_desc;
    job_desc.job_id         = unicode2utf(_room_name.GetBuffer()) + "_merge";
    job_desc.publish_url    = _merge_config.publish_url;
    job_desc.width          = _merge_config.job_width;
    job_desc.height         = _merge_config.job_height;
    job_desc.fps            = _merge_config.job_fps;
    job_desc.bitrate        = _merge_config.job_bitrate;
    job_desc.min_bitrate    = _merge_config.job_min_bitrate;
    job_desc.max_bitrate    = _merge_config.job_max_bitrate;
    job_desc.stretch_mode = (qiniu_v2::MergeStretchMode)(_merge_config.job_stretch_mode);
    job_desc.is_hold_last_frame = _merge_config.hold_last_frame;

    qiniu_v2::MergeLayer background;
    background.layer_url    = _merge_config.background_url;
    background.pos_x        = _merge_config.background_x;
    background.pos_y        = _merge_config.background_y;
    background.layer_width  = _merge_config.background_width;
    background.layer_height = _merge_config.background_height;

    qiniu_v2::MergeLayerList watermarks;
    qiniu_v2::MergeLayer mark;
    mark.layer_url      = _merge_config.watermark_url;
    mark.pos_x          = _merge_config.watermark_x;
    mark.pos_y          = _merge_config.watermark_y;
    mark.layer_width    = _merge_config.watermark_width;
    mark.layer_height   = _merge_config.watermark_height;
    watermarks.emplace_back(mark);

    _custom_merge_id = job_desc.job_id;
    _rtc_room_interface->CreateMergeJob(job_desc, background, watermarks);
}

void CRtcDemoV2::AdjustMergeStreamLayouts()
{
    if (_contain_admin_flag) {
        struct Pos
        {
            int pos_x = 0;
            int pos_y = 0;
        };
        Pos merge_opts[9];
        for (int i(0); i < 3; ++i) {
            for (int j(0); j < 3; ++j) {
                merge_opts[i * 3 + j] = {j * CANVAS_WIDTH / 3, i * CANVAS_HEIGHT / 3};
            }
        }
        qiniu_v2::MergeOptInfoList add_tracks_list;
        list<string> remove_tracks_list;
        int num(-1);
        for (auto&& itor : _local_tracks_list) {
            qiniu_v2::MergeOptInfo merge_opt;
            merge_opt.track_id = itor->GetTrackId();
            merge_opt.is_video = true;
            if (itor->GetKind().compare("audio") == 0) {
                merge_opt.is_video = false;
                add_tracks_list.emplace_back(merge_opt);
                continue;
            }
            // Demo 保证视频九宫格布局
            if (itor->IsMaster()) {
                merge_opt.pos_x = 0;
                merge_opt.pos_y = 0;
                merge_opt.pos_z = 0;
                merge_opt.width = CANVAS_WIDTH;
                merge_opt.height = CANVAS_HEIGHT;
            } else {
                ++num;
                merge_opt.pos_x = merge_opts[num].pos_x;
                merge_opt.pos_y = merge_opts[num].pos_y;
                merge_opt.pos_z = 1;
                merge_opt.width = CANVAS_WIDTH / 3;
                merge_opt.height = CANVAS_HEIGHT / 3;
            }
            add_tracks_list.emplace_back(merge_opt);
        }
        for (auto&& itor : _remote_tracks_map) {
            qiniu_v2::MergeOptInfo merge_opt;
            merge_opt.track_id = itor.second->track_info_ptr->GetTrackId();
            merge_opt.is_video = true;
            if (itor.second->track_info_ptr->GetKind().compare("audio") == 0) {
                merge_opt.is_video = false;
                add_tracks_list.emplace_back(merge_opt);
                continue;
            }
            // Demo 保证视频九宫格布局
            ++num;
            merge_opt.pos_x = merge_opts[num].pos_x;
            merge_opt.pos_y = merge_opts[num].pos_y;
            merge_opt.pos_z = 1;
            merge_opt.width = CANVAS_WIDTH / 3;
            merge_opt.height = CANVAS_HEIGHT / 3;
            add_tracks_list.emplace_back(merge_opt);
        }
        _rtc_room_interface->SetMergeStreamlayouts(add_tracks_list, remove_tracks_list);
    }


    // 自定义合流，根据界面配置使用一个本地或者远端 track 
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_MERGE))->GetCheck()) {

        qiniu_v2::MergeOptInfoList add_tracks_list;
        list<string> remove_tracks_list;
        for (auto&& itor : _local_tracks_list) {
            qiniu_v2::MergeOptInfo merge_opt;
            merge_opt.track_id = itor->GetTrackId();
            merge_opt.is_video = true;
            if (itor->GetKind().compare("audio") == 0 && _merge_config.merge_local_audio) {
                merge_opt.is_video = false;
                add_tracks_list.emplace_back(merge_opt);
                continue;
            }
            
            if (_merge_config.merge_local_video) {
                // 这里只做演示，用户可根据自己需求设置相应 video track 合流时的填充模式
                if (1 == ((CButton*)GetDlgItem(IDC_CHECK_STRETCH_MODE))->GetCheck()) {
                    merge_opt.stretchMode = qiniu_v2::SCALE_TO_FIT;
                }
                merge_opt.pos_x = _merge_config.local_video_x;
                merge_opt.pos_y = _merge_config.local_video_y;
                merge_opt.pos_z = 1;
                merge_opt.width = _merge_config.local_video_width;
                merge_opt.height = _merge_config.local_video_height;
                merge_opt.is_support_sei = true;
                add_tracks_list.emplace_back(merge_opt);
            }
        }

        for (auto&& itor : _remote_tracks_map) {
            qiniu_v2::MergeOptInfo merge_opt;
            merge_opt.track_id = itor.second->track_info_ptr->GetTrackId();
            merge_opt.is_video = true;
            if (itor.second->track_info_ptr->GetKind().compare("audio") == 0 && _merge_config.merge_remote_audio) {
                merge_opt.is_video = false;
                add_tracks_list.emplace_back(merge_opt);
                continue;
            }

            if (_merge_config.merge_remote_video) {
                // 这里只做演示，用户可根据自己需求设置相应 video track 合流时的填充模式
                if (1 == ((CButton*)GetDlgItem(IDC_CHECK_STRETCH_MODE))->GetCheck()) {
                    merge_opt.stretchMode = qiniu_v2::SCALE_TO_FIT;
                }
                merge_opt.pos_x = _merge_config.remote_video_x;
                merge_opt.pos_y = _merge_config.remote_video_y;
                merge_opt.pos_z = 1;
                merge_opt.width = _merge_config.remote_video_width;
                merge_opt.height = _merge_config.remote_video_height;
                merge_opt.is_support_sei = false;
                add_tracks_list.emplace_back(merge_opt);
            }
        }
        _rtc_room_interface->SetMergeStreamlayouts(add_tracks_list, remove_tracks_list, _custom_merge_id);
    }
}

void CRtcDemoV2::AdjustSubscribeLayouts()
{
    if (_remote_tracks_map.empty()) {
        return;
    }
    int wnd_num(0);
    RECT wnd_rc;
    GetWindowRect(&wnd_rc);
    TRACE("MainDialog rect x:%d, y:%d, right:%d, botton:%d\n", 
        wnd_rc.left, 
        wnd_rc.top,
        wnd_rc.right,
        wnd_rc.bottom
        );
    int main_wnd_height = wnd_rc.bottom - wnd_rc.top;
    const int wnd_width = 320;
    const int wnd_height = 240;
    int start_x = wnd_rc.left - wnd_width;
    int start_y = wnd_rc.top;
    for (auto&& itor : _remote_tracks_map) {
        if (itor.second->render_wnd_ptr) {
            itor.second->render_wnd_ptr->MoveWindow(
                start_x,
                start_y + wnd_height * wnd_num,
                wnd_width,
                wnd_height
            );
            if (start_y + wnd_width * wnd_num >= main_wnd_height) {
                wnd_num = 0;
                start_x += wnd_width;
            } else {
                ++wnd_num;
            }
        }
    }
}

void CRtcDemoV2::OnBnClickedCheckDesktopAudio()
{
    // TODO: Add your control notification handler code here
    if (!_rtc_audio_interface) {
        return;
    }
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_DESKTOP_AUDIO))->GetCheck()) {
        _rtc_audio_interface->MixDesktopAudio(true);
    } else {
        _rtc_audio_interface->MixDesktopAudio(false);
    }
}

void CRtcDemoV2::OnBnClickedBtnKickout()
{
    // TODO: Add your control notification handler code here
    if (!_rtc_room_interface || !_rtc_room_interface->IsJoined()) {
        return;
    }
    if (!_contain_admin_flag) {
        MessageBox(_T("您当前没有踢人权限，请使用 admin 进行登录！"));
        return;
    } else {
        int index = _user_list_ctrl.GetSelectionMark();
        if (index == -1) {
            MessageBox(_T("请选中要踢出的用户！"));
            return;
        }
        // 所选择的用户当前没有发布媒体流
        CString user_id = _user_list_ctrl.GetItemText(index, 0);

        if (_rtc_room_interface) {
            _rtc_room_interface->KickoutUser(unicode2utf(user_id.GetBuffer()).c_str());
        }
    }
}

void CRtcDemoV2::OnBnClickedCheckMuteAudio()
{
    // TODO: Add your control notification handler code here
    if (!_rtc_room_interface) {
        return;
    }
    // 静默本地音频 Track，一端仅有一路音频 Track
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_MUTE_AUDIO))->GetCheck()) {
        _rtc_room_interface->MuteAudio(true);
    } else {
        _rtc_room_interface->MuteAudio(false);
    }

}

void CRtcDemoV2::OnBnClickedCheckMuteVideo()
{
    // TODO: Add your control notification handler code here
    if (!_rtc_room_interface) {
        return;
    }
    bool mute_flag = false;
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_MUTE_VIDEO))->GetCheck()) {
        mute_flag = true;
    }
    // 静默或取消静默本地所有的视频 Track
    // 这里对所有的视频 Track 进行控制
    lock_guard<recursive_mutex> lck(_mutex);
    for (auto&& itor : _local_tracks_list) {
        if (itor->GetKind().compare("video") == 0) {
            _rtc_room_interface->MuteVideo(itor->GetTrackId(), mute_flag);
        }
    }
}


void CRtcDemoV2::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    // TODO: Add your message handler code here and/or call default
    lock_guard<recursive_mutex> lck(_mutex);
    if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER_RECORD) {
        int pos = ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_RECORD))->GetPos();
        if (_rtc_audio_interface) {
            // 调整 SDK 内部音量
            _rtc_audio_interface->SetAudioVolume(
                unicode2utf(_user_id.GetBuffer()),
                pos / 100.0f
            );
        }
    } else if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER_PLAYOUT) {
        int pos = ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_PLAYOUT))->GetPos();

        if (_rtc_audio_interface) {
            // 调整所有用户的音量（这里只是为了方便演示，实际可根据需要控制指定用户的音量）
            for (auto&& itor : _user_list) {
                if (itor.compare(unicode2utf(_user_id.GetBuffer())) != 0) {
                    _rtc_audio_interface->SetAudioVolume(
                        itor,
                        pos / 100.0f
                    );
                }
            }
        }
    }

    __super::OnHScroll(nSBCode, nPos, pScrollBar);
    __super::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CRtcDemoV2::OnCbnSelchangeComboMicrophone()
{
    // 输入音频设备配置
    if (!_rtc_audio_interface) {
        return;
    }
    int audio_recorder_device_index(-1);
    audio_recorder_device_index = ((CComboBox *)GetDlgItem(IDC_COMBO_MICROPHONE))->GetCurSel();
    audio_recorder_device_index = (audio_recorder_device_index == CB_ERR) ? 0 : audio_recorder_device_index;

    if (audio_recorder_device_index >= 0) {
        qiniu_v2::AudioDeviceInfo dev_info = _microphone_dev_map[audio_recorder_device_index];

        qiniu_v2::AudioDeviceSetting audio_setting;
        audio_setting.device_index = dev_info.device_index;
        audio_setting.device_type = qiniu_v2::AudioDeviceSetting::wdt_DefaultDevice;
        _rtc_audio_interface->SetRecordingDevice(audio_setting);
    }
}

void CRtcDemoV2::OnCbnSelchangeComboPlayout()
{
    // 播放音频设备配置
    if (!_rtc_audio_interface) {
        return;
    }
    int audio_playout_device_index(-1);
    audio_playout_device_index = ((CComboBox *)GetDlgItem(IDC_COMBO_PLAYOUT))->GetCurSel();
    audio_playout_device_index = (audio_playout_device_index == CB_ERR) ? 0 : audio_playout_device_index;

    if (audio_playout_device_index >= 0) {
        qiniu_v2::AudioDeviceInfo dev_info = _playout_dev_map[audio_playout_device_index];
        
        qiniu_v2::AudioDeviceSetting audio_setting;
        audio_setting.device_index = dev_info.device_index;
        audio_setting.device_type = qiniu_v2::AudioDeviceSetting::wdt_DefaultDevice;
        _rtc_audio_interface->SetPlayoutDevice(audio_setting);
    }
}

afx_msg LRESULT CRtcDemoV2::OnHandleMessage(WPARAM wParam, LPARAM lParam)
{
    MergeDialog::MergeConfig *config = (MergeDialog::MergeConfig *)lParam;
    _merge_config = *config;
    return 0;
}

afx_msg LRESULT CRtcDemoV2::OnSendMessage(WPARAM wParam, LPARAM lParam)
{
    if (_rtc_room_interface) {
        LPTSTR lpMessage = (LPTSTR)lParam;
        _bstr_t bstr("");
        LPTSTR strTmp = lpMessage;
        bstr = strTmp;
        std::string strMsg = bstr;
        list<string> users_list;
        _rtc_room_interface->SendCustomMessage(users_list, "", string_to_utf8(strMsg));
    }
    return 0;
}

void CRtcDemoV2::OnBnClickedButtonMerge()
{
    MergeDialog dlgMerge;
    dlgMerge._merge_config = _merge_config;
    dlgMerge.DoModal();
}

void CRtcDemoV2::OnBnClickedButtonSendMsg()
{
    // TODO: 在此添加控件通知处理程序代码
    _dlg_msg._user_id = _user_id;
    _dlg_msg.DoModal();
}


void CRtcDemoV2::OnCbnSelchangeComboLocalRotate()
{
    // TODO: 在此添加控件通知处理程序代码
    int rotation_sel = ((CComboBox*)GetDlgItem(IDC_COMBO_LOCAL_ROTATION))->GetCurSel();
    qiniu_v2::VideoRotation video_rotation = qiniu_v2::VideoRotation::kVideoRotation_0;
    switch (rotation_sel)
    {
    case 0:
        video_rotation = qiniu_v2::VideoRotation::kVideoRotation_0;
        break;
    case 1:
        video_rotation = qiniu_v2::VideoRotation::kVideoRotation_90;
        break;
    case 2:
        video_rotation = qiniu_v2::VideoRotation::kVideoRotation_180;
        break;
    case 3:
        video_rotation = qiniu_v2::VideoRotation::kVideoRotation_270;
        break;
    default:
        break;
    }

    for (auto&& itor : _local_tracks_list) {
        if (itor->GetKind().compare("video") == 0) {
            if (_rtc_video_interface) {
                _rtc_video_interface->SetVideoRotation(itor->GetTrackId(), video_rotation);
            }
        }
    }
}


void CRtcDemoV2::OnCbnSelchangeComboRemoteRotate()
{
    // TODO: 在此添加控件通知处理程序代码
    int rotation_sel = ((CComboBox*)GetDlgItem(IDC_COMBO_REMOTE_ROTATION))->GetCurSel();
    qiniu_v2::VideoRotation video_rotation = qiniu_v2::VideoRotation::kVideoRotation_0;
    switch (rotation_sel)
    {
    case 0:
        video_rotation = qiniu_v2::VideoRotation::kVideoRotation_0;
        break;
    case 1:
        video_rotation = qiniu_v2::VideoRotation::kVideoRotation_90;
        break;
    case 2:
        video_rotation = qiniu_v2::VideoRotation::kVideoRotation_180;
        break;
    case 3:
        video_rotation = qiniu_v2::VideoRotation::kVideoRotation_270;
        break;
    default:
        break;
    }

    for (auto&& itor : _remote_tracks_map) {
        if (itor.second->track_info_ptr->GetKind().compare("video") == 0) {
            if (_rtc_video_interface) {
                _rtc_video_interface->SetVideoRotation(itor.first, video_rotation);
            }
        }
    }
}

void CRtcDemoV2::OnCbnSelchangeComboSubscribeProfile()
{
    // TODO: 在此添加控件通知处理程序代码
    int profile_sel = ((CComboBox*)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))->GetCurSel();
    qiniu_v2::QNTrackProfile video_profile = qiniu_v2::QNTrackProfile::HIGH;
    switch (profile_sel)
    {
    case 0:
        video_profile = qiniu_v2::QNTrackProfile::HIGH;
        break;
    case 1:
        video_profile = qiniu_v2::QNTrackProfile::MEDIUM;
        break;
    case 2:
        video_profile = qiniu_v2::QNTrackProfile::LOW;
        break;
    default:
        break;
    }

    // 这里只是演示时是将所有订阅流一起切成设置的 profile，用户实际根据自己需求，更改相应track的profile，
    // 想要切换成哪种 profile，只要将对应的 mChooseToSub 设置为 true。
    lock_guard<recursive_mutex> lck(_mutex);
    qiniu_v2::TrackInfoList sub_tracks_list;

    for (auto&& itor : _remote_tracks_map) {
        if (itor.second->track_info_ptr->GetKind().compare("video") == 0) {
            auto tmp_track_ptr = qiniu_v2::QNTrackInfo::Copy(itor.second->track_info_ptr);
            for (auto&& layer_itor : tmp_track_ptr->GetLayerInfo()) {
                if (layer_itor.mProfile == video_profile) {
                    layer_itor.mChooseToSub = true;
                }
            }
            sub_tracks_list.emplace_back(tmp_track_ptr);
        }
    }
    _rtc_room_interface->UpdateSubscribeTracks(sub_tracks_list);
}

void CRtcDemoV2::OnBnClickedButtonSimulcast()
{
    // TODO: 在此添加控件通知处理程序代码
    CString btn_str;
    GetDlgItemText(IDC_BUTTON_SIMULCAST, btn_str);
    if (btn_str.CompareNoCase(_T("开启多流")) == 0) {
        SetDlgItemText(IDC_BUTTON_SIMULCAST, _T("关闭多流"));
        _enable_simulcast = true;
    }
    else {
        SetDlgItemText(IDC_BUTTON_SIMULCAST, _T("开启多流"));
        _enable_simulcast = false;
    }
}


void CRtcDemoV2::OnBnClickedButtonForward()
{
    // TODO: 在此添加控件通知处理程序代码
    CString btn_str;
    GetDlgItemText(IDC_BUTTON_FORWARD, btn_str);
    // 登录房间
    if (btn_str.CompareNoCase(_T("单流转推")) == 0) {
        SetDlgItemText(IDC_BUTTON_FORWARD, _T("停止单流转推"));
        _custom_forward_id = "window-forward";
        qiniu_v2::ForwardOptInfo forwardInfo;
        forwardInfo.audio_only = false;
        forwardInfo.is_internal = true;
        forwardInfo.job_id = _custom_forward_id;
        forwardInfo.publish_url = "rtmp://pili-publish.qnsdk.com/sdk-live/window-forward";
        _forward_audio_flag = false;
        _forward_video_flag = false;
        for (auto&& itor : _local_tracks_list) {
            if (itor->GetKind().compare("audio") == 0 && !_forward_audio_flag) {
                _forward_audio_flag = true;
                forwardInfo.track_id_list.emplace_back(itor->GetTrackId());
            }
            if (itor->GetKind().compare("video") == 0 && !_forward_video_flag) {
                _forward_video_flag = true;
                forwardInfo.track_id_list.emplace_back(itor->GetTrackId());
            }
        }
        if (_rtc_room_interface) {
            _rtc_room_interface->CreateForwardJob(forwardInfo);
            _contain_forward_flag = true;
        }
    } else {
        SetDlgItemText(IDC_BUTTON_FORWARD, _T("单流转推"));
        if (_rtc_room_interface) {
            _rtc_room_interface->StopForwardJob(_custom_forward_id, 0);
            _contain_forward_flag = false;
        }
    }
}


void CRtcDemoV2::OnBnClickedButtonExtraData()
{
    // TODO: 在此添加控件通知处理程序代码IDC_BUTTON_EXTRA_DATA
    CString btn_str;
    GetDlgItemText(IDC_BUTTON_EXTRA_DATA, btn_str);
    if (btn_str.CompareNoCase(_T("关闭数据加解密")) == 0) {
        SetDlgItemText(IDC_BUTTON_EXTRA_DATA, _T("开启数据加解密"));
        _enable_encryptor_decryptor = false;
    } else {
        _rtc_audio_interface->EnableLocalAudioPacketCallBack(true);
        _rtc_audio_interface->EnableRemoteAudioPacketCallBack(true);
        SetDlgItemText(IDC_BUTTON_EXTRA_DATA, _T("关闭数据加解密"));
        _enable_encryptor_decryptor = true;
    }
}


void CRtcDemoV2::OnBnClickedCheckCameraImage()
{
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_CAMERA_IMAGE))->GetCheck()) {
        _rtc_video_interface->PushCameraTrackWithImage("pause_publish.jpeg");
    } else {
        _rtc_video_interface->PushCameraTrackWithImage("");
    }
}

void CRtcDemoV2::OnBnClickedCheckCameraMirror()
{
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_CAMERA_MIRROR))->GetCheck()) {
        _rtc_video_interface->CameraCaptureMirror(true);
    } else {
        _rtc_video_interface->CameraCaptureMirror(false);
    }
}

void CRtcDemoV2::OnBnClickedBtnSei()
{
    CString btn_str;
    GetDlgItemText(IDC_BTN_SEI, btn_str);
    list<string> tracks_id_list;
    for (auto&& itor : _local_tracks_list) {
        if (itor->GetKind().compare("video") == 0) {
            tracks_id_list.emplace_back(itor->GetTrackId());
        }
    }

    if (tracks_id_list.size() != 0) {
        if (btn_str.CompareNoCase(_T("开启SEI")) == 0) {
            SetDlgItemText(IDC_BTN_SEI, _T("关闭SEI"));
            _rtc_video_interface->SetLocalVideoSei(tracks_id_list, string_to_utf8("七牛SEI" + std::to_string(rand())), -1);
        } else {
            SetDlgItemText(IDC_BTN_SEI, _T("开启SEI"));
            _rtc_video_interface->SetLocalVideoSei(tracks_id_list, "", 0);
        }
    }
}
