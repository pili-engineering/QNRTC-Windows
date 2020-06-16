// MergeDialog.cpp : implementation file
//

#include "stdafx.h"
#include "MergeDialog.h"
#include "charactor_convert.h"

// MergeDialog dialog

IMPLEMENT_DYNAMIC(MergeDialog, CDialogEx)

MergeDialog::MergeDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_MERGE, pParent)
{
}

MergeDialog::~MergeDialog()
{
}

void MergeDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL MergeDialog::OnInitDialog()
{
    // 默认值
    SetDlgItemText(IDC_EDIT_PUB_URL, utf2unicode(_merge_config.publish_url).c_str());
    SetDlgItemText(IDC_EDIT_MERGE_W, utf2unicode(std::to_string(_merge_config.job_width)).c_str());
    SetDlgItemText(IDC_EDIT_MERGE_H, utf2unicode(std::to_string(_merge_config.job_height)).c_str());
    SetDlgItemText(IDC_EDIT_MERGE_FPS, utf2unicode(std::to_string(_merge_config.job_fps)).c_str());
    SetDlgItemText(IDC_EDIT_MERGE_BITRATE, utf2unicode(std::to_string(_merge_config.job_bitrate / 1000)).c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_STRETCH_MODE))->InsertString(-1, utf2unicode("ASPECT_FILL").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_STRETCH_MODE))->InsertString(-1, utf2unicode("ASPECT_FIT").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_STRETCH_MODE))->InsertString(-1, utf2unicode("SCALE_TO_FIT").c_str());
    ((CComboBox *)GetDlgItem(IDC_COMBO_STRETCH_MODE))->SetCurSel(0);

    SetDlgItemText(IDC_EDIT_WATER_URL, utf2unicode(_merge_config.watermark_url).c_str());
    SetDlgItemText(IDC_EDIT_WATER_W, utf2unicode(std::to_string(_merge_config.watermark_width)).c_str());
    SetDlgItemText(IDC_EDIT_WATER_H, utf2unicode(std::to_string(_merge_config.watermark_height)).c_str());
    SetDlgItemText(IDC_EDIT_WATER_X, utf2unicode(std::to_string(_merge_config.watermark_x)).c_str());
    SetDlgItemText(IDC_EDIT_WATER_Y, utf2unicode(std::to_string(_merge_config.watermark_y)).c_str());

    SetDlgItemText(IDC_EDIT_BG_URL, utf2unicode(_merge_config.background_url).c_str());
    SetDlgItemText(IDC_EDIT_BG_W, utf2unicode(std::to_string(_merge_config.background_width)).c_str());
    SetDlgItemText(IDC_EDIT_BG_H, utf2unicode(std::to_string(_merge_config.background_height)).c_str());
    SetDlgItemText(IDC_EDIT_BG_X, utf2unicode(std::to_string(_merge_config.background_x)).c_str());
    SetDlgItemText(IDC_EDIT_BG_Y, utf2unicode(std::to_string(_merge_config.background_y)).c_str());

    ((CButton*)GetDlgItem(IDC_CHECK_LOCAL_AUDIO))->SetCheck(_merge_config.merge_local_audio ? 1 : 0);
    ((CButton*)GetDlgItem(IDC_CHECK_LOCAL_VIDEO))->SetCheck(_merge_config.merge_local_video ? 1 : 0);
    SetDlgItemText(IDC_EDIT_LOCAL_W, utf2unicode(std::to_string(_merge_config.local_video_width)).c_str());
    SetDlgItemText(IDC_EDIT_LOCAL_H, utf2unicode(std::to_string(_merge_config.local_video_height)).c_str());
    SetDlgItemText(IDC_EDIT_LOCAL_X, utf2unicode(std::to_string(_merge_config.local_video_x)).c_str());
    SetDlgItemText(IDC_EDIT_LOCAL_Y, utf2unicode(std::to_string(_merge_config.local_video_y)).c_str());

    ((CButton*)GetDlgItem(IDC_CHECK_REMOTE_AUDIO))->SetCheck(_merge_config.merge_remote_audio ? 1 : 0);
    ((CButton*)GetDlgItem(IDC_CHECK_REMOTE_VIDEO))->SetCheck(_merge_config.merge_remote_video ? 1 : 0);
    SetDlgItemText(IDC_EDIT_REMOTE_W, utf2unicode(std::to_string(_merge_config.remote_video_width)).c_str());
    SetDlgItemText(IDC_EDIT_REMOTE_H, utf2unicode(std::to_string(_merge_config.remote_video_height)).c_str());
    SetDlgItemText(IDC_EDIT_REMOTE_X, utf2unicode(std::to_string(_merge_config.remote_video_x)).c_str());
    SetDlgItemText(IDC_EDIT_REMOTE_Y, utf2unicode(std::to_string(_merge_config.remote_video_y)).c_str());

    return TRUE;
}

BEGIN_MESSAGE_MAP(MergeDialog, CDialogEx)
    ON_BN_CLICKED(IDC_BUTTON_MERGE_OK, &MergeDialog::OnBnClickedButtonMergeOk)
    ON_BN_CLICKED(IDC_BUTTON_MERGE_CANCEL, &MergeDialog::OnBnClickedButtonMergeCancel)
END_MESSAGE_MAP()

void MergeDialog::OnBnClickedButtonMergeOk()
{
    CString tmp_cstr;

    GetDlgItemText(IDC_EDIT_PUB_URL, tmp_cstr);
    _merge_config.publish_url   =  unicode2utf(tmp_cstr.GetBuffer());

    GetDlgItemText(IDC_EDIT_MERGE_W, tmp_cstr);
    _merge_config.job_width = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());

    GetDlgItemText(IDC_EDIT_MERGE_H, tmp_cstr);
    _merge_config.job_height    = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());

    GetDlgItemText(IDC_EDIT_MERGE_FPS, tmp_cstr);
    _merge_config.job_fps       = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());

    _merge_config.job_stretch_mode = ((CComboBox*)GetDlgItem(IDC_COMBO_STRETCH_MODE))->GetCurSel();

    GetDlgItemText(IDC_EDIT_MERGE_BITRATE, tmp_cstr);
    _merge_config.job_bitrate   = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str()) * 1000;
    _merge_config.job_min_bitrate   = _merge_config.job_bitrate - (200 * 1000);
    _merge_config.job_max_bitrate   = _merge_config.job_bitrate + (200 * 1000);

    GetDlgItemText(IDC_EDIT_WATER_URL, tmp_cstr);
    _merge_config.watermark_url = unicode2utf(tmp_cstr.GetBuffer());

    GetDlgItemText(IDC_EDIT_WATER_W, tmp_cstr);
    _merge_config.watermark_width   = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());
    GetDlgItemText(IDC_EDIT_WATER_H, tmp_cstr);
    _merge_config.watermark_height  = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());

    GetDlgItemText(IDC_EDIT_WATER_X, tmp_cstr);
    _merge_config.watermark_x   = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());
    GetDlgItemText(IDC_EDIT_WATER_Y, tmp_cstr);
    _merge_config.watermark_y   = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());

    GetDlgItemText(IDC_EDIT_BG_URL, tmp_cstr);
    _merge_config.background_url = unicode2utf(tmp_cstr.GetBuffer());

    GetDlgItemText(IDC_EDIT_BG_W, tmp_cstr);
    _merge_config.background_width   = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());
    GetDlgItemText(IDC_EDIT_BG_H, tmp_cstr);
    _merge_config.background_height = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());

    GetDlgItemText(IDC_EDIT_BG_X, tmp_cstr);
    _merge_config.background_x  = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());
    GetDlgItemText(IDC_EDIT_BG_Y, tmp_cstr);
    _merge_config.background_y  = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());

    _merge_config.merge_local_audio = ((CButton*)GetDlgItem(IDC_CHECK_LOCAL_AUDIO))->GetCheck();
    _merge_config.merge_local_video = ((CButton*)GetDlgItem(IDC_CHECK_LOCAL_VIDEO))->GetCheck();

    GetDlgItemText(IDC_EDIT_LOCAL_W, tmp_cstr);
    _merge_config.local_video_width     = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());
    GetDlgItemText(IDC_EDIT_LOCAL_H, tmp_cstr);
    _merge_config.local_video_height     = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());
    GetDlgItemText(IDC_EDIT_LOCAL_X, tmp_cstr);
    _merge_config.local_video_x     = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());
    GetDlgItemText(IDC_EDIT_LOCAL_Y, tmp_cstr);
    _merge_config.local_video_y     = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());


    _merge_config.merge_remote_audio = ((CButton*)GetDlgItem(IDC_CHECK_REMOTE_AUDIO))->GetCheck();
    _merge_config.merge_remote_video = ((CButton*)GetDlgItem(IDC_CHECK_REMOTE_VIDEO))->GetCheck();
    GetDlgItemText(IDC_EDIT_REMOTE_W, tmp_cstr);
    _merge_config.remote_video_width     = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());
    GetDlgItemText(IDC_EDIT_REMOTE_H, tmp_cstr);
    _merge_config.remote_video_height     = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());
    GetDlgItemText(IDC_EDIT_REMOTE_X, tmp_cstr);
    _merge_config.remote_video_x     = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());
    GetDlgItemText(IDC_EDIT_REMOTE_Y, tmp_cstr);
    _merge_config.remote_video_y     = std::stoi(unicode2utf(tmp_cstr.GetBuffer()).c_str());

    ::SendMessage(this->GetParent()->m_hWnd, MERGE_MESSAGE_ID /* ID */, 0, (LPARAM)&_merge_config);

    CDialog::OnOK();
}


void MergeDialog::OnBnClickedButtonMergeCancel()
{
    CDialog::OnCancel();
}
