// maindlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////


#pragma once


#include "AboutDlg.h"
#include "Util.h"
#include "WndLayout.h"


class CMainDlg : public CDialogImpl<CMainDlg>, public CMessageFilter
{
public:
	enum { IDD = IDD_DIALOG_MAIN };


	CMainDlg()
	{
	}

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)

		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)

		MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
        MESSAGE_HANDLER(WM_DROPFILES, OnDropFiles)

	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// center the dialog on the screen
		CenterWindow();

		// set icons
		HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
			IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
		SetIcon(hIcon, TRUE);
		HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
			IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		SetIcon(hIconSmall, FALSE);

		// Add "About..." menu item to system menu.

		// IDM_ABOUTBOX must be in the system command range.
		_ASSERTE((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
		_ASSERTE(IDM_ABOUTBOX < 0xF000);

		CMenu SysMenu = GetSystemMenu(FALSE);
		if(::IsMenu(SysMenu))
		{
			TCHAR szAboutMenu[256];
			if(::LoadString(_Module.GetResourceInstance(), IDS_ABOUTBOX, szAboutMenu, 255) > 0)
			{
				SysMenu.AppendMenu(MF_SEPARATOR);
				SysMenu.AppendMenu(MF_STRING, IDM_ABOUTBOX, szAboutMenu);
			}
		}
		SysMenu.Detach();

		// register object for message filtering
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		pLoop->AddMessageFilter(this);

        m_strAppName.LoadString(IDS_APP_NAME);

        // 
        Util::EnableDrop(m_hWnd);

        InitLayout();

		return TRUE;
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
        return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CloseDialog(wID);
		return 0;
	}

	void CloseDialog(int nVal)
	{
		DestroyWindow();
		::PostQuitMessage(nVal);
	}

	LRESULT OnSysCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		UINT uCmdType = (UINT)wParam;

		if((uCmdType & 0xFFF0) == IDM_ABOUTBOX)
		{
			CAboutDlg dlg;
			dlg.DoModal();
		}
		else
			bHandled = FALSE;

		return 0;
	}

    LRESULT OnDropFiles(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
    {
        bHandled = TRUE;
        HDROP hDrop = reinterpret_cast<HDROP>(wParam);

        TCHAR szPath[MAX_PATH * 2] = {0};
        UINT uCount = ::DragQueryFile(hDrop, 0xFFFFFFFF, szPath, _countof(szPath));
        if(uCount == 0)
        {
            return 0;
        }

        BOOL bResult = TRUE;
        CString strLastPath;
        for(UINT i=0; i<uCount; ++ i)
        {
            if(::DragQueryFile(hDrop, i, szPath, _countof(szPath)) > 0)
            {
            }
        }


        return 0;
    }

    // Layout
    void InitLayout()
    {
        m_WndLayout.Init(m_hWnd);
    }

private:
    CWndLayout      m_WndLayout;
    CString         m_strAppName;
};
