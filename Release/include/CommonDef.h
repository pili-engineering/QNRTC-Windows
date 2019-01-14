#pragma once
#include <string>
#include <vector>
#include <list>

namespace qiniu_v2 {

#ifdef __QINIU_DLL_EXPORT_ 
#define QINIU_EXPORT_DLL __declspec(dllexport)
#else
#define QINIU_EXPORT_DLL __declspec(dllimport)
#endif

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
    };

    typedef list<MergeOptInfo> MergeOptInfoList;
    typedef list<UserInfo> UserInfoList;

    class QNTrackInfo;
    typedef list<QNTrackInfo*> TrackInfoList;

    // Track 描述信息 
    // 使用场景 1、开发者在发布时可以根据 CreateVideoTrackInfo 或 CreateAudioTrackInfo 进行构造 
    // 使用场景 2、在各种消息回调中，SDK 向上层传递 Track 信息 
    class QINIU_EXPORT_DLL QNTrackInfo
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
        // @param is_master_ 是否为“主”流，默认为 false；
        //  如果需要与 v1 的接口进行互通，则将其中某一路 Track 置为 true
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
            bool  is_master_ = false
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

    protected:
        QNTrackInfo() {}
        QNTrackInfo(const QNTrackInfo&) = delete;
        QNTrackInfo operator = (const QNTrackInfo&) = delete;
    };
}