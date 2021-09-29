// StatsDialog.cpp: 实现文件
//
#include "stdafx.h"
#include "StatsDialog.h"
#include "afxdialogex.h"
#include "charactor_convert.h"

// StatsDialog 对话框

IMPLEMENT_DYNAMIC(StatsDialog, CDialogEx)

StatsDialog::StatsDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_STATS, pParent)
{
}

StatsDialog::~StatsDialog()
{
}

void StatsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_RICHEDIT2_STATS, _stats_rich_edit_ctrl);
    _stats_rich_edit_ctrl.SetWindowTextW(_T(""));
    _stats_rich_edit_ctrl.UpdateData();
    _stats_rich_edit_ctrl.Invalidate();
}


BEGIN_MESSAGE_MAP(StatsDialog, CDialogEx)
END_MESSAGE_MAP()


// StatsDialog 消息处理程序
void StatsDialog::OnReceiveStatsText(const std::string& msgText)
{
    wchar_t dest_buf[1024] = { 0 };
    _snwprintf(dest_buf, sizeof(dest_buf), _T("%s"),
        utf2unicode(msgText).c_str());

    TRACE(dest_buf);

    int line_count = _stats_rich_edit_ctrl.GetLineCount();
    if (line_count >= 1000) {
        // 此控件可存储数据量有限，为避免卡顿，及时清除
        _stats_rich_edit_ctrl.SetWindowTextW(_T(""));
        _stats_rich_edit_ctrl.UpdateData();
        _stats_rich_edit_ctrl.Invalidate();
    }
    _stats_rich_edit_ctrl.SetSel(-1, -1);
    _stats_rich_edit_ctrl.ReplaceSel(dest_buf);
    _stats_rich_edit_ctrl.ReplaceSel(_T("\n"));
    _stats_rich_edit_ctrl.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
}