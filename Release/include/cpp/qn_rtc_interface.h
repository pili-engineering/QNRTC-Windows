#ifndef QN_RTC_INTERFACE_H
#define QN_RTC_INTERFACE_H

#include "qn_rtc_client_interface.h"
#include "qn_track_interface.h"

namespace qiniu {

struct QNRTCSetting {
  bool is_aec3_enabled = true;
  bool is_maintain_resolution_enabled = false;
  bool is_encoder_quality_mode_enabled = false;
  bool is_audio_route_to_speakerphone = true;
  QNLogLevel log_level = kLogInfo;
  QNTransportPolicy policy = kForceUDP;
  QNVideoEncoderType encoder_type = kEncodeOpenH264;
};

struct QNVideoEncoderConfig {
  int32_t width;
  int32_t height;
  int32_t frame_rate;
  int32_t bitrate;  // kbps
};

struct QNVideoCaptureConfig {
  int32_t width;
  int32_t height;
  int32_t frame_rate;
};

struct QNAudioQuality {
  int32_t sample_rate;
  int32_t channel_count;
  int32_t bits_per_sample;
  int32_t bitrate;  // kbps
};

struct QNCustomAudioTrackConfig {
  QNAudioQuality audio_quality;
  string tag;  // 自定义标签
};

struct QNMicrophoneAudioTrackConfig {
  QNAudioQuality audio_quality;
  string tag;  // 自定义标签
};

struct QNCameraVideoTrackConfig {
  QNVideoEncoderConfig encoder_config;
  QNVideoCaptureConfig capture_config;
  std::string id;              // 设备 id
  std::string tag;             // 自定义标签
  bool multi_profile_enabled;  // 是否开启多流
#if defined(QN_ANDROID) || defined(QN_IOS)
  QNCameraFacing camera_facing; // 前后置摄像头
#endif
};

#ifndef QN_LINUX
struct QNScreenVideoTrackConfig {
  QNVideoEncoderConfig encoder_config;
  std::string id;              // 窗口 id
  std::string tag;             // 自定义标签
  bool multi_profile_enabled;  // 是否开启多流
  bool is_window_graphics_capture_enabled;
};
#endif  // QN_LINUX

struct QNCustomVideoTrackConfig {
  QNVideoEncoderConfig encoder_config;
  std::string tag;             // 自定义标签
  bool multi_profile_enabled;  // 是否开启多流
};

class QINIU_EXPORT_DLL QNRTCEventListener {
 public:
  /**
   * 摄像头插拔消息通知
   *
   * @param device_state 设备状态
   * @param device_name 设备名
   */
  virtual void OnVideoDeviceStateChanged(QNVideoDeviceState device_state,
                                         const std::string& device_name){};

  /**
   * 音频设备插拔消息通知
   *
   * @param device_state 设备状态
   * @param device_guid 设备 guid
   */
  virtual void OnAudioDeviceStateChanged(QNAudioDeviceState device_state,
                                         const std::string& device_guid){};

  /**
   * 音频路由改变通知， 仅适用于 Android 和 iOS 平台
   *
   * @param device 当前的音频路由信息
   */
  virtual void OnAudioRouteChanged(QNAudioDevice device){};

 protected:
  ~QNRTCEventListener(){};
};

class QINIU_EXPORT_DLL QNRTC {
 public:
  /**
   * 获取版本号
   *
   * @param ver 版本号
   */
  static void GetVersion(std::string& ver);

  /**
   * 配置 SDK 日志信息，如果名字相同，则进程每次启动会覆盖原来的日志文件
   * 日志文件大小上限为 100 M，SDK 内部写入时会自动检测
   *
   * @param level 日志等级，等级越低日志越多
   * @param dir_name 日志存储目录，如果不存在则自动创建
   * @param file_name 日志文件名
   */
  static int32_t SetLogFile(QNLogLevel level, const std::string& dir_name,
                            const std::string& file_name);

  /**
   * 初始化
   */
  static void Init(QNRTCSetting& setting, QNRTCEventListener* listener);

  /**
   * 反初始化
   */
  static void DeInint();

  /**
   * 获取摄像头数量
   *
   * @return 返回设备数量
   */
  static int32_t GetCameraCount();

  /**
   * 获取指定序号的摄像头设备信息
   * 首先通过 GetCameraCount 获取摄像头数量
   *
   * @param index 设备序号，<= GetCameraCount()
   *
   * @return QNCameraInfo 结构体
   */
  static const QNCameraInfo& GetCameraInfo(int32_t index);

#ifndef QN_LINUX
  /**
   * 获取可进行画面采集的屏幕、窗口数量；如需刷新则再次调用即可
   *
   * @return 返回可以进行采集的屏幕、窗口数量
   */
  static int32_t GetScreenWindowCount();

  /**
   * 获取指定 index 的屏幕窗口信息，根据此信息可以进行对应的画面采集
   *
   * @param index，<= GetScreenWindowCount()
   *
   * @return ScreenWindowInfo 结构体
   */
  static const QNScreenWindowInfo& GetScreenWindowInfo(int32_t index);
#endif  // QN_LINUX

  /**
   * 用于获取音频采集设备数量
   *
   * @return 音频采集设备的数量
   */
  static int32_t GetAudioRecordingDeviceCount();

  /**
   * 获取指定 index 音频采集设备信息
   *
   * @param index，<= GetAudioRecordingDeviceCount()
   * @param audio_info 输出参数，用于返回指定的音频设备信息
   */
  static QNAudioDeviceInfo& GetAudioRecordingDeviceInfo(int32_t index);

  /**
   * 用于获取音频播放设备数量
   *
   * @return 音频播放设备的数量
   */
  static int32_t GetAudioPlaybackDeviceCount();

  /**
   * 获取指定 index 音频播放设备信息
   *
   * @param index，<= GetAudioPlaybackDeviceCount()
   * @param audio_info 输出参数，用于返回指定的音频设备信息
   */
  static QNAudioDeviceInfo& GetAudioPlaybackDeviceInfo(int32_t index);

  /**
   * 设置连麦使用的音频录制设备，不调用则使用系统默认录制设备
   * 连麦过程中设置无效，需在发布音频 Track 前调用
   *
   * @param index，<= GetAudioRecordingDeviceCount()
   *
   * @return 成功返回 0，其它请参考错误码列表
   */
  static int32_t SetAudioRecordingDevice(int32_t index);

  /**
   * 设置连麦使用的音频播放设备，不调用则使用系统默认播放设备
   * 连麦过程中设置无效，需在订阅任何音频 Track 前调用
   *
   * @param index，<= GetAudioPlaybackDeviceCount()
   *
   * @return 成功返回 0，其它请参考错误码列表
   */
  static int32_t SetAudioPlaybackDevice(int32_t index);

  /**
   * 创建 QNRTCClient 实例
   */
  static QNRTCClient* CreateClient(QNClientEventListener* listener);

  /**
   * 使用自定义配置创建 QNRTCClient 实例
   */
  static QNRTCClient* CreateClient(QNRTCClientConfig& config,
                                   QNClientEventListener* listener);

  /**
   * 释放 QNRTCClient 实例
   *
   * @param ptr, QNRTCClient 实例指针
   */
  static void DestroyClient(QNRTCClient* ptr);

  /**
   * 设置 QNRTCClient 事件监听
   */
  static void SetClientEventListener(QNClientEventListener* listener);

  /**
   * 创建 QNMicrophoneAudioTrack 实例
   *
   * @param config QNMicrophoneAudioTrackConfig 配置
   */
  static QNMicrophoneAudioTrack* CreateMicrophoneAudioTrack(
      QNMicrophoneAudioTrackConfig& config);

  /**
   * 创建 CreateCustomAudioTrack 实例
   *
   * @param config QNCustomAudioTrackConfig 配置
   */
  static QNCustomAudioTrack* CreateCustomAudioTrack(
      QNCustomAudioTrackConfig& config);

  /**
   * 创建 QNCameraVideoTrack 实例
   *
   * @param config QNCameraVideoTrackConfig 配置
   */
  static QNCameraVideoTrack* CreateCameraVideoTrack(
      QNCameraVideoTrackConfig& config, QNCameraEventListener* listener = nullptr);

#ifndef QN_LINUX
  /**
   * 创建 QNScreenVideoTrack 实例
   *
   * @param config QNScreenVideoTrackConfig 配置
   */
  static QNScreenVideoTrack* CreateScreenVideoTrack(
      QNScreenVideoTrackConfig& config, QNScreenEventListener* listener = nullptr);
#endif  // QN_LINUX

  /**
   * 创建 QNCustomVideoTrack 实例
   *
   * @param config QNCustomVideoTrackConfig 配置
   */
  static QNCustomVideoTrack* CreateCustomVideoTrack(
      QNCustomVideoTrackConfig& config);

  /**
   * 释放前面创建的 Track 实例
   *
   * @param ptr Track 实例指针
   */
  static void DestroyLocalTrack(QNLocalTrack* ptr);

  /**
   * 设置是否将音频路由切换到扬声器，设置为 false 后将会切换到之前的音频路由。
   * 设置生效后会触发 QNRTCEventListener.OnAudioRouteChanged
   * 回调对应的路由状态。
   *
   * @param audio_route_to_speakerphone 是否将音频路由切换到扬声器
   */
  static void SetAudioRouteToSpeakerphone(bool audio_route_to_speakerphone);

  /**
   * 设置音频播放静音。
   *
   * @param muted 是否将音频播放设置为静音
   */
  static void SetAudioPlayMute(bool muted);

  /**
   * 音频播放是否为静音。
   */
  static bool IsAudioPlayMute();

 protected:
  virtual ~QNRTC() {}
};

}  // namespace qiniu

#endif  // QN_RTC_INTERFACE_H
