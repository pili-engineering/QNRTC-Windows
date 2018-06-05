<a id="1"></a>
# 1 概述
QNRTC 是七牛云推出的一款适用于 Windows 平台的实时音视频 SDK，提供了灵活的接口，支持高度定制以及二次开发。

<a id="1.1"></a>
## 1.1 下载地址
- [Win32 Demo 以及 SDK 下载地址](http://sdk-release.qnsdk.com/Win32-RTCSDK-0.2.0.zip)


<a id="2"></a>
# 2 功能列表

| 功能                                |   版本    |
| ------------------------------      | ---------- |
| 基本的音视频通话功能                  | v0.2.0(+) |
| 内置音视频采集功能                    | v0.2.0(+) |
| 支持第三方虚拟摄像头采集               | v0.2.0(+) |
| 支持音视频设备的切换                   | v0.2.0(+) |
| 支持踢人功能                          | v0.2.0(+) |
| 支持静音功能                          | v0.2.0(+) |
| 支持视频帧率配置                       | v0.2.0(+) |
| 支持视频码率的配置                     | v0.2.0(+) |
| 支持视频尺寸配置                       | v0.2.0(+) |
| 支持网络自动重连                       | v0.2.0(+) |
| 支持丰富的消息回调                    | v0.2.0(+) |
| 支持纯音频互动                        | v0.2.0(+) |
| 支持获取通话质量统计信息               | v0.2.0(+) |


<a id="3"></a>
# 3 总体设计

本 SDK 基于 `Microsoft Visual Studio 2015` 进行开发，使用了运行时库的多线程静态版本，即 `MTD/MT` 配置，目前提供 Win32 平台架构的开发包。

<a id="3.1"></a>
## 3.1 基本规则

为了方便理解和使用，对于 SDK 的接口设计，我们遵循了如下的规则：

- 每一个接口类，均以 `QNRTC` 开头

<a id="3.2"></a>
## 3.2 核心接口类

核心接口类说明如下：

|   接口类名            |        功能        |   备注  |
|   ------             |        ---         |   ---  |
| QNRTCEngine          | 全局引擎类          | 整体初始化、释放以及日志等级、文件名配置等 |
| QNRTCRoom            | 房间管理核心接口类   | 包含房间控制相关的接口，及各种回调 |
| QNRTCAudio           | 音频接口类          | 包含音频设备枚举、参数配置和数据回调等|
| QNRTCVideo           | 视频接口类          | 包含视频设备枚举、预览、参数配置和数据回调等|

<a id="3.3"></a>
## 3.3 回调相关类

回调相关类说明如下：

|      接口名      |     功能        |   备注  |
|     ------     |      ---       |   ---   |
| QNRTCRoom::QNRTCRoomListener |  房间相关的所有回调   | 包括用户加入/离开房间、发布/取消发布音视频以及房间状态等回调通知|
| QNRTCAudio::QNRTCAudioListener | 连麦音频的相关回调 | 包括音频 PCM 数据及设备插拔状态回调 |
| QNRTCVideo::QNRTCVideoListener | 连麦视频的相关回调 | 包括视频 YUV 数据及设备插拔状态回调 |

另外，统计信息回调中的 `StatisticsReport`，提供了通话过程中的各用户的统计信息，包括通话过程中实时的音视频码率、帧率、丢包率等信息。

<a id="4"></a>
# 4 阅读对象

本文档为技术文档，需要阅读者：

- 具有基本的 Windows 端的开发能力
- 准备接入七牛云实时音视频 SDK

<a id="5"></a>
# 5 开发准备

<a id="5.1"></a>
## 5.1 开发语言以及系统要求

- 开发语言：C++11 
- 系统要求：Windows Vista 及以上版本 Windows 系统

<a id="5.2"></a>
## 5.2 开发环境配置

- Visual Studio 2015 [下载地址](https://www.visualstudio.com/zh-hans/vs/older-downloads/)
- Win32 Platform

<a id="5.3"></a>
## 5.3 导入 SDK

将下载好的 SDK 拷贝到你的工程目录中，添加 `$(ProjectDir)Release\include` 到头文件引用目录，添加 `$(ProjectDir)Release\lib` 到依赖库查找目录中，并拷贝 `QNRtcStreamingD.dll` 和 `QNRtcStreaming.dll` 分别到 `Debug` 和 `Release` 模式的运行目录下，然后在代码中通过静态加载的方式加载 SDK 库，如下：

```   
#include "qn_rtc_room.h"
#include "qn_rtc_audio.h"
#include "qn_rtc_video.h"
#include "qn_rtc_errorcode.h"

using namespace qiniu;

#ifdef _DEBUG
#pragma comment(lib, "QNRtcStreamingD.lib")
#else
#pragma comment(lib, "QNRtcStreaming.lib")
#endif // _DEBUG
```

<a id="6"></a>
# 6 快速开始

<a id="6.1"></a>
## 6.1 初始化连麦相关对象

调用 `QNRTCEngine` 接口进行全局初始化：

```
QNRTCEngine::Init();
// 配置日志输出等级、目录及文件名，不调用 = 不输出
QNRTCEngine::SetLogParams(qiniu::LOG_INFO, "rtc-log", "rtc.log");
```

创建房间 `QNRTCRoom` 实例（全局唯一）：

```
QNRTCRoom* _rtc_room_interface = QNRTCRoom::ObtainRoomInterface();
```

获取连麦音频 `QNRTCAudio` 接口实例指针，连麦视频 `QNRTCVideo` 接口实例指针：

```
QNRTCAudio* _rtc_audio_interface = _rtc_room_interface->ObtainAudioInterface();
QNRTCVideo* _rtc_video_interface = _rtc_room_interface->ObtainVideoInterface();
```

设置房间回调、音频回调以及视频回调:

```
// this 为实现了所有回调接口类的对象指针
_rtc_room_interface->SetRoomListener(this);
_rtc_audio_interface->SetAudioListener(this);
_rtc_video_interface->SetVideoListener(this);
```

其中各个 listener （`QNRTCRoom::QNRTCRoomListener`、`QNRTCAudio::QNRTCAudioListener`以及`QNRTCVideo::QNRTCVideoListener`）的回调相关说明，详见 [代理回调](##7.4代理回调)

<a id="6.2"></a>
## 6.2 设置摄像头相关参数

设置视频采集、预览、发布等相关参数

```
CameraSetting camera_setting;
camera_setting.render_hwnd = GetDlgItem(IDC_STATIC_VIDEO_PREVIEW)->m_hWnd;
camera_setting.device_name = unicode2utf(cur_dev_name.GetBuffer());
camera_setting.device_id   = cur_dev_id;
camera_setting.width       = std::get<0>(tuple_size);
camera_setting.height      = std::get<1>(tuple_size);
camera_setting.max_fps     = 15;
camera_setting.bitrate     = 500000;
```

<a id="6.3"></a>
## 6.3 加入房间

```
_rtc_room_interface->JoinRoom(_room_token);
```

此处 `_room_token` 需要 App 从 App Server 中获取，App Server 如何生成 token 可[查阅文档](https://github.com/pili-engineering/QNRTC-Server/blob/master/docs/api.md#5-roomtoken-%E7%9A%84%E8%AE%A1%E7%AE%97)。

<a id="6.4"></a>
## 6.4 发布音/视频

当 `enable_audio_` 为 `true` 时可发布音频，而 `enable_video_` 为 `true` 时则可发布视频。

```
_rtc_room_interface->Publish(true, true);
```

<a id="6.5"></a>
## 6.5 订阅远端用户

在远端用户发布媒体流的回调通知中订阅该用户

```
void CRtcXXX::OnRemotePublish(const std::string& user_id_, bool has_audio_, bool has_video_){
    // itor->second.render_wnd_ptr->m_hWnd 为渲染视频的窗口句柄
    _rtc_room_interface->Subscribe(user_id_, itor->second.render_wnd_ptr->m_hWnd);
}
```

也可以在远端发布后，根据产品需求在合适的时机通过调用如下的方法来订阅：

```   
virtual int32_t Subscribe(const std::string& user_id_, void* render_hwnd_) = 0;
```

注：用于渲染视频的窗口句柄，在 MFC 中直接使用窗口类的 m_hWnd 成员变量获取，Qt 中通过窗口类的 winId() 方法进行获取。

<a id="7"></a>
# 7 功能使用

<a id="7.1"></a>
## 7.1 视频配置

<a id="7.1.1"></a>
### 7.1.1 枚举摄像头列表及其采集属性

视频设备属性结构体 `CameraDeviceInfo` 

```   
/**
* 摄像头采集属性
*/
typedef struct _TCameraCapability
{
	int                 width;
	int                 height;
	int                 max_fps;
	VideoCaptureType    video_type;
} CameraCapability;

typedef std::vector<CameraCapability> CameraCapabilityVec;

/**
* 摄像头基本信息及其支持的采集属性
*/
typedef struct _TCameraDeviceInfo
{
	std::string         device_id;
	std::string         device_name;
	CameraCapabilityVec capability_vec;
}CameraDeviceInfo;
```

以上可通过 `QNRTCVideo::GetCameraInfo(uint32_t device_index_)` 获取；

<a id="7.1.2"></a>
### 7.1.2 视频参数配置

通过 `CameraSetting` 设置指定摄像头设备信息和采集（同编码）尺寸、帧率、码率以及预览窗口句柄：

```
typedef struct _TCameraSetting
{
    std::string device_id;   
    std::string device_name;   
    int         width       = 640;      // 宽度
    int         height      = 480;      // 高度
    int         max_fps     = 15;       // 帧率：默认为 15fps
    int         bitrate     = 300000;   // 视频码率，单位：bps
    void*       render_hwnd = nullptr;  // 视频渲染窗口句柄
}CameraSetting;
```
注：需在视频预览或发布前，调用 `QNRTCVideo::SetCameraParams(CameraSetting& camera_setting_)` 进行配置，连麦过程中调用无效。

<a id="7.2"></a>
## 7.2 音频配置

<a id="7.2.1"></a>
### 7.2.1 音频设备枚举

音频设备信息结构体：
```
/**
* Audio device information
*/
typedef struct _TAudioDeviceInfo
{
	enum AudioDeviceType
	{
		adt_invalid = -1,
		adt_record,
		adt_playout,
	};
	uint32_t        device_index;
	AudioDeviceType device_type               = adt_invalid;
	char device_name[QNRTC_MAX_DEVICE_LENGHT] = { 0 };
	char device_id[QNRTC_MAX_DEVICE_LENGHT]   = { 0 };
}AudioDeviceInfo;
```

以上可通过 `QNRTCAudio::GetAudioDeviceInfo(
            AudioDeviceInfo::AudioDeviceType device_type_, uint32_t device_index_, 
            __out AudioDeviceInfo& audio_info_)` 进行获取，如：

```
// 枚举音频采集设备列表
for (int i(0); i < _rtc_audio_interface->GetAudioDeviceCount(AudioDeviceInfo::adt_record); ++i) {
	AudioDeviceInfo audio_info;
	if (_rtc_audio_interface->GetAudioDeviceInfo(AudioDeviceInfo::adt_record, i, audio_info) == 0) {
        ...
		);
	}
}

// 枚举音频播放设备列表
for (int i(0); i < _rtc_audio_interface->GetAudioDeviceCount(AudioDeviceInfo::adt_record); ++i) {
	AudioDeviceInfo audio_info;
	if (_rtc_audio_interface->GetAudioDeviceInfo(AudioDeviceInfo::adt_playout, i, audio_info) == 0) {
		...
	}
}
```

<a id="7.2.2"></a>
### 7.2.2 音频设备配置

SDK 目前支持指定特定的音频输入、输出设备进行连麦，相关控制接口为：

```
typedef struct _TAudioDeviceSetting
{
	enum WindowsDeviceType
	{
		wdt_DefaultCommunicationDevice = -1,
		wdt_DefaultDevice = -2
	};
	uint16_t            device_index;   //speaker or playout device index
	WindowsDeviceType   device_type = wdt_DefaultDevice;
}AudioDeviceSetting;

// 指定音频输入设备进行连麦
virtual int SetRecordingDevice(AudioDeviceSetting recording_device_setting_) = 0;
// 指定音频输出设备进行连麦
virtual int SetPlayoutDevice(AudioDeviceSetting playout_device_setting_) = 0;
```
注：通话过程中指定设备无效，需在通话开始前进行相关操作。

<a id="7.3"></a>
## 7.3 连麦相关操作

主要包括加入、离开房间，发布、取消、静默本地媒体流，订阅、取消订阅远端媒体流，以及其它用户进入、离开房间的管理、通知，以及服务端合流相关的配置操作等。

<a id="7.3.1"></a>
### 7.3.1 加入/离开房间

- 加入房间

```   
virtual int32_t JoinRoom(const std::string& room_token_) = 0;
```

此方法为异步方法，执行结果在 `QNRTCRoomListener::OnJoinResult` 进行通知，错误码请参考错误码列表；

- 离开房间

```   
virtual int32_t LeaveRoom() = 0;
```

此方法为用户主动退出房间，为同步方法，没有回调通知；

<a id="7.3.2"></a>
### 7.3.2 发布/取消发布 

- 发布本地音/视频流，成功后远端可以订阅收看

```   
virtual int32_t Publish(bool enable_audio_ = true, bool enable_video_ = true) = 0;
```

此方法为异步方法，执行结果在 `QNRTCRoomListener::OnLocalPublishResult` 进行通知，错误码请参考错误码列表；

- 取消发布本地音/视频流

```   
virtual int32_t UnPublish() = 0;
```

此方法为同步方法，没有异步回调通知接口；

<a id="7.3.3"></a>
### 7.3.3 订阅/取消订阅 

- 订阅远端用户的音/视频

```   
virtual int32_t Subscribe(const std::string& user_id_, void* render_hwnd_) = 0;
```

此方法为异步方法，执行结果在 `QNRTCRoomListener::OnSubscribeResult` 进行通知，错误码请参考错误码列表；

- 取消订阅远端用户的音/视频

```   
virtual int32_t UnSubscribe(const std::string& user_id_) = 0;
```

此方法为同步方法，没有异步回调通知接口；

<a id="7.3.4"></a>
### 7.3.4 踢人

将用户 user_id_ 踢出房间

```   
virtual int32_t KickoutUser(const std::string& user_id_) = 0;
```

此方法需要拥有 `admin` 权限（获取`RoomToken`时指定）才可以踢人成功，此方法为异步方法，执行结果通过 `QNRTCRoomListener::OnKickoutResult` 进行通知；

<a id="7.3.5"></a>
### 7.3.5 Mute 本地音/视频

- 静默本地已发布视频流，置为 `true` 后，远端用户用户看到的视频画面将为黑色，本地预览也为黑色，置为 `false` 后，取消静默，恢复原状；

```
virtual int32_t MuteVideo(bool mute_flag_) = 0;   
```

- 静默本地已发布音频流，置为 `true` 后，远端用户用户将无法听你到你的声音，置为 `false` 后，取消静默，恢复原状；

```   
virtual int32_t MuteAudio(bool mute_flag_) = 0;
```

<a id="7.4"></a>
## 7.4 `QNRTCRoom::QNRTCRoomListener` 代理回调

>主要是房间管理和通知的异步回调。

<a id="7.4.1"></a>
### 7.4.1 用户加入房间的回调

```
// JoinRoom 加入房间的异步操作结果通知
// @param error_code_ 为执行结果，为 0 表示加入房间成功，其它请参考错误码列表
// @param error_str_ 为加入房间失败时，对失败原因的描述，成功时为空字符串
// @param suer_date_vec_ 当加入房间成功时，在此房间内已有的用户信息列表
virtual void OnJoinResult(int32_t error_code_, const std::string& error_str_,
                const UserDataInfoVec& user_data_vec_) = 0;
```

用户信息 `UserDataInfo` 的定义：

```
typedef struct _TUserDataInfo
{
	std::string user_id;                    // 房间内用户 ID
	bool        audio_published = false;    // 音频是否已发布
	bool        video_published = false;    // 视频是否已发布
	bool        audio_mute      = false;    // 是否静默了音频
	bool        video_mute      = false;    // 是否静默了视频
}UserDataInfo;

typedef std::vector<UserDataInfo>   UserDataInfoVec;
```

<a id="7.4.2"></a>
### 7.4.2 房间状态变化的回调

```
// 房间网络状态变化通知
// @brief 网络断开时，SDK 内部会自动进行重连，每次重连时，会通过此接口回调通知 rs_reconnecting 状态
// @param status_ 房间当前的网络状态
virtual void OnStateChanged(RoomState status_) = 0;
```

房间状态 `RoomState` 定义：

```   
enum RoomState
{
    rs_idle,            // 空闲状态，未连接
    rs_connecting,      // 连接中
    rs_connected,       // 已连接
    rs_reconnecting,    // 连接断开，并在自动重连中
};
```

<a id="7.4.3"></a>
### 7.4.3 发布本地音视频回调

```
// @param error_code_ Publish 方法执行结果，0 为成功，其它请参考错误码列表
// @param error_str_ error_code_ 非 0 时，用于错误描述字符串
virtual void OnLocalPublishResult(int32_t error_code_, const std::string& error_str_) = 0;
```

<a id="7.4.4"></a>
### 7.4.4 远端用户加入／离开房间的回调

- 远端用户加入房间的回调

```
// @param user_id_ 远端用户在此房间内的唯一标识
// @param user_data_ 用户自定义数据字符串，暂为空
virtual void OnRemoteUserJoin(const std::string& user_id_, const std::string& user_data_) = 0;
```

- 远端用户离开房间的回调

```
// @param user_id_ 远端用户在此房间内的唯一标识
// @param error_code_ 用户离开房间的原因，0 为主动离开，其它请参考错误码列表，一般为被踢出房间，即 Err_Kickout_Of_Room : 10006
virtual void OnRemoteUserLeave(const std::string& user_id_, int32_t error_code_) = 0;
```

<a id="7.4.5"></a>
### 7.4.5 远端用户发布／取消发布音视频的回调

- 远端用户发布音视频的回调

```
// @param user_id_ 远端用户在此房间内的唯一标识
// @param enable_audio_ 此用户是否发布了音频
// @param enable_video_ 此用户是否发布了视频
virtual void OnRemotePublish(const std::string& user_id_, bool enable_audio_, bool enable_video_) = 0;
```

- 远端用户取消发布音视频的回调

```   
virtual void OnRemoteUnPublish(const std::string& user_id_) = 0;
```

<a id="7.4.6"></a>
### 7.4.6 订阅远端用户的回调

- 订阅远端用户的回调

```
// Subscribe 方法执行结果
// @param user_id_ 远端用户在此房间内的唯一标识
// @param error_code_ 0 为成功，其它请参考错误码列表
// @param error_str_ error_code_ 非 0 时，用于错误描述字符串
virtual void OnSubscribeResult(const std::string& user_id_, int32_t error_code_, const std::string& error_str_) = 0;
```

<a id="7.4.7"></a>
### 7.4.7 远端用户音/视频 Mute 状态的回调

- 远端用户静默（或取消静默）了其音、视频流

```
// @param user_id_ 远端用户在此房间内的唯一标识
// @param mute_audio_ 用户是否静默了音频
// @param mute_video_ 用户是否静默了视频
virtual void OnRemoteStreamMute(const std::string& user_id_, const bool mute_audio_, bool mute_video_) = 0;
```

<a id="7.4.8"></a>
### 7.4.8 踢人回调

本地调用 `KickoutUser` 方法的执行结果通知，此操作需要 `admin` 权限

```
// KickoutUser 方法执行结果
// @param user_id_ 远端用户在此房间内的唯一标识
// @param error_code_ 0 为成功，其它请参考错误码列表
// @param error_str_ error_code_ 非 0 时，用于错误描述字符串
virtual void OnKickoutResultconst std::string& user_id_,
                int32_t error_code_, const std::string& error_str_) = 0;
```

<a id="7.4.9"></a>
### 7.4.9 SDK 运行过程中的错误回调

SDK 运行过程中发生错误会通过该方法回调，具体错误码的定义可以参阅 `qn_rtc_errorcode.h` 文件

```
// @param error_code_ 错误码
// @param error_str_ error_code_ 非 0 时，用于错误描述字符串
virtual void OnError(int32_t error_code_, const std::string& error_str_) = 0;
```

错误码列表如下：

|  QNRTCErrorDomain  |  错误码  |        功能         |
|       ------        |     ---     |        ---        |
| `Err_Token_Error`        | 10001 | token 错误  |
| `Err_Token_Expired`       | 10002 | token 过期 |
| `Err_Room_INSTANCE_Closed`  | 10003 | 房间实例关闭 |
| `Err_ReconnToken_Error` | 10004 | 重连 token 错误 |
| `Err_Room_Closed`          | 10005 | 房间已关闭 |
| `Err_Kickout_Of_Room`    | 10006 | 房间内踢人失败 |
| `Err_Room_Full`         | 10011 | 房间已满 |
| `Err_ROOM_Not_Exist`        | 10012 | 房间不存在 |
| `Err_User_Not_Exist`       | 10021 | 用户不存在 |
| `Err_User_Already_Exist`    | 10022 | 用户已存在 |
| `Err_Publish_Stream_Not_Exist` | 10031 | 流不存在 |
| `Err_Publish_Stream_Info_Not_Match` | 10032 | 流信息不匹配 |
| `Err_Publish_Stream_Already_exist` | 10033 | 流已存在 |
| `Err_Publish_Stream_Not_Ready` | 10034 | 流未完成 |
| `Err_Subscribe_Stream_Not_Exist` | 10041 | 订阅不存在流 |
| `Err_Subscribe_Stream_Info_Not_Match` | 10042 | 订阅不匹配信息流 |
| `Err_Subscribe_Stream_Already_Exist` | 10043 | 订阅已订阅的流 |
| `Err_Cannot_Subscribe_Self`       | 10044 | 无法订阅自己 |
| `Err_No_Permission`        | 10051 | 未许可 |
| `Err_Server_Unavailable`   | 10052 | 服务不可用 |
| `Err_Invalid_Parameters`    | 11000 | 参数错误 |
| `Err_Internal_Null_Pointer` | 11001 | sdk 内部空指针 |
| `Err_Cannot_Destroy_In_Self_Thread` | 11005 | 不可在自己的线程中释放自己 |
| `Err_Cannot_Change_When_Using` | 11006 | 采集中不可更改设备 |
| `Err_Failed_Set_Recorder_Device` | 11007 | 设置录音设备失败 |
| `Err_Failed_Set_Communication_Recorder_Device` | 11008 | 设置交互采集设备失败 |
| `Err_Failed_Set_Playout_Device` | 11009 | 设置普通音频输出设备失败 |
| `Err_Failed_Set_Communication_Playout_Device` | 11010 | 设置音频通信输出设备失败 |
| `Err_Failed_Get_Volume` | 11011 | 获取输入或输出设备音量失败 |
| `Err_Failed_Set_volume` | 11012 | 设置输入或输出设备音量失败 |
| `Err_Playout_Mute_Failed` | 11013 | 音频输出设备静音失败 |
| `Err_Recorder_Mute_Failed` | 11014 | 音频输入设备静音失败 |
| `Err_Operator_Failed` | 11015 | 操作失败 |
| `Err_Room_Already_Joined` | 11016 | 已进入房间 |
| `Err_Network_Disconnect` | 11017 | 网络失去连接 |
| `Err_No_This_User` | 11018 | 无该用户 |
| `Err_No_This_User_Stream_Info` | 11019 | 无该用户流信息 |
| `Err_Device_Busy` | 11020 | 设备繁忙 |
| `Err_Device_Open_Failed` | 11021 | 设备打开失败 |
| `Err_No_This_Device` | 11022 | 不存在该设备 |
| `Err_Already_Published` | 11023 | 已发布 |
| `Err_Already_UnPublished` | 11024 | 已取消发布 |
| `Err_No_Publish_Record` | 11025 | 本地无发布记录 |
| `Err_Already_Subscribed` | 11026 | 已订阅或正在订阅 |
| `Err_Stream_ConnId_Empty` | 11027 | 订阅流时，流 ID 或 Conn ID 为空 |
| `Err_Already_UnSubscribed` | 11028 | 已取消订阅或正在取消订阅 |
| `Err_Decode_RoomToken_Failed` | 11029 | 解码 room token 失败 |
| `Err_Parse_Json_Failed` | 11030 | 解析 json 字符串失败 |
| `Err_Parse_Json_RoomName_Failed` | 11031 | 解析 json 字符串获取房间名失败 |
| `Err_Parse_Json_UserId_Failed` | 11032 | 解析 json 字符串获取用户 id 失败 |
| `Err_Parse_Json_AppId_Failed` | 11033 | 解析 json 字符串获取 appId 失败 |
| `Err_Request_Access_Token_Failed` | 11034 | 请求 Access Token 失败 |
| `Err_Parse_Access_Token_Failed` | 11035 | 解析 Access Token 失败 |
| `Err_Parse_Room_Server_Address_Failed` | 11036 | 解析房间服务地址失败 |
| `Err_Get_AccessToken_Timeout` | 11040 | 请求获取 Access Token 超时 |

<a id="7.4.10"></a>
### 7.4.10 统计信息的回调

```
// 通话质量统计信息回调，可通过 EnableStatisticCallback 进行设置
virtual void OnStatisticsUpdated(const StatisticsReport& statistics_) = 0;
```

设置统计信息回调的时间间隔

```   
virtual void    EnableStatisticCallback(int32_t period_second_ = 5) = 0;
```

<a id="7.5"></a>
## 7.5 `QNRTCVideo::QNRTCVideoListener` 代理回调

<a id="7.5.1"></a>
### 7.5.1 视频 YUV 数据回调

```
// @param raw_data_ 指向 YUV 数据的内存地址
// @param data_len_ raw_data_ 数据长度，单位：Byte
// @param video_type_ 导出的数据格式，一般为 kI420，即：YUV420P
// @param user_id_ 数据所属用户 ID
virtual void OnVideoFrame(const unsigned char* raw_data_, int data_len_, qiniu::VideoCaptureType video_type_, const std::string& user_id_) = 0;
```

本地预览、发布，订阅远端用户视频流的原始视频流数据的回调，一般为 YUV420P 格式；

<a id="7.5.2"></a>
### 7.5.2 视频设备插拔状态的回调

```
virtual void OnVideoDeviceStateChanged(VideoDeviceState device_state_, const std::string& device_name_) = 0;
```

视频设备仅监控当前已打开的设备；如果在用户正在预览、发布的过程中，插拔了此设备，则必须重新开启预览、发布，才能恢复正常的工作。

<a id="7.6"></a>
## 7.6 `QNRTCAudio::QNRTCAudioListener` 代理回调

<a id="7.6.1"></a>
### 7.6.1 连麦音频 PCM 数据回调

```
// @param audio_data_ 音频 PCM 数据
// @param bits_per_sample_ 采样位深
// @param sample_rate_ 采样率
// @param number_of_channels_ 声道数
// @param number_of_frames_ 采样点数
// @param user_id_ 数据所属用户 ID
virtual void OnAudioPCMFrame(const void* audio_data_, int bits_per_sample_,
                int sample_rate_, size_t number_of_channels_, 
                size_t number_of_frames_, const std::string& user_id_) = 0;
```

<a id="7.6.2"></a>
### 7.6.2 音频设备状态变化的回调

```
// @param device_state_ 音频设备状态
// @param device_guid_ 设备唯一标识：GUID
virtual void OnAudioDeviceStateChanged(
                AudioDeviceState device_state_, const std::string& device_guid_) = 0;
```

监控系统中所有的音频输入、输出设备的插拔状态；

<a id="7.7"></a>
## 7.7 其他功能

<a id="7.7.1"></a>
### 7.7.1 服务端合流

- 开启合流功能设置

```
// 设置服务器合流参数
// @param user_id_ 用户 ID
// @param pos_x_ 起始横轴坐标，原点坐标为左上角
// @param pos_y_ 起始纵轴坐标，原点坐标为左上角
// @param pos_z_ 窗口层次，0 表示最底层
// @param width_ 此用户媒体流合流后在画布中的宽度
// @param height_ 此用户媒体流合流后在画布中的高度
// @param is_visible_ 是否可见
// @return 0：成功，其它请参考错误码
virtual int32_t SetMergeStreamLayout(const std::string& user_id_, 
            int32_t pos_x_, int32_t pos_y_, int32_t pos_z_, 
            int32_t width_, int32_t height_, bool is_visible_) = 0;
```

- 关闭合流功能

```
virtual int32_t StopMergeStream() = 0;
```

建议一个房间内同时只有一个用户可以控制服务端合流的配置，当此用户离开房间时，务必调用 `StopMergeStream` 取消合流操作，以免旁路推流（RTMP 流）出现黑屏现象。

<a id="7.7.2"></a>
### 7.7.2 关于 SDK 的日志文件

开启 SDK 文件日志，不调用则不记录

```
// @param level_ 日志级别
// @param dir_name_ 日志文件存放目录
// @param file_name_ 日志文件名
static int QNRTCEngine::SetLogParams(QNLogLevel level_,
            const std::string& dir_name_, const std::string& file_name_);
```

<a id="8"></a>
# 8 历史记录
- 0.2.0
    - 基本的音视频通话功能
    - 内置音视频采集功能
    - 支持第三方虚拟摄像头采集
    - 支持音视频设备的切换
    - 支持踢人功能 
    - 支持静音功能 
    - 支持视频帧率配置 
    - 支持视频码率的配置 
    - 支持视频尺寸配置 
    - 支持网络自动重连 
    - 支持丰富的消息回调 
    - 支持纯音频互动 
    - 支持获取通话质量统计信息（帧率、码率、丢包率等）
    

<a id="9"></a>
# 9 FAQ

<a id="9.1"></a>
## 9.1 是否有服务端的 SDK 或者 demo 代码可以参考？

有的，请参考： [QNRTC-Server](https://github.com/pili-engineering/QNRTC-Server)