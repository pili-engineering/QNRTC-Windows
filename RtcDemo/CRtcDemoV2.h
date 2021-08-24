/*!
 * \file CRtcDemoV2.h
 *
 * \author Gobert
 * \date 十一月 2018
 *
 * 对 qiniu_v2 多 Track 版本的 API 进行演示
 */

#pragma once
#include "stdafx.h"
#include <string>
#include "charactor_convert.h"
#include <list>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <map>
#include "QNVideoInterface.h"
#include "QNAudioInterface.h"
#include "CVdieoRenderWnd.h"
#include "resource.h"
#include "MergeDialog.h"
#include "MessageDialog.h"

using namespace std;

class TrackInfoUI {
public:
    TrackInfoUI(CWnd* parent_, qiniu_v2::QNTrackInfo* track_info_)
        : track_info_ptr(track_info_)
    {
        if (track_info_->GetKind().compare("audio") != 0) {
            render_wnd_ptr = new CVdieoRenderWnd(parent_);
            if (render_wnd_ptr->Create(
                IDD_DIALOG_FULL,
                parent_
            )) {
                string wnd_title = track_info_->GetUserId() + "_" 
                    + track_info_->GetTrackId() + "_" + track_info_->GetTag();
                render_wnd_ptr->SetWindowTextW(utf2unicode(wnd_title).c_str());

                // 禁用最小化按钮
                //render_wnd_ptr->ModifyStyle(WS_MINIMIZEBOX, 0, SWP_FRAMECHANGED);

                if (parent_->IsIconic()) {
                    // 如果是最小化，则将其窗口还原，否则渲染小窗口无法显示（只有悬浮的小窗口才有此问题）
                    parent_->ShowWindow(SW_SHOWNORMAL);
                }
                render_wnd_ptr->ShowWindow(SW_SHOWNORMAL);
            }
        }
    };

    virtual ~TrackInfoUI()
    {
        if (render_wnd_ptr) {
            render_wnd_ptr->DestroyWindow();
            delete render_wnd_ptr;
            render_wnd_ptr = nullptr;
        }
    }

public:
    CVdieoRenderWnd * render_wnd_ptr = nullptr;
    qiniu_v2::QNTrackInfo* track_info_ptr = nullptr;
    static UINT WINDOW_ID;
};

// CRtcDemoV2 dialog

class CRtcDemoV2 
    : public CDialogEx
    , qiniu_v2::QNRoomInterface::QNRoomListener
    , qiniu_v2::QNAudioInterface::QNAudioListener
    , qiniu_v2::QNVideoInterface::QNVideoListener
{
	DECLARE_DYNAMIC(CRtcDemoV2)

public:
	CRtcDemoV2(CWnd* main_dlg, CWnd* pParent = nullptr);   // standard constructor
	virtual ~CRtcDemoV2();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_V2 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnDestroy();
    afx_msg void OnBnClickedCancel();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnBnClickedButtonLogin();
    afx_msg void OnBnClickedButtonPreviewVideo();
    afx_msg void OnBnClickedBtnFlush();
    afx_msg void OnBnClickedButtonPreviewScreen();
    afx_msg void OnBnClickedCheckCamera();
    afx_msg void OnBnClickedCheckScreen();
    afx_msg void OnBnClickedCheckAudio();
    afx_msg void OnBnClickedCheckImportRawData();
    afx_msg void OnBnClickedCheckDesktopAudio();
    afx_msg void OnBnClickedBtnKickout();
    afx_msg void OnBnClickedCheckMuteAudio();
    afx_msg void OnBnClickedCheckMuteVideo();
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnCbnSelchangeComboMicrophone();
    afx_msg void OnCbnSelchangeComboPlayout();
    afx_msg void OnBnClickedButtonMerge();
    virtual afx_msg LRESULT OnHandleMessage(WPARAM wParam, LPARAM lParam);
    virtual afx_msg LRESULT OnSendMessage(WPARAM wParam, LPARAM lParam);

protected:
    // 房间名、用户 ID 配置文件的读取和写入
    void         ReadConfigFile();
    void         WriteConfigFile();

    void         InitUI();

    std::tuple<int, int> FindBestVideoSize(const qiniu_v2::CameraCapabilityVec& camera_cap_vec_);

    void         StartPublish();
    void         StopPublish();

    // 数据导入线程
    void         ImportExternalRawFrame(const string& track_id_);

    void         CreateCustomMergeJob();
    void         AdjustMergeStreamLayouts();
    void         AdjustSubscribeLayouts();

private:
    // 下面为 SDK 回调接口实现
    virtual void OnJoinResult(
        int error_code_,
        const string& error_str_,
        const qiniu_v2::UserInfoList& user_vec_,
        const qiniu_v2::TrackInfoList& stream_vec_,
        bool reconnect_flag_
    );

    virtual void OnLeave(int error_code_, const string& error_str_, const string& user_id_);

    virtual void OnRoomStateChange(qiniu_v2::RoomState state_);

    virtual void OnPublishTracksResult(
        int error_code_,
        const string& error_str_,
        const qiniu_v2::TrackInfoList& track_info_list_
    );

    virtual void OnSubscribeTracksResult(
        int error_code_,
        const string& error_str_,
        const qiniu_v2::TrackInfoList& track_info_list_
    );

    virtual void OnRemoteAddTracks(
        const qiniu_v2::TrackInfoList& track_list_
    );

    virtual void OnRemoteDeleteTracks(
        const list<string>& track_list_
    );

    virtual void OnRemoteUserJoin(
        const string& user_id_,
        const string& user_data_
    );

    virtual void OnRemoteUserLeave(
        const string& user_id_,
        int error_code_
    );

    virtual void OnKickoutResult(
        const std::string& kicked_out_user_id_,
        int error_code_,
        const std::string& error_str_
    );

    virtual void OnRemoteTrackMuted(
        const string& track_id_, 
        bool mute_flag_
    );

    virtual void OnStatisticsUpdated(
        const qiniu_v2::StatisticsReport& statistics_
    );

	virtual void OnReceiveMessage(
		const qiniu_v2::CustomMessageList& custom_message_
	);

    virtual void OnRemoteUserReconnecting(const std::string& remote_user_id_);

    virtual void OnRemoteUserReconnected(const std::string& remote_user_id_);

    virtual void OnTrackMute(bool server_mute_flag_, bool local_mute_flag_, qiniu_v2::QNTrackInfo& trackInfo_);

    virtual void OnUnPublishTracksResult(
        const qiniu_v2::TrackInfoList& track_list_
    );

    virtual void OnRemoteStatisticsUpdated(
        const qiniu_v2::StatisticsReportList& statistics_list_
    );

    virtual void OnCreateMergeResult(
        const std::string& job_id_,
        int error_code_,
        const std::string& error_str_
    );

    virtual void OnStopMergeResult(
        const std::string& job_id_,
        const std::string& job_iid_,
        int error_code_,
        const std::string& error_str_
    );

    virtual void OnSetSubscribeTracksProfileResult(
        int error_code_,
        const string& error_str_,
        const qiniu_v2::TrackInfoList& track_list_
    );

    virtual void OnCreateForwardResult(
        const std::string& job_id_,
        int error_code_,
        const std::string& error_str_
    );

    virtual void OnStopForwardResult(
        const std::string& job_id_,
        const std::string& job_iid_,
        int error_code_,
        const std::string& error_str_
    );

    // 音频数据回调，本地和远端的都通过此接口
    virtual void OnAudioPCMFrame(
        const unsigned char* audio_data_,
        int bits_per_sample_,
        int sample_rate_,
        size_t number_of_channels_,
        size_t number_of_frames_,
        const std::string& user_id_
    );

    // 音频设备插拔事件通知
    virtual void OnAudioDeviceStateChanged(
        qiniu_v2::AudioDeviceState device_state_,
        const std::string& device_guid_
    );

    virtual int OnPutExtraData(
        unsigned char* extra_data_,
        int extra_data_max_size_,
        const std::string& track_id_
    );

    virtual int OnSetMaxEncryptSize(
        int frame_size_,
        const std::string& track_id_
    );

    virtual int OnEncrypt(const unsigned char* frame_,
        int frame_size_,
        unsigned char* encrypted_frame_,
        const std::string& track_id_
    );

    virtual void OnGetExtraData(
        const unsigned char* extra_data_,
        int extra_data_size_,
        const std::string& track_id_
    );

    virtual int OnSetMaxDecryptSize(
        int encrypted_frame_size_,
        const std::string& track_id_
    );

    virtual int OnDecrypt(
        const unsigned char* encrypted_frame_,
        int encrypted_size_,
        unsigned char* frame_,
        const std::string& track_id_
    );

    virtual void OnVideoDeviceStateChanged(
        qiniu_v2::VideoDeviceState device_state_, 
        const std::string& device_name_
    );
    virtual void OnVideoFrame(
        const unsigned char* raw_data_, 
        int data_len_, 
        int width_, 
        int height_, 
        qiniu_v2::VideoCaptureType video_type_, 
        const std::string& track_id_, 
        const std::string& user_id_
    );
    virtual void OnVideoFramePreview(
        const unsigned char* raw_data_, 
        int data_len_, 
        int width_, 
        int height_, 
        qiniu_v2::VideoCaptureType video_type_
    );

protected:
    CWnd *                       _main_dlg_ptr        = nullptr;
    CRichEditCtrl                _msg_rich_edit_ctrl;
    CListCtrl                    _user_list_ctrl;
    CString                      _app_id;
    CString                      _room_name;
    CString                      _user_id;
    CStatusBarCtrl               _wnd_status_bar;
    string                       _room_token;
    bool                         _contain_admin_flag;
    bool                         _contain_forward_flag = false;
    qiniu_v2::QNRoomInterface*   _rtc_room_interface  = nullptr;
    qiniu_v2::QNVideoInterface*  _rtc_video_interface = nullptr;
    qiniu_v2::QNAudioInterface*  _rtc_audio_interface = nullptr;
    list<std::function<void()>>  _call_function_list;
    recursive_mutex              _mutex;
    chrono::time_point<chrono::steady_clock> _start_tp;
    map<string, qiniu_v2::CameraDeviceInfo> _camera_dev_map;
    map<int, qiniu_v2::AudioDeviceInfo>     _microphone_dev_map;
    map<int, qiniu_v2::AudioDeviceInfo>     _playout_dev_map;
    map<int, qiniu_v2::ScreenWindowInfo>    _screen_info_map;
    qiniu_v2::TrackInfoList      _local_tracks_list;
    map<string, shared_ptr<TrackInfoUI>> _remote_tracks_map;
    list<string>                 _user_list;
    bool                         _stop_external_flag = false;
    MergeDialog::MergeConfig     _merge_config;
    std::string                  _custom_merge_id;
    std::string                  _custom_forward_id;
    thread                       _fake_video_thread;
    thread                       _fake_audio_thread;
    MessageDialog                _dlg_msg;
    bool                         _enable_simulcast = false;
    bool                         _forward_video_flag = false;
    bool                         _forward_audio_flag = false;
    bool                         _enable_encryptor_decryptor = false;
    bool                         _show_extra = true;
    qiniu_v2::TrackSourceType    _src_capturer_source = qiniu_v2::tst_Camera;
public:
    afx_msg void OnBnClickedButtonSendMsg();
    afx_msg void OnCbnSelchangeComboLocalRotate();
    afx_msg void OnCbnSelchangeComboRemoteRotate();
    afx_msg void OnCbnSelchangeComboSubscribeProfile();
    afx_msg void OnBnClickedButtonSimulcast();
    afx_msg void OnBnClickedButtonForward();
    afx_msg void OnBnClickedButtonExtraData();
    afx_msg void OnBnClickedCheckCameraImage();
    afx_msg void OnBnClickedCheckCameraMirror();
    afx_msg void OnBnClickedBtnSei();
};
