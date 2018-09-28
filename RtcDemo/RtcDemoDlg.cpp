// RtcDemoDlg.cpp : implementation file
//
#include "stdafx.h"
#include "RtcDemo.h"
#include "RtcDemoDlg.h"
#include <future>
#include <fstream>
#include <chrono>
#include "afxdialogex.h"
#include "charactor_convert.h"
#include "qn_rtc_engine.h"
#include "curl.h"
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#pragma comment(lib, "QNRtcStreamingD.lib")
#else
#pragma comment(lib, "QNRtcStreaming.lib")
#endif // _DEBUG
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "Version.lib")

#define CUSTOM_RESOURCE_ID         10000
#define CALLBACK_UI_TIMER_ID       1
#define UPDATE_TIME_TIMER_ID       2     // 更新连麦时间定时器
#define DISPLAY_NAME_HEIGHT        18    // pix
#define UPDATE_AUDIO_LEVEL         20
#define KICKOUT_USER_RESULT        30
#define UPDATE_STATISTICS_TIMER    31
#define DEVICE_STATE_CHANGE  32

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();

    // Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

                                                        // Implementation
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CRtcDemoDlg dialog

CRtcDemoDlg::CRtcDemoDlg(CWnd* pParent /*=NULL*/)
    : CDialogEx(IDD_RTCDEMO_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRtcDemoDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_PLAYER, _user_list_ctrl);
    DDX_Control(pDX, IDC_RICHEDIT_MSG, _msg_rich_edit_ctrl);
    DDX_Control(pDX, IDC_PROGRESS_LOCAL_VOLUE, _local_volume_progress);
}

BEGIN_MESSAGE_MAP(CRtcDemoDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON_LOGIN, &CRtcDemoDlg::OnBnClickedButtonJoin)
    ON_BN_CLICKED(IDC_BUTTON_PUBLISH, &CRtcDemoDlg::OnBnClickedButtonPublish)
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_WM_CREATE()
    ON_CBN_SELCHANGE(IDC_COMBO_CAMERA, &CRtcDemoDlg::OnCbnSelchangeComboCamera)
    ON_CBN_SELCHANGE(IDC_COMBO_MICROPHONE, &CRtcDemoDlg::OnCbnSelchangeComboMicrophone)
    ON_BN_CLICKED(IDC_BUTTON_PREVIEW_VIDEO, &CRtcDemoDlg::OnBnClickedButtonPreview)
    ON_BN_CLICKED(IDC_BTN_KICKOUT, &CRtcDemoDlg::OnBnClickedBtnKickout)
    ON_WM_HSCROLL()
    ON_BN_CLICKED(IDC_CHECK_MUTE_AUDIO, &CRtcDemoDlg::OnBnClickedCheckMuteAudio)
    ON_BN_CLICKED(IDC_CHECK_MUTE_VIDEO, &CRtcDemoDlg::OnBnClickedCheckMuteVideo)
    ON_CBN_SELCHANGE(IDC_COMBO_PLAYOUT, &CRtcDemoDlg::OnCbnSelchangeComboPlayout)
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_BTN_FLUSH, &CRtcDemoDlg::OnBnClickedBtnFlush)
    ON_BN_CLICKED(IDC_CHECK_ACTIVE_SCREEN, &CRtcDemoDlg::OnBnClickedCheckActiveScreen)
    ON_CBN_SELCHANGE(IDC_COMBO_SCREEN, &CRtcDemoDlg::OnCbnSelchangeComboScreen)
    ON_BN_CLICKED(IDC_CHECK_DX, &CRtcDemoDlg::OnBnClickedCheckDx)
    ON_BN_CLICKED(IDC_BUTTON_PREVIEW_SCREEN, &CRtcDemoDlg::OnBnClickedButtonPreviewScreen)
    ON_BN_CLICKED(IDC_CHECK_IMPORT_RAW_DATA, &CRtcDemoDlg::OnBnClickedCheckImportRawData)
    ON_BN_CLICKED(IDC_CHECK_DESKTOP_AUDIO, &CRtcDemoDlg::OnBnClickedCheckDesktopAudio)
END_MESSAGE_MAP()

// CRtcDemoDlg message handlers

static string GetAppVersion()
{
    DWORD dwInfoSize  = 0;
    char exePath[MAX_PATH];
    char ver_buf[128] = { 0 };
    int ver_buf_len   = 0;
    memset(exePath, 0, sizeof(exePath));

    // 得到程序的自身路径
    GetModuleFileNameA(NULL, exePath, sizeof(exePath) / sizeof(char));

    // 判断是否能获取版本号
    dwInfoSize = GetFileVersionInfoSizeA(exePath, NULL);

    if (dwInfoSize == 0) {
        return "";
    } else {
        BYTE* pData = new BYTE[dwInfoSize];
        // 获取版本信息
        if (!GetFileVersionInfoA(exePath, NULL, dwInfoSize, pData)) {
            return "";
        } else {
            // 查询版本信息中的具体键值
            LPVOID lpBuffer;
            UINT uLength;
            if (!::VerQueryValue((LPCVOID)pData, _T("\\"), &lpBuffer, &uLength)) {
            } else {
                DWORD dwVerMS;
                DWORD dwVerLS;
                dwVerMS = ((VS_FIXEDFILEINFO*)lpBuffer)->dwProductVersionMS;
                dwVerLS = ((VS_FIXEDFILEINFO*)lpBuffer)->dwProductVersionLS;
                ver_buf_len = snprintf(ver_buf,
                    sizeof(ver_buf),
                    "Version : %d.%d.%d.%d    BuildTime : %s %s",
                    (dwVerMS >> 16),
                    (dwVerMS & 0xFFFF),
                    (dwVerLS >> 16),
                    (dwVerLS & 0xFFFF),
                    __DATE__,
                    __TIME__
                    );
            }
        }
        delete pData;
    }
    return string(ver_buf, ver_buf_len);
}

BOOL CRtcDemoDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL) {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty()) {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);     // Set big icon
    SetIcon(m_hIcon, FALSE);    // Set small icon
    
    // 默认选中 DirectX，即屏幕抓取时默认使用 DX
    ((CButton *)GetDlgItem(IDC_CHECK_DX))->SetCheck(1);

    //IAudioTest::CaptureTest();
    //IAudioTest::WaveInTest();

    // Init UI
    InitDemoUI();

    // 读取配置记录，并初始化 UI
    ReadConfigFile();

    qiniu::QNRTCEngine::Init();

    // 设置日志级别和文件名，否则没有日志输出
    qiniu::QNRTCEngine::SetLogParams(qiniu::LOG_INFO, "rtc-log", "rtc.log");

    _rtc_room_interface = QNRTCRoom::ObtainRoomInterface();
    if (!_rtc_room_interface) {
        return FALSE;
    }
    _rtc_room_interface->SetRoomListener(this);

    // 视频功能接口
    _rtc_video_interface = _rtc_room_interface->ObtainVideoInterface();
    if (!_rtc_video_interface) {
        return FALSE;
    }
    _rtc_video_interface->SetVideoListener(this);

    // 音频功能接口
    _rtc_audio_interface = _rtc_room_interface->ObtainAudioInterface();
    if (!_rtc_audio_interface) {
        return FALSE;
    }
    _rtc_audio_interface->SetAudioListener(this);

    // 设置连麦质量回调间隔时间，单位：秒
    _rtc_room_interface->EnableStatisticCallback(5);

    // 初始化用户列表控件
    _user_list_ctrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    _user_list_ctrl.InsertColumn(0, _T("用户 ID"), LVCFMT_LEFT, 100, 0);    //设置列
    _user_list_ctrl.InsertColumn(1, _T("用户发布流状态"), LVCFMT_LEFT, 350, 1);
    ((CButton*)GetDlgItem(IDC_CHECK_ENABLE_AUDIO))->SetCheck(1);
    ((CButton*)GetDlgItem(IDC_CHECK_ENABLE_VIDEO))->SetCheck(1);

    // 初始化视频采集设备 combobox
    int camera_count = _rtc_video_interface->GetCameraCount();
    for (int i(0); i < camera_count; ++i) {
        CameraDeviceInfo ci = _rtc_video_interface->GetCameraInfo(i);
        _camera_dev_map[ci.device_id] = ci;
        ((CComboBox *)GetDlgItem(IDC_COMBO_CAMERA))->InsertString(-1, utf2unicode(ci.device_name).c_str());
    }
    ((CComboBox *)GetDlgItem(IDC_COMBO_CAMERA))->SetCurSel(0);

    // 初始化音频采集设备列表
    int audio_rec_count = _rtc_audio_interface->GetAudioDeviceCount(AudioDeviceInfo::adt_record);
    for (int i(0); i < audio_rec_count; ++i) {
        AudioDeviceInfo audio_info;
        if (_rtc_audio_interface->GetAudioDeviceInfo(AudioDeviceInfo::adt_record, i, audio_info) == 0) {
            ((CComboBox *)GetDlgItem(IDC_COMBO_MICROPHONE))->InsertString(
                -1,
                utf2unicode(audio_info.device_name).c_str()
            );
        }
    }
    ((CComboBox *)GetDlgItem(IDC_COMBO_MICROPHONE))->SetCurSel(0);

    // 初始化音频播放设备列表
    int audio_play_count = _rtc_audio_interface->GetAudioDeviceCount(AudioDeviceInfo::adt_playout);
    for (int i(0); i < audio_play_count; ++i) {
        AudioDeviceInfo audio_info;
        if (_rtc_audio_interface->GetAudioDeviceInfo(AudioDeviceInfo::adt_playout, i, audio_info) == 0) {
            ((CComboBox *)GetDlgItem(IDC_COMBO_PLAYOUT))->InsertString(
                -1,
                utf2unicode(audio_info.device_name).c_str()
            );
        }
    }
    ((CComboBox *)GetDlgItem(IDC_COMBO_PLAYOUT))->SetCurSel(0);

    // 初始化音量控制条配置
    ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_RECORD))->SetRange(0, 255);
    ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_RECORD))->SetPos(255);
    ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_PLAYOUT))->SetRange(0, 255);
    ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_PLAYOUT))->SetPos(255);

    // 初始化屏幕录制窗口列表
    OnBnClickedBtnFlush();

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CRtcDemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    } else {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRtcDemoDlg::OnPaint()
{
    if (IsIconic()) {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    } else {
        CDialogEx::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRtcDemoDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CRtcDemoDlg::OnStateChanged(RoomState status_)
{
    if (status_ == qiniu::rs_reconnecting) {
        lock_guard<recursive_mutex> lock_(_mutex);
        _call_function_vec.emplace_back([=]() {
            // 网络断开，重连中... 用户可什么都不做，SDK 内部会不断的尝试重连
            _wndStatusBar.SetText(_T("网络断开，重连中。。。"), 1, 0);
        });
        SetTimer(CALLBACK_UI_TIMER_ID, 1, nullptr);
    }
}

void CRtcDemoDlg::OnJoinResult(int error_code_, const std::string& error_str_,
    const UserDataInfoVec& user_data_vec_)
{
    TRACE("%s", __FUNCTION__);
    lock_guard<recursive_mutex> lock_(_mutex);
    _call_function_vec.emplace_back([=]() {
        // 取消原来所有的订阅, 并释放资源
        for (auto&& itor : _user_stream_map) {
            if (itor.second.is_subscribed) {
                _rtc_room_interface->UnSubscribe(itor.first);
            }
        }
        _user_stream_map.clear();

        // 适配界面
        GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(TRUE);
        if (error_code_ != 0 || user_data_vec_.empty()) {
            CString msg_str;
            msg_str.Format(_T("登录房间失败，错误码：%d，错误信息：%s"), error_code_, utf2unicode(error_str_).c_str());
            _wndStatusBar.SetText(msg_str, 1, 0);
            std::thread([&, msg_str]() {
                MessageBox(msg_str, _T("登录失败："));
            }).detach();
            GetDlgItem(IDC_BUTTON_LOGIN)->SetWindowTextW(_T("登录"));
            return;
        }
        if (user_data_vec_.empty()) {
            _wndStatusBar.SetText(_T("服务器回复异常，没有任何玩家信息!"), 1, 0);
            std::thread([&]() {
                MessageBox(_T("服务器回复异常，没有任何玩家信息!"), _T("登录失败："));
            }).detach();
            GetDlgItem(IDC_BUTTON_LOGIN)->SetWindowTextW(_T("登录"));
            return;
        }
        _wndStatusBar.SetText(_T("登录成功!"), 1, 0);
        SetDlgItemText(IDC_BUTTON_LOGIN, _T("离开"));
        GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(TRUE);
        CString local_user_id;
        GetDlgItemText(IDC_EDIT_PLAYER_ID, local_user_id);

        // 本地记录用户信息
        for each (UserDataInfo itor in user_data_vec_)
        {
            UserStreamInfo info;
            info.user_id = itor.user_id;
            _user_stream_map[itor.user_id] = info;
        }
        _msg_rich_edit_ctrl.SetWindowTextW(_T(""));
        _user_list_ctrl.DeleteAllItems();
        for each (UserDataInfo itor in user_data_vec_)
        {
            if (local_user_id.CompareNoCase(utf2unicode(itor.user_id).c_str()) == 0) {
                // not show self
                continue;
            }
            _user_list_ctrl.InsertItem(0, utf2unicode(itor.user_id).c_str());
            _user_list_ctrl.SetItemText(0, 0, utf2unicode(itor.user_id).c_str());

            CString publish_state_str;
            publish_state_str.Format(_T("Audio:%s,Video:%s,Mute-Audio:%s,Mute-Video:%s"),
                itor.audio_published ? _T("true") : _T("false"),
                itor.video_published ? _T("true") : _T("false"),
                itor.audio_mute ? _T("true") : _T("false"),
                itor.video_mute ? _T("true") : _T("false")
            );

            _user_list_ctrl.SetItemText(0, 1, publish_state_str);
            
            // 自动订阅所有远端用户数据流
            if (itor.video_published || itor.audio_published) {
                OnRemotePublish(itor.user_id, itor.audio_published, itor.video_published);
            }
        }

        // 记录开始连麦的系统时间，并开启定时器
        _start_time = chrono::system_clock::now();
        SetTimer(UPDATE_TIME_TIMER_ID, 1000, nullptr);

        // 音量条更新定时器
        SetTimer(UPDATE_AUDIO_LEVEL, 50, nullptr);
    });
    SetTimer(CALLBACK_UI_TIMER_ID, 1, nullptr);
}

void CRtcDemoDlg::OnLeave(int error_code_,
    const std::string& error_str_, const std::string& kicked_user_id_)
{
    lock_guard<recursive_mutex> lock_(_mutex);
    _call_function_vec.emplace_back([&, error_code_, error_str_, kicked_user_id_]() {
        if (error_code_ == Err_Kickout_Of_Room) {
            // 必须新开一个线程进行弹窗
            std::thread([&] {
                char buf[1024] = { 0 };
                snprintf(buf,
                    sizeof(buf),
                    "您被 %s 踢出了房间！\r\n 如果您要重新登录房间，请重新手动登录！",
                    kicked_user_id_.c_str());
                MessageBox(utf2unicode(buf).c_str());
            }).detach();
        }
        _wndStatusBar.SetText(_T("您已离开房间！"), 1, 0);
        SetDlgItemText(IDC_BUTTON_LOGIN, _T("登录"));
        _user_list_ctrl.DeleteAllItems();
        SetDlgItemText(IDC_BUTTON_PREVIEW_VIDEO, _T("预览"));
        SetDlgItemText(IDC_BUTTON_PUBLISH, _T("发布"));
        GetDlgItem(IDC_STATIC_VIDEO_PREVIEW)->Invalidate();
        _user_stream_map.clear();
        _rtc_room_interface->LeaveRoom();
    });
    SetTimer(CALLBACK_UI_TIMER_ID, 1, nullptr);
}

void CRtcDemoDlg::OnRemoteUserJoin(const std::string& user_id_, const std::string& user_data_)
{
    lock_guard<recursive_mutex> lock_(_mutex);
    _call_function_vec.emplace_back([&, user_id_, user_data_]() {
        auto itor = _user_stream_map.find(user_id_);
        if (itor != _user_stream_map.end()) {
            CString str;
            for (int i = 0; i < _user_list_ctrl.GetItemCount(); i++) {
                str = _user_list_ctrl.GetItemText(i, 0);
                if (str.CompareNoCase(utf2unicode(user_id_).c_str()) == 0) {
                    _user_list_ctrl.DeleteItem(i);
                    break;
                }
            }
            _user_stream_map.erase(itor);
        }
        _user_list_ctrl.InsertItem(0, utf2unicode(user_id_).c_str());
        _user_list_ctrl.SetItemText(0, 1, _T("Audio:false,Video:false,Mute-Audio:false,Mute-Video:false"));

        CString str;
        str.Format(_T("%s 加入了房间！"), utf2unicode(user_id_).c_str());
        _wndStatusBar.SetText(
            str, 1, 0);

        UserStreamInfo info;
        info.user_id = user_id_;
        _user_stream_map[user_id_] = info;
    });
    SetTimer(CALLBACK_UI_TIMER_ID, 1, nullptr);
}

void CRtcDemoDlg::OnRemoteUserLeave(const std::string& user_id_, int error_code)
{
    lock_guard<recursive_mutex> lock_(_mutex);
    _call_function_vec.emplace_back([&, user_id_, error_code]() {
        auto itor = _user_stream_map.find(user_id_);
        if (itor != _user_stream_map.end()) {
            CString str;
            for (int i = 0; i < _user_list_ctrl.GetItemCount(); i++) {
                str = _user_list_ctrl.GetItemText(i, 0);
                if (str.CompareNoCase(utf2unicode(itor->first).c_str()) == 0) {
                    _user_list_ctrl.DeleteItem(i);
                    break;
                }
            }
            itor->second.render_wnd_ptr.reset();
            _user_stream_map.erase(itor);

            str.Format(_T("%s 离开了房间！"), utf2unicode(user_id_).c_str());
            _wndStatusBar.SetText(
                str, 1, 0);
        }
    });
    SetTimer(CALLBACK_UI_TIMER_ID, 1, nullptr);
}

void CRtcDemoDlg::OnRemotePublish(const std::string& user_id_, bool has_audio_, bool has_video_)
{
    lock_guard<recursive_mutex> lock_(_mutex);
    _call_function_vec.emplace_back([&, user_id_, has_audio_, has_video_]() {
        auto itor = _user_stream_map.find(user_id_);
        if (itor == _user_stream_map.end()) {
            return;
        }
        itor->second.audio_published = has_audio_;
        itor->second.video_published = has_video_;
        itor->second.is_subscribed = false;
        CString str, item_str;
        for (int i = 0; i < _user_list_ctrl.GetItemCount(); i++) {
            str = _user_list_ctrl.GetItemText(i, 0);
            if (str.CompareNoCase(utf2unicode(itor->first).c_str()) == 0) {
                item_str.Format(_T("Audio:%s,Video:%s,Mute-Audio:%s,Mute-Video:%s"),
                    itor->second.audio_published ? _T("true") : _T("false"),
                    itor->second.video_published ? _T("true") : _T("false"),
                    itor->second.audio_mute ? _T("true") : _T("false"),
                    itor->second.video_mute ? _T("true") : _T("false")
                );                    
                _user_list_ctrl.SetItemText(i, 1, item_str.GetBuffer());
                break;
            }
        }
        itor->second.display_name_ptr.reset(new CStatic, [](CWnd* ptr_) {
            if (ptr_) {
                ptr_->Invalidate();
                ptr_->DestroyWindow();
                delete ptr_;
            }
        });
        itor->second.render_wnd_ptr.reset(new CStatic, [](CWnd* ptr_) {
            if (ptr_) {
                ptr_->Invalidate();
                ptr_->DestroyWindow();
                delete ptr_;
            }
        });
        itor->second.volume_ptr.reset(new CProgressCtrl, [](CWnd* ptr_) {
            if (ptr_) {
                ptr_->DestroyWindow();
                delete ptr_;
            }
        });
        CRect rc = GetRenderWndPos();
        itor->second.render_wnd_ptr->Create(
            _T(""),
            WS_CHILD | WS_VISIBLE | WS_DISABLED,
            rc,
            GetDlgItem(IDC_STATIC_PLAY),
            CUSTOM_RESOURCE_ID + ++_resource_id);
        itor->second.display_name_ptr->Create(
            utf2unicode(itor->first).c_str(),
            WS_CHILD | WS_VISIBLE | WS_DISABLED || SS_CENTER,
            CRect(0, 0, rc.right, DISPLAY_NAME_HEIGHT),
            GetDlgItem(IDC_STATIC_PLAY),
            CUSTOM_RESOURCE_ID + ++_resource_id);
        itor->second.volume_ptr->Create(
            WS_CHILD | /*WS_VISIBLE |*/ PBS_SMOOTH | PBS_VERTICAL,
            CRect(0, 0, rc.right, DISPLAY_NAME_HEIGHT),
            GetDlgItem(IDC_STATIC_PLAY),
            CUSTOM_RESOURCE_ID + ++_resource_id);
        itor->second.volume_ptr->SetRange(0, 100);
        itor->second.volume_ptr->SetPos(0);
        
        itor->second.display_name_ptr->ShowWindow(SW_SHOWNORMAL);
        itor->second.render_wnd_ptr->ShowWindow(SW_SHOWNORMAL);
        itor->second.volume_ptr->ShowWindow(SW_SHOWNORMAL);

        //if (!_full_dlg_ptr) {
        //    _full_dlg_ptr.reset(new CFullScreenDlg, [](CFullScreenDlg* ptr_) {
        //        if (ptr_) {
        //            ptr_->Invalidate();
        //            ptr_->DestroyWindow();
        //            delete ptr_;
        //        }
        //    });
        //    _full_dlg_ptr->Create(IDD_DIALOG_FULL);
        //    _full_dlg_ptr->ShowWindow(SW_SHOWMAXIMIZED);
        //}


        _rtc_room_interface->Subscribe(itor->first, has_video_?itor->second.render_wnd_ptr->m_hWnd:NULL);
        //_rtc_room_interface->Subscribe(itor->first, has_video_ ? _full_dlg_ptr->m_hWnd : NULL);
        if (_contain_admin_flag) {
            AdjustMergeStreamPosition();
        }
        str.Format(_T("%s 发布了媒体流！"), utf2unicode(user_id_).c_str());
        _wndStatusBar.SetText(
            str, 1, 0);
    }
    );
    SetTimer(CALLBACK_UI_TIMER_ID, 1, nullptr);
}

void CRtcDemoDlg::OnRemoteUnPublish(const std::string& user_id_)
{
    lock_guard<recursive_mutex> lock_(_mutex);
    _call_function_vec.emplace_back([&, user_id_]() {
        auto itor = _user_stream_map.find(user_id_);
        if (itor == _user_stream_map.end()) {
            return;
        }
        itor->second.audio_published = false;
        itor->second.video_published = false;
        itor->second.is_subscribed   = false;
        CString str;
        for (int i = 0; i < _user_list_ctrl.GetItemCount(); i++) {
            str = _user_list_ctrl.GetItemText(i, 0);
            if (str.CompareNoCase(utf2unicode(itor->first).c_str()) == 0) {
                _user_list_ctrl.SetItemText(i, 1, _T("Audio:false,Video:false,Mute-Audio:false,Mute-Video:false"));
                break;
            }
        }
        itor->second.render_wnd_ptr.reset();
        itor->second.display_name_ptr.reset();

        if (_contain_admin_flag) {
            AdjustMergeStreamPosition();
        }
        str.Format(_T("%s 取消发布了媒体流！"), utf2unicode(user_id_).c_str());
        _wndStatusBar.SetText(
            str, 1, 0);
    }
    );
    SetTimer(CALLBACK_UI_TIMER_ID, 1, nullptr);
}

void CRtcDemoDlg::OnLocalPublishResult(int error_code_, const std::string& error_str_)
{
    TRACE("%s", __FUNCTION__);
    if (0 == error_code_) {
        lock_guard<recursive_mutex> lock_(_mutex);
        _call_function_vec.emplace_back([&, error_code_, error_str_]() {
            _publish_flag = true;
            SetDlgItemText(IDC_STATIC_PUBLISH_STREAM_ID, _T("publish success"));
            SetDlgItemText(IDC_BUTTON_PUBLISH, _T("取消发布"));
            //SetDlgItemText(IDC_BUTTON_PREVIEW_VIDEO, _T("预览"));
            ((CButton *)GetDlgItem(IDC_CHECK_MUTE_AUDIO))->SetCheck(0);
            ((CButton *)GetDlgItem(IDC_CHECK_MUTE_VIDEO))->SetCheck(0);
            if (_contain_admin_flag) {
                _rtc_room_interface->SetMergeStreamLayout(
                    unicode2utf(_user_id.GetBuffer()), 0, 0, 0, Canvas_Width, Canvas_Height, false, false);
            }
            _wndStatusBar.SetText(_T("本地流发布成功！"), 1, 0);
        });
        SetTimer(CALLBACK_UI_TIMER_ID, 1, nullptr);
    } else {
        std::thread([] {
            ::AfxMessageBox(_T("发布失败！请确认音视频采集设备可以正常打开！ "));
        }).detach();
    }
}

void CRtcDemoDlg::OnSubscribeResult(const std::string& user_id_,
    int error_code_, const std::string& error_str_)
{
    lock_guard<recursive_mutex> lock_(_mutex);
    _call_function_vec.emplace_back([&, user_id_, error_code_, error_str_]() {
        if (0 != error_code_) {
            CString str;
            str.Format(_T("订阅用户:%s 数据流失败, error code：%d， erro string：%s"),
                utf2unicode(user_id_).c_str(), error_code_, utf2unicode(error_str_).c_str());

            _wndStatusBar.SetText(str, 1, 0);

            std::thread([=]() {
                MessageBox(str);
            }).detach();

            return;
        }
        lock_guard<recursive_mutex> lock_(_mutex);
        if (_user_stream_map.empty()) {
            //没有任何用户、媒体流记录
            return;
        }
        auto itor = _user_stream_map.find(user_id_);
        if (itor != _user_stream_map.end()) {
            if (itor->first.compare(user_id_) == 0) {
                itor->second.is_subscribed = true;
            }
        }
        CString str;
        str.Format(_T("订阅 %s 数据流成功！"), utf2unicode(user_id_).c_str());
        _wndStatusBar.SetText(str, 1, 0);
    });
    SetTimer(CALLBACK_UI_TIMER_ID, 1, nullptr);
}

void CRtcDemoDlg::OnKickoutResult(const std::string& user_id_,
    int error_code_, const std::string& error_str_)
{
    lock_guard<recursive_mutex> lock_(_mutex);
    _call_function_vec.emplace_back([&, user_id_, error_code_, error_str_]() {
        char buf[1024] = { 0 };
        if (0 == error_code_) {
            snprintf(buf, 1024, "踢出用户：%s 成功！ ", user_id_.c_str());
        } else {
            snprintf(buf, 1024, "踢出用户：%s 失败！ 错误码：%d， %s",
                user_id_.c_str(), error_code_, error_str_.c_str());
        }
        _wndStatusBar.SetText(utf2unicode(buf).c_str(), 1, 0);
        MessageBox(utf2unicode(buf).c_str(), _T("踢出用户： "));
    });
    SetTimer(KICKOUT_USER_RESULT, 1, nullptr);
}

void CRtcDemoDlg::OnRemoteStreamMute(const std::string& user_id_,
    bool mute_audio_, bool mute_video_)
{
    lock_guard<recursive_mutex> lock_(_mutex);
    _call_function_vec.emplace_back([&, user_id_, mute_audio_, mute_video_]() {

        auto itor = _user_stream_map.find(user_id_);
        if (itor == _user_stream_map.end()) {
            return;
        }
        itor->second.audio_mute = mute_audio_;
        itor->second.video_mute = mute_video_;
        CString str;
        for (int i = 0; i < _user_list_ctrl.GetItemCount(); i++) {
            str = _user_list_ctrl.GetItemText(i, 0);
            if (str.CompareNoCase(utf2unicode(user_id_).c_str()) == 0) {
                wchar_t buf[128] = { 0 };
                wsprintf(buf, 
                    _T("Audio:%s,Video:%s,Mute-Audio:%s,Mute-Video:%s"),
                    itor->second.audio_published?_T("true"):_T("false"),
                    itor->second.video_published ? _T("true") : _T("false"),
                    itor->second.audio_mute ? _T("true") : _T("false"),
                    itor->second.video_mute ? _T("true") : _T("false")
                );
                _user_list_ctrl.SetItemText(i, 1, buf);
                break;
            }
        }
    });
    SetTimer(CALLBACK_UI_TIMER_ID, 1, nullptr);
}

void CRtcDemoDlg::OnError(int error_code_, const std::string& error_str_)
{
    std::thread([=]() {
        char buf[1024] = { 0 };
        snprintf(buf, sizeof(buf), "Error code:%d, error string:%s", error_code_, error_str_.c_str());
        _wndStatusBar.SetText(utf2unicode(buf).c_str(), 1, 0);
        MessageBox(utf2unicode(buf).c_str());
    }
    ).detach();
}

void CRtcDemoDlg::OnStatisticsUpdated(const StatisticsReport& statistics_)
{
    lock_guard<recursive_mutex> lock_(_mutex);
    _call_function_vec.emplace_back([&, statistics_]() {
        char dest_buf[1024] = { 0 };
        snprintf(dest_buf,
            sizeof(dest_buf),
            "用户:%s 音频：码率:%d, 丢包率:%0.3f;"
            " 视频: 分辨率:%d*%d, 帧率:%d, 码率:%d, 丢包率:%0.3f",
            statistics_.user_id.c_str(),
            statistics_.audio_bitrate,
            statistics_.audio_packet_lost_rate,
            statistics_.video_width,
            statistics_.video_height,
            statistics_.video_frame_rate,
            statistics_.video_bitrate,
            statistics_.video_packet_lost_rate
        );
        TRACE(utf2unicode(dest_buf).c_str());
        InsertMsgEditText(utf2unicode(dest_buf).c_str());
    });
    SetTimer(UPDATE_STATISTICS_TIMER, 1, nullptr);
}

void CRtcDemoDlg::OnAudioPCMFrame(const unsigned char* audio_data_,
    int bits_per_sample_, int sample_rate_, size_t number_of_channels_,
    size_t number_of_frames_, const std::string& user_id_)
{
}

void CRtcDemoDlg::OnAudioDeviceStateChanged(
    AudioDeviceState new_device_state_, const std::string& device_guid_)
{
    lock_guard<recursive_mutex> lock_(_mutex);
    _call_function_vec.emplace_back([&, new_device_state_, device_guid_]() {
        if (new_device_state_ != ads_active) {
            char msg_buf[1024] = { 0 };
            snprintf(msg_buf,
                sizeof(msg_buf),
                "音频设备：%s 被拔出或失效了，可能会影响您正常的连麦！",
                device_guid_.c_str());

            string msg_str(msg_buf);
            _wndStatusBar.SetText(utf2unicode(msg_str).c_str(), 1, 0);
            std::thread([=]() {
                MessageBox(utf2unicode(msg_str).c_str());
            }).detach();
        } else if (new_device_state_ == ads_active) {
            char msg_buf[1024] = { 0 };
            snprintf(msg_buf,
                sizeof(msg_buf),
                "音频设备：%s 已插入！",
                device_guid_.c_str());

            string msg_str(msg_buf);
            _wndStatusBar.SetText(utf2unicode(msg_str).c_str(), 1, 0);
            std::thread([=]() {
                MessageBox(utf2unicode(msg_str).c_str());
            }).detach();
        }
    });
    SetTimer(DEVICE_STATE_CHANGE, 1, nullptr);
}

void CRtcDemoDlg::OnVideoFrame(const unsigned char* raw_data_,
    int data_len_, int width_, int height_, 
    qiniu::VideoCaptureType video_type_, const std::string& user_id_)
{

}

void CRtcDemoDlg::OnVideoFramePreview(const unsigned char* raw_data_,
    int data_len_, int width_, int height_, qiniu::VideoCaptureType video_type_)
{
}

void CRtcDemoDlg::OnVideoDeviceStateChanged(
    VideoDeviceState new_device_state_, const std::string& device_id_)
{
    lock_guard<recursive_mutex> lock_(_mutex);
    _call_function_vec.emplace_back([&, new_device_state_, device_id_]() {
        if (new_device_state_ != vds_active) {
            char msg_buf[1024] = { 0 };
            snprintf(msg_buf,
                sizeof(msg_buf),
                "视频设备：%s 被拔出，如果您正在连麦中，请插入此设备后重新发布！",
                device_id_.c_str());

            string msg_str(msg_buf);
            _wndStatusBar.SetText(utf2unicode(msg_str).c_str(), 1, 0);
            MessageBox(utf2unicode(msg_str).c_str());
        } else if (new_device_state_ != vds_lost) {
            //设备插入
            char msg_buf[1024] = { 0 };
            snprintf(msg_buf,
                sizeof(msg_buf),
                "视频设备：%s 已插入！",
                device_id_.c_str());

            string msg_str(msg_buf);
            _wndStatusBar.SetText(utf2unicode(msg_str).c_str(), 1, 0);
            MessageBox(utf2unicode(msg_str).c_str());
        }
    });
    SetTimer(DEVICE_STATE_CHANGE, 1, nullptr);
}

void CRtcDemoDlg::OnBnClickedButtonJoin()
{
    KillTimer(UPDATE_TIME_TIMER_ID);

    CString btn_str;
    GetDlgItemText(IDC_BUTTON_LOGIN, btn_str);
    if (btn_str.CompareNoCase(_T("登录")) == 0) {
        GetDlgItemText(IDC_EDIT_APPID, _app_id);
        GetDlgItemText(IDC_EDIT_ROOM_ID, _room_name);
        GetDlgItemText(IDC_EDIT_PLAYER_ID, _user_id);
        if (_room_name.IsEmpty() || _user_id.IsEmpty()) {
            MessageBox(_T("Room ID and Player ID can't be NULL!"));
            return;
        }
        GetDlgItem(IDC_BUTTON_LOGIN)->SetWindowText(_T("登录中"));
        GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(FALSE);

        if (_join_room_thread.joinable()) {
            _join_room_thread.join();
        }
        _join_room_thread = std::thread([this]() {
            //向 AppServer 获取 token 
            _room_token.clear();
            int ret = GetRoomToken(unicode2utf(_room_name.GetBuffer()),
                unicode2utf(_user_id.GetBuffer()), _room_token);
            if (ret != 0) {
                _wndStatusBar.SetText(_T("获取房间 token 失败，请检查您的网络是否正常！"), 1, 0);
                MessageBox(_T("获取房间 token 失败，请检查您的网络是否正常！"));
                GetDlgItem(IDC_BUTTON_LOGIN)->SetWindowText(_T("登录"));
                GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(TRUE);
                return;
            }
            _wndStatusBar.SetText(_T("获取房间 token 成功！"), 1, 0);

            // 重新初始化 SDK 接口指针
            _rtc_room_interface = QNRTCRoom::ObtainRoomInterface();
            if (!_rtc_room_interface) {
                return;
            }
            _rtc_room_interface->SetRoomListener(this);

            // 视频功能接口
            _rtc_video_interface = _rtc_room_interface->ObtainVideoInterface();
            _rtc_video_interface->SetVideoListener(this);

            // 音频功能接口
            _rtc_audio_interface = _rtc_room_interface->ObtainAudioInterface();
            _rtc_audio_interface->SetAudioListener(this);

            // 设置音频播放设备
            int audio_playout_device_index = ((CComboBox *)GetDlgItem(IDC_COMBO_PLAYOUT))->GetCurSel();
            audio_playout_device_index = (audio_playout_device_index == CB_ERR) ? 0 : audio_playout_device_index;
            qiniu::AudioDeviceSetting audio_set;
            audio_set.device_index = audio_playout_device_index;
            audio_set.device_type = qiniu::AudioDeviceSetting::wdt_DefaultDevice;
            _rtc_audio_interface->SetPlayoutDevice(audio_set);

            _rtc_room_interface->JoinRoom(_room_token);

            WriteConfigFile();
        });
    } else {
        // LeaveRoom
        if (_fake_video_thread.joinable()) {
            _stop_fake_flag = true;
            _fake_video_thread.join();
        }
        if (_contain_admin_flag) {
            _rtc_room_interface->StopMergeStream();
        }
        // 释放 SDK 资源
        KillTimer(UPDATE_AUDIO_LEVEL);
        _rtc_room_interface->LeaveRoom();
        _rtc_room_interface->Release();
        _rtc_room_interface = nullptr;
        _rtc_video_interface = nullptr;
        _rtc_audio_interface = nullptr;

        SetDlgItemText(IDC_BUTTON_LOGIN, _T("登录"));
        SetDlgItemText(IDC_BUTTON_PUBLISH, _T("发布"));
        SetDlgItemText(IDC_BUTTON_PREVIEW_VIDEO, _T("预览"));
        SetDlgItemText(IDC_BUTTON_PREVIEW_SCREEN, _T("预览屏幕"));
        GetDlgItem(IDC_BUTTON_PUBLISH)->Invalidate();
        _user_list_ctrl.DeleteAllItems();
        _user_stream_map.clear();
        _wndStatusBar.SetText(_T("当前未登录房间！"), 1, 0);

        Invalidate();
    }
}

void CRtcDemoDlg::OnBnClickedButtonPublish()
{
    // TODO: Add your control notification handler code here
    if (!_rtc_room_interface) {
        return;
    }
    CString str;
    GetDlgItemText(IDC_BUTTON_PUBLISH, str);
    if (0 == str.CompareNoCase(_T("发布"))) {
        bool enable_audio, enable_video;
        enable_audio =
            (BST_CHECKED == ((CButton *)GetDlgItem(IDC_CHECK_ENABLE_AUDIO))->GetCheck()) ? true : false;
        enable_video =
            (BST_CHECKED == ((CButton *)GetDlgItem(IDC_CHECK_ENABLE_VIDEO))->GetCheck()) ? true : false;

        if (BST_CHECKED == ((CButton *)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->GetCheck()) {
            // 模拟导入外部数据
            ImportExternalRawFrame();
        } else {
            // 使用 SDK 内部音视频采集
            _rtc_audio_interface->EnableAudioFakeInput(false);
            _rtc_video_interface->EnableVideoFakeCamera(false);

            CameraSetting camera_setting;
            CString video_dev_name;
            string video_dev_id;
            int audio_recorder_device_index(-1);

            GetDlgItem(IDC_COMBO_CAMERA)->GetWindowTextW(video_dev_name);
            audio_recorder_device_index = ((CComboBox *)GetDlgItem(IDC_COMBO_MICROPHONE))->GetCurSel();
            audio_recorder_device_index = (audio_recorder_device_index == CB_ERR) ? 0 : audio_recorder_device_index;

            // 视频相关配置，摄像头 或者 屏幕分享
            if (((CButton *)GetDlgItem(IDC_CHECK_ACTIVE_SCREEN))->GetCheck() == BST_CHECKED) {
                OnBnClickedCheckActiveScreen();
            } else {
                if (video_dev_name.IsEmpty()) {
                    if (enable_video) {
                        MessageBox(_T("您当前没有任何视频设备！"));
                        enable_video = false;
                    }
                }
                if (enable_video) {
                    auto itor = _camera_dev_map.begin();
                    while (itor != _camera_dev_map.end()) {
                        if (itor->second.device_name.compare(unicode2utf(video_dev_name.GetBuffer())) == 0) {
                            video_dev_id = itor->first;
                            break;
                        }
                        ++itor;
                    }
                    // 获取适中的尺寸
                    auto tuple_size = FindBestVideoSize(itor->second.capability_vec);

                    camera_setting.device_name = unicode2utf(video_dev_name.GetBuffer());
                    camera_setting.device_id = video_dev_id;
                    camera_setting.width = std::get<0>(tuple_size);
                    camera_setting.height = std::get<1>(tuple_size);
                    camera_setting.max_fps = 15;
                    camera_setting.bitrate = 500000;
                }
            }
            camera_setting.render_hwnd = GetDlgItem(IDC_STATIC_VIDEO_PREVIEW2)->m_hWnd;

            _rtc_room_interface->ObtainVideoInterface()->SetCameraParams(camera_setting);

            // 音频配置
            if (enable_audio) {
                enable_audio = (audio_recorder_device_index < 0) ? false : true;
                /* 如果没有音频输入设备，则不发布音频 */
                if (audio_recorder_device_index >= 0) {
                    AudioDeviceSetting audio_setting;
                    audio_setting.device_index = audio_recorder_device_index;
                    audio_setting.device_type = qiniu::AudioDeviceSetting::wdt_DefaultDevice;
                    if (0 != _rtc_audio_interface->SetRecordingDevice(audio_setting)) {
                        std::thread([this] {
                            MessageBox(_T("设置音频输入设备失败，应用程序将继续连麦，但不再发布音频流！"));
                        }).detach();
                        audio_recorder_device_index = -1;
                        _wndStatusBar.SetText(_T("设置音频输入设备失败，应用程序将继续连麦，但不再发布音频流！"), 1, 0);
                    }
                } else {
                    _wndStatusBar.SetText(_T("您当前没有任何音频输入设备，将仅发布视频流！"), 1, 0);
                }
            }

            if (!enable_audio && !enable_video) {
                _wndStatusBar.SetText(_T("不能同时不发布音、视频！"), 1, 0);
                MessageBox(_T("不能同时不发布音、视频！"));
                return;
            }
        }
        int ret = _rtc_room_interface->Publish(enable_audio, enable_video);
        if (Err_Already_Published == ret) {
            _rtc_room_interface->UnPublish();
            _rtc_room_interface->Publish(enable_audio, enable_video);
        }
    } else {
        _stop_fake_flag = true;
        if (_fake_video_thread.joinable()) {
            _fake_video_thread.join();
        }
        if (_fake_audio_thread.joinable()) {
            _fake_audio_thread.join();
        }
        if (_contain_admin_flag) {
            _rtc_room_interface->StopMergeStream();
        }

        _rtc_room_interface->UnPublish();

        SetDlgItemText(IDC_STATIC_PUBLISH_STREAM_ID, _T(""));
        SetDlgItemText(IDC_BUTTON_PUBLISH, _T("发布"));
        GetDlgItem(IDC_STATIC_VIDEO_PREVIEW2)->Invalidate();
        _wndStatusBar.SetText(_T("停止发布本地流！"), 1, 0);
    }
}

void CRtcDemoDlg::OnDestroy()
{
    KillTimer(UPDATE_TIME_TIMER_ID);
    KillTimer(UPDATE_AUDIO_LEVEL);
    _stop_fake_flag = true;
    if (_fake_video_thread.joinable()) {
        _fake_video_thread.join();
    }
    if (_fake_audio_thread.joinable()) {
        _fake_audio_thread.join();
    }
    if (_join_room_thread.joinable()) {
        _join_room_thread.join();
    }
    if (_rtc_room_interface) {
        if (_contain_admin_flag) {
            _rtc_room_interface->StopMergeStream();
        }
        _rtc_room_interface->Release();
        _rtc_room_interface = nullptr;
    }

    qiniu::QNRTCEngine::Release();

    _user_stream_map.clear();

    _wndStatusBar.DestroyWindow();

    __super::OnDestroy();
}

int CRtcDemoDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (__super::OnCreate(lpCreateStruct) == -1)
        return -1;

    // TODO:  Add your specialized creation code here
    // 固定主窗口大小 
    DWORD   dwStyle = GetWindowLong(m_hWnd, GWL_STYLE);
    dwStyle &= ~(WS_SIZEBOX);
    SetWindowLong(m_hWnd, GWL_STYLE, dwStyle);

    return 0;
}

void CRtcDemoDlg::OnCbnSelchangeComboCamera()
{
    // TODO: Add your control notification handler code here
}


void CRtcDemoDlg::OnCbnSelchangeComboMicrophone()
{
    // TODO: Add your control notification handler code here
}


void CRtcDemoDlg::OnBnClickedButtonPreview()
{
    // TODO: Add your control notification handler code here
    if (!_rtc_video_interface) {
        return;
    }
    CString str;
    //GetDlgItemText(IDC_BUTTON_PUBLISH, str);
    //if (0 == str.CompareNoCase(_T("取消发布"))) {
    //    MessageBox(_T("发布中，不能预览！"));
    //    return;
    //}
    GetDlgItemText(IDC_BUTTON_PREVIEW_VIDEO, str);
    if (0 == str.CompareNoCase(_T("预览"))) {

        CameraSetting camera_setting;
        CString cur_dev_name;
        string cur_dev_id;

        if (((CButton *)GetDlgItem(IDC_CHECK_ACTIVE_SCREEN))->GetCheck() == BST_CHECKED) {
            OnBnClickedCheckActiveScreen();
        } else {
            GetDlgItem(IDC_COMBO_CAMERA)->GetWindowTextW(cur_dev_name);
            if (cur_dev_name.IsEmpty()) {
                MessageBox(_T("您当前没有任何视频设备！"));
                return;
            }
            auto itor = _camera_dev_map.begin();
            while (itor != _camera_dev_map.end()) {
                if (itor->second.device_name.compare(unicode2utf(cur_dev_name.GetBuffer())) == 0) {
                    cur_dev_id = itor->first;
                    break;
                }
                ++itor;
            }
            auto tuple_size = FindBestVideoSize(itor->second.capability_vec);
            camera_setting.device_name = unicode2utf(cur_dev_name.GetBuffer());
            camera_setting.device_id = cur_dev_id;
            camera_setting.width = std::get<0>(tuple_size);
            camera_setting.height = std::get<1>(tuple_size);
            camera_setting.max_fps = 15;
            camera_setting.bitrate = 500000;
        }
        camera_setting.render_hwnd = GetDlgItem(IDC_STATIC_VIDEO_PREVIEW2)->m_hWnd;

        if (0 == _rtc_video_interface->PreviewCamera(camera_setting)) {
            _wndStatusBar.SetText(_T("开启预览成功！"), 1, 0);
            SetDlgItemText(IDC_BUTTON_PREVIEW_VIDEO, _T("取消预览"));
        } else {
            _wndStatusBar.SetText(_T("开启预览失败！"), 1, 0);
            MessageBox(_T("预览失败！"));
        }
    } else {

        OnBnClickedCheckActiveScreen();

        if (0 == _rtc_video_interface->UnPreviewCamera()) {
            _wndStatusBar.SetText(_T("停止本地预览！"), 1, 0);
            SetDlgItemText(IDC_BUTTON_PREVIEW_VIDEO, _T("预览"));
        } else {
            _wndStatusBar.SetText(_T("取消本地预览失败！"), 1, 0);
            MessageBox(_T("取消预览失败！"));
        }
        GetDlgItem(IDC_STATIC_VIDEO_PREVIEW2)->Invalidate();
    }
}

BOOL CRtcDemoDlg::PreTranslateMessage(MSG* pMsg)
{
    return __super::PreTranslateMessage(pMsg);
}

CRect&& CRtcDemoDlg::GetRenderWndPos()
{
    CRect parent_rc, child_tc;
    GetDlgItem(IDC_STATIC_PLAY)->GetClientRect(&parent_rc);
    child_tc.left  = 0;
    child_tc.top = DISPLAY_NAME_HEIGHT;
    child_tc.right = parent_rc.Height();
    child_tc.top = parent_rc.Height();
    return std::move(child_tc);
}

void CRtcDemoDlg::AdjustRenderWndPos()
{
    int pos_x(0);
    CRect parent_rc;
    GetDlgItem(IDC_STATIC_PLAY)->GetClientRect(&parent_rc);
    for (auto itor : _user_stream_map) {
        if (itor.second.render_wnd_ptr
            && itor.second.render_wnd_ptr->GetParent() == GetDlgItem(IDC_STATIC_PLAY)) {

            CRect dest_rc(pos_x,
                parent_rc.top + DISPLAY_NAME_HEIGHT,
                pos_x + parent_rc.Height() - 20,
                parent_rc.Height());
            itor.second.render_wnd_ptr->MoveWindow(&dest_rc);

            CRect dest_rc2(pos_x,
                parent_rc.top,
                pos_x + parent_rc.Height(),
                DISPLAY_NAME_HEIGHT);
            itor.second.display_name_ptr->MoveWindow(&dest_rc2);

            CRect dest_rc3(pos_x + parent_rc.Height() - 20,
                parent_rc.top,
                pos_x + parent_rc.Height(),
                parent_rc.Height());
            itor.second.volume_ptr->MoveWindow(&dest_rc3);

            TRACE("%s, left:%d, top:%d, right:%d, botton:%d\n", 
                __FUNCTION__, dest_rc.left, dest_rc.top, dest_rc.right, dest_rc.bottom);
            pos_x += parent_rc.Height();
        }
    }
}

void CRtcDemoDlg::OnBnClickedBtnKickout()
{
    // TODO: Add your control notification handler code here
    int index = _user_list_ctrl.GetSelectionMark();
    if (index == -1) {
        MessageBox(_T("请选中要踢出的用户！"));
        return;
    }
    //所选择的用户当前没有发布媒体流
    CString user_id = _user_list_ctrl.GetItemText(index, 0);

    if (_rtc_room_interface) {
        _rtc_room_interface->KickoutUser(unicode2utf(user_id.GetBuffer()).c_str());
    }
}

void CRtcDemoDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    // TODO: Add your message handler code here and/or call default
    if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER_RECORD) {
        int old_pos(0);
        int pos = ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_RECORD))->GetPos();
        if (_rtc_audio_interface) {
            old_pos = _rtc_audio_interface->GetAudioVolume(AudioDeviceInfo::adt_record);

            if (pos == 0) {
                _rtc_audio_interface->SetAudioMuteFlag(AudioDeviceInfo::adt_record, true);
            } else {
                _rtc_audio_interface->SetAudioMuteFlag(AudioDeviceInfo::adt_record, false);

                // 调节系统麦克风配置的音量
                //if (_rtc_audio_interface->SetAudioVolume(AudioDeviceInfo::adt_record, pos) < 0) {
                //    MessageBox(_T("设置录制音量失败！"));
                //}

                // 调整 SDK 内部音量
                _rtc_audio_interface->SetAudioVolume(
                    unicode2utf(_user_id.GetBuffer()), 
                    1.0f * pos / 255
                );
            }
        }
    } else if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER_PLAYOUT) {
        int old_pos(0);
        int pos = ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_PLAYOUT))->GetPos();

        if (_rtc_audio_interface) {
            old_pos = _rtc_audio_interface->GetAudioVolume(AudioDeviceInfo::adt_playout);

            if (pos == 0) {
                _rtc_audio_interface->SetAudioMuteFlag(AudioDeviceInfo::adt_playout, true);
            } else {
                _rtc_audio_interface->SetAudioMuteFlag(AudioDeviceInfo::adt_playout, false);

                _rtc_audio_interface->SetAudioVolume(AudioDeviceInfo::adt_playout, pos);
            }
        }
    }

    __super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CRtcDemoDlg::OnBnClickedCheckMuteAudio()
{
    // TODO: Add your control notification handler code here
    int check_flag = ((CButton*)GetDlgItem(IDC_CHECK_MUTE_AUDIO))->GetCheck();
    if (_rtc_room_interface) {
        _rtc_room_interface->MuteAudio(check_flag > 0 ? false : true);
    }
}

void CRtcDemoDlg::OnBnClickedCheckMuteVideo()
{
    // TODO: Add your control notification handler code here
    int check_flag = ((CButton*)GetDlgItem(IDC_CHECK_MUTE_VIDEO))->GetCheck();
    if (_rtc_room_interface) {
        _rtc_room_interface->MuteVideo(check_flag > 0 ? false : true);
    }
}

void CRtcDemoDlg::OnCbnSelchangeComboPlayout()
{
    // TODO: Add your control notification handler code here
    int audio_playout_device_index = ((CComboBox *)GetDlgItem(IDC_COMBO_PLAYOUT))->GetCurSel();
    if (audio_playout_device_index >= 0 && _rtc_audio_interface) {
        qiniu::AudioDeviceSetting audio_setting;
        audio_setting.device_index = audio_playout_device_index;
        audio_setting.device_type = qiniu::AudioDeviceSetting::wdt_DefaultDevice;

//         int ret = _rtc_audio_interface->SetPlayoutDevice(audio_setting);
//         if (ret != QNRTC_OK) {
//             std::thread([this]() {
//                 MessageBox(_T("设置音频输出设备失败，您可能听不到对方的声音，或者没有按照您指定的设备进行播放！"));
//             }).detach();
//         }
    }
}

size_t WriteBuffer(void *src_, size_t src_size_, size_t blocks_, void *param_)
{
    string *str = (string*)(param_);
    str->append((char *)src_, src_size_ * blocks_);

    return str->size();
}

int CRtcDemoDlg::GetRoomToken(const string room_name_, const string user_id_, string& token_)
{
    if (room_name_.empty() || user_id_.empty()) {
        return -1;
    }
    CString appId_str;
    string appId = "d8lk7l4ed";
    GetDlgItemText(IDC_EDIT_APPID, appId_str);
    if (!appId_str.IsEmpty()) {
        appId = unicode2utf(appId_str.GetBuffer());
    }

    curl_global_init(CURL_GLOBAL_ALL);
    auto curl = curl_easy_init();

    // set options
    char url_buf[1024] = { 0 };
    string tmp_uid = user_id_;

    // 服务端合流的默认限制：user id 等于 admin 则拥有合流的权限
    if (strnicmp(const_cast<char*>(tmp_uid.c_str()), "admin", tmp_uid.length()) == 0) {
        snprintf(url_buf,
            sizeof(url_buf),
            "https://api-demo.qnsdk.com/v1/rtc/token/admin/app/%s/room/%s/user/%s",
            appId.c_str(),
            room_name_.c_str(),
            user_id_.c_str());
        _contain_admin_flag = true;
    } else {
        snprintf(url_buf,
            sizeof(url_buf),
            "https://api-demo.qnsdk.com/v1/rtc/token/app/%s/room/%s/user/%s",
            appId.c_str(),
            room_name_.c_str(),
            user_id_.c_str());
        _contain_admin_flag = false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url_buf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteBuffer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &token_);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);

    // send request now
    int status(0);
    CURLcode result = curl_easy_perform(curl);
    if (result == CURLE_OK) {
        long code;
        result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        if (result == CURLE_OK) {
            if (code != 200) {
                status = -2; // server auth failed
            } else {
                status = 0; //success
            }
        } else {
            status = -3; //connect server timeout
        }
    } else {
        status = -3; //connect server timeout
    }
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return status;
}

std::tuple<int, int> CRtcDemoDlg::FindBestVideoSize(const CameraCapabilityVec& camera_cap_vec_)
{
    if (camera_cap_vec_.empty()) {
        return{ 0,0 };
    }
    // 高宽比例
    float wh_ratio = 1.0f * 3 / 4;
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

void CRtcDemoDlg::AdjustMergeStreamPosition()
{
    lock_guard<recursive_mutex> lock_(_mutex);
    int user_num(0), pos_num(0);
    for (auto&& itor : _user_stream_map) {
        if (itor.first.compare(unicode2utf(_user_id.GetBuffer())) == 0) {
            continue;
        }
        ++user_num;
        pos_num = 0;
        for (int y(2); y >= 0; --y) {
            for (int x(2); x >= 0; --x) {
                ++pos_num;
                if (pos_num == user_num) {
                    _rtc_room_interface->SetMergeStreamLayout(
                        itor.first, 
                        Canvas_Width / 3 * x, 
                        Canvas_Height / 3 * y,
                        pos_num, 
                        Canvas_Width / 3, 
                        Canvas_Height / 3, 
                        false,
                        false);
                }
            }
        }
    }
}

void CRtcDemoDlg::ReadConfigFile()
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

void CRtcDemoDlg::WriteConfigFile()
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

void CRtcDemoDlg::InitDemoUI()
{
    _wndStatusBar.Create(WS_CHILD | WS_VISIBLE | SBT_OWNERDRAW, CRect(0, 0, 0, 0), this, 0);
    RECT rc;
    GetWindowRect(&rc);
    int strPartDim[3] = { rc.right / 5, rc.right / 5 * 3, -1 };
    _wndStatusBar.SetParts(3, strPartDim);
    //设置状态栏文本  
    _wndStatusBar.SetText(_T("通话时长：00:00::00"), 0, 0);
    _wndStatusBar.SetText(_T("连麦状态"), 1, 0);
    _wndStatusBar.SetText(utf2unicode(GetAppVersion()).c_str(), 2, 0);

    // 初始化麦克风音量条控件
    _local_volume_progress.SetRange(0, 100);
    _local_volume_progress.SetPos(0);
}

void CRtcDemoDlg::ImportExternalRawFrame()
{
    // 模拟导入视频数据,当前使用当前目录下指定的音视频文件
    _rtc_video_interface->EnableVideoFakeCamera(true);
    CameraSetting cs;
    cs.width = 1600;
    cs.height = 800;
    cs.max_fps = 30;
    cs.bitrate = 2000000;
    cs.render_hwnd = GetDlgItem(IDC_STATIC_VIDEO_PREVIEW2)->m_hWnd;
    _rtc_video_interface->SetCameraParams(cs);
    if (_fake_video_thread.joinable()) {
        _stop_fake_flag = true;
        _fake_video_thread.join();
    }
    _fake_video_thread = thread([&] {
        FILE* fp = nullptr;
        fopen_s(&fp, "foreman_320x240.yuv", "rb");
        uint8_t *buf = (uint8_t*)malloc(320 * 240* 3 / 2);
        if (!fp || !buf) {
            MessageBox(_T("foreman_320x240.yuv 文件打开失败，请确认此文件件是否存在!"));
            return;
        }
        size_t ret(0);
        _stop_fake_flag = false;
        chrono::system_clock::time_point start_tp = chrono::system_clock::now();
        while (!_stop_fake_flag) {
            ret = fread_s(buf, 320 * 240 * 3 / 2, 1, 320 * 240 * 3 / 2, fp);
            if (ret > 0) {
                _rtc_video_interface->InputVideoFrame(
                    buf,
                    320 * 240 * 3 / 2,
                    320,
                    240,
                    chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - start_tp).count(),
                    qiniu::VideoCaptureType::kI420,
                    qiniu::kVideoRotation_0);
            } else {
                fseek(fp, 0, SEEK_SET);
                continue;
            }
            this_thread::sleep_for(chrono::milliseconds(1000 / 30)); // 15 fps
        }
        free(buf);
        fclose(fp);
    });

    // 模拟导入音频数据
    _rtc_audio_interface->EnableAudioFakeInput(true);
    if (_fake_audio_thread.joinable()) {
        _stop_fake_flag = true;
        _fake_audio_thread.join();
    }
    _fake_audio_thread = thread([&] {
        FILE* fp = nullptr;
        fopen_s(&fp, "44100hz_16bits_2channels.pcm", "rb");
        if (!fp) {
            MessageBox(_T("PCM 文件:44100hz_16bits_2channels.pcm 打开失败，请确认此文件件是否存在!"));
            return;
        }
        // 每次导入 20 ms 的数据，即 441 * 2 个 samples
        uint8_t *buf = (uint8_t*)malloc(441 * 2 * 2 * 2);

        size_t ret(0);
        _stop_fake_flag = false;
        chrono::system_clock::time_point start_tp = chrono::system_clock::now();
        int64_t audio_frame_count(0);
        while (!_stop_fake_flag) {
            if (chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - start_tp).count() >= audio_frame_count * 20000) {
            } else {
                this_thread::sleep_for(chrono::microseconds(10));
                continue;
            }

            ret = fread_s(buf, 441 * 2 *4, 1, 441 * 2 * 4, fp);
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

void CRtcDemoDlg::InsertMsgEditText(LPCTSTR msg_)
{
    if (!msg_) {
        return;
    }
    int line_count = _msg_rich_edit_ctrl.GetLineCount();
    if (line_count >= 1000) {
        // 此控件可存储数据量有限，为避免卡顿，及时清除
        _msg_rich_edit_ctrl.SetWindowTextW(_T(""));
        _msg_rich_edit_ctrl.UpdateData();
        _msg_rich_edit_ctrl.Invalidate();
    }
    _msg_rich_edit_ctrl.SetSel(-1, -1);
    _msg_rich_edit_ctrl.ReplaceSel(_T("\n"));
    _msg_rich_edit_ctrl.ReplaceSel(msg_);
    _msg_rich_edit_ctrl.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
}

std::function<void()> CRtcDemoDlg::GetFunc()
{
    lock_guard<recursive_mutex> lock_(_mutex);
    if (_call_function_vec.empty()) {
        return nullptr;
    } else {
        auto func = _call_function_vec.front();
        _call_function_vec.pop_front();
        return func;
    }    
}

void CRtcDemoDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == UPDATE_TIME_TIMER_ID) {
        chrono::seconds df_time
            = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - _start_time);
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
        _wndStatusBar.SetText(time_buf, 0, 0);
    } else if (UPDATE_AUDIO_LEVEL == nIDEvent) {
        if (!_rtc_audio_interface) {
            return;
        }
        for (auto&& itor : _user_stream_map) {
            uint32_t volume = _rtc_audio_interface->GetAudioLevel(itor.first);
            if (itor.first.compare(unicode2utf(_user_id.GetBuffer())) == 0) {
                _local_volume_progress.SetPos(volume);
            } else {
                if (itor.second.volume_ptr) {
                    itor.second.volume_ptr->SetPos(volume);
                }
            }
        }
    } else {
        KillTimer(nIDEvent);
        auto func = GetFunc();
        while (func) {
            func();
            func = GetFunc();
        }
        AdjustRenderWndPos();
    }
}

void CRtcDemoDlg::OnBnClickedBtnFlush()
{
    // TODO: Add your control notification handler code here
    if (!_rtc_video_interface) {
        return;
    }
    ((CComboBox *)GetDlgItem(IDC_COMBO_SCREEN))->ResetContent();
    _screen_wnd_map.clear();
    int count = _rtc_video_interface->GetScreenWindowCount();
    for (int i(0); i < count; ++i) {
        ScreenWindowInfo info = _rtc_video_interface->GetScreenWindowInfo(i);
        _screen_wnd_map[info.id] = info;
        ((CComboBox *)GetDlgItem(IDC_COMBO_SCREEN))->AddString(
            utf2unicode(info.title).c_str()
        );
    }
    ((CComboBox *)GetDlgItem(IDC_COMBO_SCREEN))->SetCurSel(0);
}

void CRtcDemoDlg::OnBnClickedCheckActiveScreen()
{
    // 判断是否激活了屏幕录制
    if (!_rtc_video_interface) {
        return;
    }
    CString wnd_title;
    GetDlgItem(IDC_COMBO_SCREEN)->GetWindowTextW(wnd_title);
    int screencasts_state = ((CButton *)GetDlgItem(IDC_CHECK_ACTIVE_SCREEN))->GetCheck();
    int directx_state     = ((CButton *)GetDlgItem(IDC_CHECK_DX))->GetCheck();
    if (screencasts_state == 1) {
        ((CButton *)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->SetCheck(0);
        for (auto&& itor : _screen_wnd_map) {
            if (itor.second.title.compare(unicode2utf(wnd_title.GetBuffer())) == 0) {
                _rtc_video_interface->EnableAndSetScreenSourceId(
                    itor.first, 
                    directx_state?true:false
                );
                break;
            }
        }
    } else {
        // 关闭屏幕共享
        _rtc_video_interface->EnableAndSetScreenSourceId(
            -1, 
            directx_state ? true : false
        );
    }
}

void CRtcDemoDlg::OnBnClickedCheckImportRawData()
{
    // TODO: Add your control notification handler code here
    if (1 == ((CButton *)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->GetCheck()) {
        ((CButton *)GetDlgItem(IDC_CHECK_ACTIVE_SCREEN))->SetCheck(0);
        OnBnClickedCheckActiveScreen();
    }
}

void CRtcDemoDlg::OnCbnSelchangeComboScreen()
{
    // 判断是否激活了屏幕录制
    if (!_rtc_video_interface) {
        return;
    }
    CString wnd_title, btn_text;
    GetDlgItem(IDC_COMBO_SCREEN)->GetWindowTextW(wnd_title);
    GetDlgItem(IDC_BUTTON_PREVIEW_VIDEO2)->GetWindowTextW(btn_text);
    int screencasts_state = ((CButton *)GetDlgItem(IDC_CHECK_ACTIVE_SCREEN))->GetCheck();
    int directx_state = ((CButton *)GetDlgItem(IDC_CHECK_DX))->GetCheck();

    // 如果是正在预览状态，则实时切换窗口
    if (btn_text.CompareNoCase(_T("取消预览")) == 0) {
        for (auto&& itor : _screen_wnd_map) {
            if (itor.second.title.compare(unicode2utf(wnd_title.GetBuffer())) == 0) {
                _rtc_video_interface->PreviewScreenSource(
                    itor.first,
                    GetDlgItem(IDC_STATIC_VIDEO_PREVIEW)->m_hWnd,
                    directx_state ? true : false
                );
                break;
            }
        }
        GetDlgItem(IDC_BUTTON_PREVIEW_VIDEO2)->SetWindowTextW(_T("取消预览"));
    }

    // 判断是否在发布状态，如果是也及时切换窗口
    OnBnClickedCheckActiveScreen();
}


void CRtcDemoDlg::OnBnClickedCheckDx()
{
    // TODO: Add your control notification handler code here
    OnBnClickedCheckActiveScreen();
}

void CRtcDemoDlg::OnBnClickedButtonPreviewScreen()
{
    // TODO: Add your control notification handler code here
    // 判断是否激活了屏幕录制
    if (!_rtc_video_interface) {
        return;
    }
    CString wnd_title, btn_text;
    GetDlgItem(IDC_COMBO_SCREEN)->GetWindowTextW(wnd_title);
    GetDlgItem(IDC_BUTTON_PREVIEW_VIDEO2)->GetWindowTextW(btn_text);
    int directx_state = ((CButton *)GetDlgItem(IDC_CHECK_DX))->GetCheck();

    if (btn_text.CompareNoCase(_T("预览屏幕")) == 0) {
        for (auto&& itor : _screen_wnd_map) {
            if (itor.second.title.compare(unicode2utf(wnd_title.GetBuffer())) == 0) {
                _rtc_video_interface->PreviewScreenSource(
                    itor.first,
                    GetDlgItem(IDC_STATIC_VIDEO_PREVIEW)->m_hWnd,
                    directx_state ? true : false
                );
                break;
            }
        }
        GetDlgItem(IDC_BUTTON_PREVIEW_VIDEO2)->SetWindowTextW(_T("取消预览"));
    } else {
        // 关闭屏幕共享
        _rtc_video_interface->UnPreviewScreenSource();
        GetDlgItem(IDC_BUTTON_PREVIEW_VIDEO2)->SetWindowTextW(_T("预览屏幕"));
        GetDlgItem(IDC_STATIC_VIDEO_PREVIEW)->Invalidate();
    }
}

void CRtcDemoDlg::OnBnClickedCheckDesktopAudio()
{
    // TODO: Add your control notification handler code here
    bool enable_desktop_audio_capture =
        (BST_CHECKED == ((CButton *)GetDlgItem(IDC_CHECK_DESKTOP_AUDIO))->GetCheck()) ? true : false;
    if (_rtc_audio_interface) {
        _rtc_audio_interface->MixDesktopAudio(enable_desktop_audio_capture, 0.5f);
    }
}
