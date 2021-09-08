/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "PreferencesStdPages.h"

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

// Editor
#include "EditorPreferencesPageGeneral.h"
#include "EditorPreferencesPageFiles.h"
#include "EditorPreferencesPageViewportGeneral.h"
#include "EditorPreferencesPageViewportManipulator.h"
#include "EditorPreferencesPageViewportCamera.h"
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
        [](){ return new CEditorPreferencesPage_ViewportCamera(); },
        [](){ return new CEditorPreferencesPage_ViewportManipulator(); },
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
    return static_cast<int>(m_pageCreators.size());
}

IPreferencesPage* CStdPreferencesClassDesc::CreateEditorPreferencesPage(int index)
{
    return (index >= m_pageCreators.size()) ? nullptr : m_pageCreators[index]();
}

