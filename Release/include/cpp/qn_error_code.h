#ifndef QN_ERROR_CODE_H
#define QN_ERROR_CODE_H

namespace qiniu {
#define QNRTC_OK 0
#define QNRTC_SUCCEEDED(rv) (rv == AVD_OK)

// Server error code
/* Token error */
#define Err_Token_Error 10001

/* Token is expired */
#define Err_Token_Expired 10002

/* Room closed*/
#define Err_Room_Closed 10005

/* Room is full*/
#define Err_Room_Full 10011

/* User already exist */
#define Err_User_Already_Exist 10022

/* No permission */
#define Err_No_Permission 10051

/* Invalid parameter */
#define Err_Invalid_Parameter 10053

/* Media capability not supported */
#define Err_Media_Not_Support 10054

/* Direct stream ID already exist */
#define Err_Stream_Exist 10077

/* Join auth failed */
#define Err_Auth_Failed 21001

/* Operation in invalid state */
#define Err_Invalid_State 21002

/* Reconnect failed */
#define Err_Reconnect_Failed 21003

/* Network request timeout */
#define Err_Network_Timeout 21004

/* Error fatal */
#define Err_Fatal 21005

/* CDN stream not exist */
#define Err_Stream_Not_Exist 21006

/* Server unavailable */
#define Err_Server_Unavailable 21007

/* Operation Timeout */
#define Err_Operation_Timeout 21008

/*camera open failed, no permission or be occupied */
#define Err_Camera_Init_Failed 23001

/*camera is evicted by other process */
#define Err_Camera_Capture_Failed 23002

/* Microphone init failed */
#define Err_Mic_Init_Failed 23006

/* Microphone capture failed */
#define Err_Mic_Capture_Failed 23007

/* Speaker init failed */
#define Err_Speaker_Init_failed 23008

/* Resample failed */
#define Err_Audio_Mixing_Resample_Failed 22001

/* Can not find audio track when extract the mixing audio */
#define Err_Audio_Mixing_Audio_Not_Found 22002

/* IO exception when set audio file */
#define Err_Audio_Mixing_IO_Exception 22003

/* Device maybe can not support decode the mixing audio */
#define Err_Audio_Mixing_Decoder_Exception 22004

/* Seek before audio mixing */
#define Err_Audio_Mixing_Seek_Failed 22005

/* AUGraph error, iOS Only */
#define Err_Audio_Mixing_Grap 22006

/* AUNode error, iOS Only */
#define Err_Audio_Mixing_Node 22007

/* Error happened when read audio data, iOS Only */
#define Err_Audio_Mixing_Read_Data 22008

/* Wrong property error, iOS Only */
#define Err_Audio_Mixing_Property 22009

/* Setting callback error, iOS Only */
#define Err_Audio_Mixing_Callback 22010

/* relay token invalid */
#define Err_Relay_Token_Invalid 24000

/* media relay already started */
#define Err_Relay_Already_Start 24001

/* media relay not started */
#define Err_Relay_Not_Start 24002

/* destination room not existed */
#define Err_Relay_Destination_Room_Not_Existed 24003

/* player in dest room */
#define Err_Relay_Player_In_Dest_Room 24004

/* media relay start failed */
#define Err_Relay_Start_Failed 24005

/* invalid client mode */
#define Err_Invalid_Client_Mode 24006

/* invlid client role */
#define Err_Invalid_Client_Role 24007

}  // namespace qiniu

#endif  // QN_ERROR_CODE_H
