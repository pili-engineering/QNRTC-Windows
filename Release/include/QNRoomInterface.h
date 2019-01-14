#pragma once
#include "CommonDef.h"

namespace qiniu_v2 {

    using namespace std;

    class QNVideoInterface;
    class QNAudioInterface;

    // 房间管理接口，此为单实例接口 
    class QINIU_EXPORT_DLL QNRoomInterface
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

        protected:
            virtual ~QNRoomListener() {}
        };

    public:
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

        // 配置信令信令间隔，影响监控网络断开的灵敏度
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

        // 配置各个 Track 的合流参数
        // @param add_tracks_list_ 新增 Tracks 的合流配置
        // @param remove_tracks_list 去除 Tracks 的合流配置，Track Id 链表
        // @return 0:操作成功，具体合流结果请通过观看旁路直播进行查看 
        virtual int SetMergeStreamlayouts(
            const MergeOptInfoList& add_tracks_list_,
            const list<string>& remove_tracks_id_list
        ) = 0;

        // 停止所有此房间内的所有合流任务
        // 房间未连接的情况下停止无效
        // @return 0:操作成功，具体合流结果请通过观看旁路直播进行查看 
        virtual int StopMergeStream() = 0;

    protected:
        virtual ~QNRoomInterface() {}
    };
}