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

#ifndef CRYINCLUDE_EDITOR_PREFERENCESSTDPAGES_H
#define CRYINCLUDE_EDITOR_PREFERENCESSTDPAGES_H
#pragma once

#include "Include/IPreferencesPage.h"
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/vector.h>

//////////////////////////////////////////////////////////////////////////
class CStdPreferencesClassDesc
    : public IPreferencesPageClassDesc
    , public IPreferencesPageCreator
{
    ULONG m_refCount;
    AZStd::vector< AZStd::function< IPreferencesPage*() > > m_pageCreators;
public:
    CStdPreferencesClassDesc();
    virtual ~CStdPreferencesClassDesc() = default;

    //////////////////////////////////////////////////////////////////////////
    // IUnkown implementation.
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(const IID& riid, void** ppvObj);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
    //////////////////////////////////////////////////////////////////////////

    virtual REFGUID ClassID();

    //////////////////////////////////////////////////////////////////////////
    virtual int GetPagesCount();
    virtual IPreferencesPage* CreateEditorPreferencesPage(int index) override;
};

#endif // CRYINCLUDE_EDITOR_PREFERENCESSTDPAGES_H

