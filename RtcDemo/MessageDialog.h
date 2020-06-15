#pragma once
#include "stdafx.h"
#include "resource.h"
#include "afxdialogex.h"
#include <string>

// MessageDialog 对话框

class MessageDialog : public CDialogEx
{
	DECLARE_DYNAMIC(MessageDialog)

public:
	MessageDialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~MessageDialog();

#define SEND_MESSAGE_ID    101

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_MESSAGE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonSendMsg();
	void OnReceiveMessage(const std::string& playerId, const std::string& msgText);

public:
	CString                       _user_id;

protected:
	CRichEditCtrl                _msg_rcv_snd_rich_edit_ctrl;
};
