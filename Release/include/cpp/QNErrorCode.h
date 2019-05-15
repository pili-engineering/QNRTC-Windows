#pragma once

namespace qiniu_v2
{
#define QNRTC_OK 0
#define QNRTC_FAILED -1
#define QNRTC_SUCCEEDED(rv) (rv == AVD_OK)
    
// Server error code
/* Token error */
#define Err_Token_Error 10001

/* Token is expired */
#define Err_Token_Expired 10002

/* Room instance closed */
#define Err_Room_INSTANCE_Closed 10003

/* Reconnect token error*/
#define Err_ReconnToken_Error 10004

/* Room closed*/
#define Err_Room_Closed 10005

/* Kicked out of the room */
#define Err_Kickout_Of_Room 10006
        
/* Room is full*/
#define Err_Room_Full 10011

/* Room not exist */
#define Err_Room_Not_Exist 10012

/* User not exist */
#define Err_User_Not_Exist 10021

/* User already exist */
#define Err_User_Already_Exist 10022

/* Publish stream not exist */
#define Err_Publish_Stream_Not_Exist 10031

/* Publish stream info not match */
#define  Err_Publish_Stream_Info_Not_Match 10032

/* Publish stream already exist */
#define Err_Publish_Stream_Already_exist 10033

/* Publish stream not ready */
#define Err_Publish_Stream_Not_Ready 10034

/* Publish stream no audio and video */
#define Err_Publish_Stream_No_Audio_Video 10035

/* Subscribe stream not exist */
#define Err_Subscribe_Stream_Not_Exist 10041

/* Subscribe stream info not match */
#define Err_Subscribe_Stream_Info_Not_Match 10042

/* Subscribe stream already exist */
#define Err_Subscribe_Stream_Already_Exist 10043

/* Can't subscribe yourself */
#define Err_Cannot_Subscribe_Self 10044

/* No permission */
#define Err_No_Permission 10051

/* Server unavailable */
#define Err_Server_Unavailable 10052

/* Invalid parameter */
#define Err_Invalid_Parameter 10053

/* Media capability not supported */
#define Err_Media_Not_Support 10054

/* Too many operations, try again later */
#define Err_Too_Many_Operation 10055

/* Publisher Disconnected, or not exist */
#define Err_Publisher_Disconnect 10061

/* Subscriber Disconnected, or not exist */
#define Err_Subscriber_Disconnect 10062

/* Not support multi master audio or video */
#define Err_Multi_Master_AV 10063


// Client error code
/* Invalid params */
#define Err_Invalid_Parameters 11000

/* SDK internal Null pointer */
#define Err_Internal_Null_Pointer 11001

/* Can not destroy in own thread */
#define Err_Cannot_Destroy_In_Self_Thread 11005

/* Can not change device when recording */
#define Err_Cannot_Change_When_Using 11006

/* Failed to set recorder device */
#define Err_Failed_Set_Recorder_Device 11007

/* Failed to set communication recorder device */
#define Err_Failed_Set_Communication_Recorder_Device 11008

/* Failed to set playout device */
#define Err_Failed_Set_Playout_Device 11009

/* Failed to set communication playout device */
#define Err_Failed_Set_Communication_Playout_Device 11010

/* Failed to get recorder or playout volume */
#define Err_Failed_Get_Volume 11011

/* Failed to set recorder or playout volume*/
#define Err_Failed_Set_volume 11012

/* Playout mute failed */
#define Err_Playout_Mute_Failed 11013

/* Recorder mute failed */
#define Err_Recorder_Mute_Failed 11014

/* Operator failed */
#define Err_Operator_Failed 11015

/* Room already joined */
#define Err_Room_Already_Joined 11016

/* Network disconnected */
#define Err_Network_Disconnect 11017

/* No this user */
#define Err_No_This_User 11018

/* No this user's stream info */
#define Err_No_This_User_Stream_Info 11019

/* Device busy */
#define Err_Device_Busy 11020

/* Device open failed */
#define Err_Device_Open_Failed 11021

/* No this Device */
#define Err_No_This_Device 11022

/* Already published*/
#define Err_Already_Published 11023

/* Already unpublished*/
#define Err_Already_UnPublished 11024

/* no publish record */
#define Err_No_Publish_Record 11025

/* Already subscribed or subscribing */
#define Err_Already_Subscribed 11026

/* Stream's connid exception */
#define Err_Stream_ConnId_Empty 11027

/* Already unsubscribed or unsubscribing */
#define Err_Already_UnSubscribed 11028

/* Decode room token failed */
#define Err_Decode_RoomToken_Failed 11029

/* Parse json string failed */
#define Err_Parse_Json_Failed 11030

/* Parse json get room name failed */
#define Err_Parse_Json_RoomName_Failed 11031

/* Parse json get user id failed */
#define Err_Parse_Json_UserId_Failed 11032

/* Parse json get appId failed */
#define Err_Parse_Json_AppId_Failed 11033

/* Request access token failed */
#define Err_Request_Access_Token_Failed 11034

/* Parse access token failed */
#define Err_Parse_Access_Token_Failed 11035

/* Parse room server address failed */
#define Err_Parse_Room_Server_Address_Failed 11036

/* Connect server to get accesstoken timeout */
#define Err_Get_AccessToken_Timeout 11040

/* Video capture module not running*/
#define Err_VideoCapture_Not_Running 11050

/* Create offer sdp failed*/
#define Err_CreateOffer_Failed 11060

/* recording not started*/
#define Err_Record_Not_Start 11061

/* Resample failed*/
#define Err_Internal_Resamle_Failed 11062

/* System no memory*/
#define Err_No_Memory 11063

/* Memory not enough*/
#define Err_Memory_Not_Enough 11064

/* Tracks publish all failed*/
#define Err_Tracks_Publish_All_Failed 11070

/* Tracks publish partial failed*/
#define Err_Tracks_Publish_Partial_Failed 11071

/* No this track info */
#define Err_No_This_Track_Info 12000

/* Pre publish operator not complete*/
#define Err_Pre_Publish_Not_Complete 12002

/* Pre subscribe operator not complete*/
#define Err_Pre_Subscribe_Not_Complete 12003

/* Subscribe timeout*/
#define  Err_Suscribe_Timeout 12004

/* Publish timeout*/
#define  Err_Publish_Timeout 12005

/* Platform not support */
#define Err_Platform_Not_Support 13000

/* license expired */
#define Err_License_expired 14000
}
