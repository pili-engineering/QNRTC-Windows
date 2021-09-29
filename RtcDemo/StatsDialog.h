#pragma once
#include "stdafx.h"
#include "resource.h"
#include "afxdialogex.h"
#include <string>


// StatsDialog 对话框

class StatsDialog : public CDialogEx
{
	DECLARE_DYNAMIC(StatsDialog)

public:
	StatsDialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~StatsDialog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_STATS };
#endif
    void OnReceiveStatsText(const std::string& msgText);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
protected:
    CRichEditCtrl                _stats_rich_edit_ctrl;
};
