#pragma once
#include "QNTrackInterface.h"

namespace qiniu {
    class QNRTCClient;
    struct QNCustomAudioTrackConfig
    {
        int bitrate;             // 码率
        string tag;              // 自定义标签
    };

    struct QNMicrophoneAudioTrackConfig
    {
        int bitrate;            // 码率
        string tag;             // 自定义标签
    };

    struct QNCameraVideoTrackConfig
    {
        int width;              // 宽
        int height;             // 高
        int frame_rate;         // 帧率
        int bitrate;            // 码率
        std::string device_id;  // 设备 id
        std::string tag;        // 自定义标签
        void*  render_hwnd;     // 本地渲染窗口句柄
        bool multi_profile_enabled;  //是否开启多流
    };

    struct QNScreenVideoTrackConfig
    {
        int frame_rate;         // 帧率
        int bitrate;            // 码率
        std::string device_id;  // 窗口 id
        std::string tag;        // 自定义标签
        void*  render_hwnd;     // 本地渲染窗口句柄
    };

    struct QNCustomVideoTrackConfig
    {
        int width;              // 宽
        int height;             // 高
        int frame_rate;         // 帧率
        int bitrate;            // 码率
        std::string tag;        // 自定义标签
        void*  render_hwnd;     // 本地渲染窗口句柄
    };

    class QINIU_EXPORT_DLL QNRTC
    {
    public:
        /**
         * 获取版本号
         *
         * @param ver 版本号
         */
        static void GetVersion(std::string& ver);

        /**
         * 配置 SDK 日志信息，如果名字相同，则进程每次启动会覆盖原来的日志文件
         * 日志文件大小上限为 100 M，SDK 内部写入时会自动检测
         *
         * @param level_ 日志等级，等级越低日志越多
         * @param dirName 日志存储目录，如果不存在则自动创建
         * @param fileName 日志文件名
         */
        static int SetLogFileEnabled(QNLogLevel level, const std::string& dirName, const std::string& fileName);

        /**
         * 创建 QNRTCClient 实例
         */
        static QNRTCClient* CreateClient();

        /**
         * 释放 QNRTCClient 实例
         * 
         * @param ptr QNRTCClient实例指针
         */
        static void DestroyRtcClient(QNRTCClient* ptr);

        /**
         * 创建 QNMicrophoneAudioTrack 实例
         *
         * @param config QNMicrophoneAudioTrackConfig 配置
         */
        static QNMicrophoneAudioTrack* CreateMicrophoneAudioTrack(QNMicrophoneAudioTrackConfig& config);

        /**
         * 释放 QNMicrophoneAudioTrack 实例
         *
         * @param ptr QNMicrophoneAudioTrac k实例指针
         */
        static void DestroyMicrophoneAudioTrack(QNMicrophoneAudioTrack* ptr);

        /**
         * 创建 CreateCustomAudioTrack 实例
         *
         * @param config QNCustomAudioTrackConfig 配置
         */
        static QNCustomAudioTrack* CreateCustomAudioTrack(QNCustomAudioTrackConfig& config);

        /**
         * 释放 QNCustomAudioTrack 实例
         *
         * @param ptr QNCustomAudioTrack 实例指针
         */
        static void DestroyCustomAudioTrack(QNCustomAudioTrack* ptr);

        /**
         * 创建 QNCameraVideoTrack 实例
         *
         * @param config QNCameraVideoTrackConfig 配置
         */
        static QNCameraVideoTrack* CreateCameraVideoTrack(QNCameraVideoTrackConfig& config);

        /**
         * 释放 QNCameraVideoTrack 实例
         *
         * @param ptr QNCameraVideoTrack 实例指针
         */
        static void DestroyCameraVideoTrack(QNCameraVideoTrack* ptr);

        /**
         * 创建 QNScreenVideoTrack 实例
         *
         * @param config QNScreenVideoTrackConfig 配置
         */
        static QNScreenVideoTrack* CreateScreenVideoTrack(QNScreenVideoTrackConfig& config);

        /**
         * 释放 QNScreenVideoTrack 实例
         *
         * @param ptr QNScreenVideoTrack 实例指针
         */
        static void DestroyScreenVideoTrack(QNScreenVideoTrack* ptr);

        /**
         * 创建 QNCustomVideoTrack 实例
         *
         * @param config QNCustomVideoTrackConfig 配置
         */
        static QNCustomVideoTrack* CreateCustomVideoTrack(QNCustomVideoTrackConfig& config);

        /**
         * 释放 QNCustomVideoTrack 实例
         *
         * @param ptr QNCustomVideoTrack 实例指针
         */
        static void DestroyCustomVideoTrack(QNCustomVideoTrack* ptr);

    protected:
        virtual ~QNRTC() {}
    };
}