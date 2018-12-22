// RtcDemoDlg.h : header file
// 对 qiniu 单 Track 版本的 API 进行演示
#pragma once
#include <map>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <chrono>
#include "qn_rtc_room.h"
#include "qn_rtc_video.h"
#include "qn_rtc_audio.h"
#include "qn_rtc_errorcode.h"
#include "afxcmn.h"
#include "CVdieoRenderWnd.h"

using namespace std;
using namespace qiniu;

class CRtcDemoV2;

// CRtcDemoDlg dialog
class CRtcDemoDlg 
    : public CDialogEx
    , QNRTCRoom ::QNRTCRoomListener
    , QNRTCAudio::QNRTCAudioListener
    , QNRTCVideo::QNRTCVideoListener
{
// Construction
public:
    CRtcDemoDlg(CWnd* pParent = NULL);                  // standard constructor

                                                        // Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_RTCDEMO_DIALOG };
#endif

    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
    // Implementation QNRTCRoomListener
    void OnStateChanged(RoomState status_) override;
    void OnJoinResult(int error_code_, const std::string& error_str_,
        const UserDataInfoVec& user_data_vec_) override;
    void OnLeave(int error_code_, 
        const std::string& error_str_, const std::string& kicked_user_id_) override;
    void OnRemoteUserJoin(const std::string& user_id_, const std::string& user_data_) override;
    void OnRemoteUserLeave(const std::string& user_id_, int error_code_) override;
    void OnRemotePublish(const std::string& user_id_, bool has_audio_, bool has_video_) override;
    void OnRemoteUnPublish(const std::string& user_id_) override;
    void OnLocalPublishResult(int error_code_, const std::string& error_str_) override;
    void OnSubscribeResult(const std::string& user_id_,
        int error_code_, const std::string& error_str_) override;
    void OnKickoutResult(const std::string& user_id_, 
        int error_code_, const std::string& error_str_) override;
    void OnRemoteStreamMute(const std::string& user_id_, 
        bool mute_audio_, bool mute_video_) override;
    void OnError(int error_code_, const std::string& error_str_) override;
    void OnStatisticsUpdated(const StatisticsReport& statistics_) override;

    // Implementation QNRTCAudioListener
    void OnAudioPCMFrame(
        const unsigned char* audio_data_, int bits_per_sample_,
        int sample_rate_, size_t number_of_channels_,
        size_t number_of_frames_, const std::string& user_id_) override;
    void OnAudioDeviceStateChanged(
        AudioDeviceState new_device_state_, const std::string& device_guid_) override;

    // Implementation QNRTCVideoListener
    void OnVideoFrame(const unsigned char* raw_data_, int data_len_, 
        int width_, int height_, qiniu::VideoCaptureType video_type_, const std::string& user_id_
    ) override;
    void OnVideoFramePreview(const unsigned char* raw_data_, int data_len_,
        int width_, int height_, qiniu::VideoCaptureType video_type_);
    void OnVideoDeviceStateChanged(
        VideoDeviceState new_device_state_, const std::string& device_id_) override;

protected:
    // Generated message map functions
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()
    afx_msg void OnBnClickedButtonJoin();
    afx_msg void OnBnClickedButtonPublish();
    afx_msg void OnDestroy();
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnCbnSelchangeComboCamera();
    afx_msg void OnCbnSelchangeComboMicrophone();
    afx_msg void OnBnClickedButtonPreview();
    afx_msg void OnBnClickedBtnKickout();
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnBnClickedCheckMuteAudio();
    afx_msg void OnBnClickedCheckMuteVideo();
    afx_msg void OnCbnSelchangeComboPlayout();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnBnClickedBtnFlush();
    afx_msg void OnBnClickedCheckActiveScreen();
    afx_msg void OnCbnSelchangeComboScreen();
    afx_msg void OnBnClickedCheckDx();
    afx_msg void OnBnClickedButtonPreviewScreen();
    afx_msg void OnBnClickedCheckImportRawData();
    afx_msg void OnBnClickedCheckDesktopAudio();
    afx_msg void OnBnClickedButtonV2();

    virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
    // 从 IDC_STATIC_PLAY 窗口获取一个闲置的区域进行视频渲染
    CRect&&      GetRenderWndPos();

    // 自动调整渲染窗口位置
    void         AdjustRenderWndPos();

    std::tuple<int, int> FindBestVideoSize(const CameraCapabilityVec& camera_cap_vec_);

    // 每次远端用户发布，或取消发布后，重新调整下合流参数的配置
    void         AdjustMergeStreamPosition();

    // 房间名、用户 ID 配置文件的读取和写入
    void         ReadConfigFile();
    void         WriteConfigFile();

    // 初始化界面下方的状态条
    void         InitDemoUI();

    // 导入外部音视频数据，注：数据源需要自己准备 
    void         ImportExternalRawFrame();

    void         InsertMsgEditText(LPCTSTR msg_);

    std::function<void()> GetFunc();

private:
    typedef struct _TUserStreamInfo
    {
        string user_id;
        bool   audio_published = false;
        bool   video_published = false;
        bool   audio_mute      = false;
        bool   video_mute      = false;
        bool   is_subscribed   = false;   //是否已经被订阅 
        shared_ptr<CStatic> render_wnd_ptr   = nullptr;
        shared_ptr<CStatic> display_name_ptr = nullptr;
        shared_ptr<CProgressCtrl> volume_ptr = nullptr;
    }UserStreamInfo;

    const int Canvas_Width  = 480;        // 合流的默认画布宽度，可由服务端进行配置 
    const int Canvas_Height = 848;        // 合流的默认画布高度，可由服务端进行配置 

    HICON                           m_hIcon;
    CStatusBarCtrl                  _wndStatusBar;
    QNRTCRoom*                      _rtc_room_interface  = nullptr;
    QNRTCVideo*                     _rtc_video_interface = nullptr;
    QNRTCAudio*                     _rtc_audio_interface = nullptr;
    CListCtrl                       _user_list_ctrl;
    unordered_map<string, UserStreamInfo>     _user_stream_map;// key:user id; value:struct PlayerStreamInfo
    recursive_mutex                 _mutex;
    unordered_map<string, CameraDeviceInfo>   _camera_dev_map; // key:device id; value:CameraDeviceInfo
    map<string, string>             _recording_dev_map;        // key:device id; value:device name
    map<string, string>             _playout_dev_map;          // key:device id; value:device name
    bool                            _publish_flag        = false;
    thread                          _join_room_thread;         // temporary thread handle that asynchronously join room
    CString                         _app_id;
    CString                         _room_name;
    CString                         _user_id;
    string                          _room_token;
    int                             _resource_id         = 0;
    list<std::function<void()>>     _call_function_vec;
    bool                            _contain_admin_flag  = false;
    chrono::time_point<chrono::system_clock> _start_time;      // joined room time point
    thread                          _fake_video_thread;
    thread                          _fake_audio_thread;
    atomic_bool                     _stop_fake_flag      = false;
    unordered_map<int, ScreenWindowInfo>      
                                    _screen_wnd_map;           // screen windows map, key:source id
    CRichEditCtrl                   _msg_rich_edit_ctrl;       // 系统消息展示控件
    CProgressCtrl                   _local_volume_progress;    // 用于展示本地麦克风音量条
    shared_ptr<CVdieoRenderWnd>      _full_dlg_ptr        = nullptr;
    CRtcDemoV2*                     _rtc_demo_dlg_v2     = nullptr;
};
