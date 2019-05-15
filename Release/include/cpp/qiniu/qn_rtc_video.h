#pragma once
#include "qn_rtc_engine.h"

namespace qiniu
{
    /** 
    * The list of camera output formats
    */
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
    };

    /**
    * capture capability of camera
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
    * camera information and all the capability of it supports
    */
    typedef struct _TCameraDeviceInfo
    {
        std::string         device_id;
        std::string         device_name;
        CameraCapabilityVec capability_vec;
    } CameraDeviceInfo;

    typedef std::vector<CameraDeviceInfo> CameraDeviceInfoVec;

    /** 
    * Set this to QNRTCVideo::SetCameraParams before publish video stream
    */
    typedef struct _TCameraSetting
    {
        std::string device_id;              //camera device id, can't be empty
        std::string device_name;            //camera device name, can't be empty
        int         width       = 640;      //camera capture and encode width
        int         height      = 480;      //camera capture and encode height
        int         max_fps     = 30;       //video frames per second
        int         bitrate     = 300000;   //video encode bitrate, unit:bps
        void*       render_hwnd = nullptr;  //video render window hwnd,MFC:HWND; QT:winId
    } CameraSetting;

    /**
    * Video device state
    */
    enum VideoDeviceState
    {
        vds_active  = 0x00000001,           //new video device is activated
        vds_lost    = 0x00000002,           //the video device is lost
    };

    /** 
    * enum for clockwise rotation.
    */
    enum VideoRotation
    {
        kVideoRotation_0   = 0,
        kVideoRotation_90  = 90,
        kVideoRotation_180 = 180,
        kVideoRotation_270 = 270
    };

    /**
    * Screen windows info
    */
    typedef struct _TScreenWindowInfo
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
        std::string  title;     // windows title
        bool         is_screen; // is screen, true:sreen; false:window
    } ScreenWindowInfo;

    /*!
     * \class QNRTCVideo
     *
     * \brief Video operator interface
     */
#ifdef _WIN32
    class QINIU_EXPORT_DLL QNRTCVideo
#else
    class QNRTCVideo
#endif // WIN32
    {
    public:
        /*!
        * \class QNRTCVideoListener
        *
        * \brief Video module related event monitoring interface, user needs to implemented 
        *        this interface, and through QNRTCVideo::SetVideoListener set to QNRTCVideo 
        *        to receive the corresponding event notification
        */
        class QNRTCVideoListener
        {
        public:
            /** Video frames callback
            * @param [in] raw_data_ 
            *        pointer to video frame data
            * @param [in] data_len_
            *        video frame data size
            * @param [in] width_
            *        video frame width
            * @param [in] height_
            *        video frame height
            * @param [in] video_type_
            *        video frame raw data format
            * @param [in] user_id_
            *        user id of this video source 
            */
            virtual void OnVideoFrame(
                const unsigned char* raw_data_, 
                int data_len_, 
                int width_, 
                int height_, 
                qiniu::VideoCaptureType video_type_, 
                const std::string& user_id_
            ) = 0;

            /** Video frames callback when preview local camera
            * @param [in] raw_data_
            *        pointer to video frame data
            * @param [in] data_len_
            *        video frame data size
            * @param [in] width_
            *        video frame width
            * @param [in] height_
            *        video frame height
            * @param [in] video_type_
            *        video frame raw data format
            */
            virtual void OnVideoFramePreview(
                const unsigned char* raw_data_,
                int data_len_,
                int width_,
                int height_,
                qiniu::VideoCaptureType video_type_
            ) {};

            /** Video device plug-in event notification,only for devices in use
            * @param [in] device_state_
            *        current video device state
            * @param [in] device_name_
            *        video device name
            */
            virtual void OnVideoDeviceStateChanged(
                VideoDeviceState device_state_, 
                const std::string& device_name_
            ) = 0;

        protected:
            virtual ~QNRTCVideoListener() {}
        };
    public:
        /** Get the number of local cameras
        * @return the number of local cameras
        */
        virtual int GetCameraCount() = 0;

        /** Get specified camera devices information
        * @param [in] device_index_
        *        output local camera's device informations
        * @return return an object of CameraDeviceInfo
        */
        virtual const CameraDeviceInfo& GetCameraInfo(unsigned int device_index_) = 0;

        /** Set the video module event listener interface, you must set it to nullptr 
        *   before exiting the room
        * @param [in] listener_ptr_ 
        *        the instance pointer of the QNRTCVideoListener interface is implemented
        */
        virtual void SetVideoListener(QNRTCVideo::QNRTCVideoListener* listener_ptr_) = 0;

        /** Get the pointer of QNRTCVideoListener set by the user
        * @return The user sets an instance pointer to the QNRTCVideoListener interface
        */
        virtual QNRTCVideo::QNRTCVideoListener* GetVideoListener() = 0;

        /** Set camera params before publish video stream, or else it will use default params
        * @param [in out] camera_setting_
        *        set the camera params before publish, if the camera not support those params, 
        *        it will output the most similar params and publish
        * @return return 0 if success or an error code
        */
        virtual int SetCameraParams(CameraSetting& camera_setting_) = 0;

        /** Preview the camera by specified params
        *   video frames data callback through QNRTCVideoListener::OnVideoFramePreview
        * @param [in out] camera_setting_
        *        set the camera params previewed, if the camera not support those params,
        *        it will output the most similar params and publish
        * @return return 0 if success or an error code
        */
        virtual int PreviewCamera(CameraSetting& camera_setting_) = 0;

        /** Cancel preview camera
        * @return return 0 if success or an error code
        */
        virtual int UnPreviewCamera() = 0;

        /** Enable or disable external data import feature
        * @param [in] enable_flag_ 
        *        true:enable, false:disable;
        * @return return 0 if success or an error code
        * @brief developer must call this method before InputVideoFrame
        */
        virtual int EnableVideoFakeCamera(bool enable_flag_) = 0;

        /** Import video frame data when external data import feature enabled
        * @param [in] data_
        *        video frame data pointer
        * @param [in] data_size_
        *        video frame data size
        * @param [in] width_
        *        picture width
        * @param [in] height_
        *        picture height
        * @param [in] timestamp_us_
        *        video frame timestamp, unit:microsecond
        * @param [in] raw_type_
        *        video frame type, currently supports three formats: kI420 kYUY2 kRGB24
        * @param [in] rotation_
        *        clockwise rotation, default is kVideoRotation_0
        * @param [in] mirror_flag_
        *        whether the input video frame need mirror
        * @return return 0 if success or an error code
        * @brief developer must call EnableVideoFakeCamera(true) first
        */
        virtual int InputVideoFrame(
            const unsigned char* data_, 
            const unsigned int& data_size_,
            const unsigned int& width_,
            const unsigned int& height_,
            const unsigned long long& timestamp_us_,
            qiniu::VideoCaptureType raw_type_,
            qiniu::VideoRotation rotation_ = kVideoRotation_0,
            bool mirror_flag_ = false
            ) = 0;

        /** Whether enabled video fake camera future
        * @return true: enable, false: disable
        */
        virtual bool IsEnableVideoFakeCamera() = 0;

        /** Get the number of screen windows
        * @return the number of local screen windows
        * @brief not thread safe
        */
        virtual int GetScreenWindowCount() = 0;

        /** Get specified screen window's information
        * @param [in] screen window's index
        * @return return an object of ScreenWindowInfo
        * @brief not thread safe
        */
        virtual ScreenWindowInfo& GetScreenWindowInfo(const int& index_) const = 0;

        /** Enable or disable screen share,if source_id_ >= 0 ,enable; if source_id_ < 0, disable;
        * @param [in] source_id_
        *        screen window source id, obtain by method:GetScreenWindowInfo
        * @param [in] allow_directx_capturer_
        *        allowing directx based capturer or not, this capturer works on windows 7
        *        with platform update / windows 8 or upper.
        * @return return 0 if success, or an error code
        */
        virtual int EnableAndSetScreenSourceId(const int& source_id_, bool allow_directx_capturer_ = false) = 0;

        /** Get screen source id, if < 0, not enable
        * @return enable and set screen source's id, if not enable, it will return -1
        */
        virtual int GetScreenSourceId() = 0;

        /** Preview specify screen or window source id
        *   video frames data callback through QNRTCVideoListener::OnVideoFramePreview
        * @param [in] source_id_
        *        screen window source id, obtain by method:GetScreenWindowInfo
        * @param [in] render_hwnd_
        *        video render window hwnd,MFC:HWND; QT:winId
        * @param [in] allow_directx_capturer_
        *        allowing directx based capturer or not, this capturer works on windows 7
        *        with platform update / windows 8 or upper.
        * @return return 0 if success, or an error code
        */
        virtual int PreviewScreenSource(
            const int& source_id_, 
            void* render_hwnd_, 
            bool allow_directx_capturer_ = false
        ) = 0;

        /** Cancel preview screen source
        * @return return 0 if success or an error code
        */
        virtual int UnPreviewScreenSource() = 0;

        /** Mirroring the specified user, mirror = left and right rotation
        * @param [in] user_id_
        *        specified user id
        * @param [in] mirror_flag_
        *        mirror flag, true or false
        * @return return 0 if success, or an error code
        */
        virtual int SetMirrorWhenDisplay(const std::string& user_id_, bool mirror_flag_) = 0;

        /** Process video raw picture, crop or mirror; Current only support format:kI420
        * @param [in] src_data_
        *        pointer to source raw data
        * @param [in] src_width_
        *        source picture width
        * @param [in] src_height_
        *        source picture height
        * @param [in] src_data_size_
        *        source data size
        * @param [in] picture_fmt_
        *        source picture format, current only supported kI420
        * @param [in] mirror_flag_
        *        whether mirror source picture flag
        * @param [in] origin_x_
        *        crop x pos from top left
        * @param [in] origin_y_
        *        crop y pos from top left
        * @param [in] dest_width_
        *        crop width from source picture
        * @param [in] dest_height_
        *        crop height from source picture
        * @param [in] dest_data_
        *        dest data buffer pointer
        * @param [in] max_dest_data_size_
        *        dest_data_'s buffer size
        * @param [out] dest_data_size_
        *        memory dest_data_'s max size
        * @return return 0 if success, or an error code
        */
        virtual int CropRawPicture(
            unsigned char* src_data_,
            const unsigned int& src_width_,
            const unsigned int& src_height_,
            const unsigned int& src_data_size_,
            qiniu::VideoCaptureType picture_fmt_,
            bool mirror_flag_,
            const int& origin_x_,
            const int& origin_y_,
            const int& dest_width_,
            const int& dest_height_,
            unsigned char* dest_data_,
            const unsigned int& max_dest_data_size_,
            unsigned int& dest_data_size_
            ) = 0;

        /** Convert raw video picture to I420 format, support format: kRGB24,kABGR,kARGB,kBGRA
        * @param [in] src_data_
        *        pointer to source raw data
        * @param [in] src_width_
        *        source picture width
        * @param [in] src_height_
        *        source picture height
        * @param [in] src_data_size_
        *        source data size
        * @param [in] src_picture_fmt_
        *        source picture format
        * @param [in] dest_data_
        *        dest data buffer pointer
        * @param [in] max_dest_data_size_
        *        dest_data_'s buffer size
        * @param [out] dest_data_size_
        *        memory dest_data_'s max size
        * @return return 0 if success, or an error code
        */
        virtual int ConvertToI420(
            unsigned char* src_data_,
            const unsigned int& src_width_,
            const unsigned int& src_height_,
            const unsigned int& src_data_size_,
            qiniu::VideoCaptureType src_picture_fmt_,
            unsigned char* dest_data_,
            const unsigned int& max_dest_data_size_,
            unsigned int& dest_data_size_
        ) = 0;

        /** Enable or disable video rendering, default d3d render is enabled.
        * @param [in] enable_d3d_
        *        true:use d3d render; false:use gdi render;
        */
        virtual void EnableD3dRender(bool enable_d3d_ = true) = 0;
        
    protected:
        virtual ~QNRTCVideo() {}
    };
}

