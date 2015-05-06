#include "StdAfx.h"
#include "IconvWorker.h"

#include "FileMap.h"

#include <cassert>
#include <Shlwapi.h>

BYTE g_byUtf8BOM[] = {0xEF, 0xBB, 0xBF};
BYTE g_byUnicodeBOM[] = {0xFF, 0xFE};


BOOL WriteFileHelper(HANDLE hFile, LPCVOID pBuffer, int nSize)
{
    DWORD dwWritten = 0;
    BOOL bResult = ::WriteFile(hFile, pBuffer, nSize, &dwWritten, NULL);
    if(!bResult)
        bResult = bResult;
    bResult = bResult && (dwWritten == nSize);
    if(!bResult)
        bResult = bResult;
    return bResult;
}

CIconvWorker::CIconvWorker(void)
{
    m_arrFiles = NULL;
    m_bStop = FALSE;
    m_strTargetPath = _T("");
    m_bOverwrite = FALSE;
    m_bWriteBom = TRUE;
    m_nSrcCodepage = CodeAuto;
    m_nDstCodepage = CodeAuto;
    m_pBuffer = NULL;
    m_nBufferSize = 0;
}

CIconvWorker::~CIconvWorker(void)
{
    ReleaseBuffer();
}

void CIconvWorker::SetCodepage(CodePageValue nSrcCodepage, CodePageValue nDstCodepage)
{
    m_nSrcCodepage = nSrcCodepage;
    m_nDstCodepage = nDstCodepage;
}

void CIconvWorker::SetFiles(ATL::CSimpleArray<CString>* arrFiles)
{
    m_arrFiles = arrFiles;
}

void CIconvWorker::SetOverwrite(BOOL bOverwrite)
{
    m_bOverwrite = bOverwrite;
}

void CIconvWorker::SetWriteBom(BOOL bWriteBom)
{
    m_bWriteBom = bWriteBom;
}

void CIconvWorker::SetTargetPath(LPCTSTR szTargetPath)
{
    m_strTargetPath = szTargetPath;
    m_strTargetPath.Replace(_T('/'), _T('\\'));
    if(m_strTargetPath.GetLength() > 0
        && m_strTargetPath[m_strTargetPath.GetLength() - 1] != _T('\\'))
    {
        m_strTargetPath += _T('\\');
    }
}

BOOL CIconvWorker::Convert(ATL::CSimpleArray<CString>* failedFiles, ATL::CSimpleArray<CString>* outFiles)
{
    m_bStop = FALSE;

    if(failedFiles)
        failedFiles->RemoveAll();
    if(outFiles)
        outFiles->RemoveAll();

    CString strSrcTemp = GetTempFilePath();
    int nCount = m_arrFiles->GetSize();
    for(int i=0; !m_bStop && i<nCount; ++ i)
    {
        CString strSrc = (*m_arrFiles)[i];

        // Get File Data
        CFileMap m;
        if(!m.MapFile(strSrc))
        {
            if(failedFiles)
                failedFiles->Add(strSrc);
            continue;
        }

        const BYTE * pData = static_cast<const BYTE *>(m.GetData());
        int nSize = m.GetSize();

        CodePageValue nSrcCodepage = m_nSrcCodepage;
        if(nSrcCodepage == CodeAuto)
            nSrcCodepage = GetFileCodepage(pData, nSize);
        else
            GetFileCodepage(pData, nSize);

        CString strDstPath;
        CodePageValue nDstCodepage = m_nDstCodepage;
        if(nSrcCodepage == nDstCodepage)
        {
            m.Close();
            if(!m_bOverwrite)
            {
                // Copy to dst
                strDstPath = GetDstPath(strSrc);
                if(CopyFile(strSrc, strDstPath, FALSE))
                {
                    if(outFiles)
                        outFiles->Add(strSrc);
                }
                else
                {
                    if(failedFiles)
                        failedFiles->Add(strSrc);
                }
            }
            continue;
        }

        for(;;)
        {
            BOOL bResult = FALSE;
            if((nSrcCodepage == CodeUnicode && nDstCodepage != CodeUnicode)
                || (nSrcCodepage != CodeUnicode && nDstCodepage == CodeUnicode))
            {
                if(!ConvFile(pData, nSize, strSrcTemp, nSrcCodepage, nDstCodepage))
                    break;

                // Copy to dst
                strDstPath = GetDstPath(strSrc);

                m.Close();
                if(!CopyFile(strSrcTemp, strDstPath, FALSE))
                    break;
                if(outFiles)
                    outFiles->Add(strDstPath);
                bResult = TRUE;
            }
            else
            {
                // nSrcCodepage != CodeUnicode && nDstCodepage != CodeUnicode
                // Convert to unicode first
                if(!ConvFile(pData, nSize, strSrcTemp, nSrcCodepage, CodeUnicode))
                    break;

                m.Close();

                // Convert to dst codepage
                CString strDstTemp = GetTempFilePath();
                CFileMap mTemp;
                if(!mTemp.MapFile(strSrcTemp))
                    break;

                pData = static_cast<const BYTE *>(mTemp.GetData());
                nSize = mTemp.GetSize();
                GetFileCodepage(pData, nSize);
                if(!ConvFile(pData, nSize, strDstTemp, CodeUnicode, nDstCodepage))
                    break;

                strDstPath = GetDstPath(strSrc);
                mTemp.Close();
                if(!CopyFile(strDstTemp, strDstPath, FALSE))
                    break;
                if(outFiles)
                    outFiles->Add(strDstPath);
                bResult = TRUE;
            }

            if(!bResult)
                failedFiles->Add(strSrc);

            break;
        }
    }

    return (failedFiles->GetSize() == 0);
}

void CIconvWorker::Stop()
{
    m_bStop = TRUE;
}

CString CIconvWorker::GetDstPath(LPCTSTR szSrcPath)
{
    if(m_bOverwrite)
        return szSrcPath;

    LPCTSTR szFileName = ::PathFindFileName(szSrcPath);
    CString strResult = m_strTargetPath + szFileName;
    return strResult;
}

CodePageValue CIconvWorker::GetFileCodepage(const BYTE *& pData, int& nSize)
{
    if(nSize >= 3 && memcmp(g_byUtf8BOM, pData, 3) == 0)
    {
        pData += 3;
        nSize -= 3;
        return CodeUtf8;
    }
    else if(nSize >= 2 && memcmp(g_byUnicodeBOM, pData, 2) == 0)
    {
        pData += 2;
        nSize -= 2;
        return CodeUnicode;
    }
    else
    {
        return CodeAnsi;
    }
}

CString CIconvWorker::GetTempFilePath()
{
    TCHAR szTmpPath[MAX_PATH];
    TCHAR szTmpFile[MAX_PATH];
    ::GetTempPath(MAX_PATH, szTmpPath);
    ::GetTempFileName(szTmpPath, _T("iconv"), rand(), szTmpFile);
    return szTmpFile;
}

BOOL CIconvWorker::ConvFile(const BYTE * pData, int nSize, LPCTSTR szDstPath, CodePageValue nSrcCodepage, CodePageValue nDstCodepage)
{
    BOOL bMultiByteToWideChar = FALSE;
    DWORD dwCodepage = GetRealCodepage(nSrcCodepage, nDstCodepage, bMultiByteToWideChar);
    if(dwCodepage == -1)
        return FALSE;

    LPBYTE pBuffer = NULL;
    int nBufferSize = 0;
    if(bMultiByteToWideChar)
    {
        nBufferSize = ::MultiByteToWideChar(dwCodepage,
            0,
            (LPCSTR)pData,
            nSize,
            NULL,
            0);
        if(nBufferSize <= 0)
            return (nSize == 0);

        pBuffer = GetBuffer(nBufferSize * 2 + 2);
        if(pBuffer == NULL)
            return FALSE;

        nBufferSize = ::MultiByteToWideChar(dwCodepage,
            0,
            (LPCSTR)pData,
            nSize,
            (LPWSTR)pBuffer,
            nBufferSize);
        if(nBufferSize == 0)
        {
            return FALSE;
        }
        nBufferSize = nBufferSize * 2;
    }
    else
    {
        nBufferSize = ::WideCharToMultiByte(dwCodepage,
            0,
            (LPCTSTR)pData,
            nSize / 2,
            0,
            0,
            0,
            0);
        if(nBufferSize == 0)
            return (nSize == 0);

        pBuffer = GetBuffer(nBufferSize + 1);
        if(::WideCharToMultiByte(dwCodepage,
            0,
            (LPCTSTR)pData,
            nSize / 2,
            (LPSTR)pBuffer,
            nBufferSize,
            0,
            0) == 0)
        {
            return FALSE;
        }
    }

    HANDLE hFile = ::CreateFile(szDstPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if(hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    BOOL bResult = FALSE;

    // Write BOM

    if(m_bWriteBom)
    {
        if(nDstCodepage == CodeUnicode)
        {
            bResult = WriteFileHelper(hFile, g_byUnicodeBOM, sizeof(g_byUnicodeBOM));
        }
        else if(nDstCodepage == CodeUtf8)
        {
            bResult = WriteFileHelper(hFile, g_byUtf8BOM, sizeof(g_byUtf8BOM));
        }
        else
        {
            bResult = TRUE;
        }
    }

    if(bResult)
    {
        // Write Content
        SetLastError(0);
        bResult = WriteFileHelper(hFile, pBuffer, nBufferSize);
        if(!bResult)
        {
            bResult = bResult;
        }
    }
    else
    {
        bResult = bResult;
    }

    ::CloseHandle(hFile);

    return bResult;
}

DWORD CIconvWorker::GetRealCodepage(CodePageValue nSrcCodepage, CodePageValue nDstCodepage, BOOL& bMultiByteToWideChar)
{
    DWORD dwCodepage = -1;
    assert(nSrcCodepage == CodeUnicode || nDstCodepage == CodeUnicode);
    bMultiByteToWideChar = (nDstCodepage == CodeUnicode);

    struct
    {
        CodePageValue codepage;
        DWORD dwCodepage;
    } data[] =
    {
        {CodeAnsi,      CP_ACP},
        {CodeUnicode,   CP_ACP},
        {CodeUtf8,      CP_UTF8},
        {CodeChinese,   936},
    };

    CodePageValue codepage = (nSrcCodepage == CodeUnicode) ? nDstCodepage : nSrcCodepage;
    for(int i=0; i<_countof(data); ++ i)
    {
        if(codepage == data[i].codepage)
            dwCodepage = data[i].dwCodepage;
    }
    return dwCodepage;
}

LPBYTE CIconvWorker::GetBuffer(int nSize)
{
    if(nSize <= m_nBufferSize)
        return m_pBuffer;
    ReleaseBuffer();

    m_pBuffer = static_cast<LPBYTE>(malloc(nSize));
    if(m_pBuffer)
        m_nBufferSize = nSize;
    return m_pBuffer;
}

void CIconvWorker::ReleaseBuffer()
{
    if(m_pBuffer)
    {
        free(m_pBuffer);
        m_pBuffer = NULL;
        m_nBufferSize = 0;
    }
}
