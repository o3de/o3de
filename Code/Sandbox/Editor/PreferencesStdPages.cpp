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

#include "EditorDefs.h"

#include "PreferencesStdPages.h"

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

// Editor
#include "EditorPreferencesPageGeneral.h"
#include "EditorPreferencesPageFiles.h"
#include "EditorPreferencesPageViewportGeneral.h"
#include "EditorPreferencesPageViewportGizmo.h"
#include "EditorPreferencesPageViewportMovement.h"
#include "EditorPreferencesPageViewportDebug.h"
#include "EditorPreferencesPageExperimentalLighting.h"
#include "EditorPreferencesPageAWS.h"


//////////////////////////////////////////////////////////////////////////
// Implementation of ClassDesc for standard Editor preferences.
//////////////////////////////////////////////////////////////////////////

CStdPreferencesClassDesc::CStdPreferencesClassDesc()
    : m_refCount(0)
{
    m_pageCreators = {
        [](){ return new CEditorPreferencesPage_General(); },
        [](){ return new CEditorPreferencesPage_Files(); },
        [](){ return new CEditorPreferencesPage_ViewportGeneral(); },
        [](){ return new CEditorPreferencesPage_ViewportMovement(); },
        [](){ return new CEditorPreferencesPage_ViewportGizmo(); },
        [](){ return new CEditorPreferencesPage_ViewportDebug(); }
    };

    m_pageCreators.push_back([]() { return new CEditorPreferencesPage_ExperimentalLighting(); });

    if (AzToolsFramework::IsComponentWithServiceRegistered(AZ_CRC_CE("AWSCoreEditorService")))
    {
        m_pageCreators.push_back([]() { return new CEditorPreferencesPage_AWS(); });
    }
}

HRESULT CStdPreferencesClassDesc::QueryInterface(const IID& riid, void** ppvObj)
{
    if (riid == __uuidof(IPreferencesPageCreator))
    {
        *ppvObj = (IPreferencesPageCreator*)this;
        return S_OK;
    }
    return E_NOINTERFACE;
}

//////////////////////////////////////////////////////////////////////////
ULONG CStdPreferencesClassDesc::AddRef()
{
    m_refCount++;
    return m_refCount;
};

//////////////////////////////////////////////////////////////////////////
ULONG CStdPreferencesClassDesc::Release()
{
    ULONG refs = --m_refCount;
    if (m_refCount <= 0)
    {
        delete this;
    }
    return refs;
}

//////////////////////////////////////////////////////////////////////////
REFGUID CStdPreferencesClassDesc::ClassID()
{
    // {95FE3251-796C-4e3b-82F0-AD35F7FFA267}
    static const GUID guid = {
        0x95fe3251, 0x796c, 0x4e3b, { 0x82, 0xf0, 0xad, 0x35, 0xf7, 0xff, 0xa2, 0x67 }
    };
    return guid;
}

//////////////////////////////////////////////////////////////////////////
int CStdPreferencesClassDesc::GetPagesCount()
{
    return m_pageCreators.size();
}

IPreferencesPage* CStdPreferencesClassDesc::CreateEditorPreferencesPage(int index)
{
    return (index >= m_pageCreators.size()) ? nullptr : m_pageCreators[index]();
}

