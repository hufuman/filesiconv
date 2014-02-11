// maindlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////


#pragma once


#include "AboutDlg.h"
#include "Util.h"
#include "WndLayout.h"
#include "IconvWorker.h"


class CMainDlg : public CDialogImpl<CMainDlg>, public CMessageFilter
{
public:
	enum { IDD = IDD_DIALOG_MAIN };


	CMainDlg()
	{
        m_hThread = NULL;
	}

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	BEGIN_MSG_MAP(CMainDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
        MESSAGE_HANDLER(WM_DROPFILES, OnDropFiles)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)

		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)

        COMMAND_ID_HANDLER(IDC_BTN_ADD_FILES, OnBtnAddFiles)
        COMMAND_ID_HANDLER(IDC_CHK_OVERWRITE, OnChkOverwrite)
        COMMAND_ID_HANDLER(IDC_BTN_BROWSE, OnBtnBrowse)
        COMMAND_ID_HANDLER(IDC_BTN_CONVERT, OnBtnConvert)

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

        m_ListPaths.Attach(GetDlgItem(IDC_LIST_SRC));
        m_ComboSourceCodepage.Attach(GetDlgItem(IDC_COMBO_SOURCE));
        m_ComboTargetCodepage.Attach(GetDlgItem(IDC_COMBO_TARGET));

        int nIndex = m_ComboSourceCodepage.AddString(_T("Auto(According to BOM"));
        m_ComboSourceCodepage.SetItemData(nIndex, CodeAuto);

        InitCodepages(m_ComboSourceCodepage);
        InitCodepages(m_ComboTargetCodepage);

        InitLayout();

		return TRUE;
	}

    void InitCodepages(CComboBox& combo)
    {
        struct
        {
            CodePageValue nCodepageValue;
            LPCTSTR szCodePageName;
        } codepages[] =
        {
            {CodeAnsi,      _T("System")},
            {CodeUtf8,      _T("Utf8")},
            {CodeChinese,   _T("Chinese")},
        };
        int nIndex = 0;
        for(int i=0; i<_countof(codepages); ++ i)
        {
            nIndex = combo.AddString(codepages[i].szCodePageName);
            combo.SetItemData(i, codepages[i].nCodepageValue);
        }
        combo.SetCurSel(0);
    }

    LRESULT OnBtnAddFiles(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        ATL::CSimpleArray<CString> listFiles;
        if(Util::BrowseForFiles(listFiles, m_hWnd))
        {
            int nCount = listFiles.GetSize();
            for(int i=0; i<nCount; ++ i)
            {
                AddPath(listFiles[i]);
            }
        }
        return 0;
    }

    LRESULT OnChkOverwrite(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        BOOL bChecked = (IsDlgButtonChecked(IDC_CHK_OVERWRITE) == BST_CHECKED);
        GetDlgItem(IDC_EDIT_TARGET).EnableWindow(!bChecked);
        GetDlgItem(IDC_BTN_BROWSE).EnableWindow(!bChecked);
        return 0;
    }

    LRESULT OnBtnBrowse(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        CString strPath = Util::BrowseForFolder(m_hWnd);
        if(!strPath.IsEmpty())
        {
            SetDlgItemText(IDC_EDIT_TARGET, strPath);
        }
        return 0;
    }

    LRESULT OnBtnConvert(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        CString strTemp;

        // 0. check target path
        BOOL bOverwrite = (IsDlgButtonChecked(IDC_CHK_OVERWRITE) == BST_CHECKED);
        m_Worker.SetOverwrite(TRUE);
        if(!bOverwrite)
        {
            GetDlgItemText(IDC_EDIT_TARGET, strTemp);
            if(strTemp.IsEmpty())
            {
                CString strMsg;
                strMsg.LoadString(IDS_ERR_EMPTY_TARGET);
                MessageBox(strMsg, m_strAppName);
                return 0;
            }
            m_Worker.SetTargetPath(strTemp);
        }

        // 1. disabled all controls
        EnableUI(FALSE);

        // 2. set info
        int nSrcIndex = m_ComboSourceCodepage.GetCurSel();
        int nDstIndex = m_ComboTargetCodepage.GetCurSel();
        CodePageValue nSrcCodepage = static_cast<CodePageValue>(m_ComboSourceCodepage.GetItemData(nSrcIndex));
        CodePageValue nDstCodepage = static_cast<CodePageValue>(m_ComboTargetCodepage.GetItemData(nDstIndex));
        m_Worker.SetCodepage(nSrcCodepage, nDstCodepage);

        m_Worker.SetFiles(&m_arrListFiles);

        // 3. start thread
        m_hThread = (HANDLE)_beginthreadex(0, 0, &ThreadProc, (void*)this, 0, 0);

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
            return 0;

        BOOL bResult = TRUE;
        CString strLastPath;
        for(UINT i=0; i<uCount; ++ i)
        {
            if(::DragQueryFile(hDrop, i, szPath, _countof(szPath)) > 0)
            {
                AddPath(szPath);
            }
        }

        return 0;
    }

    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
    {
        bHandled = FALSE;
        m_Worker.Stop();
        ::WaitForSingleObject(m_hThread, INFINITE);
        ::CloseHandle(m_hThread);
        m_hThread = NULL;
        return 0;
    }

    // Layout
    void InitLayout()
    {
        m_WndLayout.Init(m_hWnd);
        m_WndLayout.AddControlById(IDC_LIST_SRC, Layout_HFill | Layout_VFill);

        m_WndLayout.AddControlById(IDC_LABEL_STEP2, Layout_Left | Layout_Bottom);
        m_WndLayout.AddControlById(IDC_COMBO_SOURCE, Layout_Left | Layout_Bottom);

        m_WndLayout.AddControlById(IDC_LABEL_STEP3, Layout_Left | Layout_Bottom);
        m_WndLayout.AddControlById(IDC_COMBO_TARGET, Layout_Left | Layout_Bottom);

        m_WndLayout.AddControlById(IDC_LABEL_STEP4, Layout_Left | Layout_Bottom);
        m_WndLayout.AddControlById(IDC_CHK_OVERWRITE, Layout_Left | Layout_Bottom);
        m_WndLayout.AddControlById(IDC_EDIT_TARGET, Layout_HFill | Layout_Bottom);
        m_WndLayout.AddControlById(IDC_BTN_BROWSE, Layout_Right | Layout_Bottom);

        m_WndLayout.AddControlById(IDC_LABEL_FINALLY, Layout_Left | Layout_Bottom);
        m_WndLayout.AddControlById(IDC_BTN_CONVERT, Layout_Left | Layout_Bottom);
    }

    void AddPath(LPCTSTR szPath)
    {
        if(szPath == NULL || szPath[0] == 0)
            return;

        DWORD dwAttr = ::GetFileAttributes(szPath);
        if(dwAttr == INVALID_FILE_ATTRIBUTES)
            return;

        if((dwAttr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
            return;

        CString strPath(szPath);
        strPath.Replace(_T('/'), _T('\\'));
        if(strPath[strPath.GetLength() - 1] != _T('\\'))
            strPath += _T('\\');

        int nCount = m_arrListFiles.GetSize();
        for(int i=0; i<nCount; ++ i)
        {
            if(m_arrListFiles[i].CompareNoCase(szPath) == 0)
                return;
        }
        m_ListPaths.AddString(strPath);
    }

    static unsigned CALLBACK ThreadProc(void* pData)
    {
        CMainDlg* pThis = reinterpret_cast<CMainDlg*>(pData);

        return 0;
    }

    void EnableUI(BOOL bEnable)
    {
        GetDlgItem(IDC_BTN_ADD_FILES).EnableWindow(bEnable);
        GetDlgItem(IDC_BTN_REMOVE).EnableWindow(bEnable);
        GetDlgItem(IDC_LIST_SRC).EnableWindow(bEnable);
        GetDlgItem(IDC_COMBO_SOURCE).EnableWindow(bEnable);
        GetDlgItem(IDC_COMBO_TARGET).EnableWindow(bEnable);
        GetDlgItem(IDC_CHK_OVERWRITE).EnableWindow(bEnable);

        if(IsDlgButtonChecked(IDC_CHK_OVERWRITE) == BST_CHECKED)
        {
            GetDlgItem(IDC_EDIT_TARGET).EnableWindow(bEnable);
            GetDlgItem(IDC_BTN_BROWSE).EnableWindow(bEnable);
        }

        GetDlgItem(IDC_BTN_CONVERT).EnableWindow(bEnable);
    }

private:
    CWndLayout      m_WndLayout;
    CString         m_strAppName;

    HANDLE          m_hThread;
    ATL::CSimpleArray<CString> m_arrListFiles;

    CIconvWorker    m_Worker;
    CListBox        m_ListPaths;
    CComboBox       m_ComboSourceCodepage;
    CComboBox       m_ComboTargetCodepage;
};
