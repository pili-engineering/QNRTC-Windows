#pragma once
#include <string>

using namespace std;

extern int GetRoomToken(
    const string& app_id_, 
    const string room_name_, 
    const string user_id_, 
    const std::string& host_name_,
    const int time_out_,
    string& token_
);

extern string GetAppVersion(const string& date_str, const string& time_str);