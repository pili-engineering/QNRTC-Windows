#ifndef QN_COMMON_DEF_H
#define QN_COMMON_DEF_H

#include <stdint.h>

#include <list>
#include <map>
#include <string>
#include <vector>

#include "qn_error_code.h"

namespace qiniu {

#if defined _WIN32
#ifdef __QINIU_DLL_EXPORT_
#define QINIU_EXPORT_DLL __declspec(dllexport)
#else
#define QINIU_EXPORT_DLL __declspec(dllimport)
#endif
#else  // Linux
#ifdef __QINIU_DLL_EXPORT_
#define QINIU_EXPORT_DLL __attribute__((visibility("default")))
#else
#define QINIU_EXPORT_DLL
#endif
#endif  // WIN32

#define QNRTC_MAX_DEVICE_LENGTH 128

#define VIDEO_KIND_TYPE "video"
#define AUDIO_KIND_TYPE "audio"

using namespace std;

// SDK 日志等级
enum QNLogLevel { kLogNone, kLogVerbose, kLogInfo, kLogWarning, kLogError };

// 图片类型
enum QNImageType { kImagJpeg, kImagPng };

// 大小流对应配置
enum QNTrackProfile { kLow, kMedium, kHigh };

#ifndef QN_LINUX
// 前置和后置摄像头
enum QNCameraFacing {
  kCameraFacingFront,  // 前置
  kCameraFacingBack    // 后置
};

enum QNRenderMode {
  kRenderD3D,
  kRenderGDI,
  kRenderIOS,
  kRenderDefault,
};

enum QNScreenCaptureSourceType { kWindow, kScreen };

struct QNFocusCoordinates {
  float x;
  float y;
  int width;
  int height;
};

// SDK 提供对桌面和窗口的画面采集，以下为可以采集的屏幕或窗口信息
struct QNScreenWindowInfo {
  std::string id;                  // 窗口 ID，唯一标识
  std::string title;               // 窗口标题
  QNScreenCaptureSourceType type;  // 窗口类型
};

#endif  // QN_LINUX

// 画面渲染窗口填充方式
enum QNStretchMode {
  kStretchInvalid = -1,  // 无效值
  kStretchFill,  // 在保持长宽比的前提下，缩放视频，使其充满容器
  kStretchFit,  // 在保持长宽比的前提下，缩放视频，使其在容器内完整显示，边缘部分填充黑边
  kStretchScaleToFit  // 缩放视频，使其填充满容器，可能导致拉伸变形
};

// 视频数据格式
enum QNVideoFrameType {
  kUnknown,
  kI420,
  kIYUV,
  kRGB24,
  kABGR,
  kARGB,
  kARGB4444,
  kRGB565,
  kARGB1555,
  kYUY2,
  kYV12,
  kUYVY,
  kMJPEG,
  kNV21,
  kNV12,
  kBGRA
};

// 表示原始视频数据的旋转角度，主要用于对原始视频数据进行处理的功能接口中
enum QNVideoRotation {
  kVideoRotation0 = 0,
  kVideoRotation90 = 90,
  kVideoRotation180 = 180,
  kVideoRotation270 = 270
};

// 媒体传输协议配置
enum QNTransportPolicy {
  kForceUDP,  // media transfer forced use udp
  kForceTCP,  // media transfer forced use tcp
  kPreferUDP  // media transfer use udp first, and if udp don't work, then
              // downgrade using tcp
};

enum QNAudioDevice { SPEAKER_PHONE, EARPIECE, WIRED_HEADSET, BLUETOOTH, NONE };

// 摄像头设备状态，SDK 提供设备插拔状态的监控，以下用于标识设备被插入还是拔出
// 设备状态变化后，建议通过 GetCameraCount 重新获取摄像头设备列表
enum QNVideoDeviceState {
  kVideoDeviceActive = 0x00000001,  // 设备插入
  kVideoDeviceLost = 0x00000002     // 设备拔出
};

// 音频设备当前状态，用与设备插拔的检测和通知
enum QNAudioDeviceState {
  kAudioDeviceActive = 0x00000001,      // 新的可用设备
  kAudioDeviceDisabled = 0x00000002,    // 设备失效
  kAudioDeviceNotPresent = 0x00000004,  // 设备不存在
  kAudioDeviceLost = 0x00000008         // 设备被拔出
};

enum QNConnectionState {
  kDisconnected,
  kConnecting,
  kConnected,
  kReconnecting,
  kReconnected
};

enum QNVideoEncoderType {
  kEncodeOpenH264,  // 默认 Open264 编码器
#ifndef QN_LINUX
  kEncodeH264QSV,           // Intel  集显编码器
  kEncodeH264NVENC,         // NVIDIA 独显编码器
  kEncodeH264MobileHwCodec  // 硬件编码器
#endif                      // QN_LINUX
};

//跨房转推状态
enum QNMediaRelayState {
  kRelaySuccess = 0,
  kRelayStopped,
  kInvalidToken,
  kRelayNoRoom,
  kRelayRoomClosed,
  kRelayPlayerExisted,
  kRelayUnknown = 0XFF,
};

//跨房媒体转发信息
struct QNMediaRelayInfo {
  std::string room_name;
  std::string relay_token;
};

//跨房媒体转发配置信息
struct QNMediaRelayConfiguration {
  QNMediaRelayInfo src_room_info;
  std::vector<QNMediaRelayInfo> dest_room_infos;
};

//使用场景
enum QNClientMode {
  kRtc,  //通信场景
  kLive  //直播场景
};

//用户角色
enum QNClientRole {
  kBroadcaster,  //主播
  kAudience      //观众
};

//用来配置 QNRTCClient 的相关信息
struct QNRTCClientConfig {
  QNClientMode mode;  //使用场景
  QNClientRole role;  //用户角色
};

struct QNConnectionDisconnectedInfo {
  enum Reason {
    kLeave,       // 主动退出
    kKickOut,     // 被服务器踢出房间
    kRoomClosed,  // 房间被关闭
    kRoomFull,    // 房间人数已满
    kRoomError    // 异常断开
  };
  Reason reason;
  int32_t error_code;
  std::string error_message;
};

// 摄像头支持的采集能力
struct QNCameraCapability {
  int32_t width;
  int32_t height;
  int32_t max_fps;
  QNVideoFrameType video_frame_type;
};

typedef std::vector<QNCameraCapability> CameraCapabilityVec;

// 摄像头设备信息
struct QNCameraInfo {
  std::string id;    // 设备 ID，用于系统内做标识
  std::string name;  // 设备名，用于展示给用户，较友好
  CameraCapabilityVec capabilities;  // 此摄像头所支持的采集能力列表
};

// 音频设备信息
struct QNAudioDeviceInfo {
  std::string id;    // 设备 ID，用于系统内做标识
  std::string name;  // 设备名，用于展示给用户，较友好
  bool is_default;   // 此设备是否为默认设备
};

// merge job 配置信息
struct MergeJobInfo {
  std::string job_id;
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
};

class QNLocalAudioTrack;
class QNLocalVideoTrack;
// 单路转推配置信息，通过 SDK 将参数发送到服务端
// 服务端按照指定的参数进行 CDN 转推
struct QNDirectLiveStreamingConfig {
  QNLocalAudioTrack* local_audio_track = nullptr;  // 单路转推的音频track
  QNLocalVideoTrack* local_video_track = nullptr;  // 单路转推的视频track
  std::string stream_id;  // 单路转推任务 ID，由客户端设置，不可为空
  std::string publish_url;  // rtmp 转推地址
};

// 合流背景、水印配置参数
struct QNTranscodingLiveStreamingImage {
  std::string layer_url;  // http网络图片地址
  int32_t x;              // 在合流画面中的x坐标
  int32_t y;              // 在合流画面中的y坐标
  int32_t layer_width;    // 该图片占宽
  int32_t layer_height;   // 该图片占高
};

typedef std::list<QNTranscodingLiveStreamingImage> QNTranscodingImageList;

// 自定义合流配置信息
struct QNTranscodingLiveStreamingConfig {
  std::string stream_id;    // 合流任务 ID，保证唯一
  std::string publish_url;  // 自定义合流推流地址
  QNTranscodingLiveStreamingImage merge_background;  // 合流背景
  QNTranscodingImageList merge_watermark;            // 合流水印配置链表
  int32_t width;                                     // 合流画布宽
  int32_t height;                                    // 合流画布高
  int32_t fps;                                       // 合流帧率
  int32_t bitrate;                                   // 合流码率bps
  int32_t min_bitrate;                               // 最小码率
  int32_t max_bitrate;                               // 最大码率
  bool is_hold_last_frame;  // 合流停止时是否保持最后一帧画面
  QNStretchMode stretch_mode = kStretchFill;  // 合流画面填充模式
};

// 旁路直播合流配置信息，通过 SDK 将参数发送到服务端
// 服务端按照指定的参数进行合流并推出 RTMP 流
struct QNTranscodingLiveStreamingTrack {
  std::string track_id;  // Track ID，房间内唯一
  int32_t x = 0;  // 此路流（即此 Track）在 RTMP 流画布中的 X 坐标
  int32_t y = 0;  // 此路流（即此 Track）在 RTMP 流画布中的 Y 坐标
  int32_t z = 0;  // 此路流（即此 Track）在 RTMP 流画布中的 Z 坐标
  int32_t width = 0;  // 此路流（即此 Track）在 RTMP
                      // 流画布中的宽度，缩放、裁减方式根据后端配置决定
  int32_t height = 0;  // 此路流（即此 Track）在 RTMP
                       // 流画布中的高度，缩放、裁减方式根据后端配置决定
  QNStretchMode stretch_mode =
      kStretchInvalid;  // 设置视频 Track 在合流时的填充模式，如果不做单独设置，
                        // 填充模式将继承 CreateMergeJob 的 stretch_mode
  bool is_support_sei =
      false;  // 是否支持私有 SEI 数据插入，只支持一路 track 设置
};

struct QNLiveStreamingErrorInfo {
  enum Type {
    kStart,
    kStop,
    kUpdate,
  };
  Type type;
  int32_t code;
  std::string message;
};

//自定义消息接收回调信息
struct QNCustomMessage {
  std::string msg_id;      // 消息唯一 ID
  std::string msg_sendid;  // 消息发送者的 user ID
  std::string msg_text;    // 消息内容
  int32_t msg_stamp;       // 消息时间戳
};

struct QNLocalAudioTrackStats {
  int32_t uplink_bitrate;   // 上行音频码率
  int32_t uplink_rtt;       // 上行网络 rtt
  int32_t uplink_lostrate;  // 上行网络丢包率
};

struct QNLocalVideoTrackStats {
  QNTrackProfile profile;    // 该路 track 的 profile
  int32_t uplink_framerate;  // 上行视频帧率
  int32_t uplink_bitrate;    // 上行视频码率
  int32_t uplink_rtt;        // 上行网络 rtt
  int32_t uplink_lostrate;   // 上行网络丢包率
};

struct QNRemoteAudioTrackStats {
  int32_t downlink_bitrate;   // 下行音频码率
  int32_t downlink_lostrate;  // 下行网络丢包率
  int32_t uplink_rtt;         // 上行网络 rtt
  int32_t uplink_lostrate;    // 上行网络丢包率
};

struct QNRemoteVideoTrackStats {
  QNTrackProfile profile;  // 该路 track 的 profile
  int32_t downlink_framerate;  // 下行视频帧率（即自己拉取的视频的帧率）
  int32_t downlink_bitrate;   // 下行视频码率
  int32_t downlink_lostrate;  // 下行网络丢包率
  int32_t uplink_rtt;  // 上行网络 rtt （即远端用户发布视频的网络链路的 rtt）
  int32_t uplink_lostrate;  // 上行网络丢包率
};

typedef std::list<MergeJobInfo> MergeJobInfoList;
typedef std::list<QNTranscodingLiveStreamingTrack> QNTranscodeingTrackList;
typedef std::list<QNLocalVideoTrackStats> QNLocalVideoStatsList;
typedef std::map<std::string, QNLocalAudioTrackStats> QNLocalAudioTrackStatsMap;
typedef std::map<std::string, QNRemoteAudioTrackStats>
    QNRemoteAudioTrackStatsMap;
typedef std::map<std::string, QNLocalVideoStatsList> QNLocalVideoTracksStatsMap;
typedef std::map<std::string, QNRemoteVideoTrackStats>
    QNRemoteVideoTrackStatsMap;

}  // namespace qiniu

#endif  // QN_COMMON_DEF_H
