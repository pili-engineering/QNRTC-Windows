/*!
 * \file CRtcDemoV2.h
 *
 * \author Gobert
 * \date 十一月 2018
 *
 * 对 qiniu_v2 多 Track 版本的 API 进行演示
 */

#pragma once
#include <chrono>
#include <condition_variable>
#include <list>
#include <map>
#include <mutex>
#include <string>
#include "CVdieoRenderWnd.h"
#include "MergeDialog.h"
#include "MessageDialog.h"
#include "StatsDialog.h"
#include "charactor_convert.h"
#include "qn_rtc_client_interface.h"
#include "qn_rtc_interface.h"
#include "qn_track_interface.h"
#include "resource.h"
#include "stdafx.h"

using namespace std;
using namespace qiniu;
class TrackInfoUI {
 public:
  TrackInfoUI(CWnd* parent, QNRemoteTrack* track_info)
      : track_info_ptr_(track_info) {
    if (track_info->IsVideo()) {
      render_wnd_ptr_ = new CVdieoRenderWnd(parent);
      if (render_wnd_ptr_->Create(IDD_DIALOG_FULL, parent)) {
        string wnd_title = track_info->GetUserID() + "_" +
                           track_info->GetTrackID() + "_" +
                           track_info->GetTag();
        render_wnd_ptr_->SetWindowTextW(utf2unicode(wnd_title).c_str());

        // 禁用最小化按钮
        // render_wnd_ptr->ModifyStyle(WS_MINIMIZEBOX, 0, SWP_FRAMECHANGED);

        if (parent->IsIconic()) {
          // 如果是最小化，则将其窗口还原，否则渲染小窗口无法显示（只有悬浮的小窗口才有此问题）
          parent->ShowWindow(SW_SHOWNORMAL);
        }
        render_wnd_ptr_->ShowWindow(SW_SHOWNORMAL);
      }
    }
  };

  virtual ~TrackInfoUI() {
    if (render_wnd_ptr_) {
      render_wnd_ptr_->DestroyWindow();
      delete render_wnd_ptr_;
      render_wnd_ptr_ = nullptr;
    }
  }

 public:
  CVdieoRenderWnd* render_wnd_ptr_ = nullptr;
  QNRemoteTrack* track_info_ptr_ = nullptr;
  static UINT WINDOW_ID;
};

// CRtcDemoV2 dialog

class CRtcDemoV2 : public CDialogEx,
                   public QNClientEventListener,
                   public QNRTCEventListener,
                   public QNPublishResultCallback,
                   public QNTrackInfoChangedListener,
                   public QNLiveStreamingListener,
                   public QNCameraEventListener {
  DECLARE_DYNAMIC(CRtcDemoV2)

 public:
  CRtcDemoV2(CWnd* main_dlg, CWnd* pParent = nullptr);  // standard constructor
  virtual ~CRtcDemoV2();
// Dialog Data
#ifdef AFX_DESIGN_TIME
  enum { IDD = IDD_DIALOG_V2 };
#endif

  virtual void DoDataExchange(CDataExchange* pDX);  // DDX/DDV support
  virtual BOOL OnInitDialog();

  DECLARE_MESSAGE_MAP()

 private:
  // 房间名、用户 ID 配置文件的读取和写入
  void ReadConfigFile();
  void WriteConfigFile();

  void InitUI();
  std::tuple<int, int> FindBestVideoSize(
      const qiniu::CameraCapabilityVec& camera_cap_vec_);
  void StartPublish();

  // 数据导入线程
  void ImportExternalVideoRawFrame();
  void ImportExternalAudioRawFrame();

  void AdjustSubscribeLayouts();
  void CreateCustomMergeJob(bool open);
  void AdjustMergeStreamLayouts();
  void AdjustStatsLayouts();

 public:
  // implements of QNClientEventListener
  virtual void OnConnectionStateChanged(
      QNConnectionState state, const QNConnectionDisconnectedInfo* info);
  virtual void OnUserJoined(const std::string& remote_user_id,
                            const std::string& user_data);
  virtual void OnUserLeft(const std::string& remote_user_id);
  virtual void OnUserReconnecting(const std::string& remote_user_id);
  virtual void OnUserReconnected(const std::string& remote_user_id);
  virtual void OnUserPublished(const std::string& remote_user_id,
                               const RemoteTrackList& track_list);
  virtual void OnUserUnpublished(const std::string& remote_user_id,
                                 const RemoteTrackList& track_list);
  virtual void OnSubscribed(
      const std::string& remote_user_id,
      const RemoteAudioTrackList& remote_audio_track_list,
      const RemoteVideoTrackList& remote_video_track_list);
  virtual void OnMessageReceived(const QNCustomMessage& message);
  virtual void OnMediaRelayStateChanged(const std::string& relay_room,
                                        const QNMediaRelayState state);

  // implements of QNPublishResultCallback
  virtual void OnPublished();
  virtual void OnPublishError(int error_code, const std::string& error_message);

  // implements of QNRTCEventListener
  virtual void OnVideoDeviceStateChanged(QNVideoDeviceState device_state,
                                         const std::string& device_name);

  virtual void OnAudioDeviceStateChanged(QNAudioDeviceState device_state,
                                         const std::string& device_guid);

  // implements of QNTrackInfoChangedListener
  virtual void OnVideoProfileChanged(const std::string& user_id,
                                     const std::string& track_id,
                                     QNTrackProfile profile);

  virtual void OnMuteStateChanged(const std::string& user_id,
                                  const std::string& track_id, bool muted);

  // implements of QNLiveStreamingListener
  virtual void OnStarted(const std::string& stream_id);
  virtual void OnStopped(const std::string& stream_id);
  virtual void OnTranscodingTracksUpdated(const std::string& stream_id);
  virtual void OnLiveStreamingError(
      const std::string& stream_id,
      const QNLiveStreamingErrorInfo& error_info);

  // implements of QNCameraEventListener
  virtual void OnCameraError(int error_code, const std::string& error_message);

 protected:
  QNRTCClient* rtc_client_ptr_ = nullptr;
  QNCameraVideoTrack* camera_track_ptr_ = nullptr;
  QNScreenVideoTrack* screen_track_ptr_ = nullptr;
  QNCustomVideoTrack* custom_video_track_ptr_ = nullptr;
  QNMicrophoneAudioTrack* microphone_audio_track_ptr_ = nullptr;
  QNCustomAudioTrack* custom_audio_track_ptr_ = nullptr;
  CWnd* main_dlg_ptr_ = nullptr;
  CListCtrl user_list_ctrl_;
  CString app_id_;
  CString room_name_;
  CString user_id_;
  CStatusBarCtrl wnd_status_bar_;
  string room_token_;
  bool is_joined_ = false;
  bool contain_admin_flag_;
  recursive_mutex mutex_;
  chrono::time_point<chrono::steady_clock> start_tp_;
  map<string, qiniu::QNCameraInfo> camera_dev_map_;
  map<int, qiniu::QNAudioDeviceInfo> microphone_dev_map_;
  map<int, qiniu::QNAudioDeviceInfo> playout_dev_map_;
  map<string, qiniu::QNScreenWindowInfo> screen_info_map_;
  list<string> user_list_;
  bool stop_video_external_flag_ = true;
  bool stop_audio_external_flag_ = true;
  MergeDialog::MergeConfig merge_config_;
  std::string custom_merge_id_;
  std::string custom_forward_id_;
  thread fake_video_thread_;
  thread fake_audio_thread_;
  thread stats_thread_;
  MessageDialog dlg_msg_;
  bool enable_simulcast_ = false;
  bool stop_stats_flag_ = true;
  map<string, shared_ptr<TrackInfoUI>> remote_tracks_map_;
  LocalTrackList local_published_track_list_;
  StatsDialog* dlg_stats_ = nullptr;
  bool destory_main_dialog_ = false;
  bool merge_flag_ = false;
  bool merge_local_audio_flag_ = false;
  bool merge_local_video_flag_ = false;
  bool merge_remote_audio_flag_ = false;
  bool merge_remote_video_flag_ = false;
  qiniu::QNTranscodeingTrackList remote_transcoding_tracks_list_;

 public:
  afx_msg LRESULT RenderRemoteVideoFrame(WPARAM wParam, LPARAM lParam);
  afx_msg LRESULT OnHandleMergeMessage(WPARAM wParam, LPARAM lParam);
  afx_msg LRESULT OnSendMessage(WPARAM wParam, LPARAM lParam);
  afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
  afx_msg void OnTimer(UINT_PTR nIDEvent);
  afx_msg void OnDestroy();
  afx_msg void OnBnClickedCancel();
  afx_msg void OnBnClickedButtonLogin();
  afx_msg void OnBnClickedBtnFlush();
  afx_msg void OnBnClickedBtnFlushCamera();
  afx_msg void OnBnClickedCheckCamera();
  afx_msg void OnBnClickedCheckScreen();
  afx_msg void OnBnClickedCheckMicrophone();
  afx_msg void OnBnClickedCheckImportRawVideoData();
  afx_msg void OnBnClickedCheckImportRawAudioData();
  afx_msg void OnBnClickedCheckDesktopAudio();
  afx_msg void OnBnClickedCheckMuteAudio();
  afx_msg void OnBnClickedCheckMuteVideo();
  afx_msg void OnCbnSelchangeComboMicrophone();
  afx_msg void OnCbnSelchangeComboPlayout();
  afx_msg void OnBnClickedButtonMerge();
  afx_msg void OnBnClickedButtonSendMsg();
  afx_msg void OnCbnSelchangeComboLocalRotate();
  afx_msg void OnCbnSelchangeComboRemoteRotate();
  afx_msg void OnCbnSelchangeComboSubscribeProfile();
  afx_msg void OnBnClickedButtonSimulcast();
  afx_msg void OnBnClickedButtonForward();
  afx_msg void OnBnClickedCheckCameraImage();
  afx_msg void OnBnClickedCheckCameraMirror();
  afx_msg void OnBnClickedBtnSei();
  afx_msg void OnBnClickedCheckLocalMirror();
  afx_msg void OnBnClickedCheckRemoteMirror();
  afx_msg void OnCbnSelchangeComboLocalStretchMode();
  afx_msg void OnCbnSelchangeComboRemoteStretchMode();
  afx_msg void OnBnClickedCheckClip();
  afx_msg void OnBnClickedCheckScale();
  afx_msg void OnBnClickedCheckMerge();
  afx_msg void OnBnClickedCheckImportStats();
  afx_msg void OnBnClickedBtnKickout();
  afx_msg void OnBnClickedBtnSubscribe();
};
