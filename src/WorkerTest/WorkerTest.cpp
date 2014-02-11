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
        /* 00 */ {_T("ansi_enUS.txt"),       _T("unicode_enUS.txt"),     CodeAuto, CodeUnicode},
        /* 01 */ {_T("ansi_enUS.txt"),       _T("utf8_enUS.txt"),        CodeAuto, CodeUtf8},
        /* 02 */ {_T("unicode_enUS.txt"),    _T("ansi_enUS.txt"),        CodeAuto, CodeAnsi},
        /* 03 */ {_T("unicode_enUS.txt"),    _T("utf8_enUS.txt"),        CodeAuto, CodeUtf8},
        /* 04 */ {_T("utf8_enUS.txt"),       _T("ansi_enUS.txt"),        CodeAuto, CodeAnsi},
        /* 05 */ {_T("utf8_enUS.txt"),       _T("unicode_enUS.txt"),     CodeAuto, CodeUnicode},

        /* 06 */ {_T("unicode.txt"),         _T("chinese.txt"),          CodeAuto, CodeChinese},
        /* 07 */ {_T("unicode.txt"),         _T("utf8.txt"),             CodeAuto, CodeUtf8},
        /* 08 */ {_T("utf8.txt"),            _T("chinese.txt"),          CodeAuto, CodeChinese},
        /* 09 */ {_T("utf8.txt"),            _T("unicode.txt"),          CodeAuto, CodeUnicode},

        // multi bytes to wide chars
        /* 10 */ {_T("ansi_enUS.txt"),       _T("unicode_enUS.txt"),     CodeAnsi, CodeUnicode},
        /* 11 */ {_T("utf8_enUS.txt"),       _T("unicode_enUS.txt"),     CodeUtf8, CodeUnicode},
        /* 12 */ {_T("chinese.txt"),         _T("unicode.txt"),          CodeChinese, CodeUnicode},
        /* 13 */ {_T("utf8.txt"),            _T("unicode.txt"),          CodeUtf8, CodeUnicode},

        // wide chars to multi bytes
        /* 14 */ {_T("unicode_enUS.txt"),    _T("ansi_enUS.txt"),        CodeUnicode, CodeAnsi},
        /* 15 */ {_T("unicode_enUS.txt"),    _T("utf8_enUS.txt"),        CodeUnicode, CodeUtf8},
        /* 16 */ {_T("unicode.txt"),         _T("chinese.txt"),          CodeUnicode, CodeChinese},
        /* 17 */ {_T("unicode.txt"),         _T("utf8.txt"),             CodeUnicode, CodeUtf8},

        // multi bytes to multi bytes
        /* 18 */ {_T("chinese.txt"),         _T("utf8.txt"),             CodeChinese, CodeUtf8},
        /* 19 */ {_T("utf8.txt"),            _T("chinese.txt"),          CodeUtf8, CodeChinese},
        /* 20 */ {_T("ansi_enUS.txt"),       _T("utf8_enUS.txt"),        CodeAnsi, CodeUtf8},
        /* 21 */ {_T("utf8_enUS.txt"),       _T("ansi_enUS.txt"),        CodeUtf8, CodeAnsi},

        // auto same to same
        /* 22 */ {_T("ansi_enUS.txt"),       _T("ansi_enUS.txt"),        CodeAuto, CodeAnsi},
        /* 23 */ {_T("unicode.txt"),         _T("unicode.txt"),          CodeAuto, CodeUnicode},
        /* 24 */ {_T("unicode_enUS.txt"),    _T("unicode_enUS.txt"),     CodeAuto, CodeUnicode},
        /* 25 */ {_T("utf8.txt"),            _T("utf8.txt"),             CodeAuto, CodeUtf8},
        /* 26 */ {_T("utf8_enUS.txt"),       _T("utf8_enUS.txt"),        CodeAuto, CodeUtf8},

        // not auto same to same
        /* 27 */ {_T("ansi_enUS.txt"),       _T("ansi_enUS.txt"),        CodeAnsi, CodeAnsi},
        /* 28 */ {_T("chinese.txt"),         _T("chinese.txt"),          CodeChinese, CodeChinese},
        /* 29 */ {_T("unicode.txt"),         _T("unicode.txt"),          CodeUnicode, CodeUnicode},
        /* 30 */ {_T("unicode_enUS.txt"),    _T("unicode_enUS.txt"),     CodeUnicode, CodeUnicode},
        /* 31 */ {_T("utf8.txt"),            _T("utf8.txt"),             CodeUtf8, CodeUtf8},
        /* 32 */ {_T("utf8_enUS.txt"),       _T("utf8_enUS.txt"),        CodeUtf8, CodeUtf8},
    };

    CString strTempFile = CIconvWorker::GetTempFilePath();
    CString strOutputPath = strTestCasePath + _T("tmp\\");

    BOOL bHasError = FALSE;
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
        if(i == 23)
        {
            int a = 0;
        }
        worker.Convert(&failedFiles, &outFiles);
        int nFailedCount = failedFiles.GetSize();
        if(nFailedCount == 0)
        {
            BOOL bEqual = CompareFile(strCorrectFile, outFiles[0]);
            if(!bEqual)
            {
                bHasError = TRUE;
                _tprintf(_T("TestCase[%d] failed: \r\n"), i);
                _tprintf(_T("\tFiles are not same\r\n"));
            }
            continue;
        }

        bHasError = TRUE;
        _tprintf(_T("TestCase[%d] failed: \r\n"), i);
        for(int j=0; j<nFailedCount; ++ j)
        {
            _tprintf(_T("\tFile[%d] %s\r\n"), j, failedFiles[j]);
        }
        _tprintf(_T("\r\n\r\n"));
    }

    if(!bHasError)
    {
        _tprintf(_T("All Tests Passed\r\n"));
    }

	return nRetCode;
}
