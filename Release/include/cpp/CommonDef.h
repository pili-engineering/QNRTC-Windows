#pragma once
#include <string>
#include <vector>
#include <list>
#include "QNErrorCode.h"

namespace qiniu {

#if defined _WIN32
#ifdef __QINIU_DLL_EXPORT_ 
#define QINIU_EXPORT_DLL __declspec(dllexport)
#else
#define QINIU_EXPORT_DLL __declspec(dllimport)
#endif
#else // Linux
#ifdef __QINIU_DLL_EXPORT_ 
#define QINIU_EXPORT_DLL __attribute__((visibility("default")))
#else
#define QINIU_EXPORT_DLL
#endif
#endif // WIN32

#define QNRTC_MAX_DEVICE_LENGTH 128

#define VIDEO_KIND_TYPE "video"
#define AUDIO_KIND_TYPE "audio"

    using namespace std;

    // SDK 日志等级 
    enum QNLogLevel
    {
        LOG_DEBUG = 0,
        LOG_INFO,
        LOG_WARNING,
        LOG_ERROR,
    };

    enum QNConnectionState
    {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        RECONNECTING,
        RECONNECTED,
    };

    enum TrackState
    {
        _eIdle = 0,
        _eWaiting = 1,
        _eConnecting = 2,
        _eConnected = 3,
        _eDisconnecting = 4,
        _eDisconnected = 5
    };

    // 房间连接状态 
    enum RoomState
    {
        rs_idle,            // 空闲，未连接 
        rs_connecting,      // 连接中 
        rs_connected,       // 已连接 
        rs_reconnecting,    // 已断开，并在自动重连中(SDK 内部自动重连) 
    };

    // Track 类型，或者可以理解为数据源类型 
    enum QNTrackSourceType
    {
        tst_Invalid = -1,   // 无效类型 
        tst_Microphone,     // 音频数据：数据源为 SDK 内部的麦克风采集 
        tst_ExternalPCM,    // 音频数据：数据源为外部导入的 PCM 数据 
        tst_Camera,         // 视频数据: 数据源为摄像头采集 
        tst_ScreenCasts,    // 视频数据：数据源为屏幕采集或窗口采集 
        tst_ExternalYUV,    // 视频数据：数据源为外部数据导入，YUV 或 RGB 格式 
        tst_ExternalH264,   // 视频数据：数据源为外部 H.264 裸码流 
    };

    // 媒体传输协议配置 
    enum QNIcePolicy
    {
        forceUdp  = 0,      // media transfer forced use udp
        forceTcp  = 1,      // media transfer forced use tcp
        preferUdp = 2,      // media transfer use udp first, and if udp don't work, then downgrade using tcp
    };

    // 画面渲染窗口填充方式 
    enum QNStretchMode
    {
        ASPECT_INVALID = -1, // 无效值 
        ASPECT_FILL  = 0,   // 在保持长宽比的前提下，缩放视频，使其充满容器 
        ASPECT_FIT   = 1,   // 在保持长宽比的前提下，缩放视频，使其在容器内完整显示，边缘部分填充黑边 
        SCALE_TO_FIT = 2,   // 缩放视频，使其填充满容器，可能导致拉伸变形 
    };

    // 大小流对应配置
    enum QNTrackProfile {
        HIGH,
        MEDIUM,
        LOW,
    };

    // 视频数据格式 
    enum QNVideoSourceType
    {
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
        kBGRA,
        kH264, // 目前仅针对 Linux SDK
    };

    // 画面帧处理模式 
    enum QNVideoProcessMode
    {
        p_None,
        p_Crop,                  //裁剪
        p_Scale                  //缩放
    };

    // 摄像头设备状态，SDK 提供设备插拔状态的监控，以下用于标识设备被插入还是拔出
 // 设备状态变化后，建议通过 GetCameraCount 重新获取摄像头设备列表 
    enum QNVideoDeviceState
    {
        vds_active = 0x00000001,            // 设备插入 
        vds_lost = 0x00000002,            // 设备拔出 
    };

    // 表示原始视频数据的旋转角度，主要用于对原始视频数据进行处理的功能接口中 
    enum QNVideoRotation
    {
        kVideoRotation_0 = 0,
        kVideoRotation_90 = 90,
        kVideoRotation_180 = 180,
        kVideoRotation_270 = 270
    };

    struct QNConnectionDisconnectedInfo
    {
        enum Reason {
            LEAVE,          // 主动退出
            KICKED_OUT,     // 被服务器踢出房间
            ROOM_CLOSED,    // 房间被关闭
            ROOM_FULL,      // 房间人数已满
            ROOM_ERROR      // 异常断开
        };
        Reason reason;
        int error_code = QNRTC_OK;
        std::string error_message = "";
    };


    // 用户信息 
    struct UserInfo
    {
        string user_id;      // 用户 Id，房间内唯一标识，同一房间不能重复登录 
        string user_data;    // 用户自定义数据，在 JoinRoom 时指定，通过服务器进行透传 
    };

    //自定义消息接收回调信息 
    struct  QNCustomMessage
    {
        std::string msg_id;        // 消息唯一id 
        std::string msg_sendid;    // 消息发送者的user id 
        std::string msg_text;      // 消息内容 
        int         msg_stamp;     // 消息时间戳 
    };

    // 摄像头支持的采集能力 
    struct QNCameraCapability
    {
        int                 width;
        int                 height;
        int                 max_fps;
        QNVideoSourceType    video_type;
    };

    typedef std::vector<QNCameraCapability> CameraCapabilityVec;

    // 摄像头设备信息 
    struct QNCameraDeviceInfo
    {
        std::string         device_id;      // 设备 Id，用于系统内做标识 
        std::string         device_name;    // 设备名，用于展示给用户，较友好 
        CameraCapabilityVec capability_vec; // 此摄像头所支持的采集能力列表 
    };

    typedef std::vector<QNCameraDeviceInfo> CameraDeviceInfoVec;

    // 摄像头预览配置，在调用 PreviewCamera 接口时使用 
    struct QNCameraPreviewSetting
    {
        std::string device_id;              // 设备 Id
        std::string device_name;            // 设备名 
        int         width = 640;
        int         height = 480;
        int         max_fps = 15;
        void*       render_hwnd = nullptr;  //video render window hwnd,MFC:HWND; QT:winId
    };

    // 窗口预览配置，在调用 PreviewCamera 接口时使用
    struct QNScreenPreviewSetting
    {
        int         screen_id;              // 窗口 Id
        void*       render_hwnd = nullptr;  //video render window hwnd,MFC:HWND; QT:winId
    };

    // SDK 提供对桌面和窗口的画面采集，以下为可以采集的屏幕或窗口信息 
    struct QNScreenWindowInfo
    {
#if defined _WIN32
#if defined _WIN64
        long long    id;        // 窗口 Id，唯一标识 
#else
        int          id;
#endif // _WIN64
#else 
        long long    id;
#endif // _WIN32
        std::string  title;     // 窗口标题 
        bool         is_screen; // true:显示器; false:窗口 
    };

    // 当导入 H.264 裸码流时，用于封装单个 Nal 
    // 目前仅针对 Linux SDK 有效 
    struct H264Nal
    {
        unsigned char* payload = nullptr;
        size_t payload_size = 0;
    };

    // 已编码视频帧数据 
    struct EncodedFrame {
        unsigned int encoded_width = 0;
        unsigned int encoded_height = 0;
        unsigned int time_stamp = 0;
        long long ntp_time_ms = 0; // NTP time of the capture time in local timebase in milliseconds.
        long long capture_time_ms = 0;
        bool key_frame = false;
        QNVideoRotation rotation = kVideoRotation_0;
        unsigned char* buffer = nullptr;
        size_t buffer_size = 0; // playload size
    };

    // 已解码视频帧数据
    struct DecodedFrame
    {
        unsigned char* buffer = nullptr;
        size_t buffer_size = 0;
        size_t width = 0;
        size_t height = 0;
        unsigned long long timestamp_us = 0;
        QNVideoRotation rotation = kVideoRotation_0;
        QNVideoSourceType raw_type;
    };

    enum QNVideoEncodeType
    {
        kEncodedefault,       // 默认 Open264 编码器 
        kEncodeQsv,           // Intel  集显编码器 
        kEncodeNvenc,         // NVIDIA 独显编码器 
    };

    typedef std::vector<QNVideoEncodeType> EncoderCapabilityVec;

    struct QNEncoderCapability
    {
        EncoderCapabilityVec capability_vec;
    };

    // 音频设备配置，用于指定设备进行录制或者播放 
    struct QNAudioDeviceSetting
    {
        enum WindowsAudioDeviceType
        {
            wdt_DefaultCommunicationDevice = -1, // 通信设备 
            wdt_DefaultDevice = -2  // 普通设备 
        };
        int                 device_index;                    // 设备编号；注意：0 不代表默认设备 
        WindowsAudioDeviceType   device_type = wdt_DefaultDevice; // 设备类型，建议默认为普通设备 
    };

    // 音频设备当前状态，用与设备插拔的检测和通知 
    enum QNAudioDeviceState
    {
        ads_active = 0x00000001,     // 新的可用设备 
        ads_disabled = 0x00000002,     // 设备失效 
        ads_notpresent = 0x00000004,     // 设备不存在 
        ads_unplugged = 0x00000008,     // 设备被拔出 
        ads_mask_all = 0x0000000F,
    };

    // 音频设备信息 
    struct QNAudioDeviceInfo
    {
        enum QNAudioDeviceType
        {
            adt_invalid = -1,
            adt_record,
            adt_playout,
        };
        int device_index = 0;
        bool is_default = false;
        QNAudioDeviceType device_type = adt_invalid;
        char device_name[QNRTC_MAX_DEVICE_LENGTH] = { 0 };
        char device_id[QNRTC_MAX_DEVICE_LENGTH] = { 0 };

        QNAudioDeviceInfo& operator = (const QNAudioDeviceInfo& info_)
        {
            device_index = info_.device_index;
            device_type = info_.device_type;
            is_default = info_.is_default;
#if defined _WIN32
            strncpy_s(device_name, info_.device_name, strlen(info_.device_name));
            strncpy_s(device_id, info_.device_id, strlen(info_.device_id));
#else
            strncpy(device_name, info_.device_name, strlen(info_.device_name));
            strncpy(device_id, info_.device_id, strlen(info_.device_id));
#endif //WIN32
            return *this;
        }
    };

    // 单路转推配置信息，通过 SDK 将参数发送到服务端 
    // 服务端按照指定的参数进行 CDN 转推 
    struct QNDirectLiveStreamingConfig
    {
        std::string         local_audio_track = "";  // 单路转推的音频track 
        std::string         local_video_track = "";  // 单路转推的视频track 
        std::string         stream_id = "";          // 单路转推任务 id，由客户端设置，不可为空 
        std::string         publish_url = "";        // rtmp 转推地址 
        bool                is_internal = true;      // 是否使用七牛内部转推，默认为 true 
        int                 delay_ms = 0;            // 延迟停止转推的时间
    };

    // 合流背景、水印配置参数 
    struct QNTranscodingLiveStreamingImage {
        std::string layer_url;  // http网络图片地址 
        int pos_x = 0;          // 在合流画面中的x坐标 
        int pos_y = 0;          // 在合流画面中的y坐标 
        int layer_width = 0;    // 该图片占宽 
        int layer_height = 0;   // 该图片占高 
    };

    typedef list<QNTranscodingLiveStreamingImage> QNTranscodingImageList;

    // 自定义合流配置信息 
    struct QNTranscodingLiveStreamingConfig
    {
        std::string stream_id = "";                         // 合流任务id，保证唯一 
        std::string publish_url = "";                       // 自定义合流推流地址 
        QNTranscodingLiveStreamingImage merge_background;   // 合流背景 
        QNTranscodingImageList   merge_watermark;           // 合流水印配置链表 
        int width = 0;                                      // 合流画布宽 
        int height = 0;                                     // 合流画布高 
        int fps = 0;                                        // 合流帧率 
        int bitrate = 0;                                    // 合流码率bps 
        int min_bitrate = 0;                                // 最小码率 
        int max_bitrate = 0;                                // 最大码率 
        bool is_hold_last_frame = false;                    // 合流停止时是否保持最后一帧画面 
        QNStretchMode stretch_mode = ASPECT_FILL;           // 合流画面填充模式 
        int delay_ms = 0;                                   // 延迟停止合流转推的时间
    };

    // 旁路直播合流配置信息，通过 SDK 将参数发送到服务端 
    // 服务端按照指定的参数进行合流并推出 RTMP 流 
    struct QNTranscodingLiveStreamingTrack
    {
        std::string track_id;       // Track Id，房间内唯一 
        bool   is_video;            // 是否为视频类型，如果为 false， 则以下参数无效 
        int    pos_x;               // 此路流（即此 Track）在 RTMP 流画布中的 X 坐标 
        int    pos_y;               // 此路流（即此 Track）在 RTMP 流画布中的 Y 坐标 
        int    pos_z;               // 此路流（即此 Track）在 RTMP 流画布中的 Z 坐标 
        int    width;               // 此路流（即此 Track）在 RTMP 流画布中的宽度，缩放、裁减方式根据后端配置决定 
        int    height;              // 此路流（即此 Track）在 RTMP 流画布中的高度，缩放、裁减方式根据后端配置决定 
        QNStretchMode stretchMode = ASPECT_INVALID;   // 设置视频 Track 在合流时的填充模式，如果不做单独设置，
                                                         // 填充模式将继承 CreateMergeJob 的 stretchMode 
        bool is_support_sei = false;   // 是否支持私有 SEI 数据插入，只支持一路 track 设置 
    };

    struct QNLiveStreamingErrorInfo
    {
        enum Type {
            START,
            STOP,
            UPDATE,
        };
        Type type;
        int code;
        std::string message;
    };

    struct QNLocalAudioTrackStats
    {
        string trackId = "";
        int uplinkBitrate = 0;                // 上行音频码率
        int uplinkRTT = 0;                    // 上行网络 rtt
        int uplinkLostRate = 0;               // 上行网络丢包率
    }; 

    struct QNLocalVideoTrackStats
    {
        string trackId = "";
        QNTrackProfile profile = HIGH;           // 该路 track 的 profile
        int uplinkFrameRate = 0;              // 上行视频帧率
        int uplinkBitrate = 0;                // 上行视频码率
        int uplinkRTT = 0;                    // 上行网络 rtt
        int uplinkLostRate = 0;               // 上行网络丢包率
        int width = 0;
        int height = 0;
    };

    struct QNRemoteAudioTrackStats
    {
        string trackId = "";
        int downlinkBitrate = 0;             // 下行音频码率
        int downlinkLostRate = 0;            // 下行网络丢包率
        int uplinkRTT = 0;                   // 上行网络 rtt
        int uplinkLostRate = 0;              // 上行网络丢包率
    };

    struct QNRemoteVideoTrackStats
    {
        string trackId = "";
        QNTrackProfile profile = HIGH;          // 该路 track 的 profile
        int downlinkFrameRate = 0;           // 下行视频帧率（即自己拉取的视频的帧率）
        int downlinkBitrate = 0;             // 下行视频码率
        int downlinkLostRate = 0;            // 下行网络丢包率
        int uplinkRTT = 0;                   // 上行网络 rtt （即远端用户发布视频的网络链路的 rtt）
        int uplinkLostRate = 0;              // 上行网络丢包率
        int width = 0;
        int height = 0;
    };

    typedef std::list<QNTranscodingLiveStreamingTrack> QNTranscodeingTrackList;
    typedef std::list<QNAudioDeviceInfo> AudioDeviceInfoList;
    typedef std::list<UserInfo> UserInfoList;
    typedef std::list<QNCustomMessage> CustomMessageList;
    typedef std::list<QNLocalVideoTrackStats> QNLocalVideoStatsList;
    typedef std::list<QNRemoteVideoTrackStats> QNRemoteVideoStatsList;
}
