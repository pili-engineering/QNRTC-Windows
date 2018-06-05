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
    }CameraDeviceInfo;

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
        int         max_fps     = 15;       //video frames per second
        int         bitrate     = 300000;   //video encode bitrate, unit:bps
        void*       render_hwnd = nullptr;  //video render window hwnd,MFC:HWND; QT:winId
    }CameraSetting;

    /**
    * Video device state
    */
    enum VideoDeviceState
    {
        vds_active  = 0x00000001,           //new video device is activated
        vds_lost    = 0x00000002,           //the video device is lost
    };


    /*!
     * \class QNRTCVideo
     *
     * \brief Video operator interface
     */
    class QINIU_EXPORT_DLL QNRTCVideo
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
            * @param [in] video_type_
            *        video frame raw data format
            * @param [in] stream_id_ 
            *        stream id of this video source
            * @param [in] user_id_
            *        user id of this video source 
            */
            virtual void OnVideoFrame(const unsigned char* raw_data_, int data_len_, 
                qiniu::VideoCaptureType video_type_, const std::string& user_id_) = 0;

            /** Video device plug-in event notification,only for devices in use
            * @param [in] device_state_
            *        current video device state
            * @param [in] device_name_
            *        video device name
            */
            virtual void OnVideoDeviceStateChanged(
                VideoDeviceState device_state_, const std::string& device_name_) = 0;

        protected:
            virtual ~QNRTCVideoListener() {}
        };
    public:
        /** Get the number of local cameras
        * @return the number of local cameras
        */
        virtual int32_t GetCameraCount() = 0;

        /** Get specified camera devices information
        * @param [in] device_index_
        *        output local camera's device informations
        * @return return an object of CameraDeviceInfo
        */
        virtual const CameraDeviceInfo& GetCameraInfo(uint32_t device_index_) = 0;

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
        
    protected:
        virtual ~QNRTCVideo() {}
    };
}

