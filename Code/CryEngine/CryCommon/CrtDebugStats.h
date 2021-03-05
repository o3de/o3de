/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// support for leak dumping and statistics gathering using vs Crt Debug
// should be included in every DLL below DllMain()
#ifndef CRYINCLUDE_CRYCOMMON_CRTDEBUGSTATS_H
#define CRYINCLUDE_CRYCOMMON_CRTDEBUGSTATS_H
#pragma once

#ifdef WIN32
#ifdef _DEBUG


#include <ILog.h>
#include <ISystem.h>            // CryLogAlways
#include <crtdbg.h>

// copied from DBGINT.H (not a public header!)

#define nNoMansLandSize 4

typedef struct _CrtMemBlockHeader
{
    struct _CrtMemBlockHeader* pBlockHeaderNext;
    struct _CrtMemBlockHeader* pBlockHeaderPrev;
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
} _CrtMemBlockHeader;

struct SFileInfo
{
    int blocks;
    INT_PTR bytes;                                      //AMD Port
    SFileInfo(INT_PTR b) { blocks = 1; bytes = b; };    //AMD Port
};

_CrtMemState lastcheckpoint;
bool checkpointset = false;

extern "C" void __declspec(dllexport) CheckPoint()
{
    _CrtMemCheckpoint(&lastcheckpoint);
    checkpointset = true;
};

bool pairgreater(const std::pair<string, SFileInfo>& elem1, const std::pair<string, SFileInfo>& elem2)
{
    return elem1.second.bytes > elem2.second.bytes;
}

extern "C" void __declspec(dllexport) UsageSummary([[maybe_unused]] ILog * log, char* modulename, int* extras)
{
    _CrtMemState state;

    if (checkpointset)
    {
        _CrtMemState recent;
        _CrtMemCheckpoint(&recent);
        _CrtMemDifference(&state, &lastcheckpoint, &recent);
    }
    else
    {
        _CrtMemCheckpoint(&state);
    };

    INT_PTR numblocks = state.lCounts[_NORMAL_BLOCK];   //AMD Port
    INT_PTR totalalloc = state.lSizes[_NORMAL_BLOCK];   //AMD Port

    check_convert(extras[0]) = totalalloc;
    check_convert(extras[1]) = numblocks;

    CryLogAlways("$5---------------------------------------------------------------------------------------------------");

    if (!numblocks)
    {
        CryLogAlways("$3Module %s has no memory in use", modulename);
        return;
    }
    ;

    CryLogAlways("$5Usage summary for module %s", modulename);
    CryLogAlways("%d kbytes (peak %d) in %d objects of %d average bytes\n",
        totalalloc / 1024, state.lHighWaterCount / 1024, numblocks, numblocks ? totalalloc / numblocks : 0);
    CryLogAlways("%d kbytes allocated over time\n", state.lTotalCount / 1024);

    typedef std::map<string, SFileInfo> FileMap;
    FileMap fm;

    for (_CrtMemBlockHeader* h = state.pBlockHeader; h; h = h->pBlockHeaderNext)
    {
        if (_BLOCK_TYPE(h->nBlockUse) != _NORMAL_BLOCK)
        {
            continue;
        }
        string s = h->szFileName ? h->szFileName : "NO_SOURCE";
        if (h->nLine > 0)
        {
            char buf[16];
            sprintf_s(buf, "_%d", h->nLine);
            s += buf;
        }
        FileMap::iterator it = fm.find(s);
        if (it != fm.end())
        {
            (*it).second.blocks++;
            (*it).second.bytes += h->nDataSize;
        }
        else
        {
            fm.insert(FileMap::value_type(s, SFileInfo(h->nDataSize)));
        };
    }
    ;

    typedef std::vector< std::pair<string, SFileInfo> > FileVector;
    FileVector fv;
    for (FileMap::iterator it = fm.begin(); it != fm.end(); ++it)
    {
        fv.push_back((*it));
    }
    std::sort(fv.begin(), fv.end(), pairgreater);

    for (FileVector::iterator it = fv.begin(); it != fv.end(); ++it)
    {
        CryLogAlways("%6d kbytes / %6d blocks allocated from %s\n",
            (*it).second.bytes / 1024, (*it).second.blocks, (*it).first.c_str());
    }
    ;
};

#endif // _DEBUG

#if !defined(_RELEASE) && !defined(_DLL) && defined(HANDLE)
extern "C" HANDLE _crtheap;
extern "C" HANDLE __declspec(dllexport) GetDLLHeap() {
    return _crtheap;
};
#endif

#endif // WIN32

#endif // CRYINCLUDE_CRYCOMMON_CRTDEBUGSTATS_H
