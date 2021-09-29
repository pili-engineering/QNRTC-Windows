#pragma once
#include "CommonDef.h"

namespace qiniu {
    class QNLocalTrack;
    class QNRemoteTrack;
    class QNRemoteVideoTrack;
    class QNRemoteAudioTrack;
    class QNRemoteUser;

    typedef std::list<QNLocalTrack*> LocalTrackList;
    typedef std::list<shared_ptr<QNRemoteTrack>> RemoteTrackList;
    typedef std::list<shared_ptr<QNRemoteVideoTrack>> RemoteVideoTrackList;
    typedef std::list<shared_ptr<QNRemoteAudioTrack>> RemoteAudioTrackList;
    typedef std::list<QNRemoteUser>  RemoteUserList;

    class QNRemoteUser {
    public:
        /**
         * 远端用户的 userID
         */
        std::string user_id;

        /**
         * 远端用户数据
         */
        std::string user_data;

        /**
         * 远端用户发布的音频 track 列表
         */
        RemoteAudioTrackList remote_audio_tracks;

        /**
         * 远端发布的视频 track 列表
         */
        RemoteVideoTrackList remote_video_tracks;
    };

    class QINIU_EXPORT_DLL QNLocalAudioFrameListener
    {
    public:
        /**
         * 本地采集音频数据回调
         *
         * @param data 音频数据内存指针
         * @param bitsPerSample 位宽，即每个采样点占用位数
         * @param sampleRrate 采样率
         * @param channels 声道数
         * @param samplePoints 此次回调内包含了多少采样点数
         */
        virtual void OnLocalAudioPCMFrame(
            const unsigned char* data,
            int bitsPerSample,
            int sampleRrate,
            size_t channels,
            size_t samplePoints
        ) = 0;

    protected:
        virtual ~QNLocalAudioFrameListener() {}
    };

    class QINIU_EXPORT_DLL QNLocalVideoFrameListener
    {
    public:
        /**
         * 本地采集视频数据回调
         *
         * @param trackId 视频 track id
         * @param data 视频数据内存指针
         * @param dataSize 数据长度
         * @param width 视频宽
         * @param height 视频高
         * @param videoType 视频数据类型
         */
        virtual void OnLocalVideoFrame(
            const string& trackId,
            const unsigned char* data,
            int dataSize,
            int width,
            int height,
            QNVideoSourceType videoType
        ) = 0;
    protected:
        ~QNLocalVideoFrameListener() {}
    };

    class QINIU_EXPORT_DLL QNRemoteAudioFrameListener
    {
    public:
        /**
         * 远端音频数据回调
         *
         * @param userId 此音频数据所属的用户
         * @param data 音频数据内存指针
         * @param bitsPerSample 位宽，即每个采样点占用位数
         * @param sampleRrate 采样率
         * @param channels 声道数
         * @param samplePoints 此次回调内包含了多少采样点数
         */
        virtual void OnRemoteAudioFrame(
            const string& userId,
            const unsigned char* data,
            int bitsPerSample,
            int sampleRrate,
            size_t channels,
            size_t samplePoints
        ) = 0;

    protected:
        virtual ~QNRemoteAudioFrameListener() {}
    };

    class QINIU_EXPORT_DLL QNRemoteVideoFrameListener
    {
    public:
        /**
         * 远端视频数据回调
         *
         * @param userId 此视频频数据所属的用户
         * @param trackId 视频 track id
         * @param data 视频数据内存指针
         * @param dataSize 数据长度
         * @param width 视频宽
         * @param height 视频高
         * @param videoType 视频数据类型
         */
        virtual void OnRemoteVideoFrame(
            const std::string& userId,
            const string& trackId,
            const unsigned char* data,
            int dataSize,
            int width,
            int height,
            QNVideoSourceType videoType
        ) = 0;
    protected:
        ~QNRemoteVideoFrameListener() {}
    };

    class QINIU_EXPORT_DLL QNTrackInfoChangedListener
    {
    public:
        /**
         * 视频 Track profile 改变后触发
         *
         * @param profile 当前的 profile
         */
        virtual void OnVideoProfileChanged(const std::string& trackId, QNTrackProfile profile) = 0;

        /**
         * Track 静默状态改变后触发
         *
         * @param isMuted Track 当前的静默状态
         */
        virtual void OnMuteStateChanged(bool isMuted, const std::string& remoteUserId, const RemoteTrackList& trackList) = 0;
    protected:
        ~QNTrackInfoChangedListener() {}
    };

    class QINIU_EXPORT_DLL QNTrack
    {
    public:
        /**
         * 获取 TrackId
         */
        virtual const QNTrackSourceType GetSourceType() = 0;

        /**
         * 获取 TrackId
         */
        virtual const std::string& GetTrackID() = 0;

        /**
         * 获取 userId
         */
        virtual const std::string& GetUserID() = 0;

        /**
         * 获取 Tag
         */
        virtual const std::string& GetTag() = 0;

        /**
         * 获取媒体类型
         */
        virtual const std::string& GetKind() = 0;

        /**
         * 是否 muted
         */
        virtual bool IsMuted() = 0;

    protected:
        virtual ~QNTrack() {}
    };

    class QINIU_EXPORT_DLL QNLocalTrack
        : public virtual QNTrack
    {
    public:
        /**
         * 设置静默状态
         */
        virtual void SetMuted(bool muted) = 0;

    protected:
        virtual ~QNLocalTrack() {}
    };

    class QINIU_EXPORT_DLL QNLocalAudioTrack
        : public virtual QNLocalTrack
    {
    public:
        /**
         * 设置采集的音频数据监控回调接口
         *
         * @param listener QNLocalAudioFrameListener 派生类实例指针
         */
        virtual void SetAudioFrameListener(QNLocalAudioFrameListener* listener) = 0;

        /**
         * 设置采集音频音量（不改变系统设备的音量）
         *
         * @param volume 音量缩放倍数，建议值为 0.0f ~ 2.0f
         */
        virtual void SetVolume(double volume) = 0;

        /**
         * 获取音频采集的音量
         *
         * @return 音量值
         */
        virtual float GetVolumeLevel() = 0;

        /**
         * 激活监听功能：混合本地麦克风和系统声卡的声音
         *
         * @param enable 是否激活监听功能开关
         * @param volumeScaleRatio 系统声音缩放倍数，以方便控制系统音量大小
         */
        virtual void MixDesktopAudio(bool enable, float volumeScaleRatio = 1.0f) = 0;
    protected:
        ~QNLocalAudioTrack() {}
    };

    class QINIU_EXPORT_DLL QNMicrophoneAudioTrack
        : public virtual QNLocalAudioTrack
    {
    public:
    protected:
        ~QNMicrophoneAudioTrack() {}
    };

    class QINIU_EXPORT_DLL QNCustomAudioTrack
        : public virtual QNLocalAudioTrack
    {
    public:
        /**
         * 推送自定义音频帧
         *
         * @param audioData 音频数据
         * @param dataSize 数据长度
         * @param bitsPerSample 位宽，即每个采样点占用位数
         * @param sampleRrate 采样率
         * @param channels 声道数
         * @param frames 每次导入数据所包含的采样点数
         */
        virtual int PushAudioFrame(
            const unsigned char* audioData,
            unsigned int dataSize,
            unsigned int bitsPerSample,
            unsigned int samplerate,
            unsigned int channels,
            unsigned int frames
        ) = 0;
    protected:
        ~QNCustomAudioTrack() {}
    };

    class QINIU_EXPORT_DLL QNLocalVideoTrack
        : public virtual QNLocalTrack
    {
    public:
        /**
         * 设置采集的视频数据监控回调接口
         *
         * @param listener QNLocalVideoFrameListener 派生类实例指针
         */
        virtual void SetVideoFrameListener(QNLocalVideoFrameListener* listener) = 0;

        /**
         * 设置自定义 video sei 数据插入，在使用合流功能时插入SEI，必须要保证
         * 合流时设置的帧率不超过连麦时的帧率。
         *
         * @param message 被插入的 sei 数据
         * @param repeatCount  当前 sei 数据被插入的次数，-1 表示持续插入
         * @param uuid 16 个字节, 如果传空字符串或者字符串长度不为 16 个字节，则使用默认的 uuid
         */
        virtual void SendSEI(std::string& message, int repeatCount, std::string& uuid) = 0;

        /**
         * 原始帧处理功能接口：裁剪和缩放，设置参数要求如下，如果设置不正确，则输出原始图像
         *
         * @param mode 裁剪/缩放
         * @param enable 开/关
         * @param cropX 开始裁减的 X 坐标点，原点为左上角，必须落在原图之内
         * @param cropY 开始裁减的 Y 坐标点，原点为左上角，必须落在原图之内
         * @param dstWidth 目标图像宽度，必须为 4 的整数倍，如果是裁剪模式，cropX 和 cropY，裁剪图像必须在原始图像之内
         * @param dstHeight 目标图像高度，必须为 4 的整数倍，如果是裁剪模式，cropX 和 cropY，裁剪图像必须在原始图像之内
        */
        virtual void CropAndScaleRawPicture(
            QNVideoProcessMode mode,
            bool enable,
            int cropX,
            int cropY,
            int dstWidth,
            int dstHeight) = 0;

        /**
         * 设置本地视频渲染时画面旋转角度
         *
         * @param rotation 旋转角度
         */
        virtual void SetVideoRotation(QNVideoRotation rotation) = 0;

        /**
         * 设置本地渲染窗口画面填充模式
         *
         * @param stretchMode 画面在窗口中的填充模式，默认是 ASPECT_FIT
         */
        virtual void SetStretchMode(QNStretchMode stretchMode) = 0;

        /**
         * 设置本地采集画面渲染时是否镜像
         *
         * @param mirror 是否镜像，true or false
         */
        virtual void SetRenderMirror(bool mirror) = 0;
    protected:
        virtual ~QNLocalVideoTrack() {}
    };

    class QINIU_EXPORT_DLL QNCameraVideoTrack 
        : public virtual QNLocalVideoTrack
    {
    public:
        /**
         * 设置本地采集画面是否镜像，开启镜像后，订阅端看到的是镜像画面
         *
         * @param mirror 是否镜像，true or false
         */
        virtual void SetCaptureMirror(bool mirror) = 0;

        /**
         * 将摄像头采集流替换为图片流，暂时只支持 jpeg 图片推送，此接口需要在摄像头推流成功后调用。
         *
         * @param imagePath 被推送图片路径，设置为 "" 时，关闭图片推送，恢复摄像头采集画面 
         */
        virtual void PushImage(const std::string& imagePath) = 0;

    protected:
        virtual ~QNCameraVideoTrack() {}
    };

    class QINIU_EXPORT_DLL QNScreenVideoTrack
        : public virtual QNLocalVideoTrack
    {
    public:
        /**
         * 开启 Window Graphics 窗口采集，捕获某些使用 D3D 渲染的窗口时需要开启，否则捕获不到画面 
         *
         * @param enable 是否开启，true or false
         */
        virtual void EnableWindowGraphicsCapture(bool enable) = 0;
    protected:
        virtual ~QNScreenVideoTrack() {}
    };

    class QINIU_EXPORT_DLL QNCustomVideoTrack
        : public virtual QNLocalVideoTrack
    {
    public:
        /**
         * 推送自定义视频帧
         *
         * @param data 视频数据
         * @param dataSize 数据长度
         * @param width 图像宽度
         * @param height 图像高度
         * @param timestampUs 时间戳，注意单位为:微妙
         * @param capturerType 视频原始格式，目前支持：kI420 kYUY2 kRGB24
         * @param rotation 导入后旋转角度，如果不需要旋转则使用默认值 kVideoRotation_0 即可
         * @param mirror 导入后是否镜像
         */
        virtual int PushVideoFrame(
            const unsigned char* data,
            const int dataSize,
            const int width,
            const int height,
            const long long timestampUs,
            QNVideoSourceType capturerType,
            QNVideoRotation rotation,
            bool mirror = false
        ) = 0;
    protected:
        virtual ~QNCustomVideoTrack() {}
    };

    class QINIU_EXPORT_DLL QNRemoteTrack
        : public virtual QNTrack
    {
    public:
        /**
         * 设置远端 track 状态监控回调接口
         *
         * @param listener QNTrackInfoChangedListener 派生类实例指针
         */
        virtual void SetTrackInfoChangedListener(QNTrackInfoChangedListener* listener) = 0;

        /**
         * 是否已订阅
         */
        virtual bool IsSubscribed() = 0;

        /**
         * 设置远端视频渲染窗口句柄
         *
         * @param hwnd 渲染窗口句柄，MFC：HWND； QT：winId
         */
        virtual void SetRenderHwnd(void* hwnd) = 0;
    protected:
        virtual ~QNRemoteTrack() {}
    };

    class QINIU_EXPORT_DLL QNRemoteAudioTrack
        : public virtual QNRemoteTrack
    {
    public:
        /**
         * 设置远端音频数据监控回调接口
         *
         * @param listener QNRemoteAudioFrameListener 派生类实例指针
         */
        virtual void SetAudioFrameListener(QNRemoteAudioFrameListener* listener) = 0;

        /**
         * 设置远端音频音量（不改变系统设备的音量）
         *
         * @param volume 音量缩放倍数，建议值为 0.0f ~ 2.0f
         */
        virtual void SetVolume(double volume) = 0;

        /**
         * 获取音频播放的音量
         *
         * @return 音量值
         */
        virtual float GetVolumeLevel() = 0;
    protected:
        ~QNRemoteAudioTrack() {}
    };

    class QINIU_EXPORT_DLL QNRemoteVideoTrack
        : public virtual QNRemoteTrack
    {
    public:
        /**
         * 设置远端视频数据监控回调接口
         *
         * @param listener QNRemoteVideoFrameListener 派生类实例指针
         */
        virtual void SetVideoFrameListener(QNRemoteVideoFrameListener* listener) = 0;

        /**
         * 远端视频是否开启多流
         */
        virtual bool IsMultiProfileEnabled() = 0;

        /**
         * 多流开启的情况下，可以设置订阅不同 proflie 的流
         *
         * @param profile 需要设置的 profile
         */
        virtual void SetProfile(QNTrackProfile profile) = 0;

        /**
         * 设置远端视频渲染时画面旋转角度
         *
         * @param rotation 旋转角度
         */
        virtual void SetVideoRotation(QNVideoRotation rotation) = 0;

        /**
         * 设置远端画面渲染时的窗口填充模式
         *
         * @param stretchMode 画面在窗口中的填充模式，默认是 ASPECT_FIT
         */
        virtual void SetStretchMode(QNStretchMode stretchMode) = 0;

        /**
         * 设置远端画面渲染时是否镜像
         *
         * @param mirror 是否镜像，true or false
         */
        virtual void SetRenderMirror(bool mirror) = 0;

    protected:
        ~QNRemoteVideoTrack() {}
    };

}