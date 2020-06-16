#pragma once
#include<string>

using namespace std;

#ifdef _WIN32
//#include <Windows.h>

#ifdef UNICODE

inline wstring utf2unicode(string str)
{
    //获取缓冲区的大小，并申请空间，缓冲区大小是按字符计算的 
    int len=MultiByteToWideChar(CP_ACP,0,str.c_str(),str.size(),NULL,0);
    TCHAR *buffer=new(std::nothrow) TCHAR[len+1];
    if (NULL == buffer)
    {
        return NULL;
    }
    //多字节编码转换成宽字节编码 
    MultiByteToWideChar(CP_ACP,0,str.c_str(),str.size(),buffer,len);
    buffer[len]='\0';//添加字符串结尾
    //删除缓冲区并返回值 
    wstring return_value;
    return_value.assign(buffer, len);
    delete []buffer;
    return return_value;
}

inline string unicode2utf(wstring str)
{
    string return_value;
    //获取缓冲区的大小，并申请空间，缓冲区大小是按字节计算的 
    int len=WideCharToMultiByte(CP_ACP,0,str.c_str(),str.size(),NULL,0,NULL,NULL);
    char *buffer=new(std::nothrow) char[len+1];
    if (NULL == buffer)
    {
        return NULL;
    }
    WideCharToMultiByte(CP_ACP,0,str.c_str(),str.size(),buffer,len,NULL,NULL);
    buffer[len]='\0';
    //删除缓冲区并返回值 
    return_value.assign(buffer, len);
    delete []buffer;
    return return_value;
}

inline string string_to_utf8(const string & str)
{
    int nwLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);

    wchar_t * pwBuf = new wchar_t[nwLen + 1];//一定要加1，不然会出现尾巴
    ZeroMemory(pwBuf, nwLen * 2 + 2);

    ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), pwBuf, nwLen);

    int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);

    char * pBuf = new char[nLen + 1];
    ZeroMemory(pBuf, nLen + 1);

    ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);

    std::string retStr(pBuf);

    delete[]pwBuf;
    delete[]pBuf;

    pwBuf = NULL;
    pBuf = NULL;

    return retStr;
}

inline string utf8_to_string(const string & str)
{
    int nwLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);

    wchar_t * pwBuf = new wchar_t[nwLen + 1];//一定要加1，不然会出现尾巴
    memset(pwBuf, 0, nwLen * 2 + 2);

    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), pwBuf, nwLen);

    int nLen = WideCharToMultiByte(CP_ACP, 0, pwBuf, -1, NULL, NULL, NULL, NULL);

    char * pBuf = new char[nLen + 1];
    memset(pBuf, 0, nLen + 1);

    WideCharToMultiByte(CP_ACP, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);

    std::string retStr = pBuf;

    delete[]pBuf;
    delete[]pwBuf;

    pBuf = NULL;
    pwBuf = NULL;

    return retStr;
}

#else
inline string unicode2utf(char* str)
{
    string return_value;
    //删除缓冲区并返回值
    return_value.append(str);
    return return_value;
}
#endif // UNICODE

#endif // WIN32