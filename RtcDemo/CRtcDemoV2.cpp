// CRtcDemoV2.cpp : implementation file
//
#include "stdafx.h"
#include "CRtcDemoV2.h"
#include "afxdialogex.h"
#include "resource.h"
#include <fstream>
#include "Global.h"
#include "qn_error_code.h"

#define UPDATE_TIME_DURATION_TIMER 10002  // 定时更新连麦时长
UINT TrackInfoUI::WINDOW_ID = 1000000;
#define WM_ADD_SIG_DLG_PRIV_MSG (WM_USER + 1000)
#define WM_ADD_REMOTE_VIDEO_RENDER_MSG (WM_ADD_SIG_DLG_PRIV_MSG + 1)
#define WM_REMOVE_REMOTE_VIDEO_RENDER_MSG (WM_ADD_SIG_DLG_PRIV_MSG + 2)
#define WM_REMOTE_VIDEO_RENDER_MODIFY_ROTATE_MSG (WM_ADD_SIG_DLG_PRIV_MSG + 3)
#define WM_REMOTE_VIDEO_RENDER_MODIFY_STRETCH_MSG (WM_ADD_SIG_DLG_PRIV_MSG + 4)
#define WM_REMOTE_VIDEO_RENDER_MODIFY_MIRROR_MSG (WM_ADD_SIG_DLG_PRIV_MSG + 5)

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

#define VOLUMEMAX 32767
#define VOLUMEMIN -32768
#define kQNMixVolume 87.2984313

#ifndef core_min
#define core_min(a, b) ((a) < (b) ? (a) : (b))
#endif

// 获取roomtoken,JoinRoom的第一个输入参数是此方法返回的token_。
// @param app_id_ 获取token所需打点appid
// @param room_name_  房间名
// @param user_id_  房间ID
// @param time_out_ 获取超时时间
// @param token_ 输出的roomtoken
// @return 返回0时表示成功，-1表示失败，-3表示获取超时
extern "C" QINIU_EXPORT_DLL int GetRoomToken_s(const std::string &app_id_,
                                               const std::string &room_name_,
                                               const std::string &user_id_,
                                               const std::string &host_name_,
                                               const int time_out_,
                                               std::string &token_);

// 获取音频分贝值
static float ProcessAudioLevel(const int16_t *data,
                                  const int32_t &data_size) {
  if (data_size == 0) {
    return 0.0;
  }

  long long pcmAllLenght = 0;
  // 将 buffer 内容取出，进行平方和运算
  for (int i = 0; i < data_size; i++) {
    pcmAllLenght += data[i] * data[i];
  }
  // 平方和除以数据总长度，得到音量大小。
  float mean = pcmAllLenght / (double)data_size;
  float volume = 0.0;
  if (mean != 0) {
    volume = 10 * log10(mean);
  }

  return abs(volume) / kQNMixVolume;
}

CRtcDemoV2::CRtcDemoV2(CWnd *main_dlg, CWnd *pParent /*=nullptr*/)
    : CDialogEx(IDD_DIALOG_V2, pParent), main_dlg_ptr_(main_dlg) {}

CRtcDemoV2::~CRtcDemoV2() {}

void CRtcDemoV2::DoDataExchange(CDataExchange *pDX) {
  CDialogEx::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_LIST_PLAYER, user_list_ctrl_);
}

BOOL CRtcDemoV2::OnInitDialog() {
  CDialogEx::OnInitDialog();

  ReadConfigFile();
  std::string ver;
  qiniu::QNRTC::GetVersion(ver);
  TRACE("Sdk version: %s", ver.c_str());

  QNRTC::SetLogFile(qiniu::kLogInfo, "rtc_log", "rtc.log");

  QNRTCSetting setting;
  setting.policy = kForceUDP;
  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_TCP))->GetCheck()) {
    setting.policy = kForceTCP;
  }
  setting.encoder_type = kEncodeOpenH264;
  QNRTC::Init(setting, this);

  rtc_client_ptr_ = QNRTC::CreateClient(this);
  rtc_client_ptr_->SetLiveStreamingListener(this);
  InitUI();
  return TRUE;
}

BEGIN_MESSAGE_MAP(CRtcDemoV2, CDialogEx)
ON_WM_DESTROY()
ON_BN_CLICKED(IDCANCEL, &CRtcDemoV2::OnBnClickedCancel)
ON_WM_CREATE()
ON_WM_TIMER()
ON_MESSAGE(WM_ADD_SIG_DLG_PRIV_MSG, RenderRemoteVideoFrame)
ON_MESSAGE(MERGE_MESSAGE_ID, &CRtcDemoV2::OnHandleMergeMessage)
ON_MESSAGE(SEND_MESSAGE_ID, &CRtcDemoV2::OnSendMessage)
ON_BN_CLICKED(IDC_BUTTON_LOGIN, &CRtcDemoV2::OnBnClickedButtonLogin)
ON_BN_CLICKED(IDC_BTN_FLUSH, &CRtcDemoV2::OnBnClickedBtnFlush)
ON_BN_CLICKED(IDC_CHECK_CAMERA, &CRtcDemoV2::OnBnClickedCheckCamera)
ON_BN_CLICKED(IDC_CHECK_SCREEN, &CRtcDemoV2::OnBnClickedCheckScreen)
ON_BN_CLICKED(IDC_CHECK_AUDIO, &CRtcDemoV2::OnBnClickedCheckMicrophone)
ON_BN_CLICKED(IDC_CHECK_IMPORT_RAW_DATA,
              &CRtcDemoV2::OnBnClickedCheckImportRawVideoData)
ON_BN_CLICKED(IDC_CHECK_DESKTOP_AUDIO,
              &CRtcDemoV2::OnBnClickedCheckDesktopAudio)
ON_BN_CLICKED(IDC_CHECK_MUTE_AUDIO, &CRtcDemoV2::OnBnClickedCheckMuteAudio)
ON_BN_CLICKED(IDC_CHECK_MUTE_VIDEO, &CRtcDemoV2::OnBnClickedCheckMuteVideo)
ON_WM_HSCROLL()
ON_CBN_SELCHANGE(IDC_COMBO_MICROPHONE,
                 &CRtcDemoV2::OnCbnSelchangeComboMicrophone)
ON_CBN_SELCHANGE(IDC_COMBO_PLAYOUT, &CRtcDemoV2::OnCbnSelchangeComboPlayout)
ON_BN_CLICKED(IDC_BUTTON_MERGE, &CRtcDemoV2::OnBnClickedButtonMerge)
ON_BN_CLICKED(IDC_BUTTON_SEND_MSG, &CRtcDemoV2::OnBnClickedButtonSendMsg)
ON_CBN_SELCHANGE(IDC_COMBO_LOCAL_ROTATION,
                 &CRtcDemoV2::OnCbnSelchangeComboLocalRotate)
ON_CBN_SELCHANGE(IDC_COMBO_REMOTE_ROTATION,
                 &CRtcDemoV2::OnCbnSelchangeComboRemoteRotate)
ON_CBN_SELCHANGE(IDC_COMBO_SUBSCRIBE_PROFILE,
                 &CRtcDemoV2::OnCbnSelchangeComboSubscribeProfile)
ON_BN_CLICKED(IDC_BUTTON_SIMULCAST, &CRtcDemoV2::OnBnClickedButtonSimulcast)
ON_BN_CLICKED(IDC_BUTTON_FORWARD, &CRtcDemoV2::OnBnClickedButtonForward)
ON_BN_CLICKED(IDC_CHECK_CAMERA_IMAGE, &CRtcDemoV2::OnBnClickedCheckCameraImage)
ON_BN_CLICKED(IDC_CHECK_CAMERA_MIRROR,
              &CRtcDemoV2::OnBnClickedCheckCameraMirror)
ON_BN_CLICKED(IDC_BTN_SEI, &CRtcDemoV2::OnBnClickedBtnSei)
ON_BN_CLICKED(IDC_CHECK_IMPORT_RAW_AUDIO_DATA,
              &CRtcDemoV2::OnBnClickedCheckImportRawAudioData)
ON_BN_CLICKED(IDC_CHECK_LOCAL_MIRROR, &CRtcDemoV2::OnBnClickedCheckLocalMirror)
ON_BN_CLICKED(IDC_CHECK_REMOTE_MIRROR,
              &CRtcDemoV2::OnBnClickedCheckRemoteMirror)
ON_CBN_SELCHANGE(IDC_COMBO_LOCAL_STRETCH_MODE,
                 &CRtcDemoV2::OnCbnSelchangeComboLocalStretchMode)
ON_CBN_SELCHANGE(IDC_COMBO_REMOTE_STRETCH_MODE,
                 &CRtcDemoV2::OnCbnSelchangeComboRemoteStretchMode)
ON_BN_CLICKED(IDC_CHECK_CLIP, &CRtcDemoV2::OnBnClickedCheckClip)
ON_BN_CLICKED(IDC_CHECK_SCALE, &CRtcDemoV2::OnBnClickedCheckScale)
ON_BN_CLICKED(IDC_CHECK_MERGE, &CRtcDemoV2::OnBnClickedCheckMerge)
ON_BN_CLICKED(IDC_CHECK_IMPORT_STATS, &CRtcDemoV2::OnBnClickedCheckImportStats)
ON_BN_CLICKED(IDC_BTN_KICKOUT, &CRtcDemoV2::OnBnClickedBtnKickout)
ON_BN_CLICKED(IDC_BTN_FLUSH_CAMERA, &CRtcDemoV2::OnBnClickedBtnFlushCamera)
ON_BN_CLICKED(IDC_BTN_SUBSCRIBE, &CRtcDemoV2::OnBnClickedBtnSubscribe)
END_MESSAGE_MAP()

void CRtcDemoV2::ReadConfigFile() {
  ifstream is("config");
  if (is.bad()) {
    return;
  }
  char appId_buf[128] = {0};
  char room_buf[128] = {0};
  char user_buf[128] = {0};
  char tcp_buf[128] = {0};
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
  is.getline(tcp_buf, 128);

  SetDlgItemText(IDC_EDIT_APPID, utf2unicode(appId_buf).c_str());
  SetDlgItemText(IDC_EDIT_ROOM_ID, utf2unicode(room_buf).c_str());
  SetDlgItemText(IDC_EDIT_PLAYER_ID, utf2unicode(user_buf).c_str());
  if (!strncmp(tcp_buf,"tcp",3)) {
    ((CButton *)GetDlgItem(IDC_CHECK_TCP))->SetCheck(1);
  }
  is.close();
}

void CRtcDemoV2::WriteConfigFile() {
  ofstream os("config");
  if (os.bad()) {
    return;
  }
  os.clear();
  string app_id = unicode2utf(app_id_.GetBuffer());
  string room_name = unicode2utf(room_name_.GetBuffer());
  string user_id = unicode2utf(user_id_.GetBuffer());
  os.write(app_id.c_str(), app_id.size());
  os.write("\n", 1);
  os.write(room_name.c_str(), room_name.size());
  os.write("\n", 1);
  os.write(user_id.c_str(), user_id.size());
  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_TCP))->GetCheck()) {
    os.write("\n", 1);
    os.write("tcp", 3);
  }
  os.close();
}

void CRtcDemoV2::InitUI() {
  GetDlgItem(IDC_BUTTON_FORWARD)->EnableWindow(FALSE);
  ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_ROTATION))
      ->InsertString(-1, utf2unicode("0").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_ROTATION))
      ->InsertString(-1, utf2unicode("90").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_ROTATION))
      ->InsertString(-1, utf2unicode("180").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_ROTATION))
      ->InsertString(-1, utf2unicode("270").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_ROTATION))->SetCurSel(0);

  ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_ROTATION))
      ->InsertString(-1, utf2unicode("0").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_ROTATION))
      ->InsertString(-1, utf2unicode("90").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_ROTATION))
      ->InsertString(-1, utf2unicode("180").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_ROTATION))
      ->InsertString(-1, utf2unicode("270").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_ROTATION))->SetCurSel(0);

  ((CComboBox *)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))
      ->InsertString(-1, utf2unicode("HIGH").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))
      ->InsertString(-1, utf2unicode("MEDIUM").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))
      ->InsertString(-1, utf2unicode("LOW").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))->SetCurSel(0);

  ((CComboBox *)GetDlgItem(IDC_COMBO_CLIP_CROP))
      ->InsertString(-1, utf2unicode("camera").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_CLIP_CROP))
      ->InsertString(-1, utf2unicode("video external").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_CLIP_CROP))
      ->InsertString(-1, utf2unicode("screen").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_CLIP_CROP))->SetCurSel(0);

  ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_STRETCH_MODE))
      ->InsertString(-1, utf2unicode("ASPECT_FIT").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_STRETCH_MODE))
      ->InsertString(-1, utf2unicode("ASPECT_FILL").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_STRETCH_MODE))
      ->InsertString(-1, utf2unicode("SCALE_FIT").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_STRETCH_MODE))->SetCurSel(0);

  ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_STRETCH_MODE))
      ->InsertString(-1, utf2unicode("ASPECT_FIT").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_STRETCH_MODE))
      ->InsertString(-1, utf2unicode("ASPECT_FILL").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_STRETCH_MODE))
      ->InsertString(-1, utf2unicode("SCALE_FIT").c_str());
  ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_STRETCH_MODE))->SetCurSel(0);

  if (enable_simulcast_) {
    SetDlgItemText(IDC_BUTTON_SIMULCAST, _T("关闭多流"));
  } else {
    SetDlgItemText(IDC_BUTTON_SIMULCAST, _T("开启多流"));
  }

  wnd_status_bar_.Create(WS_CHILD | WS_VISIBLE | SBT_OWNERDRAW,
                         CRect(0, 0, 0, 0), this, 0);
  RECT rc;
  GetWindowRect(&rc);
  int strPartDim[3] = {rc.right / 5, rc.right / 5 * 3, -1};
  wnd_status_bar_.SetParts(3, strPartDim);
  // 设置状态栏文本
  wnd_status_bar_.SetText(_T("通话时长：00:00::00"), 0, 0);
  wnd_status_bar_.SetText(_T("连麦状态"), 1, 0);
  wnd_status_bar_.SetText(
      utf2unicode(GetAppVersion(__DATE__, __TIME__)).c_str(), 2, 0);

  // 初始化音量控制条配置
  ((CSliderCtrl *)GetDlgItem(IDC_SLIDER_RECORD))->SetRange(0, 10);
  ((CSliderCtrl *)GetDlgItem(IDC_SLIDER_RECORD))->SetPos(10);
  ((CSliderCtrl *)GetDlgItem(IDC_SLIDER_PLAYOUT))->SetRange(0, 10);
  ((CSliderCtrl *)GetDlgItem(IDC_SLIDER_PLAYOUT))->SetPos(10);

  // 初始化用户列表控件
  user_list_ctrl_.SetExtendedStyle(LVS_EX_FULLROWSELECT);
  user_list_ctrl_.InsertColumn(0, _T("用户 ID"), LVCFMT_LEFT, 100,
                               0);  // 设置列
  user_list_ctrl_.InsertColumn(1, _T("用户发布流状态"), LVCFMT_LEFT, 350, 1);

  // 初始化视频采集设备 combobox
  int camera_count = QNRTC::GetCameraCount();
  for (int i(0); i < camera_count; ++i) {
    qiniu::QNCameraInfo ci = QNRTC::GetCameraInfo(i);
    camera_dev_map_[ci.id] = ci;
    ((CComboBox *)GetDlgItem(IDC_COMBO_CAMERA))
        ->InsertString(-1, utf2unicode(ci.name).c_str());
  }
  ((CComboBox *)GetDlgItem(IDC_COMBO_CAMERA))->SetCurSel(0);

  // 初始化屏幕窗口列表
  int screen_count = QNRTC::GetScreenWindowCount();
  for (int i(0); i < screen_count; ++i) {
    qiniu::QNScreenWindowInfo sw = QNRTC::GetScreenWindowInfo(i);
    screen_info_map_[sw.id] = sw;
    ((CComboBox *)GetDlgItem(IDC_COMBO_SCREEN))
        ->InsertString(-1, utf2unicode(sw.title).c_str());
  }
  ((CComboBox *)GetDlgItem(IDC_COMBO_SCREEN))->SetCurSel(0);

  // 初始化音频采集设备列表
  int audio_rec_count = QNRTC::GetAudioRecordingDeviceCount();
  for (int i(0); i < audio_rec_count; ++i) {
    qiniu::QNAudioDeviceInfo audio_info;
    audio_info = QNRTC::GetAudioRecordingDeviceInfo(i);
    ((CComboBox *)GetDlgItem(IDC_COMBO_MICROPHONE))
        ->InsertString(-1, utf2unicode(audio_info.name).c_str());
    if (audio_info.is_default) {
      QNRTC::SetAudioRecordingDevice(i);
    }
    microphone_dev_map_[i] = audio_info;
  }
  ((CComboBox *)GetDlgItem(IDC_COMBO_MICROPHONE))->SetCurSel(0);

  // 初始化音频播放设备列表
  int audio_play_count = QNRTC::GetAudioPlaybackDeviceCount();
  for (int i(0); i < audio_play_count; ++i) {
    qiniu::QNAudioDeviceInfo audio_info;
    audio_info = QNRTC::GetAudioPlaybackDeviceInfo(i);
    ((CComboBox *)GetDlgItem(IDC_COMBO_PLAYOUT))
        ->InsertString(-1, utf2unicode(audio_info.name).c_str());
    if (audio_info.is_default) {
      QNRTC::SetAudioPlaybackDevice(i);
    }
    playout_dev_map_[i] = audio_info;
  }
  ((CComboBox *)GetDlgItem(IDC_COMBO_PLAYOUT))->SetCurSel(0);

  ((CButton *)GetDlgItem(IDC_CHECK_CAMERA))->SetCheck(0);
  ((CButton *)GetDlgItem(IDC_CHECK_SCREEN))->SetCheck(0);
  ((CButton *)GetDlgItem(IDC_CHECK_AUDIO))->SetCheck(0);
  ((CButton *)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->SetCheck(0);
}

std::tuple<int, int> CRtcDemoV2::FindBestVideoSize(
    const qiniu::CameraCapabilityVec &camera_cap_vec_) {
  if (camera_cap_vec_.empty()) {
    return {0, 0};
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

void CRtcDemoV2::StartPublish() {
  if (!rtc_client_ptr_) {
    return;
  }

  LocalTrackList track_list;
CHECK_CAMERA:
  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_CAMERA))->GetCheck()) {
    CString video_dev_name;
    string video_dev_id;
    int audio_recorder_device_index(-1);

    GetDlgItem(IDC_COMBO_CAMERA)->GetWindowTextW(video_dev_name);
    if (video_dev_name.IsEmpty()) {
      thread([] { AfxMessageBox(_T("您当前没有任何视频设备！")); }).detach();
      goto CHECK_SCREEN;
    }
    auto itor = camera_dev_map_.begin();
    while (itor != camera_dev_map_.end()) {
      if (itor->second.name.compare(unicode2utf(video_dev_name.GetBuffer())) ==
          0) {
        video_dev_id = itor->first;
        break;
      }
      ++itor;
    }
    auto camera_size =
        FindBestVideoSize(camera_dev_map_[video_dev_id].capabilities);
    int width = std::get<0>(camera_size);
    int height = std::get<1>(camera_size);
    QNCameraVideoTrackConfig camera_info = {{width, height, 30, 2000},
                                            {width, height, 30},
                                            video_dev_id,
                                            CAMERA_TAG,
                                            enable_simulcast_};
    auto camera_track_ptr = QNRTC::CreateCameraVideoTrack(camera_info, this);
    if (camera_track_ptr) {
      camera_track_ptr_ = camera_track_ptr;
      track_list.emplace_back(camera_track_ptr);
      local_published_track_list_.emplace_back(camera_track_ptr);
    } else {
      auto itor = local_published_track_list_.begin();
      for (; itor != local_published_track_list_.end();) {
        if (*itor == camera_track_ptr_) {
          local_published_track_list_.erase(itor++);
          break;
        } else {
          itor++;
        }
      }
      QNRTC::DestroyLocalTrack(camera_track_ptr_);
      camera_track_ptr_ = nullptr;
      camera_track_ptr_ = QNRTC::CreateCameraVideoTrack(camera_info, this);
      track_list.emplace_back(camera_track_ptr_);
      local_published_track_list_.emplace_back(camera_track_ptr_);
    }
    QNView view = {GetDlgItem(IDC_STATIC_VIDEO_PREVIEW)->m_hWnd, kRenderD3D,
                   kVideoRotation0, kStretchFit, false};
    camera_track_ptr_->Play(view);
  }
CHECK_SCREEN:
  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_SCREEN))->GetCheck()) {
    CString wnd_title;
    string source_id;
    bool exist_curent_srceen = false;
    GetDlgItem(IDC_COMBO_SCREEN)->GetWindowTextW(wnd_title);
    for (auto &&itor : screen_info_map_) {
      if (itor.second.title.compare(unicode2utf(wnd_title.GetBuffer())) == 0) {
        source_id = itor.first;
        exist_curent_srceen = true;
        break;
      }
    }
    if (!exist_curent_srceen) {
      goto CHECK_CUSTOM_VIDEO;
    }
    bool is_window_graphics_capture_enabled = false;
    if (1 == ((CButton *)GetDlgItem(IDC_CHECK_WINDOWS_GRAPHICS))->GetCheck()) {
      is_window_graphics_capture_enabled = true;
    }
    QNScreenVideoTrackConfig screen_info{{1920, 1080, 30, 2000},
                                         source_id,
                                         SCREENCASTS_TAG,
                                         enable_simulcast_,
                                         is_window_graphics_capture_enabled};
    auto screen_track_ptr = QNRTC::CreateScreenVideoTrack(screen_info);
    if (screen_track_ptr) {
      screen_track_ptr_ = screen_track_ptr;
      track_list.emplace_back(screen_track_ptr);
      local_published_track_list_.emplace_back(screen_track_ptr);
    } else {
      auto itor = local_published_track_list_.begin();
      for (; itor != local_published_track_list_.end();) {
        if (*itor == screen_track_ptr_) {
          local_published_track_list_.erase(itor++);
          break;
        } else {
          itor++;
        }
      }
      QNRTC::DestroyLocalTrack(screen_track_ptr_);
      screen_track_ptr_ = nullptr;
      screen_track_ptr_ = QNRTC::CreateScreenVideoTrack(screen_info);
      track_list.emplace_back(screen_track_ptr_);
      local_published_track_list_.emplace_back(screen_track_ptr_);
    }
    QNView view = {GetDlgItem(IDC_STATIC_VIDEO_PREVIEW2)->m_hWnd, kRenderD3D,
                   kVideoRotation0, kStretchFit, false};
    screen_track_ptr_->Play(view);
  }
CHECK_CUSTOM_VIDEO:
  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->GetCheck()) {
    stop_video_external_flag_ = false;
    // 这里需要设置的是外部视频源对应的分辨率和帧率
    QNCustomVideoTrackConfig video_custom_info = {
        {426, 240, 30, 300}, EXTERNAL_VIDEO, enable_simulcast_};
    auto custom_video_track_ptr =
        QNRTC::CreateCustomVideoTrack(video_custom_info);
    if (custom_video_track_ptr) {
      custom_video_track_ptr_ = custom_video_track_ptr;
      track_list.emplace_back(custom_video_track_ptr);
      local_published_track_list_.emplace_back(custom_video_track_ptr);
    } else {
      auto itor = local_published_track_list_.begin();
      for (; itor != local_published_track_list_.end();) {
        if (*itor == custom_video_track_ptr_) {
          local_published_track_list_.erase(itor++);
          break;
        } else {
          itor++;
        }
      }
      QNRTC::DestroyLocalTrack(custom_video_track_ptr_);
      custom_video_track_ptr_ = nullptr;
      custom_video_track_ptr_ =
          QNRTC::CreateCustomVideoTrack(video_custom_info);
      track_list.emplace_back(custom_video_track_ptr_);
      local_published_track_list_.emplace_back(custom_video_track_ptr_);
    }

    QNView view = {GetDlgItem(IDC_STATIC_VIDEO_PREVIEW3)->m_hWnd, kRenderD3D,
                   kVideoRotation0, kStretchFit, false};
    custom_video_track_ptr_->Play(view);
  } else {
    stop_video_external_flag_ = true;
  }
CHECK_MICROPHONE:
  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_AUDIO))->GetCheck()) {
    ((CButton *)GetDlgItem(IDC_CHECK_IMPORT_RAW_AUDIO_DATA))->SetCheck(0);
    QNMicrophoneAudioTrackConfig micro_info = {{0, 0, 0, 32}, MICROPHONE_TAG};
    auto microphone_audio_track_ptr =
        QNRTC::CreateMicrophoneAudioTrack(micro_info);
    if (microphone_audio_track_ptr) {
      microphone_audio_track_ptr_ = microphone_audio_track_ptr;
      track_list.emplace_back(microphone_audio_track_ptr);
      local_published_track_list_.emplace_back(microphone_audio_track_ptr);
    } else {
      if (custom_audio_track_ptr_) {
        auto itor = local_published_track_list_.begin();
        for (; itor != local_published_track_list_.end();) {
          if (*itor == custom_audio_track_ptr_) {
            local_published_track_list_.erase(itor++);
            break;
          } else {
            itor++;
          }
        }
        QNRTC::DestroyLocalTrack(custom_audio_track_ptr_);
        custom_audio_track_ptr_ = nullptr;
      }
      if (microphone_audio_track_ptr_) {
        auto itor = local_published_track_list_.begin();
        for (; itor != local_published_track_list_.end();) {
          if (*itor == microphone_audio_track_ptr_) {
            local_published_track_list_.erase(itor++);
            break;
          } else {
            itor++;
          }
        }
        QNRTC::DestroyLocalTrack(microphone_audio_track_ptr_);
        microphone_audio_track_ptr_ = nullptr;
      }
      microphone_audio_track_ptr_ =
          QNRTC::CreateMicrophoneAudioTrack(micro_info);
      track_list.emplace_back(microphone_audio_track_ptr_);
      local_published_track_list_.emplace_back(microphone_audio_track_ptr_);
    }
  }
CHECK_CUSTOM_AUDIO:
  if (1 ==
      ((CButton *)GetDlgItem(IDC_CHECK_IMPORT_RAW_AUDIO_DATA))->GetCheck()) {
    stop_audio_external_flag_ = false;
    ((CButton *)GetDlgItem(IDC_CHECK_AUDIO))->SetCheck(0);
    QNCustomAudioTrackConfig custom_audio_info = {{44100, 2, 16, 32},
                                                  EXTERNAL_AUDIO};
    auto custom_audio_track_ptr =
        QNRTC::CreateCustomAudioTrack(custom_audio_info);
    if (custom_audio_track_ptr) {
      custom_audio_track_ptr_ = custom_audio_track_ptr;
      track_list.emplace_back(custom_audio_track_ptr);
      local_published_track_list_.emplace_back(custom_audio_track_ptr);
    } else {
      if (microphone_audio_track_ptr_) {
        auto itor = local_published_track_list_.begin();
        for (; itor != local_published_track_list_.end();) {
          if (*itor == microphone_audio_track_ptr_) {
            local_published_track_list_.erase(itor++);
            break;
          } else {
            itor++;
          }
        }
        QNRTC::DestroyLocalTrack(microphone_audio_track_ptr_);
        microphone_audio_track_ptr_ = nullptr;
      }
      if (custom_audio_track_ptr_) {
        auto itor = local_published_track_list_.begin();
        for (; itor != local_published_track_list_.end();) {
          if (*itor == custom_audio_track_ptr_) {
            local_published_track_list_.erase(itor++);
            break;
          } else {
            itor++;
          }
        }
        QNRTC::DestroyLocalTrack(custom_audio_track_ptr_);
        custom_audio_track_ptr_ = nullptr;
      }
      custom_audio_track_ptr_ =
          QNRTC::CreateCustomAudioTrack(custom_audio_info);
      track_list.emplace_back(custom_audio_track_ptr_);
      local_published_track_list_.emplace_back(custom_audio_track_ptr_);
    }
  } else {
    stop_audio_external_flag_ = true;
  }

  if (!track_list.empty() && is_joined_) {
    rtc_client_ptr_->Publish(track_list, this);
  }
}

void CRtcDemoV2::ImportExternalVideoRawFrame() {
  // 模拟导入视频数据,当前使用当前目录下指定的视频文件
  stop_video_external_flag_ = true;
  if (fake_video_thread_.joinable()) {
    fake_video_thread_.join();
  }

  fake_video_thread_ = thread([&] {
    FILE *fp = nullptr;
    fopen_s(&fp, "426x240.yuv", "rb");
    uint8_t *buf = (uint8_t *)malloc(426 * 240 * 3 / 2);
    if (!fp || !buf) {
      MessageBox(
          _T("foreman_320x240.yuv 文件打开失败，请确认此文件件是否存在!"));
      return;
    }
    size_t ret(0);
    stop_video_external_flag_ = false;
    chrono::system_clock::time_point start_tp = chrono::system_clock::now();
    while (!stop_video_external_flag_ && custom_video_track_ptr_) {
      ret = fread_s(buf, 426 * 240 * 3 / 2, 1, 426 * 240 * 3 / 2, fp);
      if (ret > 0) {
        custom_video_track_ptr_->PushVideoFrame(
            buf, 426 * 240 * 3 / 2, 426, 240,
            chrono::duration_cast<chrono::microseconds>(
            chrono::system_clock::now() - start_tp).count(),
            qiniu::kI420, qiniu::kVideoRotation0);
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

void CRtcDemoV2::ImportExternalAudioRawFrame() {
  // 模拟导入音频数据,当前使用当前目录下指定的音频文件
  stop_audio_external_flag_ = true;
  if (fake_audio_thread_.joinable()) {
    fake_audio_thread_.join();
  }
  // 模拟导入音频数据
  fake_audio_thread_ = thread([&] {
    FILE *fp = nullptr;
    fopen_s(&fp, "44100_16bits_2channels.pcm", "rb");
    if (!fp) {
      MessageBox(
          _T("PCM 文件:44100_16bits_2channels.pcm ")
          _T("打开失败，请确认此文件件是否存在!"));
      return;
    }
    // 每次导入 20 ms 的数据，即 441 * 2 个 samples
    uint8_t *buf = (uint8_t *)malloc(441 * 2 * 2 * 2);

    size_t ret(0);
    stop_audio_external_flag_ = false;
    chrono::system_clock::time_point start_tp = chrono::system_clock::now();
    int64_t audio_frame_count(0);
    while (!stop_audio_external_flag_ && custom_audio_track_ptr_) {
      if (chrono::duration_cast<chrono::microseconds>(
              chrono::system_clock::now() - start_tp)
              .count() >= audio_frame_count * 20000) {
      } else {
        Sleep(10);
        continue;
      }

      ret = fread_s(buf, 441 * 2 * 4, 1, 441 * 2 * 4, fp);
      if (ret >= 441 * 8) {
        custom_audio_track_ptr_->PushAudioFrame(buf, 441 * 8, 16, 44100, 2);
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

void CRtcDemoV2::AdjustSubscribeLayouts() {
  if (remote_tracks_map_.empty()) {
    return;
  }
  int wnd_num(0);
  RECT wnd_rc;
  GetWindowRect(&wnd_rc);
  TRACE("MainDialog rect x:%d, y:%d, right:%d, botton:%d\n", wnd_rc.left,
        wnd_rc.top, wnd_rc.right, wnd_rc.bottom);
  int main_wnd_height = wnd_rc.bottom - wnd_rc.top;
  const int wnd_width = 320;
  const int wnd_height = 240;
  int start_x = wnd_rc.left - wnd_width;
  int start_y = wnd_rc.top;
  for (auto &&itor : remote_tracks_map_) {
    if (itor.second->render_wnd_ptr_) {
      itor.second->render_wnd_ptr_->MoveWindow(
          start_x, start_y + wnd_height * wnd_num, wnd_width, wnd_height);
      if (start_y + wnd_width * wnd_num >= main_wnd_height) {
        wnd_num = 0;
        start_x += wnd_width;
      } else {
        ++wnd_num;
      }
    }
  }
}

void CRtcDemoV2::CreateCustomMergeJob(bool open) {
  if (!rtc_client_ptr_) {
    return;
  }
  qiniu::QNTranscodingLiveStreamingConfig job_desc;
  job_desc.stream_id = unicode2utf(room_name_.GetBuffer()) + "_merge";
  job_desc.publish_url = merge_config_.publish_url;
  job_desc.width = merge_config_.job_width;
  job_desc.height = merge_config_.job_height;
  job_desc.fps = merge_config_.job_fps;
  job_desc.bitrate = merge_config_.job_bitrate;
  job_desc.min_bitrate = merge_config_.job_min_bitrate;
  job_desc.max_bitrate = merge_config_.job_max_bitrate;
  job_desc.stretch_mode =
      (qiniu::QNStretchMode)(merge_config_.job_stretch_mode);
  job_desc.is_hold_last_frame = merge_config_.hold_last_frame;

  qiniu::QNTranscodingLiveStreamingImage background;
  background.layer_url = merge_config_.background_url;
  background.x = merge_config_.background_x;
  background.y = merge_config_.background_y;
  background.layer_width = merge_config_.background_width;
  background.layer_height = merge_config_.background_height;
  job_desc.merge_background = background;

  qiniu::QNTranscodingLiveStreamingImage mark;
  mark.layer_url = merge_config_.watermark_url;
  mark.x = merge_config_.watermark_x;
  mark.y = merge_config_.watermark_y;
  mark.layer_width = merge_config_.watermark_width;
  mark.layer_height = merge_config_.watermark_height;
  job_desc.merge_watermark.emplace_back(mark);

  custom_merge_id_ = job_desc.stream_id;
  if (open) {
    rtc_client_ptr_->StartLiveStreaming(job_desc);
  } else {
    rtc_client_ptr_->StopLiveStreaming(job_desc);
  }
}

void CRtcDemoV2::AdjustMergeStreamLayouts() {
  // 自定义合流，根据界面配置使用一个本地或者远端 track
  if (!rtc_client_ptr_) {
    return;
  }
  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_MERGE))->GetCheck()) {
    CreateCustomMergeJob(true);
    merge_flag_ = true;
    qiniu::QNTranscodeingTrackList add_tracks_list;
    list<string> remove_tracks_list;

    if (microphone_audio_track_ptr_) {
      qiniu::QNTranscodingLiveStreamingTrack merge_opt;
      merge_opt.track_id = microphone_audio_track_ptr_->GetTrackID();
      add_tracks_list.emplace_back(merge_opt);
    }

    if (camera_track_ptr_) {
      qiniu::QNTranscodingLiveStreamingTrack merge_opt;
      merge_opt.track_id = camera_track_ptr_->GetTrackID();
      if (merge_config_.merge_local_video) {
        // 这里只做演示，用户可根据自己需求设置相应 video track 合流时的填充模式
        if (1 == ((CButton *)GetDlgItem(IDC_CHECK_STRETCH_MODE))->GetCheck()) {
          merge_opt.stretch_mode = qiniu::kStretchScaleToFit;
        }
        merge_opt.x = merge_config_.local_video_x;
        merge_opt.y = merge_config_.local_video_y;
        merge_opt.z = 1;
        merge_opt.width = merge_config_.local_video_width;
        merge_opt.height = merge_config_.local_video_height;
        merge_opt.is_support_sei = true;
      }
      add_tracks_list.emplace_back(merge_opt);
    }

    RemoteUserList remoteList = rtc_client_ptr_->GetRemoteUsers();
    for (auto &&itor : remoteList) {
      for (auto &&audio_itor : itor.remote_audio_track_list) {
        if (merge_config_.merge_remote_audio) {
          qiniu::QNTranscodingLiveStreamingTrack merge_opt;
          merge_opt.track_id = audio_itor->GetTrackID();
          add_tracks_list.emplace_back(merge_opt);
          remote_transcoding_tracks_list_.emplace_back(merge_opt);
          break;
        }
      }

      for (auto &&video_itor : itor.remote_video_track_list) {
        qiniu::QNTranscodingLiveStreamingTrack merge_opt;
        merge_opt.track_id = video_itor->GetTrackID();
        if (merge_config_.merge_remote_video) {
          // 这里只做演示，用户可根据自己需求设置相应 video track
          // 合流时的填充模式
          if (1 ==
              ((CButton *)GetDlgItem(IDC_CHECK_STRETCH_MODE))->GetCheck()) {
            merge_opt.stretch_mode = qiniu::kStretchScaleToFit;
          }
          merge_opt.x = merge_config_.remote_video_x;
          merge_opt.y = merge_config_.remote_video_y;
          merge_opt.z = 1;
          merge_opt.width = merge_config_.remote_video_width;
          merge_opt.height = merge_config_.remote_video_height;
          merge_opt.is_support_sei = false;
          add_tracks_list.emplace_back(merge_opt);
          remote_transcoding_tracks_list_.emplace_back(merge_opt);
          break;
        }
      }
    }
    rtc_client_ptr_->SetTranscodingLiveStreamingTracks(custom_merge_id_,
                                                       add_tracks_list);
  } else {
    merge_flag_ = false;
    CreateCustomMergeJob(false);
  }
}

void CRtcDemoV2::AdjustStatsLayouts() {
  if (!dlg_stats_) {
    return;
  }

  RECT wnd_rc;
  GetWindowRect(&wnd_rc);
  TRACE("MainDialog rect x:%d, y:%d, right:%d, botton:%d\n", wnd_rc.left,
        wnd_rc.top, wnd_rc.right, wnd_rc.bottom);
  int main_wnd_height = wnd_rc.bottom - wnd_rc.top;
  const int wnd_width = 700;
  const int wnd_height = 400;
  int start_x = wnd_rc.right;
  int start_y = wnd_rc.top;
  dlg_stats_->MoveWindow(start_x, start_y, wnd_width, wnd_height);
}

void CRtcDemoV2::OnConnectionStateChanged(
    QNConnectionState state, const QNConnectionDisconnectedInfo* info) {
  if (state == kConnected) {
    OnUserJoined(unicode2utf(user_id_.GetBuffer()),"");
    GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(TRUE);
    GetDlgItem(IDC_BUTTON_FORWARD)->EnableWindow(TRUE);
    wnd_status_bar_.SetText(_T("登录成功！"), 1, 0);
    SetDlgItemText(IDC_BUTTON_LOGIN, _T("离开"));

    start_tp_ = chrono::steady_clock::now();
    SetTimer(UPDATE_TIME_DURATION_TIMER, 100, nullptr);
    is_joined_ = true;
    StartPublish();
  } else if (state == kDisconnected) {
    if (!destory_main_dialog_) {
      OnUserLeft(unicode2utf(user_id_.GetBuffer()));
    }
    is_joined_ = false;
    if (!info) {
      return;
    }
    if (info->reason == QNConnectionDisconnectedInfo::kKickOut) {
      wnd_status_bar_.SetText(_T("被踢出！"), 1, 0);
      PostMessage(WM_COMMAND, MAKEWPARAM(IDC_BUTTON_LOGIN, BN_CLICKED), NULL);
    } else if (info->reason == QNConnectionDisconnectedInfo::kRoomError) {
      SetDlgItemText(IDC_BUTTON_LOGIN, _T("登录"));
      if (rtc_client_ptr_) {
        rtc_client_ptr_->Leave();
      }
      PostMessage(WM_COMMAND, MAKEWPARAM(IDC_BUTTON_LOGIN, BN_CLICKED), NULL);
    } else if (info->reason == QNConnectionDisconnectedInfo::kLeave) {
      if (!destory_main_dialog_) {
        wnd_status_bar_.SetText(_T("离开房间！"), 1, 0);
      }
    }
  } else if (state == kReconnected) {
    wchar_t buff[1024] = {0};
    _snwprintf(buff, 1024, _T("%s 重连成功！"), user_id_);
    wnd_status_bar_.SetText(buff, 1, 0);
    if (!is_joined_) {
      OnUserJoined(unicode2utf(user_id_.GetBuffer()), "");
      GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(TRUE);
      GetDlgItem(IDC_BUTTON_FORWARD)->EnableWindow(TRUE);
      wnd_status_bar_.SetText(_T("登录成功！"), 1, 0);
      SetDlgItemText(IDC_BUTTON_LOGIN, _T("离开"));

      start_tp_ = chrono::steady_clock::now();
      SetTimer(UPDATE_TIME_DURATION_TIMER, 100, nullptr);
      is_joined_ = true;
      StartPublish();
    }
    is_joined_ = true;
  } else if (state == kConnecting || state == kReconnecting) {
    if (state == kReconnecting) {
      wchar_t buff[1024] = {0};
      _snwprintf(buff, 1024, _T("%s 正在重连！"),user_id_);
      wnd_status_bar_.SetText(buff, 1, 0);
    }
    is_joined_ = false;
  }
}

void CRtcDemoV2::OnUserJoined(const std::string& remote_user_id,
                              const std::string& user_data){
  lock_guard<recursive_mutex> lck(mutex_);
  user_list_.push_back(remote_user_id);

  CString str;
  for (int i = 0; i < user_list_ctrl_.GetItemCount(); i++) {
    str = user_list_ctrl_.GetItemText(i, 0);
    if (str.CompareNoCase(utf2unicode(remote_user_id).c_str()) == 0) {
      user_list_ctrl_.DeleteItem(i);
      break;
    }
  }
  user_list_ctrl_.InsertItem(0, utf2unicode(remote_user_id).c_str());
  user_list_ctrl_.SetItemText(0, 1, utf2unicode(user_data).c_str());

  wchar_t buff[1024] = {0};
  _snwprintf(buff, 1024, _T("%s 加入了房间！"),
             utf2unicode(remote_user_id).c_str());
  wnd_status_bar_.SetText(buff, 1, 0);
}

void CRtcDemoV2::OnUserLeft(const std::string& remote_user_id){
  lock_guard<recursive_mutex> lck(mutex_);
  CString str;
  for (int i = 0; i < user_list_ctrl_.GetItemCount(); i++) {
    str = user_list_ctrl_.GetItemText(i, 0);
    if (str.CompareNoCase(utf2unicode(remote_user_id).c_str()) == 0) {
      user_list_ctrl_.DeleteItem(i);
      break;
    }
  }

  auto itor = std::find(user_list_.begin(), user_list_.end(), remote_user_id);
  if (itor != user_list_.end()) {
    user_list_.erase(itor);
  }

  wchar_t buff[1024] = {0};
  _snwprintf(buff, 1024, _T("%s 离开了房间！"),
             utf2unicode(remote_user_id).c_str());
  wnd_status_bar_.SetText(buff, 1, 0);
  ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_ROTATION))->SetCurSel(0);
}

void CRtcDemoV2::OnUserReconnecting(const std::string& remote_user_id) {
  wchar_t buf[512] = {0};
  _snwprintf(buf, sizeof(buf), _T("%s 用户正在重连"),
             utf2unicode(remote_user_id).c_str());
  wnd_status_bar_.SetText(buf, 1, 0);
}

void CRtcDemoV2::OnUserReconnected(const std::string& remote_user_id) {
  wchar_t buf[512] = {0};
  _snwprintf(buf, sizeof(buf), _T("%s 用户重连成功"),
             utf2unicode(remote_user_id).c_str());
  wnd_status_bar_.SetText(buf, 1, 0);
}

void CRtcDemoV2::OnUserPublished(const std::string& remote_user_id,
                                 const RemoteTrackList& track_list) {
  if (!is_joined_) {
    return;
  }
  for (auto &&itor : track_list) {
    if (itor) {
      itor->SetTrackInfoChangedListener(this);
    }
  }
}

void CRtcDemoV2::OnUserUnpublished(const std::string& remote_user_id,
                                   const RemoteTrackList& track_list) {
  if (!is_joined_) {
    return;
  }
  for (auto &&itor : track_list) {
    if (itor) {
      string track_id = itor->GetTrackID();
      ::SendMessage(this->GetSafeHwnd(), WM_ADD_SIG_DLG_PRIV_MSG,
                  WM_REMOVE_REMOTE_VIDEO_RENDER_MSG,
                  (LPARAM)(track_id.c_str()));
    }
  }
}

void CRtcDemoV2::OnSubscribed(
    const std::string& remote_user_id,
    const RemoteAudioTrackList& remote_audio_track_list,
    const RemoteVideoTrackList& remote_video_track_list) {
  if (!is_joined_) {
    return;
  }
  if (!remote_video_track_list.empty()) {
    for (auto &&itor : remote_video_track_list) {
      if (itor) {
        ::SendMessage(this->GetSafeHwnd(), WM_ADD_SIG_DLG_PRIV_MSG,
                      WM_ADD_REMOTE_VIDEO_RENDER_MSG, (LPARAM)itor);
      }
    }
  }
}

void CRtcDemoV2::OnMessageReceived(const QNCustomMessage& message) {
  if (message.msg_text.compare("kickout") == 0) {
    PostMessage(WM_COMMAND, MAKEWPARAM(IDC_BUTTON_LOGIN, BN_CLICKED), NULL);
    wchar_t buff[1024] = {0};
    _snwprintf(buff, 1024, _T("被 %s 用户踢出！"),
               utf2unicode(message.msg_sendid).c_str());
    wnd_status_bar_.SetText(buff, 1, 0);
    return;
  }
  if (::IsWindow(dlg_msg_.m_hWnd)) {
    dlg_msg_.OnReceiveMessage(message.msg_sendid,
                              utf8_to_string(message.msg_text));
  }
}

void CRtcDemoV2::OnMediaRelayStateChanged(
    const std::string &relay_room,
    const QNMediaRelayState state){
}

void CRtcDemoV2::OnPublished() {
  wnd_status_bar_.SetText(_T("发布成功！"), 1, 0);
  if (custom_audio_track_ptr_ && !stop_audio_external_flag_) {
    ImportExternalAudioRawFrame();
  }

  if (custom_video_track_ptr_ && !stop_video_external_flag_) {
    ImportExternalVideoRawFrame();
  }
}

void CRtcDemoV2::OnPublishError(int error_code,
                                const std::string& error_message) {
  wchar_t buff[1024] = {0};
  _snwprintf(buff, 1024, _T("发布失败, error code: %d,err: %s"), error_code,
             utf2unicode(error_message).c_str());
  wnd_status_bar_.SetText(buff, 1, 0);
}

void CRtcDemoV2::OnVideoDeviceStateChanged(QNVideoDeviceState device_state,
                                           const std::string& device_name) {
  thread([&, device_state, device_name] {
    wchar_t buf[512] = {0};
    if (qiniu::kVideoDeviceLost == device_state) {
      _snwprintf(buf, 512, _T("视频设备：%s 被拔出！"),
                 utf2unicode(device_name).c_str());
      AfxMessageBox(buf, MB_OK);
    }
  }).detach();
}

void CRtcDemoV2::OnAudioDeviceStateChanged(QNAudioDeviceState device_state,
                                           const std::string& device_guid) {
  thread([&, device_state, device_guid] {
    wchar_t buf[512] = {0};
    if (qiniu::kAudioDeviceActive != device_state) {
      _snwprintf(buf, 512, _T("音频设备：%s 被拔出！"),
                 utf2unicode(device_guid).c_str());
      AfxMessageBox(buf, MB_OK);
    }
  }).detach();
}

void CRtcDemoV2::OnVideoProfileChanged(const std::string& user_id,
                                       const std::string& track_id,
                                       QNTrackProfile profile) {
  if (profile == qiniu::kHigh) {
    ((CComboBox *)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))->SetCurSel(0);
  } else if (profile == qiniu::kMedium) {
    ((CComboBox *)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))->SetCurSel(1);
  } else if (profile == qiniu::kLow) {
    ((CComboBox *)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))->SetCurSel(2);
  }
  wchar_t buff[1024] = {0};
  _snwprintf(buff, 1024, _T("切换大小流成功,%s : %s : %d "),
             utf2unicode(user_id).c_str(), utf2unicode(track_id).c_str(),
             profile);
  wnd_status_bar_.SetText(buff, 1, 0);
}

void CRtcDemoV2::OnMuteStateChanged(const std::string& user_id,
                                    const std::string& track_id, bool muted) {
  wchar_t buff[1024] = {0};
  if (muted) {
    _snwprintf(buff, 1024, _T("%s:%s  开启 mute"),
               utf2unicode(user_id).c_str(), utf2unicode(track_id).c_str());
  } else {
    _snwprintf(buff, 1024, _T("%s:%s  取消 mute"),
               utf2unicode(user_id).c_str(), utf2unicode(track_id).c_str());
  }
  wnd_status_bar_.SetText(buff, 1, 0);
}

void CRtcDemoV2::OnStarted(const std::string &stream_id) {
  wchar_t buff[1024] = {0};
  _snwprintf(buff, 1024, _T("%s  转推成功"), utf2unicode(stream_id).c_str());
  wnd_status_bar_.SetText(buff, 1, 0);
}

void CRtcDemoV2::OnStopped(const std::string &stream_id) {
  wchar_t buff[1024] = {0};
  _snwprintf(buff, 1024, _T("%s  停止转推"), utf2unicode(stream_id).c_str());
  wnd_status_bar_.SetText(buff, 1, 0);
}

void CRtcDemoV2::OnTranscodingTracksUpdated(const std::string &stream_id) {
  wchar_t buff[1024] = {0};
  _snwprintf(buff, 1024, _T("%s  转推任务更新"),
             utf2unicode(stream_id).c_str());
  wnd_status_bar_.SetText(buff, 1, 0);
}

void CRtcDemoV2::OnLiveStreamingError(const std::string &stream_id,
    const QNLiveStreamingErrorInfo &error_info) {
  wchar_t buff[1024] = {0};
  _snwprintf(buff, 1024, _T("%s 转推失败,%d : %d : %s "), utf2unicode(stream_id).c_str(),
    error_info.type, error_info.code, utf2unicode(error_info.message).c_str());
  wnd_status_bar_.SetText(buff, 1, 0);
}

void CRtcDemoV2::OnCameraError(int error_code,
                               const std::string &error_message) {
  wchar_t buff[1024] = {0};
  _snwprintf(buff, 1024, _T("摄像头打开失败: code: %d, msg:%s "), error_code,
             utf2unicode(error_message).c_str());
  wnd_status_bar_.SetText(buff, 1, 0);
}

// CRtcDemoV2 message handlers
afx_msg LRESULT CRtcDemoV2::RenderRemoteVideoFrame(WPARAM wParam,
                                                   LPARAM lParam) {
  switch (wParam) {
    case WM_ADD_REMOTE_VIDEO_RENDER_MSG: {
      shared_ptr<TrackInfoUI> ui(
          new TrackInfoUI(this, (QNRemoteVideoTrack *)lParam));
      QNView view = {ui->render_wnd_ptr_->m_hWnd, kRenderD3D, kVideoRotation0,
                     kStretchFit, false};
      ((QNRemoteVideoTrack *)lParam)->Play(view);
      remote_tracks_map_[((QNRemoteVideoTrack *)lParam)->GetTrackID()] = ui;
      AdjustSubscribeLayouts();
      break;
    }
    case WM_REMOVE_REMOTE_VIDEO_RENDER_MSG: {
      string trackid = (char *)lParam;
      remote_tracks_map_.erase(trackid);
      break;
    }
    case WM_REMOTE_VIDEO_RENDER_MODIFY_ROTATE_MSG: {
      QNVideoRotation rotation = (QNVideoRotation)lParam;
      for (auto &&itor : remote_tracks_map_) {
        QNView view = {itor.second->render_wnd_ptr_->m_hWnd, kRenderD3D,
                       rotation, kStretchFit, false};
        (dynamic_cast<QNRemoteVideoTrack *>(itor.second->track_info_ptr_))
            ->Play(view);
      }
      break;
    }
    case WM_REMOTE_VIDEO_RENDER_MODIFY_STRETCH_MSG: {
      QNStretchMode stretch_mode = (QNStretchMode)lParam;
      for (auto &&itor : remote_tracks_map_) {
        QNView view = {itor.second->render_wnd_ptr_->m_hWnd, kRenderD3D,
                       kVideoRotation0, stretch_mode, false};
        (dynamic_cast<QNRemoteVideoTrack *>(itor.second->track_info_ptr_))
            ->Play(view);
      }
      break;
    }
    case WM_REMOTE_VIDEO_RENDER_MODIFY_MIRROR_MSG: {
      bool mirror = (bool)lParam;
      for (auto &&itor : remote_tracks_map_) {
        QNView view = {itor.second->render_wnd_ptr_->m_hWnd, kRenderD3D,
                       kVideoRotation0, kStretchFit, mirror};
        (dynamic_cast<QNRemoteVideoTrack *>(itor.second->track_info_ptr_))
            ->Play(view);
      }
      break;
    }
    default:
      break;
  }
  return 0;
}

afx_msg LRESULT CRtcDemoV2::OnHandleMergeMessage(WPARAM wParam, LPARAM lParam) {
  switch (wParam) {
    case MERGE_MESSAGE_CONFIG_ID:
      merge_config_ = *(MergeDialog::MergeConfig *)lParam;
      break;
    case MERGE_MESSAGE_LOCAL_AUDIO_ID:
      if (merge_flag_) {
        bool flag = (bool)lParam;
        if (flag) {
          merge_local_audio_flag_ = true;
        } else {
          merge_local_audio_flag_ = false;
        }
        qiniu::QNTranscodeingTrackList tracks_list;
        if (microphone_audio_track_ptr_) {
          qiniu::QNTranscodingLiveStreamingTrack merge_opt;
          merge_opt.track_id = microphone_audio_track_ptr_->GetTrackID();
          tracks_list.emplace_back(merge_opt);
          if (merge_local_audio_flag_) {
            rtc_client_ptr_->SetTranscodingLiveStreamingTracks(
                custom_merge_id_, tracks_list);
          } else {
            rtc_client_ptr_->RemoveTranscodingLiveStreamingTracks(
                custom_merge_id_, tracks_list);
          }
        }
      }
      break;
    case MERGE_MESSAGE_LOCAL_VIDEO_ID:
      if (merge_flag_) {
        bool flag = (bool)lParam;
        if (flag) {
          merge_local_video_flag_ = true;
        } else {
          merge_local_video_flag_ = false;
        }
        qiniu::QNTranscodeingTrackList tracks_list;
        if (camera_track_ptr_) {
          qiniu::QNTranscodingLiveStreamingTrack merge_opt;
          merge_opt.track_id = camera_track_ptr_->GetTrackID();
          if (merge_config_.merge_local_video) {
            // 这里只做演示，用户可根据自己需求设置相应 video track
            // 合流时的填充模式
            if (1 ==
                ((CButton *)GetDlgItem(IDC_CHECK_STRETCH_MODE))->GetCheck()) {
              merge_opt.stretch_mode = qiniu::kStretchScaleToFit;
            }
            merge_opt.x = merge_config_.local_video_x;
            merge_opt.y = merge_config_.local_video_y;
            merge_opt.z = 1;
            merge_opt.width = merge_config_.local_video_width;
            merge_opt.height = merge_config_.local_video_height;
            merge_opt.is_support_sei = true;
          }
          tracks_list.emplace_back(merge_opt);
          if (merge_local_video_flag_) {
            rtc_client_ptr_->SetTranscodingLiveStreamingTracks(custom_merge_id_,
                                                               tracks_list);
          } else {
            rtc_client_ptr_->RemoveTranscodingLiveStreamingTracks(
                custom_merge_id_, tracks_list);
          }
        }
      }
      break;
    case MERGE_MESSAGE_REMOTE_AUDIO_ID:
      if (merge_flag_) {
        bool flag = (bool)lParam;
        if (flag) {
          merge_remote_audio_flag_ = true;
        } else {
          merge_remote_audio_flag_ = false;
        }
        qiniu::QNTranscodeingTrackList tracks_list;
        for (auto&& itor : remote_transcoding_tracks_list_) {
          if (itor.width == 0 && itor.height == 0) {
            tracks_list.emplace_back(itor);
            break;
          }
        }
        if (!tracks_list.empty()) {
          if (merge_remote_audio_flag_) {
            rtc_client_ptr_->SetTranscodingLiveStreamingTracks(custom_merge_id_,
                                                               tracks_list);
          } else {
            rtc_client_ptr_->RemoveTranscodingLiveStreamingTracks(
                custom_merge_id_, tracks_list);
          }
        }
      }
      break;
    case MERGE_MESSAGE_REMOTE_VIDEO_ID:
      if (merge_flag_) {
        bool flag = (bool)lParam;
        if (flag) {
          merge_remote_video_flag_ = true;
        } else {
          merge_remote_video_flag_ = false;
        }
        qiniu::QNTranscodeingTrackList tracks_list;
        for (auto &&itor : remote_transcoding_tracks_list_) {
          if (itor.width != 0 && itor.height != 0) {
            tracks_list.emplace_back(itor);
            break;
          }
        }
        if (!tracks_list.empty()) {
          if (merge_remote_video_flag_) {
            rtc_client_ptr_->SetTranscodingLiveStreamingTracks(custom_merge_id_,
                                                               tracks_list);
          } else {
            rtc_client_ptr_->RemoveTranscodingLiveStreamingTracks(
                custom_merge_id_, tracks_list);
          }
        }
      }
      break;
    default:
      break;
  }
  return 0;
}

afx_msg LRESULT CRtcDemoV2::OnSendMessage(WPARAM wParam, LPARAM lParam) {
  if (rtc_client_ptr_) {
    LPTSTR lpMessage = (LPTSTR)lParam;
    _bstr_t bstr("");
    LPTSTR strTmp = lpMessage;
    bstr = strTmp;
    std::string strMsg = bstr;
    list<string> users_list;
    rtc_client_ptr_->SendMessage(users_list, "", string_to_utf8(strMsg));
  }
  return 0;
}

void CRtcDemoV2::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) {
  // TODO: Add your message handler code here and/or call default
  lock_guard<recursive_mutex> lck(mutex_);
  if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER_RECORD) {
    int pos = ((CSliderCtrl *)GetDlgItem(IDC_SLIDER_RECORD))->GetPos();
    if (microphone_audio_track_ptr_) {
      microphone_audio_track_ptr_->SetVolume(pos / 10.0f);
    } else if (custom_audio_track_ptr_) {
      custom_audio_track_ptr_->SetVolume(pos / 10.0f);
    }
  } else if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER_PLAYOUT) {
    int pos = ((CSliderCtrl *)GetDlgItem(IDC_SLIDER_PLAYOUT))->GetPos();
    RemoteUserList remoteList = rtc_client_ptr_->GetRemoteUsers();
    for (auto &&itor : remoteList) {
      for (auto &&audio_itor : itor.remote_audio_track_list) {
        audio_itor->SetVolume(pos / 10.0f);
      }
    }
  }

  __super::OnHScroll(nSBCode, nPos, pScrollBar);
  __super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CRtcDemoV2::OnTimer(UINT_PTR nIDEvent) {
  if (UPDATE_TIME_DURATION_TIMER == nIDEvent) {
    // 更新连麦时间
    chrono::seconds df_time = chrono::duration_cast<chrono::seconds>(
        chrono::steady_clock::now() - start_tp_);
    int hour = df_time.count() / 3600;
    int minute = df_time.count() % 3600 / 60;
    int sec = df_time.count() % 3600 % 60;
    wchar_t time_buf[128] = {0};
    wsprintf(time_buf, _T("连麦时长：%02d:%02d:%02d"), hour, minute, sec);
    wnd_status_bar_.SetText(time_buf, 0, 0);
  }
}

void CRtcDemoV2::OnDestroy() {
  // 结束外部数据导入的线程
  destory_main_dialog_ = true;
  if (dlg_stats_) {
    dlg_stats_->DestroyWindow();
    delete dlg_stats_;
    dlg_stats_ = nullptr;
  }
  stop_stats_flag_ = true;
  stop_video_external_flag_ = true;
  stop_audio_external_flag_ = true;
  if (fake_audio_thread_.joinable()) {
    fake_audio_thread_.join();
  }
  if (fake_video_thread_.joinable()) {
    fake_video_thread_.join();
  }
  if (stats_thread_.joinable()) {
    stats_thread_.join();
  }

  if (rtc_client_ptr_) {
    rtc_client_ptr_->Leave();
  }

  if (camera_track_ptr_) {
    QNRTC::DestroyLocalTrack(camera_track_ptr_);
    camera_track_ptr_ = nullptr;
  }

  if (screen_track_ptr_) {
    QNRTC::DestroyLocalTrack(screen_track_ptr_);
    screen_track_ptr_ = nullptr;
  }

  if (custom_video_track_ptr_) {
    QNRTC::DestroyLocalTrack(custom_video_track_ptr_);
    custom_video_track_ptr_ = nullptr;
  }

  if (microphone_audio_track_ptr_) {
    QNRTC::DestroyLocalTrack(microphone_audio_track_ptr_);
    microphone_audio_track_ptr_ = nullptr;
  }

  if (custom_audio_track_ptr_) {
    QNRTC::DestroyLocalTrack(custom_audio_track_ptr_);
    custom_audio_track_ptr_ = nullptr;
  }

  local_published_track_list_.clear();
  remote_tracks_map_.clear();
  remote_transcoding_tracks_list_.clear();

  if (rtc_client_ptr_) {
    QNRTC::DestroyClient(rtc_client_ptr_);
    rtc_client_ptr_ = nullptr;
  }
  QNRTC::DeInint();
  CDialogEx::OnDestroy();
}

void CRtcDemoV2::OnBnClickedCancel() {
  // TODO: Add your control notification handler code here
  CDialogEx::OnCancel();
}

void CRtcDemoV2::OnBnClickedButtonLogin() {
  CString btn_str;
  GetDlgItemText(IDC_BUTTON_LOGIN, btn_str);
  // 登录房间
  if (btn_str.CompareNoCase(_T("登录")) == 0) {
    GetDlgItemText(IDC_EDIT_APPID, app_id_);
    GetDlgItemText(IDC_EDIT_ROOM_ID, room_name_);
    GetDlgItemText(IDC_EDIT_PLAYER_ID, user_id_);
    if (room_name_.IsEmpty() || user_id_.IsEmpty()) {
      MessageBox(_T("Room ID and Player ID can't be NULL!"));
      return;
    }
    WriteConfigFile();

    GetDlgItem(IDC_BUTTON_LOGIN)->SetWindowText(_T("登录中"));
    GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(FALSE);

    // 向 AppServer 获取 token
    room_token_.clear();
    int ret = GetRoomToken_s(unicode2utf(app_id_.GetBuffer()),
                             unicode2utf(room_name_.GetBuffer()),
                             unicode2utf(user_id_.GetBuffer()),
                             "api-demo.qnsdk.com", 5, room_token_);
    if (ret != 0) {
      CString msg_str;
      msg_str.Format(_T("获取房间 token 失败，请检查您的网络是否正常！Err:%d"),
                     ret);
      wnd_status_bar_.SetText(msg_str.GetBuffer(), 1, 0);
      // MessageBox(_T("获取房间 token 失败，请检查您的网络是否正常！"));
      GetDlgItem(IDC_BUTTON_LOGIN)->SetWindowText(_T("登录"));
      GetDlgItem(IDC_BUTTON_LOGIN)->EnableWindow(TRUE);

      thread([=] {
        Sleep(2000);
        PostMessage(WM_COMMAND, MAKEWPARAM(IDC_BUTTON_LOGIN, BN_CLICKED), NULL);
      }).detach();
      return;
    }
    if (_strnicmp(const_cast<char *>(unicode2utf(user_id_.GetBuffer()).c_str()),
                  "admin", unicode2utf(user_id_.GetBuffer()).length()) == 0) {
      contain_admin_flag_ = true;
    } else {
      contain_admin_flag_ = false;
    }

    wnd_status_bar_.SetText(_T("获取房间 token 成功！"), 1, 0);
    rtc_client_ptr_->Join(room_token_);
  } else {
    is_joined_ = false;
    if (dlg_stats_) {
      dlg_stats_->DestroyWindow();
      delete dlg_stats_;
      dlg_stats_ = nullptr;
    }
    stop_stats_flag_ = true;
    stop_video_external_flag_ = true;
    stop_audio_external_flag_ = true;
    if (fake_audio_thread_.joinable()) {
      fake_audio_thread_.join();
    }
    if (fake_video_thread_.joinable()) {
      fake_video_thread_.join();
    }
    if (stats_thread_.joinable()) {
      stats_thread_.join();
    }
    rtc_client_ptr_->Leave();
    remote_tracks_map_.clear();
    remote_transcoding_tracks_list_.clear();
    SetDlgItemText(IDC_BUTTON_LOGIN, _T("登录"));
    wnd_status_bar_.SetText(_T("当前未登录房间！"), 1, 0);
    user_list_ctrl_.DeleteAllItems();
    ((CButton *)GetDlgItem(IDC_CHECK_MUTE_AUDIO))->SetCheck(0);
    ((CButton *)GetDlgItem(IDC_CHECK_MUTE_VIDEO))->SetCheck(0);
    GetDlgItem(IDC_BUTTON_FORWARD)->EnableWindow(FALSE);
    ((CButton *)GetDlgItem(IDC_CHECK_IMPORT_STATS))->SetCheck(0);
    KillTimer(UPDATE_TIME_DURATION_TIMER);
    Invalidate();
  }
}

void CRtcDemoV2::OnBnClickedBtnFlush() {
  ((CComboBox *)GetDlgItem(IDC_COMBO_SCREEN))->ResetContent();
  int screen_count = QNRTC::GetScreenWindowCount();
  for (int i(0); i < screen_count; ++i) {
    qiniu::QNScreenWindowInfo sw = QNRTC::GetScreenWindowInfo(i);
    screen_info_map_[sw.id] = sw;
    ((CComboBox *)GetDlgItem(IDC_COMBO_SCREEN))
        ->InsertString(-1, utf2unicode(sw.title).c_str());
  }
  ((CComboBox *)GetDlgItem(IDC_COMBO_SCREEN))->SetCurSel(0);
}

void CRtcDemoV2::OnBnClickedBtnFlushCamera() {
  ((CComboBox *)GetDlgItem(IDC_COMBO_CAMERA))->ResetContent();
  int camera_count = QNRTC::GetCameraCount();
  for (int i(0); i < camera_count; ++i) {
    qiniu::QNCameraInfo ci = QNRTC::GetCameraInfo(i);
    camera_dev_map_[ci.id] = ci;
    ((CComboBox *)GetDlgItem(IDC_COMBO_CAMERA))
        ->InsertString(-1, utf2unicode(ci.name).c_str());
  }
  ((CComboBox *)GetDlgItem(IDC_COMBO_CAMERA))->SetCurSel(0);
}

void CRtcDemoV2::OnBnClickedCheckCamera() {
  if (!rtc_client_ptr_) {
    return;
  }
  lock_guard<recursive_mutex> lck(mutex_);
  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_CAMERA))->GetCheck()) {
    CString video_dev_name;
    string video_dev_id;
    int audio_recorder_device_index(-1);

    GetDlgItem(IDC_COMBO_CAMERA)->GetWindowTextW(video_dev_name);
    if (video_dev_name.IsEmpty()) {
      thread([] { AfxMessageBox(_T("您当前没有任何视频设备！")); }).detach();
      return;
    }

    auto itor = camera_dev_map_.begin();
    while (itor != camera_dev_map_.end()) {
      if (itor->second.name.compare(unicode2utf(video_dev_name.GetBuffer())) ==
          0) {
        video_dev_id = itor->first;
        break;
      }
      ++itor;
    }
    LocalTrackList track_list;
    // 这里只做参考，用户根据自己需求选择合适分辨率
    auto camera_size =
        FindBestVideoSize(camera_dev_map_[video_dev_id].capabilities);
    int width = std::get<0>(camera_size);
    int height = std::get<1>(camera_size);
    QNCameraVideoTrackConfig camera_info = {{width, height, 30, 2000},
                                            {width, height, 30},
                                            video_dev_id,
                                            CAMERA_TAG,
                                            enable_simulcast_};

    auto camera_track_ptr = QNRTC::CreateCameraVideoTrack(camera_info, this);
    if (camera_track_ptr) {
      camera_track_ptr_ = camera_track_ptr;
      track_list.emplace_back(camera_track_ptr);
      local_published_track_list_.emplace_back(camera_track_ptr);
    } else {
      auto itor = local_published_track_list_.begin();
      for (; itor != local_published_track_list_.end();) {
        if (*itor == camera_track_ptr_) {
          local_published_track_list_.erase(itor++);
          break;
        } else {
          itor++;
        }
      }
      QNRTC::DestroyLocalTrack(camera_track_ptr_);
      camera_track_ptr_ = nullptr;
      camera_track_ptr_ = QNRTC::CreateCameraVideoTrack(camera_info, this);
      track_list.emplace_back(camera_track_ptr_);
      local_published_track_list_.emplace_back(camera_track_ptr_);
    }
    QNView view = {GetDlgItem(IDC_STATIC_VIDEO_PREVIEW)->m_hWnd, kRenderD3D,
                   kVideoRotation0, kStretchFit, false};
    camera_track_ptr_->Play(view);

    if (!track_list.empty() && is_joined_) {
      rtc_client_ptr_->Publish(track_list, this);
      wnd_status_bar_.SetText(_T("发布摄像头"), 1, 0);
    }
  } else {
    qiniu::LocalTrackList track_list;
    if (local_published_track_list_.empty()) {
      return;
    }
    auto itor = local_published_track_list_.begin();
    for (; itor != local_published_track_list_.end();) {
      if (*itor == camera_track_ptr_ &&
        camera_track_ptr_->GetTrackID().compare("") != 0) {
        track_list.emplace_back(*itor);

        if (is_joined_) {
          rtc_client_ptr_->UnPublish(track_list);
          wnd_status_bar_.SetText(_T("取消发布摄像头"), 1, 0);
          local_published_track_list_.erase(itor++);
        }
        break;
      } else {
        itor++;
      }
    }
    GetDlgItem(IDC_STATIC_VIDEO_PREVIEW)->Invalidate();
  }
}

void CRtcDemoV2::OnBnClickedCheckScreen() {
  // TODO: Add your control notification handler code here
  if (!rtc_client_ptr_) {
    return;
  }
  lock_guard<recursive_mutex> lck(mutex_);
  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_SCREEN))->GetCheck()) {
    qiniu::LocalTrackList track_list;
    CString wnd_title;
    string source_id;
    GetDlgItem(IDC_COMBO_SCREEN)->GetWindowTextW(wnd_title);
    for (auto &&itor : screen_info_map_) {
      if (itor.second.title.compare(unicode2utf(wnd_title.GetBuffer())) == 0) {
        source_id = itor.first;
        break;
      }
    }
    bool is_window_graphics_capture_enabled = false;
    if (1 == ((CButton *)GetDlgItem(IDC_CHECK_WINDOWS_GRAPHICS))->GetCheck()) {
      is_window_graphics_capture_enabled = true;
    }
    QNScreenVideoTrackConfig screen_info{{1920, 1080, 30, 2000},
                                         source_id,
                                         SCREENCASTS_TAG,
                                         enable_simulcast_,
                                         is_window_graphics_capture_enabled};
    auto screen_track_ptr = QNRTC::CreateScreenVideoTrack(screen_info);
    if (screen_track_ptr) {
      screen_track_ptr_ = screen_track_ptr;
      track_list.emplace_back(screen_track_ptr);
      local_published_track_list_.emplace_back(screen_track_ptr);
    } else {
      auto itor = local_published_track_list_.begin();
      for (; itor != local_published_track_list_.end();) {
        if (*itor == screen_track_ptr_) {
          local_published_track_list_.erase(itor++);
          break;
        } else {
          itor++;
        }
      }
      QNRTC::DestroyLocalTrack(screen_track_ptr_);
      screen_track_ptr_ = nullptr;
      screen_track_ptr_ = QNRTC::CreateScreenVideoTrack(screen_info);
      track_list.emplace_back(screen_track_ptr_);
      local_published_track_list_.emplace_back(screen_track_ptr_);
    }
    QNView view = {GetDlgItem(IDC_STATIC_VIDEO_PREVIEW2)->m_hWnd, kRenderD3D,
                   kVideoRotation0, kStretchFit, false};
    screen_track_ptr_->Play(view);

    if (!track_list.empty() && is_joined_) {
      rtc_client_ptr_->Publish(track_list, this);
      wnd_status_bar_.SetText(_T("发布屏幕分享"), 1, 0);
    }
  } else {
    qiniu::LocalTrackList track_list;
    if (local_published_track_list_.empty()) {
      return;
    }
    auto itor = local_published_track_list_.begin();
    for (; itor != local_published_track_list_.end();) {
      if (*itor == screen_track_ptr_ && 
        screen_track_ptr_->GetTrackID().compare("") != 0) {
        track_list.emplace_back(*itor);

        if (is_joined_) {
          rtc_client_ptr_->UnPublish(track_list);
          wnd_status_bar_.SetText(_T("取消发布屏幕分享"), 1, 0);
          local_published_track_list_.erase(itor++);
        }
        break;
      } else {
        itor++;
      }
    }
    GetDlgItem(IDC_STATIC_VIDEO_PREVIEW2)->Invalidate();
  }
}

void CRtcDemoV2::OnBnClickedCheckMicrophone() {
  if (!rtc_client_ptr_) {
    return;
  }
  ((CButton *)GetDlgItem(IDC_CHECK_MUTE_AUDIO))->SetCheck(0);

  lock_guard<recursive_mutex> lck(mutex_);
  stop_audio_external_flag_ = true;
  qiniu::LocalTrackList track_list;
  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_AUDIO))->GetCheck()) {
    ((CButton *)GetDlgItem(IDC_CHECK_IMPORT_RAW_AUDIO_DATA))->SetCheck(0);
    QNMicrophoneAudioTrackConfig micro_info = {{0, 0, 0, 32}, MICROPHONE_TAG};
    auto microphone_audio_track_ptr =
        QNRTC::CreateMicrophoneAudioTrack(micro_info);
    if (microphone_audio_track_ptr) {
      microphone_audio_track_ptr_ = microphone_audio_track_ptr;
      track_list.emplace_back(microphone_audio_track_ptr);
      local_published_track_list_.emplace_back(microphone_audio_track_ptr);
    } else {
      if (custom_audio_track_ptr_) {
        auto itor = local_published_track_list_.begin();
        for (; itor != local_published_track_list_.end();) {
          if (*itor == custom_audio_track_ptr_) {
            local_published_track_list_.erase(itor++);
            break;
          } else {
            itor++;
          }
        }
        QNRTC::DestroyLocalTrack(custom_audio_track_ptr_);
        custom_audio_track_ptr_ = nullptr;
      }
      if (microphone_audio_track_ptr_) {
        auto itor = local_published_track_list_.begin();
        for (; itor != local_published_track_list_.end();) {
          if (*itor == microphone_audio_track_ptr_) {
            local_published_track_list_.erase(itor++);
            break;
          } else {
            itor++;
          }
        }
        QNRTC::DestroyLocalTrack(microphone_audio_track_ptr_);
        microphone_audio_track_ptr_ = nullptr;
      }
      microphone_audio_track_ptr_ =
          QNRTC::CreateMicrophoneAudioTrack(micro_info);
      track_list.emplace_back(microphone_audio_track_ptr_);
      local_published_track_list_.emplace_back(microphone_audio_track_ptr_);
    }
    if (!track_list.empty() && is_joined_) {
      rtc_client_ptr_->Publish(track_list, this);
      wnd_status_bar_.SetText(_T("发布麦克风"), 1, 0);
    }
  } else {
    if (local_published_track_list_.empty()) {
      return;
    }
    auto itor = local_published_track_list_.begin();
    for (; itor != local_published_track_list_.end();) {
      if (*itor == microphone_audio_track_ptr_ && 
        microphone_audio_track_ptr_->GetTrackID().compare("") != 0) {
        track_list.emplace_back(*itor);

        if (is_joined_) {
          rtc_client_ptr_->UnPublish(track_list);
          wnd_status_bar_.SetText(_T("取消发布麦克风"), 1, 0);
          local_published_track_list_.erase(itor++);
        }
        break;
      } else {
        itor++;
      }
    }
  }
}

void CRtcDemoV2::OnBnClickedCheckImportRawVideoData() {
  if (!rtc_client_ptr_) {
    return;
  }
  lock_guard<recursive_mutex> lck(mutex_);
  qiniu::LocalTrackList track_list;
  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_IMPORT_RAW_DATA))->GetCheck()) {
    stop_video_external_flag_ = false;
    // 这里需要设置的是外部视频源对应的分辨率和帧率
    QNCustomVideoTrackConfig video_custom_info = {
        {426, 240, 30, 300}, EXTERNAL_VIDEO, enable_simulcast_};
    auto custom_video_track_ptr =
        QNRTC::CreateCustomVideoTrack(video_custom_info);
    if (custom_video_track_ptr) {
      custom_video_track_ptr_ = custom_video_track_ptr;
      track_list.emplace_back(custom_video_track_ptr);
      local_published_track_list_.emplace_back(custom_video_track_ptr);
    } else {
      auto itor = local_published_track_list_.begin();
      for (; itor != local_published_track_list_.end();) {
        if (*itor == custom_video_track_ptr_) {
          local_published_track_list_.erase(itor++);
          break;
        } else {
          itor++;
        }
      }
      QNRTC::DestroyLocalTrack(custom_video_track_ptr_);
      custom_video_track_ptr_ = nullptr;
      custom_video_track_ptr_ =
          QNRTC::CreateCustomVideoTrack(video_custom_info);
      track_list.emplace_back(custom_video_track_ptr_);
      local_published_track_list_.emplace_back(custom_video_track_ptr_);
    }

    QNView view = {GetDlgItem(IDC_STATIC_VIDEO_PREVIEW3)->m_hWnd, kRenderD3D,
                   kVideoRotation0, kStretchFit, false};
    custom_video_track_ptr_->Play(view);
    if (!track_list.empty() && is_joined_) {
      rtc_client_ptr_->Publish(track_list, this);
      wnd_status_bar_.SetText(_T("发布外部视频导入"), 1, 0);
    }
  } else {
    stop_video_external_flag_ = true;
    if (local_published_track_list_.empty()) {
      return;
    }
    auto itor = local_published_track_list_.begin();
    for (; itor != local_published_track_list_.end();) {
      if (*itor == custom_video_track_ptr_ && 
        custom_video_track_ptr_->GetTrackID().compare("") != 0) {
        track_list.emplace_back(*itor);

        if (is_joined_) {
          rtc_client_ptr_->UnPublish(track_list);
          wnd_status_bar_.SetText(_T("取消发布视频外部导入"), 1, 0);
          local_published_track_list_.erase(itor++);
        }
        break;
      } else {
        itor++;
      }
    }
    GetDlgItem(IDC_STATIC_VIDEO_PREVIEW3)->Invalidate();
    if (fake_video_thread_.joinable()) {
      fake_video_thread_.join();
    }
  }
}

void CRtcDemoV2::OnBnClickedCheckImportRawAudioData() {
  if (!rtc_client_ptr_) {
    return;
  }
  lock_guard<recursive_mutex> lck(mutex_);
  LocalTrackList track_list;
  if (1 ==
      ((CButton *)GetDlgItem(IDC_CHECK_IMPORT_RAW_AUDIO_DATA))->GetCheck()) {
    stop_audio_external_flag_ = false;
    ((CButton *)GetDlgItem(IDC_CHECK_AUDIO))->SetCheck(0);
    QNCustomAudioTrackConfig custom_audio_info = {{44100, 2, 16, 32},
                                                  EXTERNAL_AUDIO};
    auto custom_audio_track_ptr =
        QNRTC::CreateCustomAudioTrack(custom_audio_info);
    if (custom_audio_track_ptr) {
      custom_audio_track_ptr_ = custom_audio_track_ptr;
      track_list.emplace_back(custom_audio_track_ptr);
      local_published_track_list_.emplace_back(custom_audio_track_ptr);
    } else {
      if (microphone_audio_track_ptr_) {
        auto itor = local_published_track_list_.begin();
        for (; itor != local_published_track_list_.end();) {
          if (*itor == microphone_audio_track_ptr_) {
            local_published_track_list_.erase(itor++);
            break;
          } else {
            itor++;
          }
        }
        QNRTC::DestroyLocalTrack(microphone_audio_track_ptr_);
        microphone_audio_track_ptr_ = nullptr;
      }
      if (custom_audio_track_ptr_) {
        auto itor = local_published_track_list_.begin();
        for (; itor != local_published_track_list_.end();) {
          if (*itor == custom_audio_track_ptr_) {
            local_published_track_list_.erase(itor++);
            break;
          } else {
            itor++;
          }
        }
        QNRTC::DestroyLocalTrack(custom_audio_track_ptr_);
        custom_audio_track_ptr_ = nullptr;
      }
      custom_audio_track_ptr_ =
          QNRTC::CreateCustomAudioTrack(custom_audio_info);
      track_list.emplace_back(custom_audio_track_ptr_);
      local_published_track_list_.emplace_back(custom_audio_track_ptr_);
    }
    if (!track_list.empty() && is_joined_) {
      rtc_client_ptr_->Publish(track_list, this);
      wnd_status_bar_.SetText(_T("发布外部音频导入"), 1, 0);
    }
  } else {
    stop_audio_external_flag_ = true;
    if (local_published_track_list_.empty()) {
      return;
    }
    auto itor = local_published_track_list_.begin();
    for (; itor != local_published_track_list_.end();) {
      if (*itor == custom_audio_track_ptr_ &&
          custom_audio_track_ptr_->GetTrackID().compare("") != 0) {
        track_list.emplace_back(*itor);

        if (is_joined_) {
          rtc_client_ptr_->UnPublish(track_list);
          wnd_status_bar_.SetText(_T("取消发布音频外部导入"), 1, 0);
          local_published_track_list_.erase(itor++);
        }
        break;
      } else {
        itor++;
      }
    }
    if (fake_audio_thread_.joinable()) {
      fake_audio_thread_.join();
    }
  }
}

void CRtcDemoV2::OnBnClickedCheckDesktopAudio() {
  if (!microphone_audio_track_ptr_ && !custom_audio_track_ptr_) {
    return;
  }

  bool mixDesktopAudio = false;
  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_DESKTOP_AUDIO))->GetCheck()) {
    mixDesktopAudio = true;
  }

  if (microphone_audio_track_ptr_) {
    microphone_audio_track_ptr_->MixAudioWithSoundCard(mixDesktopAudio);
  } else if (custom_audio_track_ptr_) {
    custom_audio_track_ptr_->MixAudioWithSoundCard(mixDesktopAudio);
  }
}

void CRtcDemoV2::OnBnClickedCheckMuteAudio() {
  if (!microphone_audio_track_ptr_ && !custom_audio_track_ptr_) {
    return;
  }
  // 静默本地音频 Track，一端仅有一路音频 Track
  bool muteAudio = false;
  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_MUTE_AUDIO))->GetCheck()) {
    muteAudio = true;
  }

  if (microphone_audio_track_ptr_) {
    microphone_audio_track_ptr_->SetMuted(muteAudio);
  } else if (custom_audio_track_ptr_) {
    custom_audio_track_ptr_->SetMuted(muteAudio);
  }
}

void CRtcDemoV2::OnBnClickedCheckMuteVideo() {
  bool muteVideo = false;
  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_MUTE_VIDEO))->GetCheck()) {
    muteVideo = true;
  }
  // 静默或取消静默本地所有的视频 Track
  // 这里对所有的视频 Track 进行控制
  lock_guard<recursive_mutex> lck(mutex_);
  if (camera_track_ptr_) {
    camera_track_ptr_->SetMuted(muteVideo);
  }

  if (screen_track_ptr_) {
    screen_track_ptr_->SetMuted(muteVideo);
  }

  if (custom_video_track_ptr_) {
    custom_video_track_ptr_->SetMuted(muteVideo);
  }
}

void CRtcDemoV2::OnCbnSelchangeComboMicrophone() {
  // 输入音频设备配置
  if (!rtc_client_ptr_) {
    return;
  }
  int audio_recorder_device_index(-1);
  audio_recorder_device_index =
      ((CComboBox *)GetDlgItem(IDC_COMBO_MICROPHONE))->GetCurSel();
  audio_recorder_device_index =
      (audio_recorder_device_index == CB_ERR) ? 0 : audio_recorder_device_index;

  if (audio_recorder_device_index >= 0) {
    QNRTC::SetAudioRecordingDevice(audio_recorder_device_index);
  }
}

void CRtcDemoV2::OnCbnSelchangeComboPlayout() {
  // 播放音频设备配置
  if (!rtc_client_ptr_) {
    return;
  }
  int audio_playout_device_index(-1);
  audio_playout_device_index =
      ((CComboBox *)GetDlgItem(IDC_COMBO_PLAYOUT))->GetCurSel();
  audio_playout_device_index =
      (audio_playout_device_index == CB_ERR) ? 0 : audio_playout_device_index;

  if (audio_playout_device_index >= 0) {
    QNRTC::SetAudioRecordingDevice(audio_playout_device_index);
  }
}

void CRtcDemoV2::OnBnClickedButtonMerge() {
  MergeDialog dlgMerge;
  dlgMerge._merge_config = merge_config_;
  dlgMerge.DoModal();
}

void CRtcDemoV2::OnBnClickedButtonSendMsg() {
  // TODO: 在此添加控件通知处理程序代码
  dlg_msg_._user_id = user_id_;
  dlg_msg_.DoModal();
}

void CRtcDemoV2::OnCbnSelchangeComboLocalRotate() {
  // TODO: 在此添加控件通知处理程序代码
  int rotation_sel =
      ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_ROTATION))->GetCurSel();
  qiniu::QNVideoRotation video_rotation =
      qiniu::QNVideoRotation::kVideoRotation0;
  switch (rotation_sel) {
    case 0:
      video_rotation = qiniu::QNVideoRotation::kVideoRotation0;
      break;
    case 1:
      video_rotation = qiniu::QNVideoRotation::kVideoRotation90;
      break;
    case 2:
      video_rotation = qiniu::QNVideoRotation::kVideoRotation180;
      break;
    case 3:
      video_rotation = qiniu::QNVideoRotation::kVideoRotation270;
      break;
    default:
      break;
  }

  if (camera_track_ptr_) {
    QNView view = {GetDlgItem(IDC_STATIC_VIDEO_PREVIEW)->m_hWnd, kRenderD3D,
                   video_rotation, kStretchFit, false};
    camera_track_ptr_->Play(view);
  }

  if (screen_track_ptr_) {
    QNView view = {GetDlgItem(IDC_STATIC_VIDEO_PREVIEW2)->m_hWnd, kRenderD3D,
                   video_rotation, kStretchFit, false};
    screen_track_ptr_->Play(view);
  }

  if (custom_video_track_ptr_) {
    QNView view = {GetDlgItem(IDC_STATIC_VIDEO_PREVIEW3)->m_hWnd, kRenderD3D,
                   video_rotation, kStretchFit, false};
    custom_video_track_ptr_->Play(view);
  }
}

void CRtcDemoV2::OnCbnSelchangeComboRemoteRotate() {
  // TODO: 在此添加控件通知处理程序代码
  int rotation_sel =
      ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_ROTATION))->GetCurSel();
  qiniu::QNVideoRotation video_rotation =
      qiniu::QNVideoRotation::kVideoRotation0;
  switch (rotation_sel) {
    case 0:
      video_rotation = qiniu::QNVideoRotation::kVideoRotation0;
      break;
    case 1:
      video_rotation = qiniu::QNVideoRotation::kVideoRotation90;
      break;
    case 2:
      video_rotation = qiniu::QNVideoRotation::kVideoRotation180;
      break;
    case 3:
      video_rotation = qiniu::QNVideoRotation::kVideoRotation270;
      break;
    default:
      break;
  }

  ::SendMessage(this->GetSafeHwnd(), WM_ADD_SIG_DLG_PRIV_MSG,
                WM_REMOTE_VIDEO_RENDER_MODIFY_ROTATE_MSG,
                (LPARAM)video_rotation);
}

void CRtcDemoV2::OnCbnSelchangeComboSubscribeProfile() {
  int profile_sel =
  ((CComboBox*)GetDlgItem(IDC_COMBO_SUBSCRIBE_PROFILE))->GetCurSel();
  qiniu::QNTrackProfile video_profile = qiniu::kHigh;
  switch (profile_sel)
  {
  case 0:
    video_profile = qiniu::kHigh;
    break;
  case 1:
    video_profile = qiniu::kMedium;
    break;
  case 2:
    video_profile = qiniu::kLow;
    break;
  default:
    break;
  }

  // 这里只是演示时是将所有订阅流一起切成设置的 profile，用户实际根据自己需求，更改相应track的profile，
  // 想要切换成哪种 profile，只要将对应的 mChooseToSub 设置为 true。
  lock_guard<recursive_mutex> lck(mutex_);
  RemoteUserList remoteList;
  if (rtc_client_ptr_) {
    remoteList = rtc_client_ptr_->GetRemoteUsers();
  }

  for (auto&& itor : remoteList) {
    for (auto &&video_itor : itor.remote_video_track_list) {
      if (video_itor->IsMultiProfileEnabled()) {
        video_itor->SetProfile(video_profile);
      }
    }
  }
}

void CRtcDemoV2::OnBnClickedButtonSimulcast() {
  CString btn_str;
  GetDlgItemText(IDC_BUTTON_SIMULCAST, btn_str);
  if (btn_str.CompareNoCase(_T("开启多流")) == 0) {
    SetDlgItemText(IDC_BUTTON_SIMULCAST, _T("关闭多流"));
    enable_simulcast_ = true;
  } else {
    SetDlgItemText(IDC_BUTTON_SIMULCAST, _T("开启多流"));
    enable_simulcast_ = false;
  }
}

void CRtcDemoV2::OnBnClickedButtonForward() {
  CString btn_str;
  custom_forward_id_ = "window-forward";
  qiniu::QNDirectLiveStreamingConfig forwardInfo;
  forwardInfo.stream_id = custom_forward_id_;
  forwardInfo.publish_url =
  "rtmp://pili-publish.qnsdk.com/sdk-live/window-forward";
  GetDlgItemText(IDC_BUTTON_FORWARD, btn_str);
  if (camera_track_ptr_) {
    forwardInfo.local_video_track = camera_track_ptr_;
  }
  if (microphone_audio_track_ptr_) {
    forwardInfo.local_audio_track = microphone_audio_track_ptr_;
  }
  if (btn_str.CompareNoCase(_T("单流转推")) == 0) {
    SetDlgItemText(IDC_BUTTON_FORWARD, _T("停止单流转推"));
    if (rtc_client_ptr_) {
      rtc_client_ptr_->StartLiveStreaming(forwardInfo);
    }
  } else {
    SetDlgItemText(IDC_BUTTON_FORWARD, _T("单流转推"));
    if (rtc_client_ptr_) {
      rtc_client_ptr_->StopLiveStreaming(forwardInfo);
    }
  }
}

void CRtcDemoV2::OnBnClickedCheckCameraImage() {
  if (!camera_track_ptr_) {
    return;
  }

  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_CAMERA_IMAGE))->GetCheck()) {
    camera_track_ptr_->PushImage("pause_publish.jpeg", kImagJpeg);
  } else {
    camera_track_ptr_->PushImage(nullptr, kImagJpeg);
  }
}

void CRtcDemoV2::OnBnClickedCheckCameraMirror() {
  if (!camera_track_ptr_) {
    return;
  }

  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_CAMERA_MIRROR))->GetCheck()) {
    camera_track_ptr_->SetMirror(true);
  } else {
    camera_track_ptr_->SetMirror(false);
  }
}

void CRtcDemoV2::OnBnClickedBtnSei() {
  CString btn_str;
  GetDlgItemText(IDC_BTN_SEI, btn_str);
  string uuid = "\x14\x16\x17\x18\x14\x16\x17\x28\x14\x16\x17\x38\x14\x16\x17\x58";
  if (btn_str.CompareNoCase(_T("开启SEI")) == 0) {
    SetDlgItemText(IDC_BTN_SEI, _T("关闭SEI"));
    if (camera_track_ptr_) {
      camera_track_ptr_->SendSEI(string_to_utf8("七牛SEI" + std::to_string(rand())), uuid, -1);
    }
    if (screen_track_ptr_) {
      screen_track_ptr_->SendSEI(string_to_utf8("七牛SEI" + std::to_string(rand())), uuid, -1);
    }
    if (custom_video_track_ptr_) {
      custom_video_track_ptr_->SendSEI(string_to_utf8("七牛SEI" + std::to_string(rand())), uuid, -1);
    }
  } else {
    SetDlgItemText(IDC_BTN_SEI, _T("开启SEI"));
    if (camera_track_ptr_) {
      camera_track_ptr_->SendSEI(string_to_utf8("七牛SEI" + std::to_string(rand())), uuid, 0);
    }
    if (screen_track_ptr_) {
      screen_track_ptr_->SendSEI(string_to_utf8("七牛SEI" + std::to_string(rand())), uuid, 0);
    }
    if (custom_video_track_ptr_) {
      custom_video_track_ptr_->SendSEI(string_to_utf8("七牛SEI" + std::to_string(rand())), uuid, 0);
    }
  }
}

void CRtcDemoV2::OnBnClickedCheckLocalMirror() {
  bool mirror_flag = false;
  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_LOCAL_MIRROR))->GetCheck()) {
    mirror_flag = true;
  } else {
    mirror_flag = false;
  }

  if (camera_track_ptr_) {
    QNView view = {GetDlgItem(IDC_STATIC_VIDEO_PREVIEW)->m_hWnd, kRenderD3D,
                   kVideoRotation0, kStretchFit, mirror_flag};
    camera_track_ptr_->Play(view);
  }

  if (screen_track_ptr_) {
    QNView view = {GetDlgItem(IDC_STATIC_VIDEO_PREVIEW2)->m_hWnd, kRenderD3D,
                   kVideoRotation0, kStretchFit, mirror_flag};
    screen_track_ptr_->Play(view);
  }

  if (custom_video_track_ptr_) {
    QNView view = {GetDlgItem(IDC_STATIC_VIDEO_PREVIEW3)->m_hWnd, kRenderD3D,
                   kVideoRotation0, kStretchFit, mirror_flag};
    custom_video_track_ptr_->Play(view);
  }
}

void CRtcDemoV2::OnBnClickedCheckRemoteMirror() {
  bool mirror_flag = false;
  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_REMOTE_MIRROR))->GetCheck()) {
    mirror_flag = true;
  } else {
    mirror_flag = false;
  }
  ::SendMessage(this->GetSafeHwnd(), WM_ADD_SIG_DLG_PRIV_MSG,
                WM_REMOTE_VIDEO_RENDER_MODIFY_MIRROR_MSG, (LPARAM)mirror_flag);
}

void CRtcDemoV2::OnCbnSelchangeComboLocalStretchMode() {
  qiniu::QNStretchMode stretch_mode = kStretchFit;
  int clip_crop_source =
      ((CComboBox *)GetDlgItem(IDC_COMBO_LOCAL_STRETCH_MODE))->GetCurSel();
  if (clip_crop_source == 0) {
    stretch_mode = kStretchFit;
  } else if (clip_crop_source == 1) {
    stretch_mode = kStretchFill;
  } else {
    stretch_mode = kStretchScaleToFit;
  }

  if (camera_track_ptr_) {
    QNView view = {GetDlgItem(IDC_STATIC_VIDEO_PREVIEW)->m_hWnd, kRenderD3D,
                   kVideoRotation0, stretch_mode, false};
    camera_track_ptr_->Play(view);
  }

  if (screen_track_ptr_) {
    QNView view = {GetDlgItem(IDC_STATIC_VIDEO_PREVIEW2)->m_hWnd, kRenderD3D,
                   kVideoRotation0, stretch_mode, false};
    screen_track_ptr_->Play(view);
  }

  if (custom_video_track_ptr_) {
    QNView view = {GetDlgItem(IDC_STATIC_VIDEO_PREVIEW3)->m_hWnd, kRenderD3D,
                   kVideoRotation0, stretch_mode, false};
    custom_video_track_ptr_->Play(view);
  }
}

void CRtcDemoV2::OnCbnSelchangeComboRemoteStretchMode() {
  qiniu::QNStretchMode stretch_mode = kStretchFit;
  int clip_crop_source =
      ((CComboBox *)GetDlgItem(IDC_COMBO_REMOTE_STRETCH_MODE))->GetCurSel();
  if (clip_crop_source == 0) {
    stretch_mode = kStretchFit;
  } else if (clip_crop_source == 1) {
    stretch_mode = kStretchFill;
  } else {
    stretch_mode = kStretchScaleToFit;
  }

  ::SendMessage(this->GetSafeHwnd(), WM_ADD_SIG_DLG_PRIV_MSG,
                WM_REMOTE_VIDEO_RENDER_MODIFY_STRETCH_MSG,
                (LPARAM)stretch_mode);
}

void CRtcDemoV2::OnBnClickedCheckClip() {
  int clip_crop_source =
      ((CComboBox *)GetDlgItem(IDC_COMBO_CLIP_CROP))->GetCurSel();
  int cropX = GetDlgItemInt(IDC_EDIT_CLIP_X, NULL, 0);
  int cropY = GetDlgItemInt(IDC_EDIT_CLIP_Y, NULL, 0);
  int width = GetDlgItemInt(IDC_EDIT_CLIP_WIDTH, NULL, 0);
  int height = GetDlgItemInt(IDC_EDIT_CLIP_HEIGHT, NULL, 0);
  bool flag = false;
  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_CLIP))->GetCheck()) {
    flag = true;
  } else {
    flag = false;
    cropX = 0;
    cropY = 0;
    width = 0;
    height = 0;
  }

  if (clip_crop_source == 0 && camera_track_ptr_) {
    if (flag) {
      camera_track_ptr_->StartCrop(cropX, cropY, width, height);
    } else {
      camera_track_ptr_->StopCrop();
    }
  } else if (clip_crop_source == 1 && custom_video_track_ptr_) {
    if (flag) {
      custom_video_track_ptr_->StartCrop(cropX, cropY, width, height);
    } else {
      custom_video_track_ptr_->StopCrop();
    }
  } else if (clip_crop_source == 2 && screen_track_ptr_) {
    if (flag) {
      screen_track_ptr_->StartCrop(cropX, cropY, width, height);
    } else {
      screen_track_ptr_->StopCrop();
    }
  }
}

void CRtcDemoV2::OnBnClickedCheckScale() {
  int clip_crop_source =
      ((CComboBox *)GetDlgItem(IDC_COMBO_CLIP_CROP))->GetCurSel();
  int width = GetDlgItemInt(IDC_EDIT_SCALE_WIDTH, NULL, 0);
  int height = GetDlgItemInt(IDC_EDIT_SCALE_HEIGHT, NULL, 0);
  bool flag = false;
  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_SCALE))->GetCheck()) {
    flag = true;
  } else {
    flag = false;
    width = 0;
    height = 0;
  }

  if (clip_crop_source == 0 && camera_track_ptr_) {
    if (flag) {
      camera_track_ptr_->StartScale(width, height);
    } else {
      camera_track_ptr_->StopScale();
    }
  } else if (clip_crop_source == 1 && custom_video_track_ptr_) {
    if (flag) {
      custom_video_track_ptr_->StartScale(width, height);
    } else {
      custom_video_track_ptr_->StopScale();
    }
  } else if (clip_crop_source == 2 && screen_track_ptr_) {
    if (flag) {
      screen_track_ptr_->StartScale(width, height);
    } else {
      screen_track_ptr_->StopScale();
    }
  }
}

void CRtcDemoV2::OnBnClickedCheckMerge() { AdjustMergeStreamLayouts(); }

void CRtcDemoV2::OnBnClickedCheckImportStats() {
  if (!rtc_client_ptr_) {
    return;
  }
  stop_stats_flag_ = true;
  if (stats_thread_.joinable()) {
    stats_thread_.join();
  }

  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_IMPORT_STATS))->GetCheck()) {
    if (!dlg_stats_) {
      dlg_stats_ = new StatsDialog;
      dlg_stats_->Create(IDD_DIALOG_STATS);
      AdjustStatsLayouts();
      dlg_stats_->ShowWindow(SW_SHOWNORMAL);
    }
    stop_stats_flag_ = false;
    stats_thread_ = thread([&] {
      while (!stop_stats_flag_) {
        QNLocalAudioTrackStatsMap local_audio_stats_map =
            rtc_client_ptr_->GetLocalAudioTrackStats();
        for (auto itor : local_audio_stats_map) {
          if (itor.second.uplink_bitrate != 0) {
            wchar_t dest_buf[1024] = {0};
            _snwprintf(dest_buf, sizeof(dest_buf),
                       _T("[发布]TrackId:%s, 码率:%d bps ,丢包率:%d, rtt:%d"),
                       utf2unicode(itor.first).c_str(),
                       itor.second.uplink_bitrate, itor.second.uplink_lostrate,
                       itor.second.uplink_rtt);
            if (dlg_stats_) {
              dlg_stats_->OnReceiveStatsText(unicode2utf(dest_buf));
            }
          }
        }

        QNLocalVideoTracksStatsMap local_video_stats_map =
            rtc_client_ptr_->GetLocalVideoTrackStats();
        for (auto stats_itor : local_video_stats_map) {
          for (auto itor : stats_itor.second) {
            string profile = "";
            if (itor.profile == kHigh) {
              profile = "high";
            } else if (itor.profile == kMedium) {
              profile = "medium";
            } else if (itor.profile == kLow) {
              profile = "low";
            }
            if (itor.uplink_bitrate != 0) {
              wchar_t dest_buf[1024] = {0};
              _snwprintf(dest_buf, sizeof(dest_buf),
                         _T("[发布]TrackId:%s, 帧率:%d ,码率:%d bps, ")
                         _T("丢包率:%d, rtt:%d, profile:%s"),
                         utf2unicode(stats_itor.first).c_str(),
                         itor.uplink_framerate, itor.uplink_bitrate,
                         itor.uplink_lostrate, itor.uplink_rtt,
                         utf2unicode(profile).c_str());
              if (dlg_stats_) {
                dlg_stats_->OnReceiveStatsText(unicode2utf(dest_buf));
              }
            }
          }
        }
        QNRemoteAudioTrackStatsMap remote_audio_stats_map =
            rtc_client_ptr_->GetRemoteAudioTrackStats();
        for (auto itor : remote_audio_stats_map) {
          if (itor.second.downlink_bitrate != 0) {
            wchar_t dest_buf[1024] = {0};
            _snwprintf(dest_buf, sizeof(dest_buf),
                       _T("[订阅]TrackId:%s, 码率:%d bps ,丢包率:%d"),
                       utf2unicode(itor.first).c_str(),
                       itor.second.downlink_bitrate,
                       itor.second.downlink_lostrate);
            if (dlg_stats_) {
              dlg_stats_->OnReceiveStatsText(unicode2utf(dest_buf));
            }
          }
        }

        QNRemoteVideoTrackStatsMap remote_video_stats_map =
            rtc_client_ptr_->GetRemoteVideoTrackStats();
        for (auto itor : remote_video_stats_map) {
          string profile = "";
          if (itor.second.profile == kHigh) {
            profile = "high";
          } else if (itor.second.profile == kMedium) {
            profile = "medium";
          } else if (itor.second.profile == kLow) {
            profile = "low";
          }
          if (itor.second.downlink_bitrate != 0) {
            wchar_t dest_buf[1024] = {0};
            _snwprintf(
                dest_buf, sizeof(dest_buf),
                _T("[订阅]TrackId:%s, 帧率:%d ,码率:%d bps, ")
                _T("丢包率:%d, profile:%s"),
                utf2unicode(itor.first).c_str(), itor.second.downlink_framerate,
                itor.second.downlink_bitrate, itor.second.downlink_lostrate,
                utf2unicode(profile).c_str());
            if (dlg_stats_) {
              dlg_stats_->OnReceiveStatsText(unicode2utf(dest_buf));
            }
          }
        }
        Sleep(1000);
      }
    });
  } else {
    stop_stats_flag_ = true;
    if (dlg_stats_) {
      dlg_stats_->DestroyWindow();
      delete dlg_stats_;
      dlg_stats_ = nullptr;
    }
  }
}

void CRtcDemoV2::OnBnClickedBtnKickout() {
  int index = user_list_ctrl_.GetSelectionMark();
  if (index == -1) {
    MessageBox(_T("请选中要踢出的用户！"));
    return;
  }
  // 所选择的用户当前没有发布媒体流
  CString user_id = user_list_ctrl_.GetItemText(index, 0);
  if (user_id_.Compare(user_id) == 0) {
    MessageBox(_T("不允许踢出自己！"));
    return;
  }
  if (rtc_client_ptr_) {
    rtc_client_ptr_->SendMessage({unicode2utf(user_id.GetBuffer()).c_str()}, "",
                                 string_to_utf8("kickout"));
  }
}

void CRtcDemoV2::OnBnClickedBtnSubscribe() {
  CString btn_str;
  GetDlgItemText(IDC_BTN_SUBSCRIBE, btn_str);
  RemoteUserList remote_user_list;
  RemoteTrackList unsub_track_list;
  RemoteTrackList sub_track_list;
  if (rtc_client_ptr_) {
    remote_user_list = rtc_client_ptr_->GetRemoteUsers();
  }
  for (auto &&itor : remote_user_list) {
    for (auto &&video_itor : itor.remote_video_track_list) {
      if (!video_itor->IsSubscribed()) {
        unsub_track_list.emplace_back(video_itor);
      } else {
        sub_track_list.emplace_back(video_itor);
      }
    }

    for (auto &&audio_itor : itor.remote_audio_track_list) {
      if (!audio_itor->IsSubscribed()) {
        unsub_track_list.emplace_back(audio_itor);
      } else {
        sub_track_list.emplace_back(audio_itor);
      }
    }
  }
  if (btn_str.CompareNoCase(_T("订阅流")) == 0) {
    SetDlgItemText(IDC_BTN_SUBSCRIBE, _T("取消订阅流"));
    if (!unsub_track_list.empty()) {
      rtc_client_ptr_->Subscribe(unsub_track_list);
    }
  } else {
    SetDlgItemText(IDC_BTN_SUBSCRIBE, _T("订阅流"));
    if (!sub_track_list.empty()) {
      rtc_client_ptr_->UnSubscribe(sub_track_list);
    }
    for (auto &&itor : sub_track_list) {
      string track_id = itor->GetTrackID();
      ::SendMessage(this->GetSafeHwnd(), WM_ADD_SIG_DLG_PRIV_MSG,
                    WM_REMOVE_REMOTE_VIDEO_RENDER_MSG,
                    (LPARAM)(track_id.c_str()));
    }
  }
}
