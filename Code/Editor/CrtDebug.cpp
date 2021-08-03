/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//#ifdef _CRTDBG_MAP_ALLOC
#ifdef CRTDBG_MAP_ALLOC
#pragma pack (push,1)
#define nNoMansLandSize 4
typedef struct MyCrtMemBlockHeader
{
    struct MyCrtMemBlockHeader* pBlockHeaderNext;
    struct MyCrtMemBlockHeader* pBlockHeaderPrev;
    char*                      szFileName;
    int                         nLine;
    size_t                      nDataSize;
    int                         nBlockUse;
    long                        lRequest;
    unsigned char               gap[nNoMansLandSize];
    /* followed by:
    *  unsigned char           data[nDataSize];
    *  unsigned char           anotherGap[nNoMansLandSize];
    */
} MyCrtMemBlockHeader;
#pragma pack (pop)

#define pbData(pblock) ((unsigned char*)((MyCrtMemBlockHeader*)pblock + 1))
#define pHdr(pbData) (((MyCrtMemBlockHeader*)pbData) - 1)


void crtdebug(const char* s, ...)
{
    char str[32768];
    va_list arg_ptr;
    va_start(arg_ptr, s);
    vsprintf(str, s, arg_ptr);
    va_end(arg_ptr);

    FILE* l = nullptr;
    azfopen(&l, "crtdump.txt", "a+t");
    if (l)
    {
        fprintf(l, "%s", str);
        fclose(l);
    }
}

int crtAllocHook(int nAllocType, void* pvData,
    size_t nSize, int nBlockUse, long lRequest,
    const unsigned char* szFileName, int nLine)
{
    if (nBlockUse == _CRT_BLOCK)
    {
        return(TRUE);
    }

    static int total_cnt = 0;
    static int total_mem = 0;
    if (nAllocType == _HOOK_ALLOC)
    {
        //total_mem += nSize;
        //total_cnt++;
        //_CrtMemState mem_state;
        //_CrtMemCheckpoint( &mem_state );
        //total_cnt = mem_state.lCounts[_NORMAL_BLOCK];
        //total_mem = mem_state.lTotalCount;
        if ((total_cnt & 0xF) == 0)
        {
            //_CrtCheckMemory();
        }

        total_cnt++;
        total_mem += nSize;

        //crtdebug( "<CRT> Alloc %d,size=%d,in: %s %d (total size=%d,num=%d)\n",lRequest,nSize,szFileName,nLine,total_mem,total_cnt );
        crtdebug("Size=%d,  [Total=%d,N=%d] [%s:%d]\n", nSize, total_mem, total_cnt, szFileName, nLine);
    }
    else if (nAllocType == _HOOK_FREE)
    {
        MyCrtMemBlockHeader* pHead;
        pHead = pHdr(pvData);

        total_cnt--;
        total_mem -= pHead->nDataSize;

        crtdebug("Size=%d,  [Total=%d,N=%d] [%s:%d]\n", pHead->nDataSize, total_mem, total_cnt, pHead->szFileName, pHead->nLine);
        //crtdebug( "<CRT> Free size=%d,in: %s %d (total size=%d,num=%d)\n",pHead->nDataSize,pHead->szFileName,pHead->nLine,total_mem,total_cnt );
        //total_mem -= nSize;
        //total_cnt--;
    }
    return TRUE;
}

int crtReportHook(int nRptType, char* szMsg, int* retVal)
{
    static int gl_num_asserts = 0;
    if (gl_num_asserts != 0)
    {
        return TRUE;
    }
    gl_num_asserts++;
    switch (nRptType)
    {
    case _CRT_WARN:
        crtdebug("<CRT WARNING> %s\n", szMsg);
        break;
    case _CRT_ERROR:
        crtdebug("<CRT ERROR> %s\n", szMsg);
        break;
    case _CRT_ASSERT:
        crtdebug("<CRT ASSERT> %s\n", szMsg);
        break;
    }
    gl_num_asserts--;
    return TRUE;
}

void InitCrt()
{
    FILE* l = nullptr;
    azfopen(&l, "crtdump.txt", "w");
    if (l)
    {
        fclose(l);
    }

    //_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG );
    //_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
    //_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_DEBUG );

    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_WNDW);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_WNDW);

    //_CrtSetDbgFlag( _CRTDBG_CHECK_ALWAYS_DF|_CRTDBG_CHECK_CRT_DF|_CRTDBG_LEAK_CHECK_DF|_CRTDBG_DELAY_FREE_MEM_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) );
    //_CrtSetDbgFlag( _CRTDBG_CHECK_CRT_DF|_CRTDBG_LEAK_CHECK_DF/*|_CRTDBG_DELAY_FREE_MEM_DF*/ | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) );
    int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    flags &= ~_CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_CRT_DF;

    _CrtSetDbgFlag(flags);

    _CrtSetAllocHook (crtAllocHook);
    _CrtSetReportHook(crtReportHook);
}

void DoneCrt()
{
    //_CrtCheckMemory();
    //_CrtDumpMemoryLeaks();
}

// Autoinit CRT.
//struct __autoinit_crt { __autoinit_crt() { InitCrt(); }; ~__autoinit_crt() { DoneCrt(); } } __autoinit_crt_var;
#endif


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
