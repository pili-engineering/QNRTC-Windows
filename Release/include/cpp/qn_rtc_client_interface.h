#ifndef QN_RTC_CLIENT_INTERFACE_H
#define QN_RTC_CLIENT_INTERFACE_H

#include "qn_track_interface.h"

namespace qiniu {

// 网络质量等级
enum QNNetworkGrade { kINVALID = -1, kEXCELLENT = 1, kGOOD, kFAIR, kPOOR };

// 网络上下行质量
struct QNNetworkQuality {
  QNNetworkGrade uplink_network_grade;
  QNNetworkGrade downlink_network_grade;
};

typedef std::map<std::string, QNNetworkQuality> QNNetworkQualityMap;

class QINIU_EXPORT_DLL QNNetworkQualityListener {
 public:
  /**
   * 近端网络状态回调
   */
  virtual void OnNetworkQualityNotified(const QNNetworkQuality& quality) = 0;

 protected:
  ~QNNetworkQualityListener() {}
};

class QINIU_EXPORT_DLL QNPublishResultCallback {
 public:
  /**
   * 发布成功后触发此回调
   */
  virtual void OnPublished() = 0;

  /**
   * 发布失败后触发此回调
   *
   * @param error_code 错误码
   * @param error_message 错误消息
   */
  virtual void OnPublishError(
      int error_code, const std::string& error_message) = 0;  // todo OnError
 protected:
  ~QNPublishResultCallback() {}
};

class QINIU_EXPORT_DLL QNLiveStreamingListener {
 public:
  /**
   * 转推任务成功创建时触发此回调
   *
   * @param stream_id 转推成功的 stream id
   */
  virtual void OnStarted(const std::string& stream_id) = 0;

  /**
   * 转推任务成功停止时触发此回调
   *
   * @param stream_id 停止转推的 stream id
   */
  virtual void OnStopped(const std::string& stream_id) = 0;

  /**
   * 转推任务配置更新时触发此回调
   *
   * @param stream_id 配置更新的 stream id
   */
  virtual void OnTranscodingTracksUpdated(const std::string& stream_id) = 0;

  /**
   * 转推任务出错时触发此回调
   * @param stream_id 出现错误的 stream id
   * @param error_info 详细错误原因
   */
  virtual void OnLiveStreamingError(
      const std::string& stream_id,
      const QNLiveStreamingErrorInfo& error_info) = 0;

 protected:
  virtual ~QNLiveStreamingListener() {}
};

class QINIU_EXPORT_DLL QNRemoteAudioMixedFrameListener {
 public:
  /**
   * 远端各用户音频数据混音后的回调
   *
   * @param data 音频 PCM 数据内存指针
   * @param data_size 数据长度
   * @param bits_per_sample 位宽，即每个采样点占用位数
   * @param sample_rate 采样率
   * @param channels 声道数
   */
  virtual void OnRemoteAudioMixedFrame(const uint8_t* data, uint32_t data_size,
                                       uint32_t bits_per_sample,
                                       uint32_t sample_rate,
                                       uint32_t channels) = 0;

 protected:
  ~QNRemoteAudioMixedFrameListener(){};
};

class QINIU_EXPORT_DLL QNClientRoleListener {
 public:
  /**
   * 设置角色成功回调
   *
   * @param new_role 新角色类型
   */
  virtual void OnClientRoleResult(QNClientRole new_role) = 0;

  /**
   * 设置角色失败回调
   *
   * @param error_code 失败 error code
   * @param error_message 失败描述
   */
  virtual void OnClientRoleError(int error_code,
                                 const std::string& error_message) = 0;

 protected:
  ~QNClientRoleListener() {}
};

class QINIU_EXPORT_DLL QNMediaRelayListener {
 public:
  /**
   * 跨房媒体转发操作成功
   *
   * 注意：此接口回调的是全量的跨房状态，包含所有跨房目标。具体每个目标房间是否转发成功，需要参考回调参数中目标房间对应的
   * QNMediaRelayState
   *
   * @param state_map 具体目标房间状态， key 为房间名， value 为状态
   */
  virtual void OnMediaRelayResult(
      std::map<std::string, QNMediaRelayState> state_map) = 0;

  /**
   * 接口失败回调
   *
   * @param error_code 失败 error code
   * @param error_message 失败描述
   */
  virtual void OnMediaRelayError(int error_code,
                                 const std::string& error_message) = 0;

 protected:
  ~QNMediaRelayListener() {}
};

class QINIU_EXPORT_DLL QNClientEventListener {
 public:
  /**
   * 房间连接状态变化时通知
   *
   * @param state 连接状态
   * @param info 断开连接时状态信息
   */
  virtual void OnConnectionStateChanged(
      QNConnectionState state, const QNConnectionDisconnectedInfo* info) = 0;

  /**
   * 远端用户加入房间时通知
   *
   * @param remote_user_id 远端用户 id
   * @param user_data 远端用户自定义消息
   */
  virtual void OnUserJoined(const std::string& remote_user_id,
                            const std::string& user_data) = 0;

  /**
   * 远端用户离开房间时通知
   *
   * @param remote_user_id 远端用户 id
   */
  virtual void OnUserLeft(const std::string& remote_user_id) = 0;

  /**
   * 远端用户重连时通知
   *
   * @param remote_user_id 远端用户 id
   */
  virtual void OnUserReconnecting(const std::string& remote_user_id) = 0;

  /**
   * 远端用户重连成功时通知
   *
   * @param remote_user_id 远端用户 id
   */
  virtual void OnUserReconnected(const std::string& remote_user_id) = 0;

  /**
   * 远端用户发布新 track 时通知
   *
   * @param remote_user_id 远端用户 id
   * @param track_list 远端用户新发布的 tracks
   */
  virtual void OnUserPublished(const std::string& remote_user_id,
                               const RemoteTrackList& track_list) = 0;

  /**
   * 远端用户取消发布 track 时通知
   *
   * @param remote_user_id 远端用户 id
   * @param track_list 远端用户取消发布的 tracks
   */
  virtual void OnUserUnpublished(const std::string& remote_user_id,
                                 const RemoteTrackList& track_list) = 0;

  /**
   * 订阅 track 成功时通知
   *
   * @param remote_user_id 订阅的 track 所属的远端用户
   * @param remote_audio_track_list 订阅的音频 tracks
   * @param remote_video_track_list 订阅的视频 tracks
   */
  virtual void OnSubscribed(
      const std::string& remote_user_id,
      const RemoteAudioTrackList& remote_audio_track_list,
      const RemoteVideoTrackList& remote_video_track_list) = 0;

  /**
   * 接收收到新的自定义消息时通知
   *
   * @param message 自定义消息
   */
  virtual void OnMessageReceived(const QNCustomMessage& message) = 0;

  /**
   * 跨发媒体转发状态变更，非主动调用触发，由目标房间状态变化引起此通知
   *
   * 目前仅当目标房间关闭时，会触发此通知
   *
   * @param relay_room     发生状态变化的房间
   * @param state         新状态，当前仅可能为 QNMediaRelayState::kRoomClosed
   */
  virtual void OnMediaRelayStateChanged(const std::string& relay_room,
                                        const QNMediaRelayState state) = 0;

 protected:
  ~QNClientEventListener() {}
};

class QINIU_EXPORT_DLL QNRTCClient {
 public:
  /**r
   * 设置直播场景下的用户角色
   * 该方法在加入频道前后均可调用。
   *
   * 注意：该方法仅适用于直播场景（ QNClientMode 为 kLive ）。
   *
   * @param client_role 直播场景里的用户角色
   */
  virtual void SetClientRole(QNClientRole client_role,
                             QNClientRoleListener* listener) = 0;

  /**
   * 配置是否自动订阅远端 track 流
   *
   * @param auto_subscribe，true 自动订阅，false 不自动订阅
   */
  virtual void SetAutoSubscribe(bool auto_subscribe) = 0;

  /**
   * 加入房间，发布、订阅的操作只有当 Join 成功后通过 OnConnectionStateChanged
   * 通知后才能调用
   *
   * @param token 由一定的规则签算生成，具体请查看开发者文档
   * @param user_data 用户自定义数据，后端会透传此数据给房间内的其它用户
   */
  virtual void Join(const std::string& token,
                    const std::string& user_data = "") = 0;

  /**
   * 离开房间，此方法为同步接口，没有对应回调
   */
  virtual void Leave() = 0;

  /**
   * 获取当前房间连接状态
   */
  virtual QNConnectionState GetConnectionState() = 0;

  /**
   * 发布本地媒体流
   *
   * @param track_list 创建的 Track 对象链表
   *
   * @return 仅当返回 0 时才会触发 QNPublishResultCallback
   * 中的回调，否则请参考错误码列表查看错误原因
   */
  virtual void Publish(LocalTrackList& track_list,
                       QNPublishResultCallback* listener) = 0;

  /**
   * 取消发布本地媒体流，此方法为同步接口，没有对应的回调
   *
   * @param track_list 取消发布的 tracks，可以使用开发者创建的 track
   * 对象也可以通过 GetPublishedTracks 获取
   *
   * @return 0:成功；其它请参考错误码列表
   */
  virtual void UnPublish(LocalTrackList& track_list) = 0;

  /**
   * 订阅远端用户发布的媒体流（tracks），不可订阅自己,成功时会触发 OnSubscribed
   * 回调
   *
   * @param track_list 需要订阅的 tracks，由开发者在 OnUserPublished
   * 回调中记录或者通过 GetRemoteUsers 获取
   */
  virtual void Subscribe(const RemoteTrackList& track_list) = 0;

  /**
   * 取消订阅远端其它用户发布的媒体流，此方法为同步接口，没有对应的回调
   *
   * @param track_list 取消发布的 tracks
   */
  virtual void UnSubscribe(const RemoteTrackList& track_list) = 0;

  /**
   * 发送自定义消息
   *
   * @param users_list 指定发送的用户列表，为空的话，发给房间内所有用户
   * @param message_id 消息 id
   * @param message 消息内容
   */
  virtual void SendMessage(const std::list<std::string>& users_list,
                           const std::string& message_id,
                           const std::string& message) = 0;

  /**
   * 设置 CDN 转推流监控回调
   *
   * @param listener QNLiveStreamingListener 派生类实例指针
   */
  virtual void SetLiveStreamingListener(QNLiveStreamingListener* listener) = 0;

  /**
   * 开启 CDN 直播流转推，转推结果通过 QNLiveStreamingListener 通知
   *
   * @param config QNDirectLiveStreamingConfig 结构体
   */
  virtual void StartLiveStreaming(QNDirectLiveStreamingConfig& config) = 0;

  /**
   * 停止 CDN 直播流转推，转推结果通过 QNLiveStreamingListener 通知
   *
   * @param config QNDirectLiveStreamingConfig 结构体
   */
  virtual void StopLiveStreaming(QNDirectLiveStreamingConfig& config) = 0;

  /**
   * 开启 CDN 合流转推，转推结果通过 QNLiveStreamingListener 通知
   *
   * @param config QNTranscodingLiveStreamingConfig 结构体
   */
  virtual void StartLiveStreaming(QNTranscodingLiveStreamingConfig& config) = 0;

  /**
   * 停止 CDN 合流转推，转推结果通过 QNLiveStreamingListener 通知
   *
   * @param config QNTranscodingLiveStreamingConfig 结构体
   */
  virtual void StopLiveStreaming(QNTranscodingLiveStreamingConfig& config) = 0;

  /**
   * 添加合流的 Tracks，结果通过 QNLiveStreamingListener 通知
   *
   * @param stream_id 合流任务 id
   * @param transcoding_track_list 增加的 track 合流配置信息
   */
  virtual void SetTranscodingLiveStreamingTracks(
      std::string& stream_id,
      QNTranscodeingTrackList& transcoding_track_list) = 0;

  /**
   * 删除合流中的 Tracks，结果通过 QNLiveStreamingListener 通知
   *
   * @param stream_id 合流任务 id
   * @param transcoding_track_list 删除的 track
   */
  virtual void RemoveTranscodingLiveStreamingTracks(
      std::string& stream_id,
      QNTranscodeingTrackList& transcoding_track_list) = 0;

  /**
   * 设置远端各 tracks 音频数据混音后的回调
   *
   * @param listener SetRemoteAudioMixedFrameListener 派生类实例指针
   */
  virtual void SetRemoteAudioMixedFrameListener(
      QNRemoteAudioMixedFrameListener* listener) = 0;

  /**
   * 获取本地发布的所有 tracks
   *
   * @return 本地发布的 tracks 信息
   */
  virtual LocalTrackList& GetPublishedTracks() = 0;

  /**
   * 获取指定远端用户发布的信息
   *
   * @param user_id 远端用户 id
   *
   * @return 指定远端用户发布的信息
   */
  virtual QNRemoteUser& GetRemoteUsers(const std::string& user_id) = 0;

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
  virtual QNLocalAudioTrackStatsMap& GetLocalAudioTrackStats() = 0;

  /**
   * 获取远端音频 track 质量统计信息
   *
   * @return QNRemoteAudioTrackStats 结构体
   */
  virtual QNRemoteAudioTrackStatsMap& GetRemoteAudioTrackStats() = 0;

  /**
   * 获取本地视频 tracks 质量统计信息
   *
   * @return QNLocalVideoStatsList 链表
   */
  virtual QNLocalVideoTracksStatsMap& GetLocalVideoTrackStats() = 0;

  /**
   * 获取远端视频 tracks 质量统计信息
   *
   * @return QNRemoteVideoStatsList 链表
   */
  virtual QNRemoteVideoTrackStatsMap& GetRemoteVideoTrackStats() = 0;

  /**
   * 设置当前应用网络质量监听
   */
  virtual void SetNetworkQualityListener(
      QNNetworkQualityListener* listener) = 0;

  /**
   * 获取当前订阅的远端用户网络质量
   */
  virtual QNNetworkQualityMap& GetUserNetworkQuality() = 0;

  /**
   * 开启跨房媒体转发
   * 如果已经开启，则调用失败
   * 当所有目标房间跨房媒体转发都失败，则本次跨房媒体转发请求视为失败，使用跨房媒体转发功能需要再次调用此方法。
   * 当有任意一个目标或多个目标房间媒体转发成功，则本次跨房媒体转发请求视为成功，具体每个房间的状态参考回调结果。
   *
   * 注意：该方法仅适用于互动直播场景中角色类型为主播的用户。
   *
   * @param configuration     跨房媒体转发配置
   * @param callback          跨房媒体转发回调接口
   */
  virtual void StartMediaRelay(QNMediaRelayConfiguration& configuration,
                               QNMediaRelayListener* listener) = 0;

  /**
   * 更新跨房媒体转发
   * 成功开启跨房媒体转发后，如果您希望将流转发到多个目标房间，或退出当前正在转发的房间，可以调用该方法。
   * 此方法为全量更新，正在跨房媒体转发中却未被包含在参数 configuration
   * 中的房间，将停止媒体转发。
   *
   * 注意：调用此方法前必须确保已经成功开启跨房媒体转发，否则将调用失败；该方法仅适用互动直播场景中角色类型为主播的用户。
   *
   * @param configuration     跨房媒体转发配置
   * @param callback          跨房媒体转发回调接口
   */
  virtual void UpdateMediaRelay(QNMediaRelayConfiguration& configuration,
                                QNMediaRelayListener* listener) = 0;

  /**
   * 停止跨房媒体转发
   * 如果未开启，则调用失败
   *
   * 注意：一旦停止，会停止在所有目标房间中的媒体转发；该方法仅适用互动直播场景中角色类型为主播的用户。
   *
   * @param callback          跨房媒体转发回调接口
   */
  virtual void StopMediaRelay(QNMediaRelayListener* listener) = 0;

 protected:
  ~QNRTCClient() {}
};

}  // namespace qiniu

#endif  // QN_RTC_CLIENT_INTERFACE_H
