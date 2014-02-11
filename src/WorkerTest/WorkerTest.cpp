// WorkerTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "WorkerTest.h"

#include "../filesiconv/FileMap.h"
#include "../filesiconv/IconvWorker.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;


CString GetTestCasesPath()
{
    TCHAR szBuffer[1024];
    ::GetModuleFileName(NULL, szBuffer, _countof(szBuffer));

    *_tcsrchr(szBuffer, _T('\\')) = 0;

    CString strPath(szBuffer);
    strPath += _T("\\testcases\\");
    if(::GetFileAttributes(strPath) != INVALID_FILE_ATTRIBUTES)
        return strPath;

    *(_tcsrchr(szBuffer, _T('\\'))) = 0;
    *(_tcsrchr(szBuffer, _T('\\'))) = 0;
    strPath  = szBuffer;
    strPath += _T("\\testcases\\");
    if(::GetFileAttributes(strPath) != INVALID_FILE_ATTRIBUTES)
        return strPath;

    return _T("");
}

BOOL CompareFile(LPCTSTR szSrc, LPCTSTR szDst)
{
    BOOL bResult = FALSE;
    CFileMap mSrc, mDst;
    for(;;)
    {
        if(!mSrc.MapFile(szSrc))
            break;

        if(!mDst.MapFile(szDst))
            break;

        LPCVOID pSrcData = mSrc.GetData();
        int nSrcSize = mSrc.GetSize();

        LPCVOID pDstData = mDst.GetData();
        int nDstSize = mDst.GetSize();

        if(nSrcSize != nDstSize)
            break;

        bResult = (memcmp(pSrcData, pDstData, nSrcSize) == 0);
        if(!bResult)
            break;
        break;
    }
    return bResult;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
        return 1;
	}

    CString strTestCasePath = GetTestCasesPath();
    if(strTestCasePath.IsEmpty())
    {
        _tprintf(_T("Fatal Error: Not Found testcases directory\n"));
        return 1;
    }

    struct stTestCases
    {
        LPCTSTR szSrcFile;
        LPCTSTR szCorrectFile;
        CodePageValue srcCode;
        CodePageValue dstCode;
    } testcases[] =
    {
        // auto
        {_T("ansi_enUS.txt"),       _T("unicode_enUS.txt"),     CodeAuto, CodeUnicode},
        {_T("ansi_enUS.txt"),       _T("utf8_enUS.txt"),        CodeAuto, CodeUtf8},
        {_T("unicode_enUS.txt"),    _T("ansi_enUS.txt"),        CodeAuto, CodeAnsi},
        {_T("unicode_enUS.txt"),    _T("utf8_enUS.txt"),        CodeAuto, CodeUtf8},
        {_T("utf8_enUS.txt"),       _T("ansi_enUS.txt"),        CodeAuto, CodeAnsi},
        {_T("utf8_enUS.txt"),       _T("unicode_enUS.txt"),     CodeAuto, CodeUnicode},

        {_T("unicode.txt"),         _T("chinese.txt"),          CodeAuto, CodeChinese},
        {_T("unicode.txt"),         _T("utf8.txt"),             CodeAuto, CodeUtf8},
        {_T("utf8.txt"),            _T("chinese.txt"),          CodeAuto, CodeChinese},
        {_T("utf8.txt"),            _T("unicode.txt"),          CodeAuto, CodeUnicode},

        // multi bytes to wide chars
        {_T("ansi_enUS.txt"),       _T("unicode_enUS.txt"),     CodeAnsi, CodeUnicode},
        {_T("utf8_enUS.txt"),       _T("unicode_enUS.txt"),     CodeUtf8, CodeUnicode},
        {_T("chinese.txt"),         _T("unicode.txt"),          CodeChinese, CodeUnicode},
        {_T("utf8.txt"),            _T("unicode.txt"),          CodeUtf8, CodeUnicode},

        // wide chars to multi bytes
        {_T("unicode_enUS.txt"),    _T("ansi_enUS.txt"),        CodeUnicode, CodeAnsi},
        {_T("unicode_enUS.txt"),    _T("utf8_enUS.txt"),        CodeUnicode, CodeUtf8},
        {_T("unicode.txt"),         _T("chinese.txt"),          CodeUnicode, CodeChinese},
        {_T("unicode.txt"),         _T("utf8.txt"),             CodeUnicode, CodeUtf8},

        // multi bytes to multi bytes
        {_T("chinese.txt"),         _T("utf8.txt"),             CodeChinese, CodeUtf8},
        {_T("utf8.txt"),            _T("chinese.txt"),          CodeUtf8, CodeChinese},
        {_T("ansi_enUS.txt"),       _T("utf8_enUS.txt"),        CodeAnsi, CodeUtf8},
        {_T("utf8_enUS.txt"),       _T("ansi_enUS.txt"),        CodeUtf8, CodeAnsi},
    };

    CString strTempFile = CIconvWorker::GetTempFilePath();
    CString strOutputPath = strTestCasePath + _T("tmp\\");

    int nFailedCount = 0;
    for(int i=0; i<_countof(testcases); ++ i)
    {
        const stTestCases& test = testcases[i];
        CString strSrcFile = strTestCasePath + test.szSrcFile;
        CString strCorrectFile = strTestCasePath + test.szCorrectFile;

        CIconvWorker worker;

        ATL::CSimpleArray<CString> arrFiles;
        arrFiles.Add(strSrcFile);
        worker.SetFiles(&arrFiles);
        worker.SetOverwrite(FALSE);
        worker.SetTargetPath(strOutputPath);

        worker.SetCodepage(test.srcCode, test.dstCode);

        ATL::CSimpleArray<CString> failedFiles;
        ATL::CSimpleArray<CString> outFiles;
        worker.Convert(&failedFiles, &outFiles);
        int nFailedCount = failedFiles.GetSize();
        if(nFailedCount == 0)
        {
            BOOL bEqual = CompareFile(strCorrectFile, outFiles[0]);
            if(!bEqual)
            {
                ++ nFailedCount;
                _tprintf(_T("Files are not same\r\n"));
            }
            continue;
        }

        ++ nFailedCount;
        _tprintf(_T("TestCase[%d] failed: \r\n"), i);
        for(int j=0; j<nFailedCount; ++ j)
        {
            _tprintf(_T("\tFile[%d] %s\r\n"), j, failedFiles[j]);
        }
        _tprintf(_T("\r\n\r\n"));
    }

    if(nFailedCount == 0)
    {
        _tprintf(_T("All Tests Passed\r\n"));
    }

	return nRetCode;
}
