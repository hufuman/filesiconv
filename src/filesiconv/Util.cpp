#include "StdAfx.h"
#include "Util.h"

#include <ShlObj.h>
#include <ShellAPI.h>

namespace Util
{
    namespace
    {
        // Allow/disallow window of low right to send message to window of high right.
        BOOL FilterWindowMessage(UINT message, DWORD dwValue)
        {
            typedef BOOL (WINAPI FAR *ChangeWindowMessageFilter_PROC)(UINT,DWORD);

            static ChangeWindowMessageFilter_PROC pfnChangeWindowMessageFilter
                = (ChangeWindowMessageFilter_PROC)::GetProcAddress (::GetModuleHandle(_T("USER32")), "ChangeWindowMessageFilter");

            BOOL bResult = TRUE;
            if(pfnChangeWindowMessageFilter != NULL)
                bResult = pfnChangeWindowMessageFilter(message, dwValue);
            return bResult;
        }
    }
    void EnableDrop(HWND hWnd)
    {
        ::DragAcceptFiles(hWnd, TRUE);

        FilterWindowMessage(0x0049 /*WM_COPYGLOBALDATA*/, 1);
        FilterWindowMessage(WM_DROPFILES, 1);
    }

    CStringA GetToken(const CStringA& strData, LPCSTR szPrefix, LPCSTR szPostfix)
    {
        int nStart = 0;
        return GetToken(strData, szPrefix, szPostfix, nStart);
    }

    CStringA GetToken(const CStringA& strData, LPCSTR szPrefix, LPCSTR szPostfix, int& nStart)
    {
        CStringA strResult;
        if(nStart < 0)
        {
            AtlThrow(E_INVALIDARG);
            return strResult;
        }

        size_t nPrefixLen = strlen(szPrefix);
        size_t nPostfixLen = strlen(szPostfix);

        int begin = strData.Find(szPrefix, nStart);
        if(begin == -1)
            return strResult;

        nStart = begin + nPrefixLen;
        int end = strData.Find(szPostfix, nStart);
        if(end == -1)
            return strResult;

        strResult = strData.Mid(begin + nPrefixLen, end - begin - nPrefixLen);
        return strResult;
    }

    CString BrowserForFolder(IN HWND hWnd)
    {
        CoInitialize(NULL);
        BROWSEINFO _info = {0};
        _info.hwndOwner = hWnd;
        _info.ulFlags = BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON | BIF_RETURNONLYFSDIRS | BIF_EDITBOX;
        PIDLIST_ABSOLUTE idlResult = SHBrowseForFolder(&_info);
        if(idlResult == NULL)
        {
            CoUninitialize();
            return _T("");
        }

        CString strResult;
        TCHAR szFilePath[MAX_PATH] = {0};
        if(SHGetPathFromIDList(idlResult, szFilePath))
        {
            strResult = szFilePath;
        }
        CoUninitialize();

        return strResult;
    }
};
