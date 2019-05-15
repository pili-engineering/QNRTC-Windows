/*!
 * \file rtc_engine.h
 *
 * \author qiniu
 * 
 * Global configuration interface
 *
 */

#pragma once
#include <map>
#include <string>
#include <vector>
#include <list>
#include "qn_rtc_errorcode.h"

#ifdef _WIN32
#ifdef __QINIU_DLL_EXPORT_ 
#define QINIU_EXPORT_DLL __declspec(dllexport)
#else
#define QINIU_EXPORT_DLL __declspec(dllimport)
#endif
#endif // WIN32

#define QNRTC_MAX_DEVICE_LENGHT 128

namespace qiniu
{
    /**
    * SDK internal log level
    */
    enum QNLogLevel
    {
        LOG_DEBUG = 0,
        LOG_INFO,
        LOG_WARNING,
        LOG_ERROR,
    };

    /** 
    * SDK internal global resource configuration interface
    */
#ifdef _WIN32
    class QINIU_EXPORT_DLL QNRTCEngine
#else
    class QNRTCEngine
#endif // WIN32
    {
    public:
        /** 
        * Initialize the SDK internal resources
        * @return return 0 if success or an error code
        */
        static int  Init();

        /**
        * Release the SDK internal resources
        * @return return 0 if success or an error code
        */
        static int  Release();

        /**
        * Sets the parameter information for the log store
        * @param [in] level_
        *        what level of logs will be logged
        * @param [in] dir_name_
        *        the log will be stored in which directory
        * @param [in] file_name_
        *        the log file name of stored
        * @return return 0 if success or an error code
        */
        static int  SetLogParams(QNLogLevel level_,
            const std::string& dir_name_, const std::string& file_name_);

    protected:
        virtual ~QNRTCEngine() {};
    };
}


