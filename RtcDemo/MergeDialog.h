#pragma once
#include "stdafx.h"
#include "resource.h"
#include "afxdialogex.h"
#include <string>

// MergeDialog dialog

class MergeDialog : public CDialogEx
{
	DECLARE_DYNAMIC(MergeDialog)

public:
	MergeDialog(CWnd* pParent = nullptr);   // standard constructor
	virtual ~MergeDialog();

    afx_msg void OnBnClickedButtonMergeOk();
    afx_msg void OnBnClickedButtonMergeCancel();

    struct MergeConfig
    {
        std::string publish_url     = "rtmp://pili-publish.live.zhangrui.qiniuts.com/live-rui/flow-2";

        int32_t job_width           = 480;
        int32_t job_height          = 480;
        int32_t job_fps             = 20;;
        int32_t job_bitrate         = 800 * 1000;
        int32_t job_max_bitrate     = 1000 * 1000;
        int32_t job_min_bitrate     = 500 * 1000;
        int32_t job_stretch_mode    = 0;

        std::string watermark_url   = "http://mars-assets.qnssl.com/qiniulogo/normal-logo-blue.png";
        int32_t watermark_x         = 0;
        int32_t watermark_y         = 0;
        int32_t watermark_width     = 80;
        int32_t watermark_height    = 80;

        std::string background_url  = "http://mars-assets.qnssl.com/qiniulog/img-slogan-white-en.png";
        int32_t background_x        = 0;
        int32_t background_y        = 0;
        int32_t background_width    = 300;
        int32_t background_height   = 100;

        bool merge_local_video      = true;
        bool merge_local_audio      = true;
        int32_t local_video_x       = 0;
        int32_t local_video_y       = 0;
        int32_t local_video_width   = 200;
        int32_t local_video_height  = 200;

        bool merge_remote_video     = true;
        bool merge_remote_audio     = true;
        int32_t remote_video_x      = 200;
        int32_t remote_video_y      = 200;
        int32_t remote_video_width  = 200;
        int32_t remote_video_height = 200;

    } _merge_config;

    #define MERGE_MESSAGE_ID    100

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_MERGE };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};
