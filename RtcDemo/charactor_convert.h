#pragma once

#ifdef WIN32
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
    return_value.append(buffer);
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
    return_value.append(buffer);
    delete []buffer;
    return return_value;
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