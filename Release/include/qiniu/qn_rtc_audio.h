#pragma once
#include "qn_rtc_engine.h"

namespace qiniu
{
    /** 
    * Audio device setting
    */
    typedef struct _TAudioDeviceSetting
    {
        enum WindowsDeviceType
        {
            wdt_DefaultCommunicationDevice = -1,
            wdt_DefaultDevice = -2
        };
        unsigned int        device_index;   //speaker or playout device index
        WindowsDeviceType   device_type = wdt_DefaultDevice;
    }AudioDeviceSetting;

    /**
    * Audio device state
    */
    enum AudioDeviceState
    {
        ads_active      = 0x00000001,     //new audio device is activated
        ads_disabled    = 0x00000002,     //the audio device is disabled
        ads_notpresent  = 0x00000004,     //the audio device is not present
        ads_unplugged   = 0x00000008,     //the audio device is unplugged
        ads_MASK_ALL    = 0x0000000F,     //includes all states: active, disabled, not present, and unplugged
    };

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
        unsigned int    device_index;
        AudioDeviceType device_type               = adt_invalid;
        char device_name[QNRTC_MAX_DEVICE_LENGHT] = { 0 };
        char device_id[QNRTC_MAX_DEVICE_LENGHT]   = { 0 };

        _TAudioDeviceInfo& operator = (const _TAudioDeviceInfo& info_) {
            device_index = info_.device_index;
            device_type  = info_.device_type;
            strncpy_s(device_name, info_.device_name, strlen(info_.device_name));
            strncpy_s(device_id, info_.device_id, strlen(info_.device_id));
            return *this;
        }
    }AudioDeviceInfo;
    
    typedef std::vector<AudioDeviceInfo> AudioDeviceInfoVec;

    /*!
    * \class QNRTCAudio
    *
    * \brief Audio operator interface
    */
    class QINIU_EXPORT_DLL QNRTCAudio
    {
    public:
        /*!
        * \class QNRTCAudioListener
        *
        * \brief Audio module related event monitoring interface, user needs to implemented
        *        this interface, and through QNRTCAudio::SetListener set to QNRTCAudio
        *        to receive the corresponding event notification
        */
        class QNRTCAudioListener
        {
        public:
            /** Audio frames callback
            * @param [in] audio_data_
            *        pointer to audio pcm data
            * @param [in] bits_per_sample_
            *        how many bits contained per sample
            * @param [in] sample_rate_
            *        audio sample rate, unit: HZ
            * @param [in] number_of_channels_
            *        audio channels number
            * @param [in] number_of_frames_
            *        how many audio samples are included in the data
            * @param [in] user_id_
            *        user id of this audio source, if empty, data after mixing
            */
            virtual void OnAudioPCMFrame(
                const unsigned char* audio_data_,
                int bits_per_sample_,
                int sample_rate_, 
                size_t number_of_channels_, 
                size_t number_of_frames_, 
                const std::string& user_id_
            ) = 0;

            /** Audio device plug-in event notification
            * @param [in] device_state_
            *        current audio device state
            * @param [in] device_guid_
            *        audio device guid 
            */
            virtual void OnAudioDeviceStateChanged(
                AudioDeviceState device_state_, 
                const std::string& device_guid_
            ) = 0;

        protected:
            virtual ~QNRTCAudioListener() {}
        };
    public:
        /** Get the number of audio devices of the specified device type
        * @param [in] device_type_
        *        audio device type: recording or playout
        * @return the number of designated device, if failed, return -1
        */
        virtual int GetAudioDeviceCount(AudioDeviceInfo::AudioDeviceType device_type_) = 0;
        
        /** Get specified playout device information
        * @param [in] device_type_
        *        audio device type: recording or playout
        * @param [in] device_index_
        *        local playout device index
        * @param [out] audio_info_
        *        if success, device information will be copied into this parameter
        * @return return 0 if success or an error code
        */
        virtual int GetAudioDeviceInfo(
            AudioDeviceInfo::AudioDeviceType device_type_,
            unsigned int device_index_,
            __out AudioDeviceInfo& audio_info_
            ) = 0;

        /** Set the audio module event listener interface, you must set it to nullptr
        *   before exiting the room
        * @param [in] listener_ptr_
        *        the instance pointer of the QNRTCAudioListener interface is implemented
        */
        virtual void SetAudioListener(QNRTCAudio::QNRTCAudioListener* listener_ptr_) = 0;

        /** Get the pointer of QNRTCAudioListener set by the user
        * @return The user sets an instance pointer to the QNRTCAudioListener interface
        */
        virtual QNRTCAudio::QNRTCAudioListener* GetAudioListener() = 0;

        /** Set which record device to use before publish audio stream,
        *   or else it will use default device
        * @param [in] recording_device_setting_
        *        set which record device to use
        * @return return 0 if success or an error code
        */
        virtual int SetRecordingDevice(AudioDeviceSetting recording_device_setting_) = 0;

        /** Set which playout device to use, or else it will use default device
        * @param [in] audio_device_setting_
        *        set which playout device to use
        * @return return 0 if success or an error code
        */
        virtual int SetPlayoutDevice(AudioDeviceSetting playout_device_setting_) = 0;
        
        /** Get system audio volume
        * @param [in] device_type_
        *        audio device type: recording or playout
        * @return return audio volume: 0 ~ 100
        */
        virtual int GetAudioVolume(AudioDeviceInfo::AudioDeviceType device_type_) = 0;

        /** Set system volume
        * @param [in] device_type_
        *        audio device type: recording or playout
        * @param [in] volume_
        *        audio volume : 0 ~ 100
        * @return return 0 if success or an error code
        */
        virtual int SetAudioVolume(AudioDeviceInfo::AudioDeviceType device_type_, int volume_) = 0;

        /** Set the volume of a specified user
        * @param [in] user_id_
        *        user id
        * @param [in] volume_
        *        audio volume, 0 ~ 100
        * @return return 0 if success or an error code
        */
        virtual int SetAudioVolume(const std::string& user_id_, double volume_) = 0;

        /** Set recording mute flag
        * @param [in] device_type_
        *        audio device type: recording or playout
        * @param [in] mute_flag_
        *        whether mute the audio device
        * @return return 0 if success or an error code
        */
        virtual int SetAudioMuteFlag(AudioDeviceInfo::AudioDeviceType device_type_, bool mute_flag_) = 0;

        /** Get recording mute flag
        * @param [in] device_type_
        *        audio device type: recording or playout
        * @return return device mute flag
        */
        virtual bool GetAudioMuteFlag(AudioDeviceInfo::AudioDeviceType device_type_) = 0;

        /** Enable or disable external data import feature
        * @param [in] enable_flag_
        *        true:enable, false:disable;
        * @return return 0 if success or an error code
        * @brief developer must call this method before InputAudioFrame
        */
        virtual int EnableAudioFakeInput(bool enable_flag_) = 0;

        /** Import audio frame data when external data import feature enabled
        * @param [in] audio_data_
        *        audio frame data pointer
        * @param [in] data_size_
        *        audio frame data size
        * @param [in] bits_per_sample_
        *        how many bits contained per sample, current only supported 16 bits per sample
        * @param [in] sample_rate_
        *        audio sample rate, unit: HZ
        * @param [in] number_of_channels_
        *        audio channels number, current only supported mono and stereo channels
        * @param [in] number_of_frames_
        *        how many audio samples are included in the data.
        * @return return 0 if success or an error code
        * @brief developer must call EnableAudioFakeInput(true) first
        */
        virtual int InputAudioFrame(
            const unsigned char* audio_data_,
            unsigned int data_size_,
            unsigned int bits_per_sample_,
            unsigned int sample_rate_,
            unsigned int number_of_channels_,
            unsigned int number_of_frames_
        ) = 0;

        /** Whether enabled audio fake input future
        * @return true: enable, false: disable
        */
        virtual bool IsEnableAudioFakeInput() = 0;

        /** Get audio level 
        * @param [in] user_id_
        *        who's audio level
        * @return 0 ~ 100，audio level >= 0 if success, else return -1
        */
        virtual unsigned int GetAudioLevel(const std::string& user_id_) = 0;

        /** Enable or disable desktop audio capture and mixed with microphone
        *   when publish audio stream. desktop audio same as system audio
        * @param [in] enable_
        *        enable or disable, internal default is disabled
        * @param [in] volume_scale_ratio_
        *        desktop audio volume scale ratio, default 1.0 does not adjust volume
        * @return return 0 if success or an error code
        */
        virtual int MixDesktopAudio(bool enable_, float volume_scale_ratio_ = 1.0f) = 0;
        
    protected:
        virtual ~QNRTCAudio() {}
    };
}
