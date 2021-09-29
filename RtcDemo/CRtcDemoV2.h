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
#include "QNTrackInterface.h"
#include "QNRTCClientInterface.h"
#include "QNRTCInterface.h"
#include "CVdieoRenderWnd.h"
#include "resource.h"
#include "MergeDialog.h"
#include "MessageDialog.h"
#include "StatsDialog.h"

using namespace std;
using namespace qiniu;
class TrackInfoUI {
public:
    TrackInfoUI(CWnd* parent_, shared_ptr<QNRemoteTrack> track_info_)
        : track_info_ptr(track_info_)
    {
        if (track_info_->GetKind().compare("audio") != 0) {
            render_wnd_ptr = new CVdieoRenderWnd(parent_);
            if (render_wnd_ptr->Create(
                IDD_DIALOG_FULL,
                parent_
            )) {
                string wnd_title = track_info_->GetUserID() + "_" 
                    + track_info_->GetTrackID() + "_" + track_info_->GetTag();
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
    shared_ptr<QNRemoteTrack> track_info_ptr = nullptr;
    static UINT WINDOW_ID;
};

// CRtcDemoV2 dialog

class CRtcDemoV2 
    : public CDialogEx
    , qiniu::QNLocalVideoFrameListener
    , qiniu::QNRemoteVideoFrameListener
    , qiniu::QNLocalAudioFrameListener
    , qiniu::QNRemoteAudioFrameListener
    , qiniu::QNRemoteAudioMixedFrameListener
    , qiniu::QNTrackInfoChangedListener
    , qiniu::QNPublishResultCallback
    , qiniu::QNClientEventListener
    , qiniu::QNLiveStreamingListener
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

    std::tuple<int, int> FindBestVideoSize(const qiniu::CameraCapabilityVec& camera_cap_vec_);

    void         StartPublish();
    void         StopPublish();

    // 数据导入线程
    void         ImportExternalVideoRawFrame();
    void         ImportExternalAudioRawFrame();

    void         CreateCustomMergeJob(bool open);
    void         AdjustMergeStreamLayouts();
    void         AdjustSubscribeLayouts();
    void         AdjustStatsLayouts();
    void         Leave();

private:
    virtual void OnLocalAudioPCMFrame(
        const unsigned char* data,
        int bitsPerSample,
        int sampleRrate,
        size_t channels,
        size_t samplePoints
    );

    virtual void OnLocalVideoFrame(
        const string& trackId,
        const unsigned char* data,
        int dataSize,
        int width,
        int height,
        QNVideoSourceType videoType
    );

    virtual void OnRemoteAudioFrame(
        const string& userId,
        const unsigned char* data,
        int bitsPerSample,
        int sampleRrate,
        size_t channels,
        size_t samplePoints
    );

    virtual void OnRemoteVideoFrame(
        const std::string& userId,
        const string& trackId,
        const unsigned char* data,
        int dataSize,
        int width,
        int height,
        qiniu::QNVideoSourceType videoType
    );

    virtual void OnPreviewVideoFrame(
        const unsigned char* data,
        int dataSize,
        int width,
        int height,
        QNVideoSourceType videoType
    );

    virtual void OnRemoteMixAudioPCMFrame(
        const unsigned char* data,
        int bitsPerSample,
        int sampleRrate,
        size_t channels,
        size_t samplePoints
    );

    virtual void OnVideoProfileChanged(const std::string& trackid, qiniu::QNTrackProfile profile);

    virtual void OnVideoDeviceStateChanged(qiniu::QNVideoDeviceState deviceState, const std::string& deviceName);

    virtual void OnAudioDeviceStateChanged(qiniu::QNAudioDeviceState deviceState, const std::string& deviceGuid);

    virtual void OnPublished();
    virtual void OnPublishError(int errorCode, const std::string& errorMessage);

    virtual void OnConnectionStateChanged(qiniu::QNConnectionState state, const qiniu::QNConnectionDisconnectedInfo& info);

    virtual void OnUserJoined(const std::string& remoteUserID, const std::string& userData);

    virtual void OnUserLeft(const std::string& remoteUserID);

    virtual void OnUserReconnecting(const std::string& remoteUserID);

    virtual void OnUserReconnected(const std::string& remoteUserID);

    virtual void OnUserPublished(const std::string& remoteUserID, const qiniu::RemoteTrackList& trackList);

    virtual void OnUserUnpublished(const std::string& remoteUserID, const qiniu::RemoteTrackList& trackList);

    virtual void OnSubscribed(
        const std::string& remoteUserID,
        const qiniu::RemoteAudioTrackList& remoteAudioTracks,
        const qiniu::RemoteVideoTrackList& remoteVideoTracks
    );

    virtual void OnMessageReceived(const qiniu::CustomMessageList& message);

    virtual void OnMuteStateChanged(bool isMuted, const std::string& remoteUserId, const qiniu::RemoteTrackList& trackList);

    virtual void OnStarted(const std::string& streamID);
    virtual void OnStopped(const std::string& streamID);
    virtual void OnTranscodingTracksUpdated(const std::string& streamID);
    virtual void OnError(const std::string& streamID, const QNLiveStreamingErrorInfo& errorInfo);

protected:
    QNRTCClient*                 _rtc_client_ptr = nullptr;
    QNCameraVideoTrack*          _camera_track_ptr = nullptr;
    QNScreenVideoTrack*          _screen_track_ptr = nullptr;
    QNCustomVideoTrack*          _custom_video_track_ptr = nullptr;
    QNMicrophoneAudioTrack*      _microphone_audio_track_ptr = nullptr;
    QNCustomAudioTrack*          _custom_audio_track_ptr = nullptr;
    CWnd *                       _main_dlg_ptr        = nullptr;
    CListCtrl                    _user_list_ctrl;
    CString                      _app_id;
    CString                      _room_name;
    CString                      _user_id;
    CStatusBarCtrl               _wnd_status_bar;
    string                       _room_token;
    bool                         _contain_admin_flag;
    bool                         _contain_forward_flag = false;
    list<std::function<void()>>  _call_function_list;
    recursive_mutex              _mutex;
    chrono::time_point<chrono::steady_clock> _start_tp;
    map<string, qiniu::QNCameraDeviceInfo> _camera_dev_map;
    map<int, qiniu::QNAudioDeviceInfo>     _microphone_dev_map;
    map<int, qiniu::QNAudioDeviceInfo>     _playout_dev_map;
    map<int, qiniu::QNScreenWindowInfo>    _screen_info_map;
    list<string>                 _user_list;
    bool                         _stop_video_external_flag = true;
    bool                         _stop_audio_external_flag = true;
    MergeDialog::MergeConfig     _merge_config;
    std::string                  _custom_merge_id;
    std::string                  _custom_forward_id;
    thread                       _fake_video_thread;
    thread                       _fake_audio_thread;
    thread                       _stats_thread;
    MessageDialog                _dlg_msg;
    bool                         _enable_simulcast = false;
    bool                         _forward_video_flag = false;
    bool                         _forward_audio_flag = false;
    bool                         _stop_stats_flag = true;
    map<string, shared_ptr<TrackInfoUI>> _remote_tracks_map;
    LocalTrackList               _local_published_track_list;
    StatsDialog*                 _stats_pDig = nullptr;
public:
    afx_msg void OnBnClickedButtonSendMsg();
    afx_msg void OnCbnSelchangeComboLocalRotate();
    afx_msg void OnCbnSelchangeComboRemoteRotate();
    afx_msg void OnCbnSelchangeComboSubscribeProfile();
    afx_msg void OnBnClickedButtonSimulcast();
    afx_msg void OnBnClickedButtonForward();
    afx_msg void OnBnClickedCheckCameraImage();
    afx_msg void OnBnClickedCheckCameraMirror();
    afx_msg void OnBnClickedBtnSei();
    afx_msg void OnBnClickedCheckImportRawAudioData();
    afx_msg void OnBnClickedCheckLocalMirror();
    afx_msg void OnBnClickedCheckRemoteMirror();
    afx_msg void OnCbnSelchangeComboLocalStretchMode();
    afx_msg void OnCbnSelchangeComboRemoteStretchMode();
    afx_msg void OnBnClickedCheckClip();
    afx_msg void OnBnClickedCheckScale();
    afx_msg void OnBnClickedCheckMerge();
    afx_msg void OnBnClickedCheckImportStats();
    afx_msg void OnBnClickedCheckHardEncoder();
    afx_msg void OnBnClickedBtnKickout();
};
