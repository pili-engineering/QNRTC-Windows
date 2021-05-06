#pragma once
#include "QNRoomInterface.h"

namespace qiniu_v2
{
    // 视频数据格式 
    enum class VideoCaptureType
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
    enum FrameProcessMode
    {
        p_Unknow,
        p_Crop,                  //裁剪
        p_Scale                  //缩放
    };

    // 摄像头支持的采集能力 
    struct CameraCapability
    {
        int                 width;
        int                 height;
        int                 max_fps;
        VideoCaptureType    video_type;
    };

    typedef std::vector<CameraCapability> CameraCapabilityVec;

    // 摄像头设备信息 
    struct CameraDeviceInfo
    {
        std::string         device_id;      // 设备 Id，用于系统内做标识 
        std::string         device_name;    // 设备名，用于展示给用户，较友好 
        CameraCapabilityVec capability_vec; // 此摄像头所支持的采集能力列表 
    };

    typedef std::vector<CameraDeviceInfo> CameraDeviceInfoVec;

    // 摄像头预览配置，在调用 PreviewCamera 接口时使用 
    struct CameraSetting
    {
        std::string device_id;              // 设备 Id
        std::string device_name;            // 设备名 
        int         width       = 640;
        int         height      = 480;
        int         max_fps     = 15;
        void*       render_hwnd = nullptr;  //video render window hwnd,MFC:HWND; QT:winId
    };

    // 摄像头设备状态，SDK 提供设备插拔状态的监控，以下用于标识设备被插入还是拔出
    // 设备状态变化后，建议通过 GetCameraCount 重新获取摄像头设备列表 
    enum VideoDeviceState
    {
        vds_active = 0x00000001,            // 设备插入 
        vds_lost   = 0x00000002,            // 设备拔出 
    };

    // 表示原始视频数据的旋转角度，主要用于对原始视频数据进行处理的功能接口中 
    enum VideoRotation
    {
        kVideoRotation_0   = 0,
        kVideoRotation_90  = 90,
        kVideoRotation_180 = 180,
        kVideoRotation_270 = 270
    };

    // SDK 提供对桌面和窗口的画面采集，以下为可以采集的屏幕或窗口信息 
    struct ScreenWindowInfo
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
        size_t payload_size    = 0;
    };

    // 已编码视频帧数据 
    struct EncodedFrame {
        unsigned int encoded_width  = 0;
        unsigned int encoded_height = 0;
        unsigned int time_stamp     = 0;
        long long ntp_time_ms       = 0; // NTP time of the capture time in local timebase in milliseconds.
        long long capture_time_ms   = 0;
        bool key_frame              = false;
        VideoRotation rotation      = kVideoRotation_0;
        unsigned char* buffer       = nullptr ;
        size_t buffer_size          = 0; // playload size
    };

    // 已解码视频帧数据
    struct DecodedFrame
    {
        unsigned char* buffer           = nullptr;
        size_t buffer_size              = 0;
        size_t width                    = 0;
        size_t height                   = 0;
        unsigned long long timestamp_us = 0;
        VideoRotation rotation          = kVideoRotation_0;
        VideoCaptureType raw_type;
    };

    // 视频模块功能接口，开发者可以进行视频相关控制
    // 由 QNRoomInterface::ObtainVideoInterface 获取
    // QNRoomInterface::DestroyRoomInterface 后自动释放 
#if defined _WIN32
    class QINIU_EXPORT_DLL QNVideoInterface
#else
    class QNVideoInterface
#endif // WIN32
    {
    public:
        // 视频模块事件监听接口 
        class QNVideoListener
        {
        public:
            // 视频帧数据回调
            // 当本地开启摄像头或者订阅了远端视频 Track 后，每一帧视频数据均会触发此回调
            // @param raw_data_ 数据内存指针
            // @param data_len_ 数据长度
            // @param width_ 图像宽度
            // @param height_ 图像高度
            // @param video_type_ 图像格式
            // @param track_id_ 所属 Track Id，自己或者远端用户发布的 Track
            // @param user_id_ 所属 User'id，自己或者远端用户，优先判断 user id
            virtual void OnVideoFrame(
                const unsigned char* raw_data_,
                int data_len_,
                int width_,
                int height_,
                qiniu_v2::VideoCaptureType video_type_,
                const std::string& track_id_,
                const std::string& user_id_
            ) = 0;

            // 视频帧数据回调，只有调用本地预览接口时触发
            // @param raw_data_ 数据内存指针
            // @param data_len_ 数据长度
            // @param width_ 图像宽度
            // @param height_ 图像高度
            // @param video_type_ 图像格式 
            virtual void OnVideoFramePreview(
                const unsigned char* raw_data_,
                int data_len_,
                int width_,
                int height_,
                qiniu_v2::VideoCaptureType video_type_
            ) = 0;

            // 设备插拔事件通知
            // @param device_state_ 设备状态，插入 or 拔出
            // @param device_name_ 设备名称 
            virtual void OnVideoDeviceStateChanged(
                VideoDeviceState device_state_,
                const std::string& device_name_
            ) = 0;

            // 仅在使用外置视频解码器时会触发
            // @param encoded_frame_ SDK 接收到完整视频帧时回调给上层进行解码
            // @param decodec_frame_ 外置解码器将 encoded_frame_ 解码后，可将解码后的数据传递到 SDK 进行渲染
            virtual void OnDecodeVideoFrame(
                const EncodedFrame& encoded_frame_, 
                DecodedFrame& decoded_frame_
            ) {}

            // 仅在使用外置视频解码器时会触发，用于配置给外置编码器
            // @param bitrate_ 新的目标码率, unit: bps
            // @param frame_rate_ 新的目标帧率 
            virtual void OnEncoderAdjustSetting(
                int bitrate_,
                int frame_rate_
            ) {}

        protected:
            virtual ~QNVideoListener() {}
        };
    public:
        // 获取摄像头数量，每调用一次刷新一次内部设备记录
        // @return 返回设备数量 
        virtual int GetCameraCount() = 0;

        // 获取指定序号的摄像头设备信息
        // 首先通过 GetCameraCount 获取摄像头数量
        // @param device_index_ 设备序号，<= GetCameraCount()
        // @return CameraDeviceInfo 结构体 
        virtual const CameraDeviceInfo& GetCameraInfo(unsigned int device_index_) = 0;

        // 设置视频模块事件监听接口 
        virtual void SetVideoListener(QNVideoInterface::QNVideoListener* listener_ptr_) = 0;

        // 获取 SetVideoListener 接口设置的视频模块事件监听接口 
        virtual QNVideoInterface::QNVideoListener* GetVideoListener() = 0;

        // 预览摄像头，开启后将会触发 OnVideoFramePreview 数据回调
        // 注意：摄像头不能重复打开
        // @param camera_setting_ 指定摄像头参数
        // @return 0:成功，其它请参考错误码列表 
        virtual int PreviewCamera(CameraSetting& camera_setting_) = 0;

        // 取消预览摄像头
        // @param device_id_ 需要取消预览的摄像头设备 Id
        // @return 0:成功，其它请参考错误码列表 
        virtual int UnPreviewCamera(const string& device_id_) = 0;

        // 导入用户自定义视频数据，前提是已发布 TrackSourceType 为 tst_ExternalYUV 的 Video Track
        // @param track_id_ 已成功发布的 TrackSourceType 为 tst_ExternalYUV 的 Video Track'id
        // @param data_ 数据内存指针
        // @param data_size_ 数据长度
        // @param width_ 图像宽度
        // @param height_ 图像高度
        // @param timestamp_us_ 时间戳，注意单位为:微妙
        // @param raw_type_ 视频原始格式，目前支持：kI420 kYUY2 kRGB24
        // @param rotation_ 导入后旋转角度，如果不需要旋转则使用默认值 kVideoRotation_0 即可
        // @param mirror_flag_ 导入后是否镜像
        // @return 成功返回 0，否则请参考错误码列表 
        virtual int InputVideoFrame(
            const string& track_id_,
            const unsigned char* data_,
            const unsigned int& data_size_,
            const unsigned int& width_,
            const unsigned int& height_,
            const unsigned long long& timestamp_us_,
            qiniu_v2::VideoCaptureType raw_type_,
            qiniu_v2::VideoRotation rotation_ = kVideoRotation_0,
            bool mirror_flag_ = false
        ) = 0;

        // 获取可进行画面采集的屏幕、窗口数量；如需刷新则再次调用即可
        // @return 返回可以进行采集的屏幕、窗口数量 
        virtual int GetScreenWindowCount() = 0;

        // 获取指定 index 的屏幕窗口信息，根据此信息可以进行对应的画面采集
        // @param index_ 需要小于等于 GetScreenWindowCount() 返回值
        // @return ScreenWindowInfo 结构体 
        virtual ScreenWindowInfo& GetScreenWindowInfo(const int& index_) const = 0;

        // 预览指定的屏幕（显示器）或者窗口
        // @param source_id_ Source Id
        // @param render_hwnd_ 渲染窗口句柄，MFC：HWND； QT：winId
        // @param allow_directx_capturer_ 是否激活 DX 采集
        // @return 成功返回 0，否则请参考错误码列表 
        virtual int PreviewScreenSource(
            const int& source_id_,
            void* render_hwnd_,
            bool allow_directx_capturer_ = true
        ) = 0;

        // 取消预览指定的屏幕、窗口
        // @param source_id_ Source Id
        // @return 成功返回 0，否则请参考错误码列表 
        virtual int UnPreviewScreenSource(int source_id_) = 0;

        // 配置渲染时是否镜像其画面（左右反转显示） 
        virtual int Mirror(const std::string& track_id_, bool mirror_flag_) = 0;

        // 原始图像处理功能接口：裁减 + 镜像，目前支持 kI420 格式
        // @param src_data_ 待处理的数据内存指针
        // @param src_width_ 原始图像宽度
        // @param src_height_ 原始图像高度
        // @param src_data_size_ 原始图像数据长度
        // @param picture_fmt_ 原始图像数据格式，目前仅支持 ：kI420
        // @param mirror_flag_ 处理前是否先镜像原始图像
        // @param origin_x_ 开始裁减的 X 坐标点，原点为左上角
        // @param origin_y_ 开始裁减的 Y 坐标点，原点为左上角
        // @param dest_width_ 目标图像宽度
        // @param dest_height_ 目标图像高度
        // @param dest_data_ 目标图像数据内存大小
        // @param max_dest_data_size_ 目标内存 dest_data_ 的内存大小，由开发者在上层管理
        // @param dest_data_size_ 处理成功后，传递目标图像数据长度
        // @return 成功返回 0，否则请参考错误码列表 
        virtual int CropRawPicture(
            unsigned char* src_data_,
            const unsigned int& src_width_,
            const unsigned int& src_height_,
            const unsigned int& src_data_size_,
            qiniu_v2::VideoCaptureType picture_fmt_,
            bool mirror_flag_,
            const int& origin_x_,
            const int& origin_y_,
            const int& dest_width_,
            const int& dest_height_,
            unsigned char* dest_data_,
            const unsigned int& max_dest_data_size_,
            unsigned int& dest_data_size_
        ) = 0;

        // 原始图像处理功能接口：格式转换，目前支持将 kRGB24,kABGR,kARGB,kBGRA 转换为 kI420 格式
        // @param src_data_ 待处理的数据内存指针
        // @param src_width_ 原始图像宽度
        // @param src_height_ 原始图像高度
        // @param src_data_size_ 原始图像数据长度
        // @param src_picture_fmt_ 原始图像数据格式，目前支持 ：kRGB24,kABGR,kARGB,kBGRA
        // @param dest_data_ 目标图像数据内存大小
        // @param max_dest_data_size_ 目标内存 dest_data_ 的内存大小，由开发者在上层管理
        // @param dest_data_size_ 处理成功后，传递目标图像数据长度
        // @return 成功返回 0，否则请参考错误码列表 
        virtual int ConvertToI420(
            unsigned char* src_data_,
            const unsigned int& src_width_,
            const unsigned int& src_height_,
            const unsigned int& src_data_size_,
            qiniu_v2::VideoCaptureType src_picture_fmt_,
            unsigned char* dest_data_,
            const unsigned int& max_dest_data_size_,
            unsigned int& dest_data_size_
        ) = 0;

        // 激活或关闭 SDK 内 D3D 视频渲染模式，默认为开启模式
        // 关闭后将使用 GDI 进行渲染，效果比 D3D 要差，但兼容性更高一些 
        virtual void EnableD3dRender(bool enable_d3d_ = true) = 0;

        // 配置 SDK 是否使用外置视频编解码器，需在 JoinRoom 前调用
        // @param enable_ 是否使用外置编解码器
        // @return 成功返回 0，否则请参考错误码列表 
        virtual int EnableExternalVideoCodec(bool enable_) = 0;

        // 当需要导入外部 H.264 裸码流时，需在 PublishTracks 成功后调用此接口初始化 SDK 内部结构
        /** example:
        auto video_track_ptr = qiniu_v2::QNTrackInfo::CreateVideoTrackInfo(
            "", EXTERNAL_TAG, NULL, width, height, fps, bitrate, qiniu_v2::tst_ExternalH264, false);
        qiniu_v2::TrackInfoList track_list;
        track_list.emplace_back(video_track_ptr);
        auto ret = _rtc_room_interface->PublishTracks(track_list);
        if (ret != 0) {
            Failed...
        }
        video_track_ptr->Release();
        **/
        // @param width_ 图像宽度
        // @param height_ 图像高度
        // @param max_fps_ 帧率
        // @param max_bitrate_ 最大码率
        // @return 成功返回 0，否则请参考错误码列表
        virtual int InitExternalEncoder(
            int32_t width_,
            int32_t height_, 
            int32_t max_fps_, 
            int32_t max_bitrate_
        ) = 0;
        
        // 导入外部 H.264 裸码流时，首先调用， PublishTracks 成功后，调用此接口初始化 SDK 内部相关参数
        // @param nals_ptr Nalu 指针数组
        // @param nals_size nals_ptr 中数组元素个数
        // @return 成功返回 0，否则请参考错误码列表 
        virtual int InputH264Frame(H264Nal** nals_ptr_, unsigned int nals_size_, uint8_t is_key_frame_) = 0;

        // 原始帧处理功能接口：裁剪和缩放，设置参数要求如下，如果设置不正确，则输出原始图像，OnVideoFrame 回调接口中会有输出宽高体现 
        // @param src_capturer_source 数据源类型，目前只支持摄像头采集和用户外部导入的 kRGB24、kYUY2、kI420 数据 
        // @param mode 裁剪/缩放 
        // @param enable 开/关 
        // @param cropX 开始裁减的 X 坐标点，原点为左上角，必须落在原图之内 
        // @param cropY 开始裁减的 Y 坐标点，原点为左上角，必须落在原图之内 
        // @param dstWidth 目标图像宽度，必须为 4 的整数倍，如果是裁剪模式，cropX 和 cropY，裁剪图像必须在原始图像之内 
        // @param dstHeight 目标图像高度，必须为 4 的整数倍，如果是裁剪模式，cropX 和 cropY，裁剪图像必须在原始图像之内 
        virtual void SetCropAndScale(
            qiniu_v2::TrackSourceType src_capturer_source,
            FrameProcessMode mode,
            bool enable,
            int cropX,
            int cropY,
            int dstWidth,
            int dstHeight) = 0;

        // 配置渲染时画面旋转角度 
        virtual int SetVideoRotation(const std::string& track_id_, VideoRotation rotation) = 0;

        // 使用摄像头 track 推送图片流：暂时只支持jpeg图片推送，此接口需要在摄像头推流成功后调用。
        // @param image_path_ 被推送图片路径，设置为 "" 时，关闭图片推送，恢复摄像头采集画面 
        // @return 成功返回 0，失败返回值小于 0 
        virtual int PushCameraTrackWithImage(const std::string& image_path_) = 0;

        // 配置本地渲染窗口画面填充模式 
        // @param stretch_mode_ 画面在窗口中的填充模式，默认是 ASPECT_FIT 
        virtual int SetStretchMode(const std::string& track_id_, MergeStretchMode stretch_mode_) = 0;

        // 配置摄像头采集是否开启镜像功能 
        // @param mirror_flag_ 设置为 true 是开启摄像头画面镜像功能，反之则关闭 
        // @return 成功返回 0，失败返回值小于 0 
        virtual int CameraCaptureMirror(bool mirror_flag_) = 0;

        // 支持自定义 video sei 数据插入，在使用合流功能时插入SEI，必须要保证 
        // 合流时设置的帧率不超过连麦时的帧率。 
        // @param tracks_id_list_ 支持设置 sei 的 track id 链表 
        // @param video_sei_content_ 被插入的 sei 数据 
        // @param video_sei_repeat_times_  当前 sei 数据被插入的次数，-1 表示持续插入。 
        virtual void SetLocalVideoSei(
            const list<string>& tracks_id_list_,
            const std::string& video_sei_content_,
            const int video_sei_repeat_times_) = 0;
    protected:
        virtual ~QNVideoInterface() {}
    };
}

