#pragma once
#include "CommonDef.h"

namespace qiniu_v2 {

    using namespace std;

    class QNVideoInterface;
    class QNAudioInterface;

    // 房间管理接口，此为单实例接口 
#if defined _WIN32
    class QINIU_EXPORT_DLL QNRoomInterface
#else    
    class QNRoomInterface
#endif // WIN32
    {
    public:
        // 房间异步回调接口 
        class QNRoomListener
        {
        public:
            // 加入房间结果回调，由 JoinRoom 或者网络断线重连后触发
            // @param error_code_ 0:成功，其它请参考错误码列表
            // @param error_str_ 当 error_code_ 非 0 时有效，用于描述错误信息
            // @param user_list_ 用户信息列表，用于传递当前房间已存在的用户信息列表
            // @param tracks_list_ 当前房间已经存在的 Track 列表
            // @param reconnect_flag_ 标识此次触发是否是 SDK 内部自动重连触发的 
            virtual void OnJoinResult(
                int error_code_, 
                const string& error_str_,
                const UserInfoList& user_list_,
                const TrackInfoList& tracks_list_,
                bool reconnect_flag_
            ) = 0;

            // 只有当自己以非正常方式离开的时候才会触发，如异地登录、被踢出房间
            // @param error_code_ 离开房间的错误码
            // @param error_str_ 错误码描述字符串
            // @param user_id_ 当被踢出房间时，传递操作者的 User Id
            virtual void OnLeave(int error_code_, const string& error_str_, const string& user_id_) = 0;
            
            // 房间连接状态变化通知
            // @param state_ 当前房间状态 
            virtual void OnRoomStateChange(RoomState state_) = 0;

            // 本地 Tracks 发布结果异步通知
            // @param error_code_ 0：发布成功；其它请参考错误码列表
            // @param error_str_ 当 error_code_ 非 0 时的错误描述信息
            // @param track_list_ 本地发布的 Tracks 列表 
            virtual void OnPublishTracksResult(
                int error_code_,
                const string& error_str_,
                const TrackInfoList& track_list_
            ) = 0;

            // 订阅远端 Tracks 结果异步通知
            // @param error_code_ 0：发布成功；其它请参考错误码列表
            // @param error_str_ 当 error_code_ 非 0 时的错误描述信息
            // @param track_list_ 订阅的远端 Tracks 列表 
            virtual void OnSubscribeTracksResult(
                int error_code_,
                const string& error_str_,
                const TrackInfoList& track_list_
            ) = 0;

            // 远端用户发布了 Tracks Id 列表，开发者可在此回调中订阅相关 Track
            // @param track_list_ Tracks Id list
            virtual void OnRemoteAddTracks(
                const TrackInfoList& track_list_
            ) = 0;

            // 远端用户取消发布的 Tracks Id 列表，开发者可在此回调中删除与其对应的上层系统资源
            // @param track_list_ Tracks Id list
            virtual void OnRemoteDeleteTracks(
                const list<string>& track_list_
            ) = 0;

            // 远端用户加入房间通知
            // @param user_id_ User Id，房间内唯一标识，不能重复
            // @param user_data_ 用户自定义数据，服务器用于透传，最大长度 1024 字节 
            virtual void OnRemoteUserJoin(
                const string& user_id_,
                const string& user_data_
            ) = 0;

            // 远端用户离开房间通知
            // @param user_id_ User Id，房间内唯一标识，不能重复
            // @param error_code_ 用户离开房间错误码，0：为正常离开，其它可参考错误码列表 
            virtual void OnRemoteUserLeave(
                const string& user_id_,
                int error_code_
            ) = 0;

            // 踢出用户结果通知，只有操作者本人会收到
            // @param kicked_out_user_id_ 被踢出房间的 User Id
            // @param error_code_ 踢人操作结果，0：成功，Err_No_Permission：没有权限
            // @param error_str_ error_code_ 非 0 时的错误描述信息 
            virtual void OnKickoutResult(
                const std::string& kicked_out_user_id_,
                int error_code_,
                const std::string& error_str_
            ) = 0;

            // 远端用户发布的 Track 改变了静默标识
            // @param track_id_ Track Id
            // @param mute_flag_ true:音频静音，或者视频画面黑屏；false：恢复音频音量或者视频画面 
            virtual void OnRemoteTrackMuted(
                const string& track_id_,
                bool mute_flag_
            ) = 0;

            // 连麦质量回调，每 5 秒钟回调一次 
            virtual void OnStatisticsUpdated(
                const StatisticsReport& statistics_
            ) = 0;
            
            //自定义消息回调。 
            virtual void OnReceiveMessage(
                const CustomMessageList& custom_message_
            ) = 0;

            // 本地 Tracks 取消发布结果异步通知 
            // @param track_list_ 本地发布的 Tracks 列表 
            virtual void OnUnPublishTracksResult(
                const TrackInfoList& track_list_
            ) = 0;

            // 远端音视频质量参数回调 
            virtual void OnRemoteStatisticsUpdated(
                const StatisticsReportList& statistics_list_
            ) = 0;

            // 创建合流任务结果反馈 
            // @param job_id_ 合流任务 id 
            // @param error_code_ 0：成功， 非 0： 失败 
            // @param error_str_ error_code_ 非 0 时的错误描述信息 
            virtual void OnCreateMergeResult(
                const std::string& job_id_,
                int error_code_,
                const std::string& error_str_
            ) = 0;

            // 停止合流任务结果反馈 
            // @param job_id_ 合流任务 id 
            // @param job_iid_ iid 用于区分 id 相同时的情况，可以不用处理此字段 
            // @param error_code_ 0：成功， 非 0： 失败 
            // @param error_str_ error_code_ 非 0 时的错误描述信息 
            virtual void OnStopMergeResult(
                const std::string& job_id_,
                const std::string& job_iid_,
                int error_code_,
                const std::string& error_str_
            ) = 0;

            // 切换远端 Tracks 大小流结果异步通知 
            // @param error_code_ 0：切换成功；其它请参考错误码列表 
            // @param error_str_ 当 error_code_ 非 0 时的错误描述信息 
            // @param track_list_ 切换 profile 的 Tracks 列表 
            virtual void OnSetSubscribeTracksProfileResult(
                int error_code_,
                const string& error_str_,
                const TrackInfoList& track_list_
            ) = 0;

            // 创建单路转推任务结果反馈 
            // @param job_id_ 单路转推的任务 id 
            // @param error_code_ 0：成功， 非 0： 失败 
            // @param error_str_ error_code_ 非 0 时的错误描述信息 
            virtual void OnCreateForwardResult(
                const std::string& job_id_,
                int error_code_,
                const std::string& error_str_
            ) = 0;

            // 停止单路转推任务结果反馈 
            // @param job_id_ 单路转推的任务 id 
            // @param job_iid_ iid 用于区分 id 相同时的情况，可以不用处理此字段 
            // @param error_code_ 0：成功， 非 0： 失败 
            // @param error_str_ error_code_ 非 0 时的错误描述信息 
            virtual void OnStopForwardResult(
                const std::string& job_id_,
                const std::string& job_iid_,
                int error_code_,
                const std::string& error_str_
            ) = 0;

            // 当远端用户进入重连时会触发此回调 
            // @param remote_user_id_ 远端用户 id 
            virtual void OnRemoteUserReconnecting(const std::string& remote_user_id_) = 0;

            // 当远端用户重连成功时会触发此回调 
            // @param remote_user_id_ 远端用户 id 
            virtual void OnRemoteUserReconnected(const std::string& remote_user_id_) = 0;

        protected:
            virtual ~QNRoomListener() {}
        };

    public:

        // 用于获取SDK版本号 
        static void GetVersion(std::string& ver);

        // 用于获取全局唯一的房间实例指针 
        static QNRoomInterface* ObtainRoomInterface();

        // 释放由 ObtainRoomInterface 返回的指针，释放后原指针为野指针 
        static void DestroyRoomInterface(QNRoomInterface* interface_ptr_);

        // 配置 SDK 日志信息，如果名字相同，则进程每次启动会覆盖原来的日志文件
        // 日志文件大小上限为 100 M，SDK 内部写入时会自动检测
        // @param level_ 日志等级，等级越低日志越多
        // @param dir_name_ 日志存储目录，如果不存在则自动创建
        // @param file_name_ 日志文件名 
        static int  SetLogParams(
            QNLogLevel level_,
            const std::string& dir_name_,
            const std::string& file_name_
        );

    public:
        // 消息循环驱动，必须由开发者在上层循环调用
        // 建议由开发者在上层主线程定时调用以触发所有事件，否则所有的事件将不会触发 
        virtual void Loop() = 0;

        // 加入房间，发布、订阅的操作只有当 JoinRoom 成功后才能调用，否则直接返回错误
        // @param room_token_ RoomToken 由一定的规则签算生成，具体请查看开发者文档
        // @param user_data_ 用户自定义数据，后端会透传此数据给房间内的其它用户
        // @return 仅当返回 0 时才会触发 OnJoinResult 回调，否则请查看错误码列表 
        virtual int JoinRoom(
            const std::string& room_token_,
            const std::string& user_data_ = ""
        ) = 0;

        // 离开房间，此方法为同步接口，没有对应回调 
        virtual int LeaveRoom() = 0;

        // 判断是否成功加入了房间
        // @return true or false
        virtual bool IsJoined() = 0;

        // 设置房间异步事件监听，此接口非线程安全，需在 JoinRoom 前配置，LeaveRoom 前不能修改
        // @param listener_ QNRoomListener 派生类实例指针，用于接收所有事件通知 
        virtual void SetRoomListener(QNRoomListener* listener_) = 0;

        // 获取视频功能接口 QNVideoInterface 实例指针，以进行视频相关操作 
        virtual QNVideoInterface* ObtainVideoInterface() = 0;

        // 获取音频功能接口 QNAudioInterface 实例指针，以进行音频相关操作 
        virtual QNAudioInterface* ObtainAudioInterface() = 0;

        // 配置信令间隔，影响监控网络断开的灵敏度
        // 间隔越低，越可以更快的检测到网络断开
        // @param interval_seconds_ 信令心跳间隔，可设置范围为 1~ 10；单位：秒 
        virtual void SetHeartBeatInterval(int interval_seconds_ = 3) = 0;

        // 发布本地媒体流
        // TrackInfo 的创建只有 TrackInfo::CreateVideoTrackInfo 和 TrackInfo::CreateAudioTrackInfo
        // @param tracks_info_list_ Tracks info 链表
        // @return 仅当返回 0 时才会触发 OnPublishTracksResult 回调，否则请参考错误码列表查看错误原因 
        virtual int PublishTracks(
            const TrackInfoList& tracks_info_list_
        ) = 0;

        // 取消发布本地媒体流，此方法为同步接口，没有对应的回调
        // @param tracks_id_list_ 取消发布的 tracks id 链表，track id 在 OnJoinResult 回调中获取并记录
        // @return 0:成功；其它请参考错误码列表 
        virtual int UnPublishTracks(
            const list<string>& tracks_id_list_
        ) = 0;
        
        // 订阅远端用户发布的媒体流（Tracks），不可订阅自己
        // @param tracks_info_list_ 需要订阅的 Tracks info，由开发者在 OnRemoteAddTracks 回调中 Copy  并记录
        // @return 仅当返回 0 时才会触发 OnSubscribeTracksResult 回调，否则请参考错误码列表查看错误原因 
        virtual int SubscribeTracks(
            const TrackInfoList& tracks_info_list_
        ) = 0;

        // 取消订阅远端其它用户发布的媒体流，此方法为同步接口，没有对应的回调
        // @param tracks_id_list_ 取消发布的 tracks id 链表
        // @return 0:成功；其它请参考错误码列表 
        virtual int UnSubscribeTracks(
            const list<string>& tracks_id_vec_
        ) = 0;

        // 踢出指定用户离开当前房间，踢人权限由 RoomToken 签算时控制
        // @param user_id_ 被踢出房间的 User Id
        // @return 仅当返回 0 时才会触发 OnKickoutResult 回调，否则请参考错误码回调 
        virtual int KickoutUser(const string& user_id_) = 0;

        // 静默本地音频 Track，远端订阅的用户将听不到自己的任何声音
        // @param mute_flag_ true：静音；false：取消静音
        // @return 0:成功；其它请参考错误码列表 
        virtual int MuteAudio(bool mute_flag_) = 0;

        // 静默本地视频 Track，远端订阅的用户看到的将会是黑屏
        // @param mute_flag_ true：黑屏；false：取消黑屏
        // @return 0:成功；其它请参考错误码列表 
        virtual int MuteVideo(const string& track_id_, bool mute_flag_) = 0;

        // 创建自定义合流任务
        // 当有合流及单路转推切换需求时，合流必须使用自定义合流，流地址一样时切换会有抢流现象，
        // 因此需要拼接 "?serialnum=xxx" 决定流的优先级，serialnum 值越大，优先级越高。
        // 切换成功后务必关闭之前的任务，否则会出现一路任务持续计费的情况
        // @param job_desc: 合流任务配置结构
        // @param merge_background: 合流背景图
        // @param merge_watermark: 合流水印图
        // @return 0:成功；其它请参考错误码列表 
        virtual int CreateMergeJob(const MergeJob& job_desc, const MergeLayer& merge_background, const MergeLayerList& merge_watermark) = 0;

        // 配置各个 Track 的合流参数
        // @param add_tracks_list_ 新增 Tracks 的合流配置
        // @param remove_tracks_list 去除 Tracks 的合流配置，Track Id 链表
        // @param job_id 合流任务id
        // @return 0:操作成功，具体合流结果请通过观看旁路直播进行查看 
        virtual int SetMergeStreamlayouts(
            const MergeOptInfoList& add_tracks_list_,
            const list<string>& remove_tracks_id_list,
            const string& job_id = string()
        ) = 0;

        // 停止所有此房间内的所有合流任务
        // 房间未连接的情况下停止无效
        // @param job_id 合流任务id
        // @param delay_ms 延迟停止合流时间，单位：ms 
        // @return 0:操作成功，具体合流结果请通过观看旁路直播进行查看 
        virtual int StopMergeStream(const string& job_id, const int delay_ms) = 0;

        // 配置媒体传输通道底层传输协议，默认为 preferUdp，当用户网络下 UDP 不通时，SDK 自动降级使用 TCP
        // @param policy_ value of Enum:IcePolicy
        virtual void SetIcePolicy(qiniu_v2::IcePolicy policy_) = 0;

        // 获取当前本地用户 Id，只有 JoinRoom 后才有效
        // @return 用户 Id
        virtual const string& GetLocalUserId() = 0;
        
        // 发送自定义消息 
        // @param users 目标用户列表，为空时则给房间中的所有人发消息（注意：不能填 null） 
        // @param messageId 消息 ID，可以为空（注意：不能填 null） 
        // @param message 消息内容（注意：不能填 null，不支持发送空消息，入参前需转成 utf8 格式） 
        // @return 0:操作成功；其它请参考错误码列表。 
        virtual int SendCustomMessage(const list<string>& users_list_, const string& message_id, const string& message_) = 0;

        // 切换订阅流的 profile 
        // @param trackInfoList 需要切换 profile 的 tracks 链表 
        virtual int UpdateSubscribeTracks(TrackInfoList& trackInfo_list_) = 0;

        // 创建单路转推任务
        // 当有合流及单路转推切换需求时，合流必须使用自定义合流，流地址一样时切换会有抢流现象，
        // 因此需要拼接 "?serialnum=xxx" 决定流的优先级， serialnum 值越大，优先级越高。
        // 切换成功后务必关闭之前的任务，否则会出现一路任务持续计费的情况
        // @param job_des 单路转推任务配置结构 
        // @return 0:操作成功，具体转推结果请通过观看旁路直播进行查看 
        virtual int CreateForwardJob(const ForwardOptInfo& job_des) = 0;

        // 停止单路转推任务
        // @param job_id 单路转推任务 id 
        // @param delay_ms 延迟停止合流时间，单位：ms 
        // @return 0:操作成功，具体转推结果请通过观看旁路直播进行查看 
        virtual int StopForwardJob(const string& job_id, const int delay_ms) = 0;

        // 开启数据统计
        // @param period_ms 每次统计间隔时间，单位毫秒 
        virtual void EnableStatistics(int period_ms) = 0;

        // 设置 dns 域名解析服务器地址 需要在 JoinRoom 之前调用 
        // @param url_ dns 服务器地址 
        virtual void SetDnsServerUrl(const std::string& url_) = 0;

    protected:
        virtual ~QNRoomInterface() {}
    };
}
