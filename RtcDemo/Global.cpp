#include "stdafx.h"
#include "Global.h"
#include "httplib.hpp"

#if defined _WIN32
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "Version.lib")
#endif

extern int GetRoomToken(const string& app_id_,
    const string room_name_,
    const string user_id_,
    const std::string& host_name_,
    const int time_out_,
    string& token_)
{
    if (room_name_.empty() || user_id_.empty()) {
        return -1;
    }
    string appId = "d8lk7l4ed";
    if (!app_id_.empty()) {
        appId = app_id_;
    }
    
    // set options
    char url_buf[1024] = { 0 };
    string tmp_uid = user_id_;

    // 服务端合流的默认限制：user id 等于 admin 则拥有合流的权限 
    if (strnicmp(const_cast<char*>(tmp_uid.c_str()), "admin", tmp_uid.length()) == 0) {
        snprintf(url_buf,
            sizeof(url_buf),
            "/v1/rtc/token/admin/app/%s/room/%s/user/%s",
            appId.c_str(),
            room_name_.c_str(),
            user_id_.c_str());
    } else {
        snprintf(url_buf,
            sizeof(url_buf),
            "/v1/rtc/token/app/%s/room/%s/user/%s",
            appId.c_str(),
            room_name_.c_str(),
            user_id_.c_str());
    }
    
    TRACE(url_buf);
    
    httplib::Client cli(host_name_.c_str(), 80, time_out_);
    // send request now
    int status(0);
    auto response_ptr = cli.Get(url_buf);
    if (response_ptr) {
        if (200 == response_ptr->status) {
            token_ = response_ptr->body;
            return 0;
        } else {
            return -1;
        }
    } else {
        return -3; //connect server timeout
    }
}

string GetAppVersion(const string& date_str, const string& time_str)
{
    DWORD dwInfoSize = 0;
    char exePath[MAX_PATH];
    char ver_buf[128] = { 0 };
    int ver_buf_len = 0;
    memset(exePath, 0, sizeof(exePath));

    // 得到程序的自身路径 
    GetModuleFileNameA(NULL, exePath, sizeof(exePath) / sizeof(char));

    // 判断是否能获取版本号 
    dwInfoSize = GetFileVersionInfoSizeA(exePath, NULL);

    if (dwInfoSize == 0) {
        return "";
    } else {
        BYTE* pData = new BYTE[dwInfoSize];
        // 获取版本信息
        if (!GetFileVersionInfoA(exePath, NULL, dwInfoSize, pData)) {
            return "";
        } else {
            // 查询版本信息中的具体键值 
            LPVOID lpBuffer;
            UINT uLength;
            if (!::VerQueryValue((LPCVOID)pData, _T("\\"), &lpBuffer, &uLength)) {
            } else {
                DWORD dwVerMS;
                DWORD dwVerLS;
                dwVerMS = ((VS_FIXEDFILEINFO*)lpBuffer)->dwProductVersionMS;
                dwVerLS = ((VS_FIXEDFILEINFO*)lpBuffer)->dwProductVersionLS;
                ver_buf_len = snprintf(ver_buf,
                    sizeof(ver_buf),
                    "Version : %d.%d.%d.%d    BuildTime : %s %s",
                    (dwVerMS >> 16),
                    (dwVerMS & 0xFFFF),
                    (dwVerLS >> 16),
                    (dwVerLS & 0xFFFF),
                    date_str.c_str(),
                    time_str.c_str()
                );
            }
        }
        delete pData;
    }
    return string(ver_buf, ver_buf_len);
}

