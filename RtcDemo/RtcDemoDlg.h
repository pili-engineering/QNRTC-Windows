// RtcDemoDlg.h : header file
//
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

using namespace std;
using namespace qiniu;

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
        const void* audio_data_, int bits_per_sample_,
        int sample_rate_, size_t number_of_channels_,
        size_t number_of_frames_, const std::string& user_id_) override;
    void OnAudioDeviceStateChanged(
        AudioDeviceState new_device_state_, const std::string& device_guid_) override;

    // Implementation QNRTCVideoListener
    void OnVideoFrame(const unsigned char* raw_data_, 
        int data_len_, qiniu::VideoCaptureType video_type_, const std::string& user_id_) override;
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

    virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
    // 从 IDC_STATIC_PLAY 窗口获取一个闲置的区域进行视频渲染
    CRect&&      GetRenderWndPos();

    // 自动调整渲染窗口位置
    void         AdjustRenderWndPos();

    // 从 AppServer 获取 AK、SK 签算后的 RoomToken
    int          GetRoomToken(const string room_name_, const string user_id_, string& token_);

    std::tuple<int, int> FindBestVideoSize(const CameraCapabilityVec& camera_cap_vec_);

    // 每次远端用户发布，或取消发布后，重新调整下合流参数的配置
    void        AdjustMergeStreamPosition();

    // 房间名、用户 ID 配置文件的读取和写入
    void        ReadConfigFile();
    void        WriteConfigFile();

    // 初始化界面下方的状态条
    void        InitStatusBar();

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
    CString                         _room_name;
    CString                         _user_id;
    string                          _room_token;
    int                             _resource_id         = 0;
    list<std::function<void()>>     _call_function_vec;
    bool                            _contain_admin_flag  = false;
    chrono::time_point<chrono::system_clock> _start_time;      // joined room time point
};
