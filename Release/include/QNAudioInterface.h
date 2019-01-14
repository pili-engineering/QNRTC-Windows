#pragma once
#include "QNRoomInterface.h"

namespace qiniu_v2 {

    // 音频设备配置，用于指定设备进行录制或者播放 
    struct AudioDeviceSetting
    {
        enum WindowsDeviceType
        {
            wdt_DefaultCommunicationDevice = -1, // 通信设备 
            wdt_DefaultDevice              = -2  // 普通设备 
        };
        int                 device_index;                    // 设备编号；注意：0 不代表默认设备 
        WindowsDeviceType   device_type = wdt_DefaultDevice; // 设备类型，建议默认为普通设备 
    };

    // 音频设备当前状态，用与设备插拔的检测和通知 
    enum AudioDeviceState
    {
        ads_active     = 0x00000001,     // 新的可用设备 
        ads_disabled   = 0x00000002,     // 设备失效 
        ads_notpresent = 0x00000004,     // 设备不存在 
        ads_unplugged  = 0x00000008,     // 设备被拔出 
        ads_MASK_ALL   = 0x0000000F,
    };

    // 音频设备信息 
    struct AudioDeviceInfo
    {
        enum AudioDeviceType
        {
            adt_invalid = -1,
            adt_record,
            adt_playout,
        };
        int device_index                          = 0;
        bool is_default                           = false;
        AudioDeviceType device_type               = adt_invalid;
        char device_name[QNRTC_MAX_DEVICE_LENGHT] = { 0 };
        char device_id[QNRTC_MAX_DEVICE_LENGHT]   = { 0 };

        AudioDeviceInfo& operator = (const AudioDeviceInfo& info_)
        {
            device_index = info_.device_index;
            device_type  = info_.device_type;
            is_default   = info_.is_default;
            strncpy_s(device_name, info_.device_name, strlen(info_.device_name));
            strncpy_s(device_id, info_.device_id, strlen(info_.device_id));
            return *this;
        }
    };

    typedef std::list<AudioDeviceInfo> AudioDeviceInfoList;

    // 音频模块功能接口，开发者可以进行音频相关控制
    // 由 QNRoomInterface::ObtainAudioInterface 获取
    // QNRoomInterface::DestroyRoomInterface 后自动释放 
    class QINIU_EXPORT_DLL QNAudioInterface
    {
    public:
        // 音频相关事件监听 
        class QNAudioListener
        {
        public:
            // 本地和远端 PCM 数据回调
            // @param audio_data_ PCM 数据内存指针
            // @param bits_per_sample_ 位宽，即每个采样点占用位数
            // @param sample_rate_ 采样率
            // @param number_of_channels_ 声道数
            // @param number_of_frames_ 此次回调内包含了多了多少采样点数
            // @param user_id_ PCM 数据所属 User Id，可以是远端用户也可以是本地用户 
            virtual void OnAudioPCMFrame(
                const unsigned char* audio_data_,
                int bits_per_sample_,
                int sample_rate_,
                size_t number_of_channels_,
                size_t number_of_frames_,
                const std::string& user_id_
            ) = 0;

            // 音频设备插拔事件通知
            // @param device_state_ 最新音频设备状态
            // @param 设备 GUID
            virtual void OnAudioDeviceStateChanged(
                AudioDeviceState device_state_,
                const std::string& device_guid_
            ) = 0;

        protected:
            virtual ~QNAudioListener() {}
        };
    public:
        // 用于获取指定类型的音频设备数量，adt_record or adt_playout
        // @param device_type_ 设备类型，录制 or 播放
        // @return 指定类型的设备数量
        // 返回非 0 时，可通过 GetAudioDeviceInfo 获取指定 index 的设备信息 
        virtual int GetAudioDeviceCount(AudioDeviceInfo::AudioDeviceType device_type_) = 0;

        // 获取指定 index 设备信息
        // @param device_type_ 设备类型
        // @param index_ 设备 index
        // @param audio_info_ 输出参数，用于返回指定的音频设备信息
        // @return 0:成功 
        virtual int GetAudioDeviceInfo(
            AudioDeviceInfo::AudioDeviceType device_type_,
            unsigned int index_,
            __out AudioDeviceInfo& audio_info_
        ) = 0;

        // 设置音频事件监控回调接口
        // @param listener_ptr_ QNAudioListener 派生类实例指针 
        virtual void SetAudioListener(QNAudioInterface::QNAudioListener* listener_ptr_) = 0;

        // 获取 SetAudioListener 配置的 QNAudioListener 派生类实例指针 
        virtual QNAudioInterface::QNAudioListener* GetAudioListener() = 0;

        // 设置连麦使用的音频录制设备，不调用则使用系统默认录制设备
        // 连麦过程中设置无效，需在发布音频 Track 前调用
        // @param recording_device_setting_ AudioDeviceSetting 结构体
        // @return 成功返回 0，其它请参考错误码列表 
        virtual int SetRecordingDevice(AudioDeviceSetting recording_device_setting_) = 0;

        // 设置连麦使用的音频播放设备，不调用则使用系统默认播放设备
        // 连麦过程中设置无效，需在订阅任何音频 Track 前调用
        // @param playout_device_setting_ AudioDeviceSetting 结构体
        // @return 成功返回 0，其它请参考错误码列表 
        virtual int SetPlayoutDevice(AudioDeviceSetting playout_device_setting_) = 0;

        // 获取指定设备类型的系统音量，adt_record or adt_playout         
        virtual int GetAudioVolume(AudioDeviceInfo::AudioDeviceType device_type_) = 0;

        // 设置指定设备类型的系统音量，adt_record or adt_playout
        // @param device_type_ adt_record or adt_playout
        // @param volume_ 0 ~ 100
        // @return 成功返回 0，否则返回 SDK 错误码 
        virtual int SetAudioVolume(AudioDeviceInfo::AudioDeviceType device_type_, int volume_) = 0;
        
        // 设置指定用户的音量（不改变系统设备的音量）
        // @param track_id_ User Id; 由于 SDK 每个用户只能发布一路音频，所以音量控制由 User Id 指定
        // @param volume_scale_ratio_ 音量缩放倍数，建议值为 0.0f ~ 2.0f
        // @return 成功返回 0，其它请参考错误码列表 
        virtual int SetAudioVolume(const std::string& user_id_, float volume_scale_ratio_) = 0;

        // 静音或取消静音指定类型的设备
        // @return 成功返回 0，其它请参考错误码列表 
        virtual int SetAudioMuteFlag(AudioDeviceInfo::AudioDeviceType device_type_, bool mute_flag_) = 0;

        // 获取指定类型的设备是否静音
        // @return true：已静音；false：未静音 
        virtual bool GetAudioMuteFlag(AudioDeviceInfo::AudioDeviceType device_type_) = 0;

        // 当使用外部数据导入时，首先通过此接口激活外部 PCM 数据导入开关
        // @param enable_flag_ 激活开关
        // @return 成功返回 0，其它请参考错误码列表 
        virtual int EnableAudioFakeInput(bool enable_flag_) = 0;

        // 实时导入外部音频 PCM 数据，仅当 EnableAudioFakeInput(true) 时有效
        // @param audio_data_ PCM 数据内存地址
        // @param data_size_ 数据大小
        // @param bits_per_sample_ 位宽，即每采样点所占位数
        // @param sample_rate_ 采样率
        // @param number_of_channels_ 声道数
        // @param number_of_frames_ 每次导入数据所包含的采样点数
        // @return 成功返回 0，其它请参考错误码列表 
        virtual int InputAudioFrame(
            const unsigned char* audio_data_,
            unsigned int data_size_,
            unsigned int bits_per_sample_,
            unsigned int sample_rate_,
            unsigned int number_of_channels_,
            unsigned int number_of_frames_
        ) = 0;

        // 是否激活了外部音频数据导入
        // @return true or false
        virtual bool IsEnableAudioFakeInput() = 0;
        
        // 激活监听功能：混合本地麦克风和系统声卡的声音，此方法非线程安全接口
        // @param enable_ 是否激活监听功能开关
        // @param volume_scale_ratio_ 系统声音缩放倍数，以方便控制系统音量大小
        // @return 成功返回 0，其它请参考错误码列表 
        virtual int MixDesktopAudio(bool enable_, float volume_scale_ratio_ = 1.0f) = 0;

    protected:
        virtual ~QNAudioInterface() {}
    };
}