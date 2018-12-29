#pragma once
#include <string>

using namespace std;

extern int GetRoomToken(
    const string& app_id_, 
    const string room_name_, 
    const string user_id_, 
    string& token_
);

extern string GetAppVersion();