// MessageDialog.cpp: 实现文件
//
#include "stdafx.h"
#include "MessageDialog.h"
#include "afxdialogex.h"
#include "charactor_convert.h"

// MessageDialog 对话框

IMPLEMENT_DYNAMIC(MessageDialog, CDialogEx)

MessageDialog::MessageDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_MESSAGE, pParent)
{

}

MessageDialog::~MessageDialog()
{
}

void MessageDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RICHEDIT2_MSG_INFO, _msg_rcv_snd_rich_edit_ctrl);
}


BEGIN_MESSAGE_MAP(MessageDialog, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_SEND_MSG, &MessageDialog::OnBnClickedButtonSendMsg)
END_MESSAGE_MAP()


// MessageDialog 消息处理程序


void MessageDialog::OnBnClickedButtonSendMsg()
{
	// TODO: 在此添加控件通知处理程序代码
	CString sndMsg;
	GetDlgItemText(IDC_EDIT_MSG_SEND, sndMsg);
	::SendMessage(this->GetParent()->m_hWnd, SEND_MESSAGE_ID , 0, (LPARAM)(LPCTSTR)sndMsg);
	
	time_t tt = time(NULL);
	struct tm *stm = localtime(&tt);

	wchar_t dest_buf[1024] = { 0 };
	_snwprintf(dest_buf, sizeof(dest_buf), _T("%04d-%02d-%2d %2d:%2d:%02d.00 [%s] %s"), 
		1900 + stm->tm_year, 
		1 + stm->tm_mon,
		stm->tm_mday,
		stm->tm_hour,
		stm->tm_min,
		stm->tm_sec,
		_user_id,
		sndMsg);

	TRACE(dest_buf);

	int line_count = _msg_rcv_snd_rich_edit_ctrl.GetLineCount();
	if (line_count >= 1000) {
		// 此控件可存储数据量有限，为避免卡顿，及时清除
		_msg_rcv_snd_rich_edit_ctrl.SetWindowTextW(_T(""));
		_msg_rcv_snd_rich_edit_ctrl.UpdateData();
		_msg_rcv_snd_rich_edit_ctrl.Invalidate();
	}
	_msg_rcv_snd_rich_edit_ctrl.SetSel(-1, -1);
	_msg_rcv_snd_rich_edit_ctrl.ReplaceSel(dest_buf);
	_msg_rcv_snd_rich_edit_ctrl.ReplaceSel(_T("\n"));
	_msg_rcv_snd_rich_edit_ctrl.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
}


void MessageDialog::OnReceiveMessage(const std::string& playerId,const std::string& msgText)
{
	time_t tt = time(NULL);
	struct tm *stm = localtime(&tt);

	wchar_t dest_buf[1024] = { 0 };
	_snwprintf(dest_buf, sizeof(dest_buf), _T("%04d-%02d-%2d %2d:%2d:%02d.00 [%s] %s"),
		1900 + stm->tm_year,
		1 + stm->tm_mon,
		stm->tm_mday,
		stm->tm_hour,
		stm->tm_min,
		stm->tm_sec,
		utf2unicode(playerId).c_str(),
		utf2unicode(msgText).c_str());

	TRACE(dest_buf);

	int line_count = _msg_rcv_snd_rich_edit_ctrl.GetLineCount();
	if (line_count >= 1000) {
		// 此控件可存储数据量有限，为避免卡顿，及时清除
		_msg_rcv_snd_rich_edit_ctrl.SetWindowTextW(_T(""));
		_msg_rcv_snd_rich_edit_ctrl.UpdateData();
		_msg_rcv_snd_rich_edit_ctrl.Invalidate();
	}
	_msg_rcv_snd_rich_edit_ctrl.SetSel(-1, -1);
	_msg_rcv_snd_rich_edit_ctrl.ReplaceSel(dest_buf);
	_msg_rcv_snd_rich_edit_ctrl.ReplaceSel(_T("\n"));
	_msg_rcv_snd_rich_edit_ctrl.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
}