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

// Description : Control D3D debug runtime output


#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_D3DDEBUG_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_D3DDEBUG_H
#pragma once


#if defined(SUPPORT_D3D_DEBUG_RUNTIME)

enum ESeverityCombination
{
    ESeverity_None = 0,
    ESeverity_Info,
    ESeverity_InfoWarning,
    ESeverity_InfoWarningError,
    ESeverity_All
};

enum
{
    MAX_NUM_DEBUG_MSG_IDS = 32
};

class CD3DDebug
{
public:
    CD3DDebug();
    ~CD3DDebug();

    bool Init(ID3D11Device* pD3DDevice);
    void Release();
    void Update(ESeverityCombination muteSeverity, const char* strMuteMsgList, const char* strBreakOnMsgList);

    // To use D3D debug info queue outside of this class,
    // you can push a copy of current settings or empty filter settings onto stack:
    //      m_d3dDebug.GetDebugInfoQueue()->PushCopyOfStorageFilter()  or   PushEmptyStorageFilter()
    //      .... change settings here
    // or push your filter settings directly
    //      m_d3dDebug.GetDebugInfoQueue()->PushStorageFilter(&myFilter);
    // Note that 'PopStorageFilter' should be called before next CD3DDebug::Update,
    // as CD3DDebug::Update modifies filter on top of stack
    ID3D11InfoQueue* GetDebugInfoQueue()
    {
        return m_pd3dDebugQueue;
    }

    string GetLastMessage();

private:
    ID3D11InfoQueue*    m_pd3dDebugQueue;

    D3D11_MESSAGE_ID    m_arrBreakOnIDsList[MAX_NUM_DEBUG_MSG_IDS];
    UINT                            m_nNumCurrBreakOnIDs;

    UINT ParseIDs(const char* strMsgIDList, D3D11_MESSAGE_ID arrMsgList[MAX_NUM_DEBUG_MSG_IDS]) const;
};

#endif // #if defined(SUPPORT_D3D_DEBUG_RUNTIME)

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_D3DDEBUG_H
