#include "stdafx.h"
#include "Global.h"
#include "curl.h"

#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "Version.lib")

size_t WriteBuffer(void *src_, size_t src_size_, size_t blocks_, void *param_)
{
    string *str = (string*)(param_);
    str->append((char *)src_, src_size_ * blocks_);

    return str->size();
}

extern int GetRoomToken(const string& app_id_, const string room_name_, const string user_id_, string& token_)
{
    if (room_name_.empty() || user_id_.empty()) {
        return -1;
    }
    string appId = "d8lk7l4ed";
    if (!app_id_.empty()) {
        appId = app_id_;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    auto curl = curl_easy_init();

    // set options
    char url_buf[1024] = { 0 };
    string tmp_uid = user_id_;

    // 服务端合流的默认限制：user id 等于 admin 则拥有合流的权限
    if (strnicmp(const_cast<char*>(tmp_uid.c_str()), "admin", tmp_uid.length()) == 0) {
        snprintf(url_buf,
            sizeof(url_buf),
            "https://api-demo.qnsdk.com/v1/rtc/token/admin/app/%s/room/%s/user/%s",
            appId.c_str(),
            room_name_.c_str(),
            user_id_.c_str());
    } else {
        snprintf(url_buf,
            sizeof(url_buf),
            "https://api-demo.qnsdk.com/v1/rtc/token/app/%s/room/%s/user/%s",
            appId.c_str(),
            room_name_.c_str(),
            user_id_.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url_buf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteBuffer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &token_);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);

    TRACE(url_buf);

    // send request now
    int status(0);
    CURLcode result = curl_easy_perform(curl);
    if (result == CURLE_OK) {
        long code;
        result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        if (result == CURLE_OK) {
            if (code != 200) {
                status = -2; // server auth failed
            } else {
                status = 0; //success
            }
        } else {
            status = -3; //connect server timeout
        }
    } else {
        status = -3; //connect server timeout
    }
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return status;
}

string GetAppVersion()
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
                    __DATE__,
                    __TIME__
                );
            }
        }
        delete pData;
    }
    return string(ver_buf, ver_buf_len);
}

