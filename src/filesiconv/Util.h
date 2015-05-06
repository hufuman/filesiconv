#pragma once

namespace Util
{

    // Enable drop for hWnd
    void EnableDrop(HWND hWnd);

    CStringA GetToken(const CStringA& strData, LPCSTR szPrefix, LPCSTR szPostfix);
    CStringA GetToken(const CStringA& strData, LPCSTR szPrefix, LPCSTR szPostfix, int& nStart);

    BOOL BrowseForFiles(ATL::CSimpleArray<CString>& listFiles, HWND hWnd);
    CString BrowseForFolder(IN HWND hWnd);

    BOOL IsFolder(LPCTSTR szFolder);
    BOOL IsFile(LPCTSTR szFile);
};
