#pragma once

namespace Util
{

    // Enable drop for hWnd
    void EnableDrop(HWND hWnd);

    CStringA GetToken(const CStringA& strData, LPCSTR szPrefix, LPCSTR szPostfix);
    CStringA GetToken(const CStringA& strData, LPCSTR szPrefix, LPCSTR szPostfix, int& nStart);

    CString BrowserForFolder(IN HWND hWnd);
};
