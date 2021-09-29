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
#define EXTERNAL_VIDEO "external_video"
#define EXTERNAL_AUDIO "external_audio"

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
}

BOOL CRtcDemoV2::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    
    ReadConfigFile();
    std::string ver;
    qiniu::QNRTC::GetVersion(ver);
    TRACE("Sdk version: %s", ver.c_str());
    QNRTC::SetLogFileEnabled(qiniu::LOG_INFO, "rtc_log", "rtc.log");
    _rtc_client_ptr = QNRTC::CreateClient();
    _rtc_client_ptr->SetQNClientEventListener(this);
    _rtc_client_ptr->SetQNPublishResultCallback(this);
    _rtc_client_ptr->SetLiveStreamingListener(this);

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
    ON_BN_CLICKED(IDC_CHECK_CAMERA_IMAGE, &CRtcDemoV2::OnBnClickedCheckCameraImage)
    ON_BN_CLICKED(IDC_CHECK_CAMERA_MIRROR, &CRtcDemoV2::OnBnClickedCheckCameraMirror)
    ON_BN_CLICKED(IDC_BTN_SEI, &CRtcDemoV2::OnBnClickedBtnSei)
    ON_BN_CLICKED(IDC_CHECK_IMPORT_RAW_AUDIO_DATA, &CRtcDemoV2::OnBnClickedCheckImportRawAudioData)
    ON_BN_CLICKED(IDC_CHECK_LOCAL_MIRROR, &CRtcDemoV2::OnBnClickedCheckLocalMirror)
    ON_BN_CLICKED(IDC_CHECK_REMOTE_MIRROR, &CRtcDemoV2::OnBnClickedCheckRemoteMirror)
    ON_CBN_SELCHANGE(IDC_COMBO_LOCAL_STRETCH_MODE, &CRtcDemoV2::OnCbnSelchangeComboLocalStretchMode)
    ON_CBN_SELCHANGE(IDC_COMBO_REMOTE_STRETCH_MODE, &CRtcDemoV2::OnCbnSelchangeComboRemoteStretchMode)
    ON_BN_CLICKED(IDC_CHECK_CLIP, &CRtcDemoV2::OnBnClickedCheckClip)
    ON_BN_CLICKED(IDC_CHECK_SCALE, &CRtcDemoV2::OnBnClickedCheckScale)
    ON_BN_CLICKED(IDC_CHECK_MERGE, &CRtcDemoV2::OnBnClickedCheckMerge)
    ON_BN_CLICKED(IDC_CHECK_IMPORT_STATS, &CRtcDemoV2::OnBnClickedCheckImportStats)
    ON_BN_CLICKED(IDC_CHECK_HARD_ENCODER, &CRtcDemoV2::OnBnClickedCheckHardEncoder)
    ON_BN_CLICKED(IDC_BTN_KICKOUT, &CRtcDemoV2::OnBnClickedBtnKickout)
END_MESSAGE_MAP()


// CRtcDemoV2 message handlers

void CRtcDemoV2::OnDestroy()
{
    // 结束外部数据导入的线程
    if (_stats_pDig) {
        _stats_pDig->DestroyWindow();
        delete _stats_pDig;
        _stats_pDig = nullptr;
    }
    _stop_stats_flag = true;
    _stop_video_external_flag = true;
    _stop_audio_external_flag = true;
    if (_fake_audio_thread.joinable()) {
        _fake_audio_thread.join();
    }
    if (_fake_video_thread.joinable()) {
        _fake_video_thread.join();
    }
    if (_stats_thread.joinable()) {
        _stats_thread.join();
    }
    Leave();
    if (_rtc_client_ptr) {
        QNRTC::DestroyRtcClient(_rtc_client_ptr);
        _rtc_client_ptr = nullptr;
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
        if (_rtc_client_ptr) {
            _rtc_client_ptr->Loop();
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
    GetDlgItem(IDC_BUTTON_FORWARD)->EnableWindow(FALSE);
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

    ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_STRETCH_MODE))->InsertString(-1, utf2unicode("ASPECT_FIT").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_STRETCH_MODE))->InsertString(-1, utf2unicode("ASPECT_FILL").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_STRETCH_MODE))->InsertString(-1, utf2unicode("SCALE_FIT").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_STRETCH_MODE))->SetCurSel(0);

    ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_STRETCH_MODE))->InsertString(-1, utf2unicode("ASPECT_FIT").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_STRETCH_MODE))->InsertString(-1, utf2unicode("ASPECT_FILL").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_STRETCH_MODE))->InsertString(-1, utf2unicode("SCALE_FIT").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_STRETCH_MODE))->SetCurSel(0);

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
    int camera_count = _rtc_client_ptr->GetCameraCount();
    for (int i(0); i < camera_count; ++i) {
        qiniu::QNCameraDeviceInfo ci = _rtc_client_ptr->GetCameraInfo(i);
        _camera_dev_map[ci.device_id] = ci;
        ((CComboBox *)GetDlgItem(IDC_COMBO_CAMERA))->InsertString(-1, utf2unicode(ci.device_name).c_str());
    }
    ((CComboBox *)GetDlgItem(IDC_COMBO_CAMERA))->SetCurSel(0);

    // 初始化屏幕窗口列表
    int screen_count = _rtc_client_ptr->GetScreenWindowCount();
    for (int i(0); i < screen_count; ++i) {
        qiniu::QNScreenWindowInfo sw = _rtc_client_ptr->GetScreenWindowInfo(i);
        _screen_info_map[sw.id] = sw;
        ((CComboBox *)GetDlgItem(IDC_COMBO_SCREEN))->InsertString(-1, utf2unicode(sw.title).c_str());
    }
    ((CComboBox *)GetDlgItem(IDC_COMBO_SCREEN))->SetCurSel(0);

    // 初始化音频采集设备列表
    int audio_rec_count = _rtc_client_ptr->GetAudioDeviceCount(qiniu::QNAudioDeviceInfo::adt_record);
    for (int i(0); i < audio_rec_count; ++i) {
        qiniu::QNAudioDeviceInfo audio_info;
        _rtc_client_ptr->GetAudioDeviceInfo(qiniu::QNAudioDeviceInfo::adt_record, i, audio_info);
        ((CComboBox *)GetDlgItem(IDC_COMBO_MICROPHONE))->InsertString(
            -1,
            utf2unicode(audio_info.device_name).c_str()
        );
        if (audio_info.is_default) {
            qiniu::QNAudioDeviceSetting ads;
            ads.device_index = audio_info.device_index;
            ads.device_type = qiniu::QNAudioDeviceSetting::wdt_DefaultDevice;
            _rtc_client_ptr->SetRecordingDevice(ads);
        }
        _microphone_dev_map[i] = audio_info;
    }
    ((CComboBox *)GetDlgItem(IDC_COMBO_MICROPHONE))->SetCurSel(0);

    // 初始化音频播放设备列表
    int audio_play_count = _rtc_client_ptr->GetAudioDeviceCount(qiniu::QNAudioDeviceInfo::adt_playout);
    for (int i(0); i < audio_play_count; ++i) {
        qiniu::QNAudioDeviceInfo audio_info;
        _rtc_client_ptr->GetAudioDeviceInfo(qiniu::QNAudioDeviceInfo::adt_playout, i, audio_info);
        ((CComboBox *)GetDlgItem(IDC_COMBO_PLAYOUT))->InsertString(
            -1,
            utf2unicode(audio_info.device_name).c_str()
        );
        if (audio_info.is_default) {
            qiniu::QNAudioDeviceSetting ads;
            ads.device_index = audio_info.device_index;
            ads.device_type = qiniu::QNAudioDeviceSetting::wdt_DefaultDevice;
            _rtc_client_ptr->SetPlayoutDevice(ads);
        }
        _playout_dev_map[i] = audio_info;
    }
    ((CComboBox *)GetDlgItem(IDC_COMBO_PLAYOUT))->SetCurSel(0);

    ((CButton*)GetDlgItem(IDC_CHECK_CAMERA))->SetCheck(1);
    ((CButton*)GetDlgItem(IDC_CHECK_SCREEN))->SetCheck(0);
    ((CButton*)GetDlgItem(IDC_CHECK_AUDIO))->SetCheck(1);
    ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->SetCheck(0);
}

std::tuple<int, int> CRtcDemoV2::FindBestVideoSize(const qiniu::CameraCapabilityVec& camera_cap_vec_)
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

void CRtcDemoV2::OnLocalAudioPCMFrame(
    const unsigned char* data,
    int bitsPerSample,
    int sampleRrate,
    size_t channels,
    size_t samplePoints)
{
}

void CRtcDemoV2::OnLocalVideoFrame(
    const string& trackId,
    const unsigned char* data,
    int dataSize,
    int width,
    int height,
    qiniu::QNVideoSourceType videoType)
{
}

void CRtcDemoV2::OnRemoteAudioFrame(
    const string& userId,
    const unsigned char* data,
    int bitsPerSample,
    int sampleRrate,
    size_t channels,
    size_t samplePoints)
{
}


void CRtcDemoV2::OnRemoteVideoFrame(
    const std::string& userId,
    const string& trackId,
    const unsigned char* data,
    int dataSize,
    int width,
    int height,
    qiniu::QNVideoSourceType videoType)
{
}

void CRtcDemoV2::OnPreviewVideoFrame(
    const unsigned char* data,
    int dataSize,
    int width,
    int height,
    QNVideoSourceType videoType)
{
}

void CRtcDemoV2::OnRemoteMixAudioPCMFrame(
    const unsigned char* data,
    int bitsPerSample,
    int sampleRrate,
    size_t channels,
    size_t samplePoints)
{
}

void CRtcDemoV2::OnVideoProfileChanged(const std::string& trackid, qiniu::QNTrackProfile profile)
{
    if (profile == qiniu::HIGH) {
        ((CComboBox*)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))->SetCurSel(0);
    } else if (profile == qiniu::MEDIUM) {
        ((CComboBox*)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))->SetCurSel(1);
    } else if (profile == qiniu::LOW) {
        ((CComboBox*)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))->SetCurSel(2);
    }
}

void CRtcDemoV2::OnVideoDeviceStateChanged(qiniu::QNVideoDeviceState deviceState, const std::string& deviceName)
{
    thread(
        [&, deviceState, deviceName] {
        wchar_t buf[512] = { 0 };
        if (qiniu::vds_lost == deviceState) {
            _snwprintf(
                buf,
                512,
                _T("视频设备：%s 被拔出！"),
                utf2unicode(deviceName).c_str()
            );
            AfxMessageBox(buf, MB_OK);
        }
    }
    ).detach();
}

void CRtcDemoV2::OnAudioDeviceStateChanged(qiniu::QNAudioDeviceState deviceState, const std::string& deviceGuid)
{
    thread(
        [&, deviceState, deviceGuid] {
        wchar_t buf[512] = { 0 };
        if (qiniu::ads_active != deviceState) {
            _snwprintf(
                buf,
                512,
                _T("音频设备：%s 被拔出！"),
                utf2unicode(deviceGuid).c_str()
            );
            AfxMessageBox(buf, MB_OK);
        }
    }
    ).detach();
}

void CRtcDemoV2::OnPublished()
{
    _local_published_track_list = _rtc_client_ptr->GetPublishedTracks();
    for (auto&& itor : _local_published_track_list) {
        if (itor->GetSourceType() == tst_ExternalYUV) {
            if (_custom_video_track_ptr && (_custom_video_track_ptr->GetTrackID().compare("") != 0)) {
                if (_stop_video_external_flag) {
                    ImportExternalVideoRawFrame();
                    _custom_video_track_ptr->SetVideoFrameListener(this);
                }
            }
        }
        if (itor->GetSourceType() == tst_ExternalPCM) {
            if (_custom_audio_track_ptr && (_custom_audio_track_ptr->GetTrackID().compare("") != 0)) {
                if (_stop_audio_external_flag) {
                    ImportExternalAudioRawFrame();
                }
                _custom_audio_track_ptr->SetAudioFrameListener(this);
            }
        }

        if (_camera_track_ptr && (_camera_track_ptr->GetTrackID().compare("") != 0)) {
            _camera_track_ptr->SetVideoFrameListener(this);
        }

        if (_screen_track_ptr && (_screen_track_ptr->GetTrackID().compare("") != 0)) {
            _screen_track_ptr->SetVideoFrameListener(this);
        }

        if (_microphone_audio_track_ptr && (_microphone_audio_track_ptr->GetTrackID().compare("") != 0)) {
            _microphone_audio_track_ptr->SetAudioFrameListener(this);
        }
    }
}

void CRtcDemoV2::OnPublishError(int errorCode, const std::string& errorMessage)
{
}

void CRtcDemoV2::OnConnectionStateChanged(qiniu::QNConnectionState state, const qiniu::QNConnectionDisconnectedInfo& info)
{
    if (state == CONNECTED) {
        GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(TRUE);
        GetDlgItem(IDC_BUTTON_FORWARD)->EnableWindow(TRUE);
        _wnd_status_bar.SetText(_T("登录成功！"), 1, 0);
        SetDlgItemText(IDC_BUTTON_LOGIN, _T("离开"));

        _start_tp = chrono::steady_clock::now();
        SetTimer(UPDATE_TIME_DURATION_TIMER, 100, nullptr);
        StartPublish();
    } else if (state == DISCONNECTED) {
        if (info.reason == QNConnectionDisconnectedInfo::KICKED_OUT) {
            _wnd_status_bar.SetText(_T("被踢出！"), 1, 0);
        } else if (info.reason == QNConnectionDisconnectedInfo::ROOM_ERROR) {
            SetDlgItemText(IDC_BUTTON_LOGIN, _T("登录"));
            Leave();
            PostMessage(WM_COMMAND, MAKEWPARAM(IDC_BUTTON_LOGIN, BN_CLICKED), NULL);
        }
    }
}

void CRtcDemoV2::OnUserJoined(const std::string& remoteUserID, const std::string& userData)
{
    lock_guard<recursive_mutex> lck(_mutex);
    _user_list.push_back(remoteUserID);

    CString str;
    for (int i = 0; i < _user_list_ctrl.GetItemCount(); i++) {
        str = _user_list_ctrl.GetItemText(i, 0);
        if (str.CompareNoCase(utf2unicode(remoteUserID).c_str()) == 0) {
            _user_list_ctrl.DeleteItem(i);
            break;
        }
    }
    _user_list_ctrl.InsertItem(0, utf2unicode(remoteUserID).c_str());
    _user_list_ctrl.SetItemText(0, 1, utf2unicode(userData).c_str());

    wchar_t buff[1024] = { 0 };
    _snwprintf(
        buff,
        1024,
        _T("%s 加入了房间！"),
        utf2unicode(remoteUserID).c_str()
    );
    _wnd_status_bar.SetText(buff, 1, 0);
}

void CRtcDemoV2::OnUserLeft(const std::string& remoteUserID)
{
    lock_guard<recursive_mutex> lck(_mutex);
    CString str;
    for (int i = 0; i < _user_list_ctrl.GetItemCount(); i++) {
        str = _user_list_ctrl.GetItemText(i, 0);
        if (str.CompareNoCase(utf2unicode(remoteUserID).c_str()) == 0) {
            _user_list_ctrl.DeleteItem(i);
            break;
        }
    }

    auto itor = std::find(_user_list.begin(), _user_list.end(), remoteUserID);
    if (itor != _user_list.end()) {
        _user_list.erase(itor);
    }

    wchar_t buff[1024] = { 0 };
    _snwprintf(
        buff,
        1024,
        _T("%s 离开了房间！"),
        utf2unicode(remoteUserID).c_str()
    );
    _wnd_status_bar.SetText(buff, 1, 0);
    ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_ROTATION))->SetCurSel(0);
}

void CRtcDemoV2::OnUserReconnecting(const std::string& remoteUserID)
{
    wchar_t buf[512] = { 0 };
    _snwprintf(buf,
        sizeof(buf),
        _T("%s 用户正在重连"),
        utf2unicode(remoteUserID).c_str()
    );
    _wnd_status_bar.SetText(buf, 1, 0);
}

void CRtcDemoV2::OnUserReconnected(const std::string& remoteUserID)
{
    wchar_t buf[512] = { 0 };
    _snwprintf(buf,
        sizeof(buf),
        _T("%s 用户重连成功"),
        utf2unicode(remoteUserID).c_str()
    );
    _wnd_status_bar.SetText(buf, 1, 0);
}

void CRtcDemoV2::OnUserPublished(const std::string& remoteUserID, const qiniu::RemoteTrackList& trackList)
{
    wchar_t buff[1024] = { 0 };
    _snwprintf(buff, 1024, _T("远端用户: %s 发布了 %d 路媒体流。"), utf2unicode(remoteUserID).c_str(), trackList.size());
    _wnd_status_bar.SetText(buff, 1, 0);

    for (auto&& itor : trackList) {
        shared_ptr<TrackInfoUI> tiu(new TrackInfoUI(this, itor));
        if (tiu->render_wnd_ptr) {
            itor->SetRenderHwnd((void*)tiu->render_wnd_ptr->m_hWnd);
        }
        _remote_tracks_map.insert_or_assign(itor->GetTrackID(), tiu);
    }

    if (_rtc_client_ptr) {
        _rtc_client_ptr->Subscribe(static_cast<qiniu::RemoteTrackList>(trackList));
    }
    AdjustSubscribeLayouts();
}

void CRtcDemoV2::OnUserUnpublished(const std::string& remoteUserID, const qiniu::RemoteTrackList& trackList)
{
    wchar_t buff[1024] = { 0 };
    _snwprintf(buff, 1024, _T("远端用户: %s 取消发布了 %d 路媒体流。"), utf2unicode(remoteUserID).c_str(), trackList.size());
    _wnd_status_bar.SetText(buff, 1, 0);

    // 释放本地资源
    if (_remote_tracks_map.empty()) {
        return;
    }
    lock_guard<recursive_mutex> lck(_mutex);
    for (auto&& itor : trackList) {
        if (_remote_tracks_map.empty()) {
            break;
        }
        auto tmp_ptr = _remote_tracks_map.begin();
        while (tmp_ptr != _remote_tracks_map.end()) {
            if (tmp_ptr->second->track_info_ptr->GetTrackID().compare(itor->GetTrackID()) == 0) {
                _remote_tracks_map.erase(tmp_ptr);
                break;
            }
            ++tmp_ptr;
        }
    }
    AdjustSubscribeLayouts();
}

void CRtcDemoV2::OnSubscribed(
    const std::string& remoteUserID,
    const qiniu::RemoteAudioTrackList& remoteAudioTracks,
    const qiniu::RemoteVideoTrackList& remoteVideoTracks)
{
    QNRemoteUser remoteUser;
    RemoteUserList remoteList;
    if (_rtc_client_ptr) {
        remoteUser = _rtc_client_ptr->GetRemoteUsers(remoteUserID);
        remoteList = _rtc_client_ptr->GetRemoteUsers();
    }

    for (auto&& itor : remoteList) {
        for (auto && audio_itor : itor.remote_audio_tracks) {
            audio_itor->SetAudioFrameListener(this);
        }

        for (auto && video_itor : itor.remote_video_tracks) {
            video_itor->SetVideoFrameListener(this);
            video_itor->SetTrackInfoChangedListener(this);
        }
    }
}

void CRtcDemoV2::OnMessageReceived(const qiniu::CustomMessageList& message)
{
    for (auto&& itor : message) {
        if (itor.msg_text.compare("kickout") == 0) {
            PostMessage(WM_COMMAND, MAKEWPARAM(IDC_BUTTON_LOGIN, BN_CLICKED), NULL);
            wchar_t buff[1024] = { 0 };
            _snwprintf(
                buff,
                1024,
                _T("被 %s 用户踢出！"),
                utf2unicode(itor.msg_sendid).c_str()
            );
            _wnd_status_bar.SetText(buff, 1, 0);
            return;
        }
        _dlg_msg.OnReceiveMessage(itor.msg_sendid, utf8_to_string(itor.msg_text));
    }
}

void CRtcDemoV2::OnMuteStateChanged(bool isMuted, const std::string& remoteUserId, const qiniu::RemoteTrackList& trackList)
{
    for (auto itor : trackList) {
        wchar_t buff[1024] = { 0 };
        if (isMuted) {
            _snwprintf(buff, 1024, _T("%s  开启 mute"), utf2unicode(itor->GetTrackID()).c_str());
        } else {
            _snwprintf(buff, 1024, _T("%s  取消 mute"), utf2unicode(itor->GetTrackID()).c_str());
        }
        _wnd_status_bar.SetText(buff, 1, 0);
    }
}

void CRtcDemoV2::OnStarted(const std::string& streamID)
{
    wchar_t buff[1024] = { 0 };
    _snwprintf(buff, 1024, _T("%s 转推成功"), utf2unicode(streamID).c_str());
    _wnd_status_bar.SetText(buff, 1, 0);
}

void CRtcDemoV2::OnStopped(const std::string& streamID)
{
    wchar_t buff[1024] = { 0 };
    _snwprintf(buff, 1024, _T("%s 停止转推成功"), utf2unicode(streamID).c_str());
    _wnd_status_bar.SetText(buff, 1, 0);
}

void CRtcDemoV2::OnTranscodingTracksUpdated(const std::string& streamID)
{
    wchar_t buff[1024] = { 0 };
    _snwprintf(buff, 1024, _T("%s 更新合流布局成功"), utf2unicode(streamID).c_str());
    _wnd_status_bar.SetText(buff, 1, 0);
}

void CRtcDemoV2::OnError(const std::string& streamID, const QNLiveStreamingErrorInfo& errorInfo)
{
    wchar_t buff[1024] = { 0 };
    _snwprintf(buff, 1024, _T("转推任务失败，type: %d, error code:%d, error msg:%s"),
        errorInfo.code, errorInfo.code, utf2unicode(errorInfo.message).c_str());
    _wnd_status_bar.SetText(buff, 1, 0);
}

void CRtcDemoV2::OnBnClickedButtonLogin()
{
    CString btn_str;
    GetDlgItemText(IDC_BUTTON_LOGIN, btn_str);
    // 登录房间
    if (btn_str.CompareNoCase(_T("登录")) == 0) {
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
        _rtc_client_ptr->SetDnsServerUrl(unicode2utf(dns_ip.GetBuffer()));
        _rtc_client_ptr->Join(_room_token);
    } else {
        if (_stats_pDig) {
            _stats_pDig->DestroyWindow();
            delete _stats_pDig;
            _stats_pDig = nullptr;
        }
        _stop_stats_flag = true;
        _stop_video_external_flag = true;
        _stop_audio_external_flag = true;
        if (_fake_audio_thread.joinable()) {
            _fake_audio_thread.join();
        }
        if (_fake_video_thread.joinable()) {
            _fake_video_thread.join();
        }
        if (_stats_thread.joinable()) {
            _stats_thread.join();
        }
        Leave();
        SetDlgItemText(IDC_BUTTON_LOGIN, _T("登录"));
        _wnd_status_bar.SetText(_T("当前未登录房间！"), 1, 0);
        _user_list_ctrl.DeleteAllItems();
        ((CButton*)GetDlgItem(IDC_CHECK_MUTE_AUDIO))->SetCheck(0);
        ((CButton*)GetDlgItem(IDC_CHECK_MUTE_VIDEO))->SetCheck(0);
        GetDlgItem(IDC_BUTTON_FORWARD)->EnableWindow(FALSE);
        ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_STATS))->SetCheck(0);
        _remote_tracks_map.clear();
        KillTimer(UPDATE_TIME_DURATION_TIMER);
        Invalidate();
    }
}

void CRtcDemoV2::OnBnClickedButtonPreviewVideo()
{
    if (!_rtc_client_ptr) {
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
        _rtc_client_ptr->UnPreviewCamera(cur_dev_id);
        GetDlgItem(IDC_COMBO_CAMERA)->EnableWindow(TRUE);
        SetDlgItemText(IDC_BUTTON_PREVIEW_VIDEO, _T("预览"));
        GetDlgItem(IDC_STATIC_VIDEO_PREVIEW3)->Invalidate();
        return;
    }

    qiniu::QNCameraPreviewSetting camera_setting;
    camera_setting.device_name = unicode2utf(cur_dev_name.GetBuffer());
    camera_setting.device_id   = cur_dev_id;
    camera_setting.width       = 640;
    camera_setting.height      = 480;
    camera_setting.max_fps     = 15;
    camera_setting.render_hwnd = GetDlgItem(IDC_STATIC_VIDEO_PREVIEW3)->m_hWnd;

    if (0 != _rtc_client_ptr->PreviewCamera(camera_setting)) {
        MessageBox(_T("预览失败！"));
    } else {
        SetDlgItemText(IDC_BUTTON_PREVIEW_VIDEO, _T("取消预览"));
        GetDlgItem(IDC_COMBO_CAMERA)->EnableWindow(FALSE);
    }
}

void CRtcDemoV2::OnBnClickedBtnFlush()
{
   if (!_rtc_client_ptr) {
        return;
    }
    ((CComboBox *)GetDlgItem(IDC_COMBO_SCREEN))->ResetContent();
    int screen_count = _rtc_client_ptr->GetScreenWindowCount();
    for (int i(0); i < screen_count; ++i) {
        qiniu::QNScreenWindowInfo sw = _rtc_client_ptr->GetScreenWindowInfo(i);
        _screen_info_map[sw.id] = sw;
        ((CComboBox *)GetDlgItem(IDC_COMBO_SCREEN))->InsertString(-1, utf2unicode(sw.title).c_str());
    }
    ((CComboBox *)GetDlgItem(IDC_COMBO_SCREEN))->SetCurSel(0);
}

void CRtcDemoV2::OnBnClickedButtonPreviewScreen()
{
    if (!_rtc_client_ptr) {
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
        _rtc_client_ptr->UnPreviewScreen(source_id);
        GetDlgItem(IDC_COMBO_SCREEN)->EnableWindow(TRUE);
        GetDlgItem(IDC_BTN_FLUSH)->EnableWindow(TRUE);
        SetDlgItemText(IDC_BUTTON_PREVIEW_SCREEN, _T("预览屏幕"));
        GetDlgItem(IDC_STATIC_VIDEO_PREVIEW)->Invalidate();
        return;
    }
    QNScreenPreviewSetting setting = { source_id,GetDlgItem(IDC_STATIC_VIDEO_PREVIEW4)->m_hWnd };
    if (0 != _rtc_client_ptr->PreviewScreen(setting) ) {
        MessageBox(_T("预览失败！"));
    } else {
        SetDlgItemText(IDC_BUTTON_PREVIEW_SCREEN, _T("取消预览"));
        GetDlgItem(IDC_COMBO_SCREEN)->EnableWindow(FALSE);
        GetDlgItem(IDC_BTN_FLUSH)->EnableWindow(FALSE);
    }
}

void CRtcDemoV2::StartPublish()
{
    if (!_rtc_client_ptr) {
        return;
    }
    LocalTrackList track_list;
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
        auto camera_size = FindBestVideoSize(_camera_dev_map[video_dev_id].capability_vec);
        int width = std::get<0>(camera_size);
        int height = std::get<1>(camera_size);
        QNCameraVideoTrackConfig camera_info = { width,height,30,2000000,video_dev_id,
            CAMERA_TAG,GetDlgItem(IDC_STATIC_VIDEO_PREVIEW)->m_hWnd,_enable_simulcast };
        _camera_track_ptr = QNRTC::CreateCameraVideoTrack(camera_info);
        if (_camera_track_ptr) {
            track_list.emplace_back(_camera_track_ptr);
        }
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
            goto CHECK_CUSTOM_VIDEO;
        }
        QNScreenVideoTrackConfig screen_info{ 30,2000000,std::to_string(source_id),
            SCREENCASTS_TAG,GetDlgItem(IDC_STATIC_VIDEO_PREVIEW2)->m_hWnd };
        _screen_track_ptr = QNRTC::CreateScreenVideoTrack(screen_info);
        if (_screen_track_ptr) {
            if (1 == ((CButton*)GetDlgItem(IDC_CHECK_WINDOWS_GRAPHICS))->GetCheck()){
                _screen_track_ptr->EnableWindowGraphicsCapture(true);
            }
            track_list.emplace_back(_screen_track_ptr);
        }
    }
CHECK_CUSTOM_VIDEO:
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->GetCheck()) {
        // 这里需要设置的是外部视频源对应的分辨率和帧率
        QNCustomVideoTrackConfig video_custom_info = { 426,240,30,300000,EXTERNAL_VIDEO,GetDlgItem(IDC_STATIC_VIDEO_PREVIEW3)->m_hWnd };
        _custom_video_track_ptr = QNRTC::CreateCustomVideoTrack(video_custom_info);
        if (_custom_video_track_ptr) {
            track_list.emplace_back(_custom_video_track_ptr);
        }
    }
CHECK_MICROPHONE:
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_AUDIO))->GetCheck()) {
        ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_AUDIO_DATA))->SetCheck(0);
        QNMicrophoneAudioTrackConfig micro_info = { 32000,MICROPHONE_TAG };
        _microphone_audio_track_ptr = QNRTC::CreateMicrophoneAudioTrack(micro_info);
        if (_microphone_audio_track_ptr) {
            track_list.emplace_back(_microphone_audio_track_ptr);
        }
    }
CHECK_CUSTOM_AUDIO:
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_AUDIO_DATA))->GetCheck()) {
        ((CButton*)GetDlgItem(IDC_CHECK_AUDIO))->SetCheck(0);
        QNCustomAudioTrackConfig custom_audio_info = { 32000,EXTERNAL_AUDIO };
        _custom_audio_track_ptr = QNRTC::CreateCustomAudioTrack(custom_audio_info);
        if (_custom_audio_track_ptr) {
            track_list.emplace_back(_custom_audio_track_ptr);
        }
    }

    if (!track_list.empty()) {
        _rtc_client_ptr->Publish(track_list);
    }
}

void CRtcDemoV2::StopPublish()
{
    if (!_rtc_client_ptr) {
        return;
    }
    lock_guard<recursive_mutex> lck(_mutex);
    _rtc_client_ptr->UnPublish(_local_published_track_list);
}

void CRtcDemoV2::OnBnClickedCheckCamera()
{
    if (!_rtc_client_ptr) {
        return;
    }
    lock_guard<recursive_mutex> lck(_mutex);
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_CAMERA))->GetCheck()) {
        if (!_rtc_client_ptr->IsJoined()) {
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
        qiniu::LocalTrackList track_list;
        auto camera_size = FindBestVideoSize(_camera_dev_map[video_dev_id].capability_vec);
        int width = std::get<0>(camera_size);
        int height = std::get<1>(camera_size);
        if (!_camera_track_ptr) {
            QNCameraVideoTrackConfig camera_info = { width,height,30,2000000,video_dev_id,
                    CAMERA_TAG,GetDlgItem(IDC_STATIC_VIDEO_PREVIEW)->m_hWnd,_enable_simulcast };
            _camera_track_ptr = QNRTC::CreateCameraVideoTrack(camera_info);
            if (_camera_track_ptr) {
                track_list.emplace_back(_camera_track_ptr);
            }
        }
        if (!track_list.empty()) {
            int ret = _rtc_client_ptr->Publish(track_list);
            if (ret != 0) {
                ((CButton*)GetDlgItem(IDC_CHECK_CAMERA))->SetCheck(0);
                MessageBox(_T("发布失败，请检查设备状态是否可用？或者不要操作太快！"));
            } else {
                // 临时禁用，待发布结果通知后再激活控件，避免频繁操作
                //((CButton*)GetDlgItem(IDC_CHECK_CAMERA))->EnableWindow(FALSE);
                _wnd_status_bar.SetText(_T("发布摄像头"), 1, 0);
            }
        }
    } else {
        qiniu::LocalTrackList track_list;
        if (_local_published_track_list.empty()) {
            if (_rtc_client_ptr->IsJoined()) {
                ((CButton*)GetDlgItem(IDC_CHECK_CAMERA))->SetCheck(1);
            }
            return;
        }
        auto itor = _local_published_track_list.begin();
        while (itor != _local_published_track_list.end()) {
            if ((*itor)->GetSourceType() == tst_Camera) {
                track_list.emplace_back(*itor);
                if (Err_Pre_Publish_Not_Complete == _rtc_client_ptr->UnPublish(track_list)) {
                    ((CButton*)GetDlgItem(IDC_CHECK_CAMERA))->SetCheck(1);
                } else {
                    if (_camera_track_ptr) {
                        _local_published_track_list.erase(itor);
                        QNRTC::DestroyCameraVideoTrack(_camera_track_ptr);
                        _camera_track_ptr = nullptr;
                    }
                    break;
                }
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
   if (!_rtc_client_ptr) {
        return;
    }
    lock_guard<recursive_mutex> lck(_mutex);
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_SCREEN))->GetCheck()) {
        if (!_rtc_client_ptr->IsJoined()) {
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

        qiniu::LocalTrackList track_list;
        if (!_screen_track_ptr) {
            QNScreenVideoTrackConfig screen_info{ 30,2000000,std::to_string(source_id),
                    SCREENCASTS_TAG,GetDlgItem(IDC_STATIC_VIDEO_PREVIEW2)->m_hWnd };
            _screen_track_ptr = QNRTC::CreateScreenVideoTrack(screen_info);
            if (_screen_track_ptr) {
                if (1 == ((CButton*)GetDlgItem(IDC_CHECK_WINDOWS_GRAPHICS))->GetCheck()) {
                    _screen_track_ptr->EnableWindowGraphicsCapture(true);
                }
                track_list.emplace_back(_screen_track_ptr);
            }
        }

        if (!track_list.empty()) {
            auto ret = _rtc_client_ptr->Publish(track_list);
            if (ret != 0) {
                ((CButton*)GetDlgItem(IDC_CHECK_SCREEN))->SetCheck(0);
                MessageBox(_T("发布失败，请检查设备状态是否可用？或者不要操作太快！"));
            } else {
                // 临时禁用，待发布结果通知后再激活控件，避免频繁操作
                //((CButton*)GetDlgItem(IDC_CHECK_SCREEN))->EnableWindow(FALSE);
                _wnd_status_bar.SetText(_T("发布屏幕分享"), 1, 0);
            }
        }
    } else {
        qiniu::LocalTrackList track_list;
        if (_local_published_track_list.empty()) {
            if (_rtc_client_ptr->IsJoined()) {
                ((CButton*)GetDlgItem(IDC_CHECK_SCREEN))->SetCheck(1);
            }
            return;
        }
        auto itor = _local_published_track_list.begin();
        while (itor != _local_published_track_list.end())
        {
            if ((*itor)->GetSourceType() == tst_ScreenCasts) {
                track_list.emplace_back(*itor);
                if (Err_Pre_Publish_Not_Complete == _rtc_client_ptr->UnPublish(track_list)) {
                    ((CButton*)GetDlgItem(IDC_CHECK_SCREEN))->SetCheck(1);
                } else {
                    if (_screen_track_ptr) {
                        _local_published_track_list.erase(itor);
                        QNRTC::DestroyScreenVideoTrack(_screen_track_ptr);
                        _screen_track_ptr = nullptr;
                    }
                    break;
                }
            }
            ++itor;
        }
        GetDlgItem(IDC_STATIC_VIDEO_PREVIEW2)->Invalidate();
        _wnd_status_bar.SetText(_T("取消发布屏幕分享"), 1, 0);
    }
}

void CRtcDemoV2::OnBnClickedCheckAudio()
{
    if (!_rtc_client_ptr) {
        return;
    }
    ((CButton*)GetDlgItem(IDC_CHECK_MUTE_AUDIO))->SetCheck(0);

    lock_guard<recursive_mutex> lck(_mutex);
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_AUDIO))->GetCheck()) {
        if (!_rtc_client_ptr->IsJoined()) {
            return;
        }
        qiniu::LocalTrackList track_list;
        if (!_microphone_audio_track_ptr) {
            QNMicrophoneAudioTrackConfig micro_info = { 32000,MICROPHONE_TAG };
            _microphone_audio_track_ptr = QNRTC::CreateMicrophoneAudioTrack(micro_info);
            if (_microphone_audio_track_ptr) {
                track_list.emplace_back(_microphone_audio_track_ptr);
            } else {
                MessageBox(_T("请关闭音频外部导入再开启麦克风采集！"));
                ((CButton*)GetDlgItem(IDC_CHECK_AUDIO))->SetCheck(0);
                return;
            }
        }

        if (!track_list.empty()) {
            auto ret = _rtc_client_ptr->Publish(track_list);
            if (ret != 0) {
                ((CButton*)GetDlgItem(IDC_CHECK_SCREEN))->SetCheck(0);
                MessageBox(_T("发布失败，请检查设备状态是否可用？或者不要操作太快！"));
            } else {
                // 临时禁用，待发布结果通知后再激活控件，避免频繁操作
                //((CButton*)GetDlgItem(IDC_CHECK_SCREEN))->EnableWindow(FALSE);
                _wnd_status_bar.SetText(_T("发布麦克风"), 1, 0);
            }
        }
    } else {
        qiniu::LocalTrackList track_list;
        if (_local_published_track_list.empty()) {
            if (_rtc_client_ptr->IsJoined()) {
                ((CButton*)GetDlgItem(IDC_CHECK_AUDIO))->SetCheck(1);
            }
            return;
        }
        auto itor = _local_published_track_list.begin();
        while (itor != _local_published_track_list.end()) {
            if ((*itor)->GetSourceType()== tst_Microphone) {
                track_list.emplace_back(*itor);
                if (Err_Pre_Publish_Not_Complete == _rtc_client_ptr->UnPublish(track_list)) {
                    ((CButton*)GetDlgItem(IDC_CHECK_AUDIO))->SetCheck(1);
                } else {
                    if (_microphone_audio_track_ptr) {
                        _local_published_track_list.erase(itor);
                        QNRTC::DestroyMicrophoneAudioTrack(_microphone_audio_track_ptr);
                        _microphone_audio_track_ptr = nullptr;
                    }
                    break;
                }
            }
            ++itor;
        }
        _wnd_status_bar.SetText(_T("取消发布麦克风"), 1, 0);
    }
}

void CRtcDemoV2::OnBnClickedCheckImportRawData()
{
    if (!_rtc_client_ptr) {
        return;
    }
    lock_guard<recursive_mutex> lck(_mutex);
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->GetCheck()) {
        if (!_rtc_client_ptr->IsJoined()) {
            return;
        }
        LocalTrackList track_list;
        if (!_custom_video_track_ptr) {
            QNCustomVideoTrackConfig video_custom_info = { 426,240,30,300000,EXTERNAL_VIDEO,GetDlgItem(IDC_STATIC_VIDEO_PREVIEW3)->m_hWnd };
            _custom_video_track_ptr = QNRTC::CreateCustomVideoTrack(video_custom_info);
            if (_custom_video_track_ptr) {
                track_list.emplace_back(_custom_video_track_ptr);
            }
        }

        if (!track_list.empty()) {
            auto ret = _rtc_client_ptr->Publish(track_list);
            if (ret != 0) {
                ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->SetCheck(0);
                MessageBox(_T("发布失败，请检查设备状态是否可用？或者不要操作太快！"));
            }
            else {
                // 临时禁用，待发布结果通知后再激活控件，避免频繁操作
                //((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->EnableWindow(FALSE);
            }
        }
    } else {
        qiniu::LocalTrackList track_list;
        if (_local_published_track_list.empty()) {
            if (_rtc_client_ptr->IsJoined()) {
                ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->SetCheck(1);
            }
            return;
        }
        auto itor = _local_published_track_list.begin();
        while (itor != _local_published_track_list.end()) {
            if ((*itor)->GetSourceType()== tst_ExternalYUV) {
                track_list.emplace_back(*itor);
                if (Err_Pre_Publish_Not_Complete == _rtc_client_ptr->UnPublish(track_list)) {
                    ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->SetCheck(1);
                } else {
                    if (_custom_video_track_ptr) {
                        _stop_video_external_flag = true;
                        _local_published_track_list.erase(itor);
                        QNRTC::DestroyCustomVideoTrack(_custom_video_track_ptr);
                        _custom_video_track_ptr = nullptr;
                    }
                }
                break;
            }
            ++itor;
        }
        GetDlgItem(IDC_STATIC_VIDEO_PREVIEW3)->Invalidate();
        if (_fake_video_thread.joinable()) {
            _fake_video_thread.join();
        }
        _wnd_status_bar.SetText(_T("取消视频发布外部导入"), 1, 0);
    }
}

void CRtcDemoV2::OnBnClickedCheckImportRawAudioData()
{
    if (!_rtc_client_ptr) {
        return;
    }
    lock_guard<recursive_mutex> lck(_mutex);
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_AUDIO_DATA))->GetCheck()) {
        if (!_rtc_client_ptr->IsJoined()) {
            return;
        }

        LocalTrackList track_list;
        if (!_custom_audio_track_ptr) {
            QNCustomAudioTrackConfig custom_audio_info = { 32000,EXTERNAL_AUDIO };
            _custom_audio_track_ptr = QNRTC::CreateCustomAudioTrack(custom_audio_info);
            if (_custom_audio_track_ptr) {
                track_list.emplace_back(_custom_audio_track_ptr);
            } else {
                MessageBox(_T("请关闭麦克风采集再开启音频外部导入！"));
                ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_AUDIO_DATA))->SetCheck(0);
                return;
            }
        }

        if (!track_list.empty()) {
            auto ret = _rtc_client_ptr->Publish(track_list);
            if (ret != 0) {
                ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_AUDIO_DATA))->SetCheck(0);
                MessageBox(_T("发布失败，请检查设备状态是否可用？或者不要操作太快！"));
            }
            else {
                // 临时禁用，待发布结果通知后再激活控件，避免频繁操作
                //((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->EnableWindow(FALSE);
            }
        }
    } else {
        qiniu::LocalTrackList track_list;
        if (_local_published_track_list.empty()) {
            if (_rtc_client_ptr->IsJoined()) {
                ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_AUDIO_DATA))->SetCheck(1);
            }
            return;
        }
        auto itor = _local_published_track_list.begin();
        while (itor != _local_published_track_list.end()) {
            if ((*itor)->GetSourceType() == tst_ExternalPCM) {
                track_list.emplace_back(*itor);
                if (Err_Pre_Publish_Not_Complete == _rtc_client_ptr->UnPublish(track_list)) {
                    ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_RAW_AUDIO_DATA))->SetCheck(1);
                } else {
                    _stop_audio_external_flag = true;
                    if (_custom_audio_track_ptr) {
                        _local_published_track_list.erase(itor);
                        QNRTC::DestroyCustomAudioTrack(_custom_audio_track_ptr);
                        _custom_audio_track_ptr = nullptr;
                    }
                }
                break;
            }
            ++itor;
        }
        if (_fake_audio_thread.joinable()) {
            _fake_audio_thread.join();
        }
        _wnd_status_bar.SetText(_T("取消音频发布外部导入"), 1, 0);
    }
}

void CRtcDemoV2::ImportExternalVideoRawFrame()
{
    // 模拟导入视频数据,当前使用当前目录下指定的视频文件
    _stop_video_external_flag = true;
    if (_fake_video_thread.joinable()) {
        _fake_video_thread.join();
    }

    _fake_video_thread = thread([&] {
        FILE* fp = nullptr;
        fopen_s(&fp, "426x240.yuv", "rb");
        uint8_t *buf = (uint8_t*)malloc(426 * 240 * 3 / 2);
        if (!fp || !buf) {
            MessageBox(_T("foreman_320x240.yuv 文件打开失败，请确认此文件件是否存在!"));
            return;
        }
        size_t ret(0);
        _stop_video_external_flag = false;
        chrono::system_clock::time_point start_tp = chrono::system_clock::now();
        while (!_stop_video_external_flag && _custom_video_track_ptr) {
            ret = fread_s(buf, 426 * 240 * 3 / 2, 1, 426 * 240 * 3 / 2, fp);
            if (ret > 0) {
                _custom_video_track_ptr->PushVideoFrame(
                    buf,
                    426 * 240 * 3 / 2,
                    426,
                    240,
                    chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - start_tp).count(),
                    qiniu::QNVideoSourceType::kI420,
                    qiniu::kVideoRotation_0);
            } else {
                fseek(fp, 0, SEEK_SET);
                continue;
            }
            Sleep(1000 / 30);
        }
        free(buf);
        fclose(fp);
    });
}

void CRtcDemoV2::ImportExternalAudioRawFrame()
{
    // 模拟导入音频数据,当前使用当前目录下指定的音频文件
    if (_fake_audio_thread.joinable()) {
        _fake_audio_thread.join();
    }
    // 模拟导入音频数据
    _stop_audio_external_flag = true;
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
        _stop_audio_external_flag = false;
        chrono::system_clock::time_point start_tp = chrono::system_clock::now();
        int64_t audio_frame_count(0);
        while (!_stop_audio_external_flag &&  _custom_audio_track_ptr) {
            if (chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - start_tp).count() >= audio_frame_count * 20000) {
            } else {
                Sleep(10);
                continue;
            }

            ret = fread_s(buf, 441 * 2 * 4, 1, 441 * 2 * 4, fp);
            if (ret >= 441 * 8) {
                _custom_audio_track_ptr->PushAudioFrame(
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

void CRtcDemoV2::CreateCustomMergeJob(bool open)
{
    if (!_rtc_client_ptr) {
        return;
    }
    qiniu::QNTranscodingLiveStreamingConfig job_desc;
    job_desc.stream_id      = unicode2utf(_room_name.GetBuffer()) + "_merge";
    job_desc.publish_url    = _merge_config.publish_url;
    job_desc.width          = _merge_config.job_width;
    job_desc.height         = _merge_config.job_height;
    job_desc.fps            = _merge_config.job_fps;
    job_desc.bitrate        = _merge_config.job_bitrate;
    job_desc.min_bitrate    = _merge_config.job_min_bitrate;
    job_desc.max_bitrate    = _merge_config.job_max_bitrate;
    job_desc.stretch_mode = (qiniu::QNStretchMode)(_merge_config.job_stretch_mode);
    job_desc.is_hold_last_frame = _merge_config.hold_last_frame;

    qiniu::QNTranscodingLiveStreamingImage background;
    background.layer_url    = _merge_config.background_url;
    background.pos_x        = _merge_config.background_x;
    background.pos_y        = _merge_config.background_y;
    background.layer_width  = _merge_config.background_width;
    background.layer_height = _merge_config.background_height;
    job_desc.merge_background = background;

    qiniu::QNTranscodingLiveStreamingImage mark;
    mark.layer_url      = _merge_config.watermark_url;
    mark.pos_x          = _merge_config.watermark_x;
    mark.pos_y          = _merge_config.watermark_y;
    mark.layer_width    = _merge_config.watermark_width;
    mark.layer_height   = _merge_config.watermark_height;
    job_desc.merge_watermark.emplace_back(mark);

    _custom_merge_id = job_desc.stream_id;
    if (open) {
        _rtc_client_ptr->StartLiveStreaming(job_desc);
    } else {
        _rtc_client_ptr->StopLiveStreaming(job_desc);
    }

}

void CRtcDemoV2::AdjustMergeStreamLayouts()
{
    // 自定义合流，根据界面配置使用一个本地或者远端 track 
    if (!_rtc_client_ptr) {
        return;
    }
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_MERGE))->GetCheck()) {
        CreateCustomMergeJob(true);
        qiniu::QNTranscodeingTrackList add_tracks_list;
        list<string> remove_tracks_list;

        if (_microphone_audio_track_ptr) {
            qiniu::QNTranscodingLiveStreamingTrack merge_opt;
            merge_opt.track_id = _microphone_audio_track_ptr->GetTrackID();
            merge_opt.is_video = false;
            add_tracks_list.emplace_back(merge_opt);
        }

        if (_camera_track_ptr) {
            qiniu::QNTranscodingLiveStreamingTrack merge_opt;
            merge_opt.track_id = _camera_track_ptr->GetTrackID();
            merge_opt.is_video = true;
            if (_merge_config.merge_local_video) {
                // 这里只做演示，用户可根据自己需求设置相应 video track 合流时的填充模式
                if (1 == ((CButton*)GetDlgItem(IDC_CHECK_STRETCH_MODE))->GetCheck()) {
                    merge_opt.stretchMode = qiniu::SCALE_TO_FIT;
                }
                merge_opt.pos_x = _merge_config.local_video_x;
                merge_opt.pos_y = _merge_config.local_video_y;
                merge_opt.pos_z = 1;
                merge_opt.width = _merge_config.local_video_width;
                merge_opt.height = _merge_config.local_video_height;
                merge_opt.is_support_sei = true;
            }
            add_tracks_list.emplace_back(merge_opt);
        }

        RemoteUserList remoteList = _rtc_client_ptr->GetRemoteUsers();
        for (auto&& itor : remoteList) {
            for (auto && audio_itor : itor.remote_audio_tracks) {
                if (_merge_config.merge_remote_audio) {
                    qiniu::QNTranscodingLiveStreamingTrack merge_opt;
                    merge_opt.track_id = audio_itor->GetTrackID();
                    merge_opt.is_video = false;
                    add_tracks_list.emplace_back(merge_opt);
                    break;
                }
            }

            for (auto && video_itor : itor.remote_video_tracks) {
                qiniu::QNTranscodingLiveStreamingTrack merge_opt;
                merge_opt.track_id = video_itor->GetTrackID();
                merge_opt.is_video = true;
                if (_merge_config.merge_remote_video) {
                    // 这里只做演示，用户可根据自己需求设置相应 video track 合流时的填充模式
                    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_STRETCH_MODE))->GetCheck()) {
                        merge_opt.stretchMode = qiniu::SCALE_TO_FIT;
                    }
                    merge_opt.pos_x = _merge_config.remote_video_x;
                    merge_opt.pos_y = _merge_config.remote_video_y;
                    merge_opt.pos_z = 1;
                    merge_opt.width = _merge_config.remote_video_width;
                    merge_opt.height = _merge_config.remote_video_height;
                    merge_opt.is_support_sei = false;
                    add_tracks_list.emplace_back(merge_opt);
                    break;
                }
            }
        }
        _rtc_client_ptr->SetTranscodingLiveStreamingTracks(_custom_merge_id, add_tracks_list);
    } else {
        CreateCustomMergeJob(false);
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

void CRtcDemoV2::AdjustStatsLayouts()
{
    if (!_stats_pDig) {
        return;
    }

    RECT wnd_rc;
    GetWindowRect(&wnd_rc);
    TRACE("MainDialog rect x:%d, y:%d, right:%d, botton:%d\n",
        wnd_rc.left,
        wnd_rc.top,
        wnd_rc.right,
        wnd_rc.bottom
    );
    int main_wnd_height = wnd_rc.bottom - wnd_rc.top;
    const int wnd_width = 700;
    const int wnd_height = 400;
    int start_x = wnd_rc.right;
    int start_y = wnd_rc.top;
    _stats_pDig->MoveWindow(
        start_x,
        start_y,
        wnd_width,
        wnd_height
    );
}

void CRtcDemoV2::Leave()
{
    if (_rtc_client_ptr) {
        _rtc_client_ptr->Leave();
    }

    if (_camera_track_ptr) {
        QNRTC::DestroyCameraVideoTrack(_camera_track_ptr);
        _camera_track_ptr = nullptr;
    }

    if (_screen_track_ptr) {
        QNRTC::DestroyScreenVideoTrack(_screen_track_ptr);
        _screen_track_ptr = nullptr;
    }

    if (_custom_video_track_ptr) {
        QNRTC::DestroyCustomVideoTrack(_custom_video_track_ptr);
        _custom_video_track_ptr = nullptr;
    }

    if (_microphone_audio_track_ptr) {
        QNRTC::DestroyMicrophoneAudioTrack(_microphone_audio_track_ptr);
        _microphone_audio_track_ptr = nullptr;
    }

    if (_custom_audio_track_ptr) {
        QNRTC::DestroyCustomAudioTrack(_custom_audio_track_ptr);
        _custom_audio_track_ptr = nullptr;
    }

    _local_published_track_list.clear();
}

void CRtcDemoV2::OnBnClickedCheckDesktopAudio()
{
    if (!_microphone_audio_track_ptr && !_custom_audio_track_ptr) {
        return;
    }

    bool mixDesktopAudio = false;
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_DESKTOP_AUDIO))->GetCheck()) {
        mixDesktopAudio = true;
    }

    if (_microphone_audio_track_ptr) {
        _microphone_audio_track_ptr->MixDesktopAudio(mixDesktopAudio);
    } else if (_custom_audio_track_ptr) {
        _custom_audio_track_ptr->MixDesktopAudio(mixDesktopAudio);
    }
}

void CRtcDemoV2::OnBnClickedCheckMuteAudio()
{
    if (!_microphone_audio_track_ptr && !_custom_audio_track_ptr) {
        return;
    }
    // 静默本地音频 Track，一端仅有一路音频 Track
    bool muteAudio = false;
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_MUTE_AUDIO))->GetCheck()) {
        muteAudio = true;
    }

    if (_microphone_audio_track_ptr) {
        _microphone_audio_track_ptr->SetMuted(muteAudio);
    } else if (_custom_audio_track_ptr) {
        _custom_audio_track_ptr->SetMuted(muteAudio);
    }
}

void CRtcDemoV2::OnBnClickedCheckMuteVideo()
{
    bool muteVideo = false;
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_MUTE_VIDEO))->GetCheck()) {
        muteVideo = true;
    }
    // 静默或取消静默本地所有的视频 Track
    // 这里对所有的视频 Track 进行控制
    lock_guard<recursive_mutex> lck(_mutex);
    if (_camera_track_ptr) {
        _camera_track_ptr->SetMuted(muteVideo);
    }

    if (_screen_track_ptr) {
        _screen_track_ptr->SetMuted(muteVideo);
    }

    if (_custom_video_track_ptr) {
        _custom_video_track_ptr->SetMuted(muteVideo);
    }
}


void CRtcDemoV2::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    // TODO: Add your message handler code here and/or call default
    lock_guard<recursive_mutex> lck(_mutex);
    if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER_RECORD) {
        int pos = ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_RECORD))->GetPos();
        if (_microphone_audio_track_ptr) {
            _microphone_audio_track_ptr->SetVolume(pos / 100.0f);
        } else if (_custom_audio_track_ptr) {
            _custom_audio_track_ptr->SetVolume(pos / 100.0f);
        }
    } else if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER_PLAYOUT) {
        int pos = ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_PLAYOUT))->GetPos();
        RemoteUserList remoteList = _rtc_client_ptr->GetRemoteUsers();
        for (auto&& itor : remoteList) {
            for (auto && audio_itor : itor.remote_audio_tracks) {
                audio_itor->SetVolume(pos / 100.0f);
            }
        }
    }
    
    __super::OnHScroll(nSBCode, nPos, pScrollBar);
    __super::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CRtcDemoV2::OnCbnSelchangeComboMicrophone()
{
    // 输入音频设备配置
    if (!_rtc_client_ptr) {
        return;
    }
    int audio_recorder_device_index(-1);
    audio_recorder_device_index = ((CComboBox *)GetDlgItem(IDC_COMBO_MICROPHONE))->GetCurSel();
    audio_recorder_device_index = (audio_recorder_device_index == CB_ERR) ? 0 : audio_recorder_device_index;

    if (audio_recorder_device_index >= 0) {
        qiniu::QNAudioDeviceInfo dev_info = _microphone_dev_map[audio_recorder_device_index];

        qiniu::QNAudioDeviceSetting audio_setting;
        audio_setting.device_index = dev_info.device_index;
        audio_setting.device_type = qiniu::QNAudioDeviceSetting::wdt_DefaultDevice;
        if (_rtc_client_ptr) {
            _rtc_client_ptr->SetRecordingDevice(audio_setting);
        }
    }
}

void CRtcDemoV2::OnCbnSelchangeComboPlayout()
{
    // 播放音频设备配置
    if (!_rtc_client_ptr) {
        return;
    }
    int audio_playout_device_index(-1);
    audio_playout_device_index = ((CComboBox *)GetDlgItem(IDC_COMBO_PLAYOUT))->GetCurSel();
    audio_playout_device_index = (audio_playout_device_index == CB_ERR) ? 0 : audio_playout_device_index;

    if (audio_playout_device_index >= 0) {
        qiniu::QNAudioDeviceInfo dev_info = _playout_dev_map[audio_playout_device_index];
        
        qiniu::QNAudioDeviceSetting audio_setting;
        audio_setting.device_index = dev_info.device_index;
        audio_setting.device_type = qiniu::QNAudioDeviceSetting::wdt_DefaultDevice;
        _rtc_client_ptr->SetPlayoutDevice(audio_setting);
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
    if (_rtc_client_ptr) {
        LPTSTR lpMessage = (LPTSTR)lParam;
        _bstr_t bstr("");
        LPTSTR strTmp = lpMessage;
        bstr = strTmp;
        std::string strMsg = bstr;
        list<string> users_list;
        _rtc_client_ptr->SendCustomMessage(users_list, "", string_to_utf8(strMsg));
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
    qiniu::QNVideoRotation video_rotation = qiniu::QNVideoRotation::kVideoRotation_0;
    switch (rotation_sel)
    {
    case 0:
        video_rotation = qiniu::QNVideoRotation::kVideoRotation_0;
        break;
    case 1:
        video_rotation = qiniu::QNVideoRotation::kVideoRotation_90;
        break;
    case 2:
        video_rotation = qiniu::QNVideoRotation::kVideoRotation_180;
        break;
    case 3:
        video_rotation = qiniu::QNVideoRotation::kVideoRotation_270;
        break;
    default:
        break;
    }

    if (_camera_track_ptr) {
        _camera_track_ptr->SetVideoRotation(video_rotation);
    }

    if (_screen_track_ptr) {
        _screen_track_ptr->SetVideoRotation(video_rotation);
    }

    if (_custom_video_track_ptr) {
        _custom_video_track_ptr->SetVideoRotation(video_rotation);
    }
}


void CRtcDemoV2::OnCbnSelchangeComboRemoteRotate()
{
    // TODO: 在此添加控件通知处理程序代码
    int rotation_sel = ((CComboBox*)GetDlgItem(IDC_COMBO_REMOTE_ROTATION))->GetCurSel();
    qiniu::QNVideoRotation video_rotation = qiniu::QNVideoRotation::kVideoRotation_0;
    switch (rotation_sel)
    {
    case 0:
        video_rotation = qiniu::QNVideoRotation::kVideoRotation_0;
        break;
    case 1:
        video_rotation = qiniu::QNVideoRotation::kVideoRotation_90;
        break;
    case 2:
        video_rotation = qiniu::QNVideoRotation::kVideoRotation_180;
        break;
    case 3:
        video_rotation = qiniu::QNVideoRotation::kVideoRotation_270;
        break;
    default:
        break;
    }
    RemoteUserList remoteList = _rtc_client_ptr->GetRemoteUsers();
    for (auto&& itor : remoteList) {
        for (auto && video_itor : itor.remote_video_tracks) {
            video_itor->SetVideoRotation(video_rotation);
        }
    }
}

void CRtcDemoV2::OnCbnSelchangeComboSubscribeProfile()
{
    int profile_sel = ((CComboBox*)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))->GetCurSel();
    qiniu::QNTrackProfile video_profile = qiniu::QNTrackProfile::HIGH;
    switch (profile_sel)
    {
    case 0:
        video_profile = qiniu::QNTrackProfile::HIGH;
        break;
    case 1:
        video_profile = qiniu::QNTrackProfile::MEDIUM;
        break;
    case 2:
        video_profile = qiniu::QNTrackProfile::LOW;
        break;
    default:
        break;
    }

    // 这里只是演示时是将所有订阅流一起切成设置的 profile，用户实际根据自己需求，更改相应track的profile，
    // 想要切换成哪种 profile，只要将对应的 mChooseToSub 设置为 true。
    lock_guard<recursive_mutex> lck(_mutex);
    RemoteUserList remoteList;
    if (_rtc_client_ptr) {
        remoteList = _rtc_client_ptr->GetRemoteUsers();
    }

    for (auto&& itor : remoteList) {
        for (auto && video_itor : itor.remote_video_tracks) {
            if (video_itor->IsMultiProfileEnabled()) {
                video_itor->SetProfile(video_profile);
            }
        }
    }
}

void CRtcDemoV2::OnBnClickedButtonSimulcast()
{
    CString btn_str;
    GetDlgItemText(IDC_BUTTON_SIMULCAST, btn_str);
    if (btn_str.CompareNoCase(_T("开启多流")) == 0) {
        SetDlgItemText(IDC_BUTTON_SIMULCAST, _T("关闭多流"));
        _enable_simulcast = true;
    } else {
        SetDlgItemText(IDC_BUTTON_SIMULCAST, _T("开启多流"));
        _enable_simulcast = false;
    }
}


void CRtcDemoV2::OnBnClickedButtonForward()
{
    CString btn_str;
    _custom_forward_id = "window-forward";
    qiniu::QNDirectLiveStreamingConfig forwardInfo;
    forwardInfo.is_internal = true;
    forwardInfo.stream_id = _custom_forward_id;
    forwardInfo.publish_url = "rtmp://pili-publish.qnsdk.com/sdk-live/window-forward";
    GetDlgItemText(IDC_BUTTON_FORWARD, btn_str);
    if (_camera_track_ptr) {
        forwardInfo.local_video_track = _camera_track_ptr->GetTrackID();
    }
    if (_microphone_audio_track_ptr) {
        forwardInfo.local_audio_track = _microphone_audio_track_ptr->GetTrackID();
    }
    if (btn_str.CompareNoCase(_T("单流转推")) == 0) {
        SetDlgItemText(IDC_BUTTON_FORWARD, _T("停止单流转推"));
        if (_rtc_client_ptr) {
            _rtc_client_ptr->StartLiveStreaming(forwardInfo);
        }
    } else {
        SetDlgItemText(IDC_BUTTON_FORWARD, _T("单流转推"));
        if (_rtc_client_ptr) {
            _rtc_client_ptr->StopLiveStreaming(forwardInfo);
            _contain_forward_flag = false;
        }
    }
}

void CRtcDemoV2::OnBnClickedCheckCameraImage()
{
    if (!_camera_track_ptr) {
        return;
    }

    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_CAMERA_IMAGE))->GetCheck()) {
        _camera_track_ptr->PushImage("pause_publish.jpeg");
    }  else {
        _camera_track_ptr->PushImage("");
    }
}

void CRtcDemoV2::OnBnClickedCheckCameraMirror()
{
    if (!_camera_track_ptr) {
        return;
    }

    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_CAMERA_MIRROR))->GetCheck()) {
        _camera_track_ptr->SetCaptureMirror(true);
    } else {
        _camera_track_ptr->SetCaptureMirror(false);
    }
}

void CRtcDemoV2::OnBnClickedBtnSei()
{
    CString btn_str;
    GetDlgItemText(IDC_BTN_SEI, btn_str);
    string uuid = "\x14\x16\x17\x18\x14\x16\x17\x28\x14\x16\x17\x38\x14\x16\x17\x58";
    if (btn_str.CompareNoCase(_T("开启SEI")) == 0) {
        SetDlgItemText(IDC_BTN_SEI, _T("关闭SEI"));
        if (_camera_track_ptr) {
            _camera_track_ptr->SendSEI(string_to_utf8("七牛SEI" + std::to_string(rand())), -1, uuid);
        }
        if (_screen_track_ptr) {
            _screen_track_ptr->SendSEI(string_to_utf8("七牛SEI" + std::to_string(rand())), -1, uuid);
        }
        if (_custom_video_track_ptr) {
            _custom_video_track_ptr->SendSEI(string_to_utf8("七牛SEI" + std::to_string(rand())), -1, uuid);
        }
    } else {
        SetDlgItemText(IDC_BTN_SEI, _T("开启SEI"));
        if (_camera_track_ptr) {
            _camera_track_ptr->SendSEI(string_to_utf8("七牛SEI" + std::to_string(rand())), 0, uuid);
        }
        if (_screen_track_ptr) {
            _screen_track_ptr->SendSEI(string_to_utf8("七牛SEI" + std::to_string(rand())), 0, uuid);
        }
        if (_custom_video_track_ptr) {
            _custom_video_track_ptr->SendSEI(string_to_utf8("七牛SEI" + std::to_string(rand())), 0, uuid);
        }
    }

}




void CRtcDemoV2::OnBnClickedCheckLocalMirror()
{
    bool mirror_flag = false;
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_LOCAL_MIRROR))->GetCheck()) {
        mirror_flag = true;
    } else {
        mirror_flag = false;
    }

    if (_camera_track_ptr) {
        _camera_track_ptr->SetRenderMirror(mirror_flag);
    }

    if (_screen_track_ptr) {
        _screen_track_ptr->SetRenderMirror(mirror_flag);
    }

    if (_custom_video_track_ptr) {
        _custom_video_track_ptr->SetRenderMirror(mirror_flag);
    }
}


void CRtcDemoV2::OnBnClickedCheckRemoteMirror()
{
    bool mirror_flag = false;
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_REMOTE_MIRROR))->GetCheck()) {
        mirror_flag = true;
    } else {
        mirror_flag = false;
    }
    RemoteUserList remoteList = _rtc_client_ptr->GetRemoteUsers();
    for (auto&& itor : remoteList) {
        for (auto && video_itor : itor.remote_video_tracks) {
            video_itor->SetRenderMirror(mirror_flag);
        }
    }
}

void CRtcDemoV2::OnCbnSelchangeComboLocalStretchMode()
{
    qiniu::QNStretchMode stretch_mode = ASPECT_FIT;
    int clip_crop_source = ((CComboBox*)GetDlgItem(IDC_COMBO_LOCAL_STRETCH_MODE))->GetCurSel();
    if (clip_crop_source == 0) {
        stretch_mode = ASPECT_FIT;
    } else if(clip_crop_source == 1){
        stretch_mode = ASPECT_FILL;
    } else {
        stretch_mode = SCALE_TO_FIT;
    }

    if (_camera_track_ptr) {
        _camera_track_ptr->SetStretchMode(stretch_mode);
    }

    if (_screen_track_ptr) {
        _screen_track_ptr->SetStretchMode(stretch_mode);
    }

    if (_custom_video_track_ptr) {
        _custom_video_track_ptr->SetStretchMode(stretch_mode);
    }
}

void CRtcDemoV2::OnCbnSelchangeComboRemoteStretchMode()
{
    qiniu::QNStretchMode stretch_mode = ASPECT_FIT;
    int clip_crop_source = ((CComboBox*)GetDlgItem(IDC_COMBO_REMOTE_STRETCH_MODE))->GetCurSel();
    if (clip_crop_source == 0) {
        stretch_mode = ASPECT_FIT;
    } else if (clip_crop_source == 1) {
        stretch_mode = ASPECT_FILL;
    } else {
        stretch_mode = SCALE_TO_FIT;
    }

    RemoteUserList remoteList = _rtc_client_ptr->GetRemoteUsers();
    for (auto&& itor : remoteList) {
        for (auto && video_itor : itor.remote_video_tracks) {
            video_itor->SetStretchMode(stretch_mode);
        }
    }
}


void CRtcDemoV2::OnBnClickedCheckClip()
{
    int clip_crop_source = ((CComboBox*)GetDlgItem(IDC_COMBO_CLIP_CROP))->GetCurSel();
    qiniu::QNTrackSourceType  src_capturer_source;
    if (clip_crop_source == 0) {
        src_capturer_source = qiniu::tst_Camera;
    } else {
        src_capturer_source = qiniu::tst_ExternalYUV;
    }
    int cropX = GetDlgItemInt(IDC_EDIT_CLIP_X, NULL, 0);
    int cropY = GetDlgItemInt(IDC_EDIT_CLIP_Y, NULL, 0);
    int width = GetDlgItemInt(IDC_EDIT_CLIP_WIDTH, NULL, 0);
    int height = GetDlgItemInt(IDC_EDIT_CLIP_HEIGHT, NULL, 0);
    bool flag = false;
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_CLIP))->GetCheck()) {
        flag = true;
    } else {
        flag = false;
        cropX = 0;
        cropY = 0;
        width = 0;
        height = 0;
    }

    if (src_capturer_source == qiniu::tst_Camera) {
        if (_camera_track_ptr) {
            _camera_track_ptr->CropAndScaleRawPicture(qiniu::p_Crop, flag, cropX, cropY, width, height);
        }
    }
    else if (src_capturer_source == qiniu::tst_ExternalYUV) {
        if (_custom_video_track_ptr) {
            _custom_video_track_ptr->CropAndScaleRawPicture(qiniu::p_Crop, flag, cropX, cropY, width, height);
        }
    }
}


void CRtcDemoV2::OnBnClickedCheckScale()
{
    int clip_crop_source = ((CComboBox*)GetDlgItem(IDC_COMBO_CLIP_CROP))->GetCurSel();
    qiniu::QNTrackSourceType  src_capturer_source;
    if (clip_crop_source == 0) {
        src_capturer_source = qiniu::tst_Camera;
    } else {
        src_capturer_source = qiniu::tst_ExternalYUV;
    }


    int width = GetDlgItemInt(IDC_EDIT_SCALE_WIDTH, NULL, 0);
    int height = GetDlgItemInt(IDC_EDIT_SCALE_HEIGHT, NULL, 0);
    bool flag = false;
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_SCALE))->GetCheck()) {
        flag = true;
    } else {
        flag = false;
        width = 0;
        height = 0;
    }

    if (src_capturer_source == qiniu::tst_Camera) {
        if (_camera_track_ptr) {
            _camera_track_ptr->CropAndScaleRawPicture(qiniu::p_Scale, flag, 0, 0, width, height);
        }
    }
    else if (src_capturer_source == qiniu::tst_ExternalYUV) {
        if (_custom_video_track_ptr) {
            _custom_video_track_ptr->CropAndScaleRawPicture(qiniu::p_Scale, flag, 0, 0, width, height);
        }
    }
}


void CRtcDemoV2::OnBnClickedCheckMerge()
{
    AdjustMergeStreamLayouts();
}


void CRtcDemoV2::OnBnClickedCheckImportStats()
{
    if (!_rtc_client_ptr) {
        return;
    }
    _stop_stats_flag = true;
    if (_stats_thread.joinable()) {
        _stats_thread.join();
    }
    

    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_IMPORT_STATS))->GetCheck()) {
        if (!_stats_pDig ){
            _stats_pDig = new  StatsDialog;
            _stats_pDig->Create(IDD_DIALOG_STATS);
            AdjustStatsLayouts();
            _stats_pDig->ShowWindow(SW_SHOWNORMAL);
        }
        _stop_stats_flag = false;
        _stats_thread = thread([&] {
            while (!_stop_stats_flag) {
                QNLocalAudioTrackStats local_audio_stats = _rtc_client_ptr->GetLocalAudioTrackStats();
                if (local_audio_stats.uplinkBitrate != 0) {
                    wchar_t dest_buf[1024] = { 0 };
                    _snwprintf(dest_buf,
                        sizeof(dest_buf),
                        _T("[发布]TrackId:%s, 码率:%d kbps ,丢包率:%d, rtt:%d"),
                        utf2unicode(local_audio_stats.trackId).c_str(),
                        local_audio_stats.uplinkBitrate,
                        local_audio_stats.uplinkLostRate,
                        local_audio_stats.uplinkRTT
                    );
                    if (_stats_pDig) {
                        _stats_pDig->OnReceiveStatsText(unicode2utf(dest_buf));
                    }
                }

                QNLocalVideoStatsList local_video_statslist = _rtc_client_ptr->GetLocalVideoTrackStats();
                for (auto itor : local_video_statslist) {
                    string profile = "";
                    if (itor.profile == HIGH) {
                        profile = "high";
                    } else if (itor.profile == MEDIUM) {
                        profile = "medium";
                    } else if (itor.profile == LOW) {
                        profile = "low";
                    }
                    if (itor.uplinkBitrate != 0) {
                        wchar_t dest_buf[1024] = { 0 };
                        _snwprintf(dest_buf,
                            sizeof(dest_buf),
                            _T("[发布]TrackId:%s, 帧率:%d ,码率:%d kbps, 丢包率:%d, rtt:%d, profile:%s, 分辨率:%d*%d"),
                            utf2unicode(itor.trackId).c_str(),
                            itor.uplinkFrameRate,
                            itor.uplinkBitrate,
                            itor.uplinkLostRate,
                            itor.uplinkRTT,
                            utf2unicode(profile).c_str(),
                            itor.width,
                            itor.height
                        );
                        if (_stats_pDig) {
                            _stats_pDig->OnReceiveStatsText(unicode2utf(dest_buf));
                        }
                    }
                }
                QNRemoteAudioTrackStats remote_audio_stats = _rtc_client_ptr->GetRemoteAudioTrackStats();
                if (remote_audio_stats.downlinkBitrate != 0) {
                    wchar_t dest_buf[1024] = { 0 };
                    _snwprintf(dest_buf,
                        sizeof(dest_buf),
                        _T("[订阅]TrackId:%s, 码率:%d kbps ,丢包率:%d"),
                        utf2unicode(remote_audio_stats.trackId).c_str(),
                        remote_audio_stats.downlinkBitrate,
                        remote_audio_stats.downlinkLostRate
                    );
                    if (_stats_pDig) {
                        _stats_pDig->OnReceiveStatsText(unicode2utf(dest_buf));
                    }
                }

                QNRemoteVideoStatsList remote_video_statslist = _rtc_client_ptr->GetRemoteVideoTrackStats();
                for (auto itor : remote_video_statslist) {
                    string profile = "";
                    if (itor.profile == HIGH) {
                        profile = "high";
                    } else if (itor.profile == MEDIUM) {
                        profile = "medium";
                    } else if (itor.profile == LOW) {
                        profile = "low";
                    }
                    if (itor.downlinkBitrate != 0) {
                        wchar_t dest_buf[1024] = { 0 };
                        _snwprintf(dest_buf,
                            sizeof(dest_buf),
                            _T("[订阅]TrackId:%s, 帧率:%d ,码率:%d kbps, 丢包率:%d, profile:%s, 分辨率:%d*%d"),
                            utf2unicode(itor.trackId).c_str(),
                            itor.downlinkFrameRate,
                            itor.downlinkBitrate,
                            itor.downlinkLostRate,
                            utf2unicode(profile).c_str(),
                            itor.width,
                            itor.height
                        );
                        if (_stats_pDig) {
                            _stats_pDig->OnReceiveStatsText(unicode2utf(dest_buf));
                        }
                    }
                }
                Sleep(1000);
            }
        });
    } else {
        _stop_stats_flag = true;
        if (_stats_pDig) {
            _stats_pDig->DestroyWindow();
            delete _stats_pDig;
            _stats_pDig = nullptr;
        }
    }
}

void CRtcDemoV2::OnBnClickedCheckHardEncoder()
{
    if (1 == ((CButton*)GetDlgItem(IDC_CHECK_HARD_ENCODER))->GetCheck()) {
        qiniu::QNEncoderCapability encode_cap_info = _rtc_client_ptr->GetSupportEncoder();
        qiniu::QNVideoEncodeType hw_type = qiniu::kEncodeNvenc;
        for (auto itor : encode_cap_info.capability_vec) {
            if (itor == hw_type) {
                _rtc_client_ptr->SetVideoEncoder(hw_type);
            }
        }
    } else {
        _rtc_client_ptr->SetVideoEncoder(kEncodedefault);
    }
}

void CRtcDemoV2::OnBnClickedBtnKickout()
{
    int index = _user_list_ctrl.GetSelectionMark();
    if (index == -1) {
        MessageBox(_T("请选中要踢出的用户！"));
        return;
    }
    // 所选择的用户当前没有发布媒体流
    CString user_id = _user_list_ctrl.GetItemText(index, 0);
    if (_user_id.Compare(user_id) == 0) {
        MessageBox(_T("不允许踢出自己！"));
        return;
    }
    if (_rtc_client_ptr) {
        _rtc_client_ptr->SendCustomMessage({unicode2utf(user_id.GetBuffer()).c_str()}, "", string_to_utf8("kickout"));
    }
}
