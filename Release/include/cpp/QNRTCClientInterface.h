#pragma once
#include "QNTrackInterface.h"

namespace qiniu {
    class QINIU_EXPORT_DLL QNPublishResultCallback
    {
    public:
        /**
         * 发布成功后触发此回调
         */
        virtual void OnPublished() = 0;

        /**
         * 发布失败后触发此回调
         *
         * @param errorCode 错误码
         * @param errorMessage 错误消息
         */
        virtual void OnPublishError(int errorCode, const std::string& errorMessage) = 0;
    protected:
        ~QNPublishResultCallback() {}
    };

    class QINIU_EXPORT_DLL QNLiveStreamingListener
    {
    public:
        /**
         * 转推任务成功创建时触发此回调
         *
         * @param streamID 转推成功的 streamID
         */
        virtual void OnStarted(const std::string& streamID) = 0;

        /**
         * 转推任务成功停止时触发此回调
         *
         * @param streamID 停止转推的 streamID
         */
        virtual void OnStopped(const std::string& streamID) = 0;

        /**
         * 转推任务配置更新时触发此回调
         *
         * @param streamID 配置更新的 streamID
         */
        virtual void OnTranscodingTracksUpdated(const std::string& streamID) = 0;

        /**
         * 转推任务出错时触发此回调
         *  @param streamID 出现错误的 streamID
         * @param errorInfo 详细错误原因
         */
        virtual void OnError(const std::string& streamID, const QNLiveStreamingErrorInfo& errorInfo) = 0;
    protected:
        virtual ~QNLiveStreamingListener() {}
    };

    class QINIU_EXPORT_DLL QNRemoteAudioMixedFrameListener
    {
    public:
        /**
         * 远端各用户音频数据混音后的回调
         *
         * @param data 音频数据内存指针
         * @param bitsPerSample 位宽，即每个采样点占用位数
         * @param sampleRrate 采样率
         * @param channels 声道数
         * @param samplePoints 此次回调内包含了多少采样点数
         */
        virtual void OnRemoteMixAudioPCMFrame(
            const unsigned char* data,
            int bitsPerSample,
            int sampleRrate,
            size_t channels,
            size_t samplePoints
        ) = 0;
    protected:
        ~QNRemoteAudioMixedFrameListener() {};
    };

    class QINIU_EXPORT_DLL QNClientEventListener
    {
    public:
        /**
         * 摄像头插拔消息通知
         *
         * @param deviceState 设备状态
         * @param deviceName 设备名
         */
        virtual void OnVideoDeviceStateChanged(QNVideoDeviceState deviceState, const std::string& deviceName) = 0;

        /**
         * 音频设备插拔消息通知
         *
         * @param deviceState 设备状态
         * @param deviceName 设备 guid
         */
        virtual void OnAudioDeviceStateChanged(QNAudioDeviceState deviceState, const std::string& deviceGuid) = 0;

        /**
         * 房间连接状态变化时通知
         *
         * @param state 连接状态
         * @param info 断开连接时状态信息
         */
        virtual void OnConnectionStateChanged(QNConnectionState state, const QNConnectionDisconnectedInfo& info) = 0;

        /**
         * 远端用户加入房间时通知
         *
         * @param remoteUserID 远端用户 id
         * @param userData 远端用户自定义消息
         */
        virtual void OnUserJoined(const std::string& remoteUserID, const std::string& userData) = 0;

        /**
         * 远端用户离开房间时通知
         *
         * @param remoteUserID 远端用户 id
         */
        virtual void OnUserLeft(const std::string& remoteUserID) = 0;

        /**
         * 远端用户重连时通知
         *
         * @param remoteUserID 远端用户 id
         */
        virtual void OnUserReconnecting(const std::string& remoteUserID) = 0;

        /**
         * 远端用户重连成功时通知
         *
         * @param remoteUserID 远端用户 id
         */
        virtual void OnUserReconnected(const std::string& remoteUserID) = 0;

        /**
         * 远端用户发布新 track 时通知
         *
         * @param remoteUserID 远端用户 id
         * @param trackList 远端用户新发布的 tracks 
         */
        virtual void OnUserPublished(const std::string& remoteUserID, const RemoteTrackList& trackList) = 0;

        /**
         * 远端用户取消发布 track 时通知
         *
         * @param remoteUserID 远端用户 id
         * @param trackList 远端用户取消发布的 tracks
         */
        virtual void OnUserUnpublished(const std::string& remoteUserID, const RemoteTrackList& trackList) = 0;

        /**
         * 订阅 track 成功时通知
         *
         * @param remoteUserID 订阅的 track 所属的远端用户
         * @param remoteAudioTracks 订阅的音频 tracks
         * @param remoteVideoTracks 订阅的视频 tracks
         */
        virtual void OnSubscribed(
            const std::string& remoteUserID,
            const RemoteAudioTrackList& remoteAudioTracks,
            const RemoteVideoTrackList& remoteVideoTracks
        ) = 0;

        /**
         * 接收收到新的自定义消息时通知
         *
         * @param message 自定义消息
         */
        virtual void OnMessageReceived(const CustomMessageList& message) = 0;

        /**
         * 摄像头和窗口捕获画面预览时数据回调
         *
         * @param data 视频数据内存指针
         * @param dataSize 数据长度
         * @param width 视频宽
         * @param height 视频高
         * @param videoType 视频数据类型
         */
        virtual void OnPreviewVideoFrame(
            const unsigned char* data,
            int dataSize,
            int width,
            int height,
            QNVideoSourceType videoType
        ) = 0;
    protected:
        ~QNClientEventListener() {}
    };

    class QINIU_EXPORT_DLL QNRTCClient
    {
    public:
        /**
         * 设置 QNRTCClient 事件监控回调
         *
         * @param listener QNClientEventListener 派生类实例指针
         */
        virtual void SetQNClientEventListener(QNClientEventListener* listener) = 0;

        /**
         * 消息循环驱动，必须由开发者在上层循环调用
         * 建议由开发者在上层主线程定时每 10ms 调用一次以触发所有事件
         */
        virtual void Loop() = 0;

        /**
         * 获取摄像头数量
         *
         * @return 返回设备数量
         */
        virtual int GetCameraCount() = 0;

        /**
         * 获取指定序号的摄像头设备信息
         * 首先通过 GetCameraCount 获取摄像头数量
         *
         * @param index 设备序号，<= GetCameraCount()
         *
         * @return CameraDeviceInfo 结构体 
         */
        virtual const QNCameraDeviceInfo& GetCameraInfo(unsigned int index) = 0;

        /**
         * 获取可进行画面采集的屏幕、窗口数量；如需刷新则再次调用即可
         *
         * @return 返回可以进行采集的屏幕、窗口数量
         */
        virtual int GetScreenWindowCount() = 0;

        /**
         * 获取指定 index 的屏幕窗口信息，根据此信息可以进行对应的画面采集
         *
         * @param index，<= GetScreenWindowCount()
         *
         * @return ScreenWindowInfo 结构体
         */
        virtual const QNScreenWindowInfo& GetScreenWindowInfo(unsigned int index) = 0;

        /**
         * 用于获取指定类型的音频设备数量，adt_record or adt_playout
         *
         * @param deviceType 设备类型，录制 or 播放
         *
         * @return 指定类型的设备数量
         */
        virtual int GetAudioDeviceCount(QNAudioDeviceInfo::QNAudioDeviceType deviceType) = 0;

        /**
         * 获取指定 index 设备信息
         *
         * @param deviceType 设备类型
         * @param index 设备 index
         * @param audioInfo 输出参数，用于返回指定的音频设备信息
         */
        virtual void GetAudioDeviceInfo(QNAudioDeviceInfo::QNAudioDeviceType deviceType, unsigned int index, QNAudioDeviceInfo& audioInfo) = 0;

        /**
         * 设置连麦使用的音频录制设备，不调用则使用系统默认录制设备
         * 连麦过程中设置无效，需在发布音频 Track 前调用
         *
         * @param recordingDeviceSetting AudioDeviceSetting 结构体
         *
         * @return 成功返回 0，其它请参考错误码列表
         */
        virtual int SetRecordingDevice(QNAudioDeviceSetting& recordingDeviceSetting) = 0;

        /**
         * 设置连麦使用的音频播放设备，不调用则使用系统默认播放设备
         * 连麦过程中设置无效，需在订阅任何音频 Track 前调用
         *
         * @param playoutDeviceSetting AudioDeviceSetting 结构体
         *
         * @return 成功返回 0，其它请参考错误码列表
         */
        virtual int SetPlayoutDevice(QNAudioDeviceSetting& playoutDeviceSetting) = 0;

        /**
         * 预览摄像头，摄像头不能重复打开
         *
         * @param cameraSetting 指定摄像头参数
         *
         * @return 0:成功，其它请参考错误码列表
         */
        virtual int PreviewCamera(QNCameraPreviewSetting& cameraSetting) = 0;

        /**
         * 取消预览摄像头
         *
         * @param screenId 需要取消预览的摄像头 id
         *
         * @return 成功返回 0，否则请参考错误码列表
         */
        virtual int UnPreviewCamera(std::string& cameraId) = 0;

        /**
         * 预览指定的屏幕（显示器）或者窗口
         *
         * @param screenSetting ScreenSetting 结构体
         *
         * @return 成功返回 0，否则请参考错误码列表
         */
        virtual int PreviewScreen(QNScreenPreviewSetting& screenSetting) = 0;

        /**
         * 取消预览指定的屏幕、窗口
         *
         * @param screenId 需要取消预览的屏幕、窗口的 id
         *
         * @return 成功返回 0，否则请参考错误码列表
         */
        virtual int UnPreviewScreen(unsigned int screenId) = 0;

        /**
         * 获取当前支持的编码器
         *
         * @return 返回支持的编码器能力集信息
         */
        virtual const QNEncoderCapability& GetSupportEncoder() = 0;

        /**
         * 配置 SDK 是使用的视频编解码器类型，需在 Join 前调用，不设置的话默认使用 Open264 编码器
         * 注意: 不支持运行过程动态修改编码器类型，硬件编码和多流功能同时开启的话，多流功能会失效。
         *
         * @param encodeType 编码器类型
         *
         * @return 成功返回 0，否则请参考错误码列表
         */
        virtual void SetVideoEncoder(QNVideoEncodeType encodeType) = 0;

        /**
         * 设置 dns 域名解析服务器地址 需要在 Join 之前调用
         * 
         * @param url dns 服务器地址
         */
        virtual void SetDnsServerUrl(const std::string& url) = 0;

        /**
         * 配置媒体传输通道底层传输协议，默认为 preferUdp，当用户网络下 UDP 不通时，SDK 自动降级使用 TCP
         *
         * @param policy_ value of Enum:IcePolicy
         */
        virtual void SetIcePolicy(QNIcePolicy policy) = 0;

        /**
         * 配置是否自动订阅远端 track 流
         *
         * @param autoSubscribe true 自动订阅，fale 不自动订阅
         */
        virtual void SetAutoSubscribe(bool autoSubscribe) = 0;

        /**
         * 加入房间，发布、订阅的操作只有当 Join 成功后通过 OnConnectionStateChanged 通知后才能调用
         *
         * @param token 由一定的规则签算生成，具体请查看开发者文档
         * @param userData 用户自定义数据，后端会透传此数据给房间内的其它用户
         */
        virtual void Join(const std::string& token, const std::string& userData = "") = 0;

        /**
         * 离开房间，此方法为同步接口，没有对应回调 
         */
        virtual void Leave() = 0;

        /**
         * 判断是否成功加入了房间
         *
         * @return true or false
         */
        virtual bool IsJoined() = 0;

        /**
         * 设置发布结果事件监控回调
         *
         * @param listener QNPublishResultCallback 派生类实例指针
         */
        virtual void SetQNPublishResultCallback(QNPublishResultCallback* listener) = 0;

        /**
         * 发布本地媒体流
         *
         * @param trackList 创建的 Track 对象链表
         *
         * @return 仅当返回 0 时才会触发 QNPublishResultCallback 中的回调，否则请参考错误码列表查看错误原因
         */
        virtual int Publish(LocalTrackList& trackList) = 0;

        /**
         * 取消发布本地媒体流，此方法为同步接口，没有对应的回调
         *
         * @param trackList 取消发布的 tracks，可以使用开发者创建的 track 对象也可以通过 GetPublishedTracks 获取
         *
         * @return 0:成功；其它请参考错误码列表
         */
        virtual int UnPublish(LocalTrackList& trackList) = 0;

        /**
         * 订阅远端用户发布的媒体流（tracks），不可订阅自己,成功时会触发 OnSubscribed 回调
         *
         * @param trackList 需要订阅的 tracks，由开发者在 OnUserPublished 回调中记录或者通过 GetRemoteUsers 获取 
         */
        virtual void Subscribe(const RemoteTrackList& trackList) = 0;

        /**
         * 取消订阅远端其它用户发布的媒体流，此方法为同步接口，没有对应的回调
         *
         * @param trackList 取消发布的 tracks
         */
        virtual void UnSubscribe(const RemoteTrackList& trackList) = 0;

        /**
         * 发送自定义消息
         *
         * @param usersList 指定发送的用户列表，为空的话，发给房间内所有用户
         * @param messageId 消息 id
         * @param message 消息内容
         *
         * @return QNRemoteVideoStatsList 链表
         */
        virtual void SendCustomMessage(const std::list<std::string>& usersList, const std::string& messageId, const std::string& message) = 0;

        /**
         * 设置 CDN 转推流监控回调
         *
         * @param listener QNLiveStreamingListener 派生类实例指针
         */
        virtual void SetLiveStreamingListener(QNLiveStreamingListener* listener) = 0;

        /**
         * 开启 CDN 直播流转推，转推结果通过 QNLiveStreamingListener 通知
         *
         * @param config QNDirectLiveStreamingConfig结构体
         */
        virtual void StartLiveStreaming(QNDirectLiveStreamingConfig& config) = 0;

        /**
         * 停止 CDN 直播流转推，转推结果通过 QNLiveStreamingListener 通知
         *
         * @param config QNDirectLiveStreamingConfig结构体
         */
        virtual void StopLiveStreaming(QNDirectLiveStreamingConfig& config) = 0;

        /**
         * 开启 CDN 合流转推，转推结果通过 QNLiveStreamingListener 通知
         *
         * @param config QNTranscodingLiveStreamingConfig结构体
         */
        virtual void StartLiveStreaming(QNTranscodingLiveStreamingConfig& config) = 0;

        /**
         * 停止 CDN 合流转推，转推结果通过 QNLiveStreamingListener 通知
         *
         * @param config QNTranscodingLiveStreamingConfig结构体
         */
        virtual void StopLiveStreaming(QNTranscodingLiveStreamingConfig& config) = 0;

        /**
         * 添加合流的 track，结果通过 QNLiveStreamingListener 通知
         *
         * @param streamID 合流任务 id
         * @param transcodingTracks 增加的 track 合流配置信息
         */
        virtual void SetTranscodingLiveStreamingTracks(std::string& streamID, QNTranscodeingTrackList& transcodingTracks) = 0;

        /**
         * 删除合流中的 track，结果通过 QNLiveStreamingListener 通知
         *
         * @param streamID 合流任务 id
         * @param transcodingTracks 删除的 track
         */
        virtual void RemoveTranscodingLiveStreamingTracks(std::string& streamID, QNTranscodeingTrackList& transcodingTracks) = 0;

        /**
         * 设置远端各 tracks 音频数据混音后的回调
         *
         * @param listener QNRemoteAudioMixedFrameListener 派生类实例指针
         */
        virtual void SetRemoteTracksMixedAudioListener(QNRemoteAudioMixedFrameListener* listener) = 0;

        /**
         * 获取本地发布的所有 tracks
         *
         * @return 本地发布的 tracks 信息
         */
        virtual LocalTrackList& GetPublishedTracks() = 0;

        /**
         * 获取指定远端用户发布的信息
         *
         * @param userId 远端用户 id
         *
         * @return 指定远端用户发布的信息
         */
        virtual QNRemoteUser& GetRemoteUsers(const std::string& userId) = 0;

        /**
         * 获取远端所有用户发布的信息
         *
         * @return 所有远端用户发布的信息
         */
        virtual RemoteUserList& GetRemoteUsers() = 0;

        /**
         * 获取本地音频 track 质量统计信息
         *
         * @return QNLocalAudioTrackStats 结构体
         */
        virtual QNLocalAudioTrackStats& GetLocalAudioTrackStats() = 0;

        /**
         * 获取远端音频 track 质量统计信息
         *
         * @return QNRemoteAudioTrackStats 结构体
         */
        virtual QNRemoteAudioTrackStats& GetRemoteAudioTrackStats() = 0;

        /**
         * 获取本地视频 tracks 质量统计信息
         *
         * @return QNLocalVideoStatsList 链表
         */
        virtual QNLocalVideoStatsList& GetLocalVideoTrackStats() = 0;

        /**
         * 获取远端视频 tracks 质量统计信息
         *
         * @return QNRemoteVideoStatsList 链表
         */
        virtual QNRemoteVideoStatsList& GetRemoteVideoTrackStats() = 0;

        /**
         * 原始图像处理功能接口：裁减 + 镜像，目前支持 kI420 格式
         *
         * @param srcData 待处理的数据内存指针
         * @param srcWidth 原始图像宽度
         * @param srcHeight 原始图像高度
         * @param srcDataSize 原始图像数据长度
         * @param pictureFmt 原始图像数据格式，目前仅支持 ：kI420
         * @param mirror 处理前是否先镜像原始图像
         * @param originX 开始裁减的 X 坐标点，原点为左上角
         * @param originY 开始裁减的 Y 坐标点，原点为左上角
         * @param destWidth 目标图像宽度
         * @param destHeight 目标图像高度
         * @param destData 目标图像数据内存大小
         * @param MaxDestDataSize 目标内存 destData 的内存大小，由开发者在上层管理
         * @param DestDataSize 处理成功后，传递目标图像数据长度
         *
         * @return 成功返回 0，否则请参考错误码列表 
         */
        virtual int CropRawPicture(
            unsigned char* srcData,
            const unsigned int& srcWidth,
            const unsigned int& srcHeight,
            const unsigned int& srcDataSize,
            qiniu::QNVideoSourceType pictureFmt,
            bool mirror,
            const int& originX,
            const int& originY,
            const int& destWidth,
            const int& destHeight,
            unsigned char* destData,
            const unsigned int& MaxDestDataSize,
            unsigned int& DestDataSize
        ) = 0;

        /**
         * 原始图像处理功能接口：格式转换，目前支持将 kRGB24, kABGR, kARGB, kBGRA 转换为 kI420 格式
         * 
         * @param srcData 待处理的数据内存指针
         * @param srcWidth 原始图像宽度
         * @param srcHeight 原始图像高度
         * @param srcDataSize 原始图像数据长度
         * @param pictureFmt 原始图像数据格式，目前支持: kRGB24, kABGR, kARGB, kBGRA
         * @param destData 目标图像数据内存大小
         * @param MaxDestDataSize 目标内存 destData 的内存大小，由开发者在上层管理
         * @param DestDataSize 处理成功后，传递目标图像数据长度
         *
         * @return 成功返回 0，否则请参考错误码列表 
         */
        virtual int ConvertToI420(
            unsigned char* srcData,
            const unsigned int& srcWidth,
            const unsigned int& srcHeight,
            const unsigned int& srcDataSize,
            qiniu::QNVideoSourceType pictureFmt,
            unsigned char* destData,
            const unsigned int& MaxDestDataSize,
            unsigned int& DestDataSize
        ) = 0;
    protected:
        ~QNRTCClient() {}
    };
}