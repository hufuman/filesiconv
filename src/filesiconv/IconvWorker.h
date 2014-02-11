#pragma once


enum CodePageValue
{
    CodeAuto,
    CodeAnsi,
    CodeUnicode,
    CodeUtf8,
    CodeChinese,
};

class CIconvWorker
{
public:
    CIconvWorker(void);
    ~CIconvWorker(void);

    void SetCodepage(CodePageValue nSrcCodepage, CodePageValue nDstCodepage);
    void SetFiles(ATL::CSimpleArray<CString>* arrFiles);
    void SetOverwrite(BOOL bOverwrite);
    void SetTargetPath(LPCTSTR szTargetPath);
    BOOL Convert(ATL::CSimpleArray<CString>* failedFiles, ATL::CSimpleArray<CString>* outFiles);

    void Stop();

    static CString GetTempFilePath();

private:
    CString GetDstPath(LPCTSTR szSrcPath);
    CodePageValue GetFileCodepage(const BYTE *& pData, int& nSize);
    BOOL    ConvFile(const BYTE *pData, int nSize, LPCTSTR szDstPath, CodePageValue nSrcCodepage, CodePageValue nDstCodepage);
    DWORD   GetRealCodepage(CodePageValue nSrcCodepage, CodePageValue nDstCodepage, BOOL& bMultiByteToWideChar);

    LPBYTE  GetBuffer(int nSize);
    void    ReleaseBuffer();

private:
    ATL::CSimpleArray<CString>*  m_arrFiles;

    BOOL        m_bStop;
    CString     m_strTargetPath;
    BOOL        m_bOverwrite;

    CodePageValue m_nSrcCodepage;
    CodePageValue m_nDstCodepage;

    LPBYTE      m_pBuffer;
    int         m_nBufferSize;
};
