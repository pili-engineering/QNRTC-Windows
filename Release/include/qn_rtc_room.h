/*!
 * \file qn_rtc_room.h
 *
 * \author qiniu
 */
#pragma once
#include "qn_rtc_engine.h"

namespace qiniu
{
    /**
    * Room state
    */
    enum RoomState
    {
        rs_idle,            // Idle, not connected
        rs_connecting,      // Connecting
        rs_connected,       // Connected
        rs_reconnecting,    // disconnected and reconnecting
    };

    /**
    * User info, include the id, and published streams id list, and user defined information
    */
    typedef struct _TUserDataInfo
    {
        std::string user_id;                    // The only sign in the room
        bool        audio_published = false;    // Is audio has been published
        bool        video_published = false;    // Is video has been published
        bool        audio_mute      = false;    // Is the audio mute
        bool        video_mute      = false;    // Is the video mute
    }UserDataInfo;

    typedef struct _TStatisticsReport
    {
        std::string user_id;
        // audio parameters      
        int         audio_bitrate          = 0;     // audio bitrate,unit:bps
        float       audio_packet_lost_rate = 0.0f;  // audio packet lost rate
        // video parameters
        int         video_width            = 0;     // video width
        int         video_height           = 0;     // video height
        int         video_frame_rate       = 0;     // video frames per second
        int         video_bitrate          = 0;     // video bitrate,unit:bps
        float       video_packet_lost_rate = 0.0f;  // video packet lost rate
    } StatisticsReport;

    typedef std::vector<UserDataInfo>   UserDataInfoVec;

    class QNRTCAudio;
    class QNRTCVideo;

    /*!
    * \class QNRTCRoom
    *
    * \brief Used for joining, leaving, publishing, subscribing, kicking out players, etc
    */
    class QINIU_EXPORT_DLL QNRTCRoom
    {
    public:
        /*!
         * \class QNRTCRoomListener
         *
         * \brief the event call back interface
         */
        class QNRTCRoomListener
        {
        public:
            /** Room state change asynchronous notification
            * @param [in] status_ 
            *        RoomState enumeration value
            */
            virtual void OnStateChanged(RoomState status_) = 0;

            /** Room login status asynchronous notification
            * @param [in] user_data_vec_ 
            *        the existing users in the room and the information they have published
            */
            virtual void OnJoinResult(
                int error_code_, 
                const std::string& error_str_,
                const UserDataInfoVec& user_data_vec_
            ) = 0;

            /** Leave room asynchronous notification, may be kicked by others or room closed or whatever
            * @param [in] error_code_ 
            *        error code
            * @param [in] error_str_ 
            *        leave the reason
            * @param [in] kicker_user_id_
            *        If it is kicked out of the room, this parameter indicates the id of the kicker
            */
            virtual void OnLeave(
                int error_code_, 
                const std::string& error_str_, 
                const std::string& kicker_user_id_
            ) = 0;

            /** Other users joined this room
            * @param [in] user_id_ 
            *        join the user id joined of this room
            * @param [in] user_data_ 
            *        user-defined data, may be empty
            */
            virtual void OnRemoteUserJoin(
                const std::string& user_id_, 
                const std::string& user_data_
            ) = 0;

            /** Other users leaved this room
            * @param [in] user_id_
            *        leave the user id of this room
            * @param [in] error_code_
            *        error code
            */
            virtual void OnRemoteUserLeave(
                const std::string& user_id_, 
                int error_code_
            ) = 0;

            /** Other users in the room has published media stream
            * @param [in] user_id_
            *        the user id that published the stream
            * @param [in] enable_audio_
            *        does it contains audio stream
            * @param [in] enable_video_
            *        does it contains video stream
            */
            virtual void OnRemotePublish(
                const std::string& user_id_, 
                bool enable_audio_, 
                bool enable_video_
            ) = 0;

            /** Other users in the room canceled of publish media stream
            * @param [in] user_id_
            *        the user id that canceled of published the stream
            */
            virtual void OnRemoteUnPublish(const std::string& user_id_) = 0;

            /** The response of publish stream
            * @param [in] error_code_ 
            *        error code
            * @param [in] error_str_
            *        error info
            */
            virtual void OnLocalPublishResult(int error_code_, const std::string& error_str_) = 0;

            /** The response of subscribe stream
            * @param [in] user_id_
            *        the user id that is subscribed
            * @param [in] error_code_
            *        error code
            * @param [in] error_str_
            *        error info
            */
            virtual void OnSubscribeResult(
                const std::string& user_id_, 
                int error_code_, 
                const std::string& error_str_
            ) = 0;

            /** The response of kickout user
            * @param [in] user_id_
            *        the user id that is kickouted
            * @param [in] error_code_
            *        error code
            * @param [in] error_str_
            *        error info
            */
            virtual void OnKickoutResult(
                const std::string& user_id_,
                int error_code_, 
                const std::string& error_str_
            ) = 0;

            /** Notification of remote user mute or unmute audio or video stream.
            * @param [in] user_id_
            *        the user id that who operated
            * @param [in] mute_audio_
            *        the mute state of the audio stream
            * @param [in] mute_video_
            *        the mute state of the video stream
            */
            virtual void OnRemoteStreamMute(
                const std::string& user_id_, 
                bool mute_audio_, 
                bool mute_video_
            ) = 0;

            /** This callback is triggered when an error occurs
            * @param [in] error_code_
            *        error code
            * @param [in] error_str_
            *        error info
            */
            virtual void OnError(int error_code_, const std::string& error_str_) = 0;

            /** Callback fired once statistics report arrived
            * @param [in] statistics_
            *        statistics info
            */
            virtual void OnStatisticsUpdated(const StatisticsReport& statistics_) = 0;

        protected:
            virtual ~QNRTCRoomListener() {}
        };

    public:
        /** Get the global unique instance pointer of the QNRTCRoom interface
        * @return returns a pointer to the QNRTCRoom, or null
        */
        static QNRTCRoom* ObtainRoomInterface();

        /** Release QNRTCRoom instance
        */
        virtual void Release() = 0;

        /**
        * Get the pointer to QNRTCAudio
        * @return pointer to QNRTCAudio
        */
        virtual QNRTCAudio* ObtainAudioInterface() = 0;

        /**
        * Get the pointer to QNRTCVideo
        * @return pointer to QNRTCVideo
        */
        virtual QNRTCVideo* ObtainVideoInterface() = 0;

        /** Set the callback interface pointer for listening room message, 
        *   and set it to nullptr before exiting the room
        * @param [in] listener_ptr_ 
        *        pointer of QNRTCRoomListener
        */
        virtual void SetRoomListener(QNRTCRoomListener* listener_ptr_) = 0;

        /** Gets the current room event listening to the callback interface pointer
        * @return returns a pointer to the QNRTCRoomListener, or null
        */
        virtual QNRTCRoomListener* GetRoomListener() = 0;

        /** To join the room, this is asynchronous method, the result by 
        *   QNRTCRoomListener::OnJoinResult to noticy
        * @param [in] room_token_
        *        login authentication string
        * @return return 0 if success or an error code
        */
        virtual int JoinRoom(const std::string& room_token_) = 0;

        /** Leave the room
        * @return return 0 if success or an error code
        */
        virtual int LeaveRoom() = 0;

        /** Determine if you have successfully entered the room
        * @return true or false
        */
        virtual bool IsJoined() = 0;
        
        /** Get the room name if you has created the room
        * @return room name
        */
        virtual const std::string& GetRoomName() = 0;

        /** Get local user id if you has joined the room
        * @return user id
        */
        virtual const std::string& GetUserId() = 0;

        /** Publish local media streams
        * @param [in] enable_audio_
        *        whether to publish audio stream
        * @param [in] enable_video_ 
        *        whether to publish video stream
        * @return return 0 if success or an error code
        */
        virtual int Publish(bool enable_audio_ = true, bool enable_video_ = true) = 0;

        /** Cancel publish local media streams
        * @return return 0 if success or an error code
        */
        virtual int UnPublish() = 0;

        /** Subscribe to the specified user's media stream
        * @param [in] user_id_
        *        the user id that will be subscribed
        * @param [in] render_hwnd_
        *        if enable_video_ is true, the window handle used to render the video image, 
        *        if it is empty, does not render video
        * @return return 0 if success or an error code
        */
        virtual int Subscribe(const std::string& user_id_, void* render_hwnd_) = 0;

        /** Cancel subscribe to the specified user's media stream
        * @param [in] user_id_
        *        the user id that will be cancel subscribed
        * @return return 0 if success or an error code
        */
        virtual int UnSubscribe(const std::string& user_id_) = 0;

        /** Kick out user
        * @param [in] user_id_
        *        the user id that will be kicked, you must have administrator privileges
        * @return return 0 if success or an error code
        */
        virtual int KickoutUser(const std::string& user_id_) = 0;

        /** Mute audio stream
        * @param [in] mute_flag_
        *        the audio track will produce silence, can be disabled and re-enabled
        * @return return 0 if success or an error code
        */
        virtual int MuteAudio(bool mute_flag_) = 0;

        /** Mute video stream
        * @param [in] mute_flag_
        *         the video track black frames, can be disabled and re-enabled
        * @return return 0 if success or an error code
        */
        virtual int MuteVideo(bool mute_flag_) = 0;

        /** Enable or disable statistics info, will got on QNRTCRoomListener::OnStatisticsUpdated
        * @param [in]period_second_ 
        *        statistics period, unit: second, if 0 will no statistics info
        */
        virtual void EnableStatisticCallback(int period_second_ = 5) = 0;
        
        /** Configuring server-side stream merge parameters
        * @param [in] user_id_ 
        *         user id
        * @param [in] pos_x_ 
        *         Starting axis coordinate, origin coordinate is upper-left corner
        * @param [in] pos_y_
        *         Start the longitudinal axis coordinates, the origin coordinates are in the upper left corner
        * @param [in] pos_z_
        *         Window level, 0 denotes the lowest level
        * @param [in] width_
        *         The width in the canvas after this user's stream merged
        * @param [in] height_ 
        *         The height in the canvas after this user's stream merged
        * @param [in] hide_video_ 
        *         Is video visibility
        * @param [in] mute_audio_
        *         Is audio mute
        * @return return 0 if success or an error code
        */
        virtual int SetMergeStreamLayout(
            const std::string& user_id_,
            int pos_x_, 
            int pos_y_, 
            int pos_z_, 
            int width_, 
            int height_, 
            bool hide_video_,
            bool mute_audio_
        ) = 0;

        /** Stop merge streams, administrator must call this interface before leave the room
        * @return return 0 if success or an error code
        */
        virtual int StopMergeStream() = 0;

    protected:
        virtual ~QNRTCRoom() {}
    };
}

