#pragma once
#include <string>
#include <vector>
#include <list>

namespace qiniu_v2 {

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

#define QNRTC_MAX_DEVICE_LENGHT 128

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

    // 房间连接状态 
    enum RoomState
    {
        rs_idle,            // 空闲，未连接 
        rs_connecting,      // 连接中 
        rs_connected,       // 已连接 
        rs_reconnecting,    // 已断开，并在自动重连中(SDK 内部自动重连) 
    };

    // Track 类型，或者可以理解为数据源类型 
    enum TrackSourceType
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
    enum IcePolicy
    {
        forceUdp  = 0,      // media transfer forced use udp
        forceTcp  = 1,      // media transfer forced use tcp
        preferUdp = 2,      // media transfer use udp first, and if udp don't work, then downgrade using tcp
    };

    // 合流画面填充方式 
    enum MergeStretchMode
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

    // 合流背景、水印配置参数 
    struct MergeLayer {
        string layer_url;       // http网络图片地址 
        int pos_x = 0;          // 在合流画面中的x坐标 
        int pos_y = 0;          // 在合流画面中的y坐标 
        int layer_width = 0;    // 该图片占宽 
        int layer_height = 0;   // 该图片占高 
    };

    typedef list<MergeLayer> MergeLayerList;

    // 自定义合流配置信息 
    struct MergeJob {
        string job_id;              // 合流任务id，保证唯一 
        string publish_url;         // 自定义合流推流地址 

        int width = 0;              // 合流画布宽 
        int height = 0;             // 合流画布高 
        int fps = 0;                // 合流帧率 
        int bitrate = 0;            // 合流码率bps 
        int min_bitrate = 0;        // 最小码率 
        int max_bitrate = 0;        // 最大码率 
        MergeStretchMode stretch_mode = ASPECT_FILL;
    };

    // 旁路直播合流配置信息，通过 SDK 将参数发送到服务端 
    // 服务端按照指定的参数进行合流并推出 RTMP 流 
    struct MergeOptInfo
    {
        string track_id;     // Track Id，房间内唯一 
        bool   is_video;     // 是否为视频类型，如果为 false， 则以下参数无效 
        int    pos_x;        // 此路流（即此 Track）在 RTMP 流画布中的 X 坐标 
        int    pos_y;        // 此路流（即此 Track）在 RTMP 流画布中的 Y 坐标 
        int    pos_z;        // 此路流（即此 Track）在 RTMP 流画布中的 Z 坐标 
        int    width;        // 此路流（即此 Track）在 RTMP 流画布中的宽度，缩放、裁减方式根据后端配置决定 
        int    height;       // 此路流（即此 Track）在 RTMP 流画布中的高度，缩放、裁减方式根据后端配置决定 
        MergeStretchMode stretchMode = ASPECT_INVALID;   // 设置视频 Track 在合流时的填充模式，如果不做单独设置，
                                                         // 填充模式将继承 CreateMergeJob 的 stretchMode 
        bool is_support_sei = false;   // 是否支持私有 SEI 数据插入，只能设置一个 track 为 true 
    };

    // 用户信息 
    struct UserInfo
    {
        string user_id;      // 用户 Id，房间内唯一标识，同一房间不能重复登录 
        string user_data;    // 用户自定义数据，在 JoinRoom 时指定，通过服务器进行透传 
    };

    // 通话质量回调信息 
    struct StatisticsReport
    {
        std::string track_id; // Track Id，房间内唯一 
        std::string user_id;  // 此 track 所属 user id
        bool        is_video; // 是否为视频 track 

        // 以下两个成员为音频 track 参数，仅当 is_video 为 false 时有效 
        int         audio_bitrate = 0;              // 音频码率，单位：bps
        float       audio_packet_lost_rate = 0.0f;  // 音频丢包率 

        // 以下成员为视频 track 参数，仅当 is_video 为 true 时有效 
        int         video_width = 0;                // 视频宽度 
        int         video_height = 0;               // 视频高度 
        int         video_frame_rate = 0;           // 视频帧率 
        int         video_bitrate = 0;              // 码率，单位：bps
        float       video_packet_lost_rate = 0.0f;  // 丢包率 
        int64_t     out_rtt = 0;                    // 数据从发送到接收端的往返延迟 
        int         network_grade;                  // 网络质量  1 - 优 2 - 良 3 - 中等 4 - 差 
    };

    //自定义消息接收回调信息 
    struct  CustomMessage
    {
        std::string msg_id;        // 消息唯一id 
        std::string msg_sendid;    // 消息发送者的user id 
        std::string msg_text;      // 消息内容 
        int         msg_stamp;     // 消息时间戳 
    };

    struct QNTrackLayerSubConfig {
        QNTrackProfile mProfile;   // 当前 profile 
        bool mChooseToSub;         // 是否需要切换为当前的 profile 
        bool mMaintainLayer;       // 开启后，订阅端 profile 会随网络自动切换 （暂时不支持） 
        bool mActive;              // 当前 profile 为生效状态 
    };

    // 单路转推配置信息，通过 SDK 将参数发送到服务端 
    // 服务端按照指定的参数进行合流并推出 RTMP 流 
    struct ForwardOptInfo
    {
        list<string>       track_id_list;       // 单路转推的音频和视频的 track id，audio_only 为 true 时，只存放音频的 track id 
        bool               audio_only;          // 是否为是否只转推音频 
        std::string        job_id;              // 单路转推任务 id，由客户端设置，不可为空，若已存在相同 id 的 job，返回错误 
        std::string        publish_url;         // 转推地址 
        bool               is_internal = true;  // 是否使用七牛内部转推，默认为 true 
    };

    typedef list<MergeOptInfo> MergeOptInfoList;
    typedef list<UserInfo> UserInfoList;
    typedef list<CustomMessage> CustomMessageList;
    typedef list<StatisticsReport> StatisticsReportList;
    typedef list<QNTrackLayerSubConfig> LayerSubConfigList;

    class QNTrackInfo;
    typedef list<QNTrackInfo*> TrackInfoList;

    // Track 描述信息 
    // 使用场景 1、开发者在发布时可以根据 CreateVideoTrackInfo 或 CreateAudioTrackInfo 进行构造 
    // 使用场景 2、在各种消息回调中，SDK 向上层传递 Track 信息 
#if defined _WIN32
    class QINIU_EXPORT_DLL QNTrackInfo
#else
    class QNTrackInfo
#endif // WIN32
    {
    protected:
        string track_id;                        // Track Id（数据流唯一标识） 
        string local_id;                        // Track 在 webrtc 中的标识 
        string user_id;                         // 此 Track 所属 User Id
        string kind;                            // VIDEO_KIND_TYPE 或 AUDIO_KIND_TYPE
        string tag;                             // Track 自定义 Tag，由用户指定并使用，SDK 仅做透传 
        bool   master               = false;    // 是否为主流，默认为 false
        bool   muted                = false;    // 是否已静默 
        int    max_bitrate          = 0;        // 最大码率，单位：bps
        int    state                = 0;        
        bool   connected            = false;    // 此 track 当前是否处于已连接状态 
        // 以下成员仅对视频 Track 有效 
        int    width                = 0;
        int    height               = 0;
        int    max_fps              = 0;
        void*  render_hwnd          = nullptr;  // 渲染窗口句柄，HWND 或 winId(),为空则不渲染，对数据回调无影响 
        string camera_device_id;                // 摄像头设备 Id，仅当 source_type 为 tst_Camera 时有效 
        TrackSourceType source_type = tst_Invalid;
        unsigned long long start_tp = 0;
        LayerSubConfigList  sub_layer_list;        // 订阅端多流信息 
        bool   multi_stream_enable  = false;       // 是否支持开启多流功能 
    public:
        // 创建视频 Track 实例，用于 PublishTracks 时使用，使用完成后调用 Release 进行释放 
        // @param camera_device_id_ 摄像头设备 Id，如果不是摄像头采集的话，可为空 
        // @param tag_ 开发者自定义 tag
        // @param render_hwnd_ 视频渲染窗口句柄，如：MFC 下窗口的 m_hWnd 或 Qt 下的 winId()，为 NULL 则不渲染 
        // @param width_ 数据源宽度 
        // @param height_ 数据源高度 
        // @param max_fps_ 最大帧率 
        // @param max_bitrate_ 最大码率，单位：bps
        // @param type_ TrackSourceType 根据具体的数据源进行制定 
        // @param is_master_ 是否为“主”流，默认为 false 
        //  如果需要与 v1 的接口进行互通，则将其中某一路 Track 置为 true 
        // @param multi_stream_enable_  是否开启多流功能，默认为 false； 
        // @return 成功：TrackInfo 指针，否则返回空指针；返回值需由 Release 方法释放 
        static QNTrackInfo* CreateVideoTrackInfo(
            const string& camera_device_id_,
            const string& tag_,
            void* render_hwnd_,
            int   width_,
            int   height_,
            int   max_fps_,
            int   max_bitrate_,
            TrackSourceType type_,
            bool  is_master_ = false,
            bool  multi_stream_enable_ = false
        );

        // 创建音频 Track 实例，用于 PublishTracks 时使用，返回值需由开发者调用 Release 进行释放
        // 全局仅能创建并发布一路 Audio Track
        // @param tag_ 开发者自定义 tag
        // @param max_bitrate_ 最大码率，单位：bps
        // @param is_master_ 是否为“主”流，默认为 false；
        // 如果需要与 v1 的接口进行互通，则将其中某一路 Track 置为 true
        // @return 成功：TrackInfo 指针，否则返回空指针；返回值需由 Release 方法释放 
        static QNTrackInfo* CreateAudioTrackInfo(
            const string& tag_,
            int   max_bitrate_,
            bool  is_master_ = false
        );

        // 释放 CreateVideoTrackInfo 或 CreateAudioTrackInfo 创建的 Track 实例 
        void Release()
        {
            delete this;
        }

        // 拷贝获取一个新的 TrackInfo 实例，返回值需由开发者调用 Release 进行释放 
        static QNTrackInfo* Copy(QNTrackInfo* track_info_);

        static void ReleaseList(TrackInfoList& track_list_);

    public:
        virtual ~QNTrackInfo() {}

        virtual const string& GetTrackId()
        {
            return track_id;
        }

        virtual const string& GetLocalId()
        {
            return local_id;
        }

        virtual const string& GetUserId()
        {
            return user_id;
        }
        
        virtual const string& GetKind()
        {
            return kind;
        }

        virtual const string& GetTag()
        {
            return tag;
        }

        virtual bool IsMaster()
        {
            return master;
        }

        virtual bool IsMuted()
        {
            return muted;
        }

        virtual int GetMaxBitRate()
        {
            return max_bitrate;
        }

        virtual int GetState()
        {
            return state;
        }

        virtual bool IsConnected()
        {
            return connected;
        }

        virtual int GetWidth()
        {
            return width;
        }

        virtual int GetHeight()
        {
            return height;
        }

        virtual int GetMaxFps()
        {
            return max_fps;
        }

        virtual void* GetRenderHwnd()
        {
            return render_hwnd;
        }

        void SetRenderHwnd(void* hwnd_)
        {
            render_hwnd = hwnd_;
        }

        virtual const string& GetCameramDeviceId()
        {
            return camera_device_id;
        }

        virtual TrackSourceType GetSourceType()
        {
            return source_type;
        }

        virtual unsigned long long GetStartTP()
        {
            return start_tp;
        }

        virtual LayerSubConfigList& GetLayerInfo()
        {
            return sub_layer_list;
        }

        virtual bool GetMultiStremState()
        {
            return multi_stream_enable;
        }


    protected:
        QNTrackInfo() {}
        QNTrackInfo(const QNTrackInfo&) = delete;
        QNTrackInfo operator = (const QNTrackInfo&) = delete;
    };
}
