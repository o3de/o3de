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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_OPTICSPROXY_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_OPTICSPROXY_H
#pragma once

#include "OpticsReference.h"

class COpticsProxy
    : public IOpticsElementBase
{
public:

    COpticsProxy(const char* name);
    ~COpticsProxy(){}

    EFlareType GetType()    {       return eFT_Proxy;   }
    bool IsGroup() const    {       return false;   }

    string GetName() const  {   return m_name;  }
    void SetName(const char* ch_name)   {   m_name = ch_name;   }
    void Load(IXmlNode* pNode);

    IOpticsElementBase* GetParent() const   {   return NULL;    }

    bool IsEnabled() const  {   return m_bEnable;   }
    void SetEnabled(bool b) { m_bEnable = b;        }

    void AddElement([[maybe_unused]] IOpticsElementBase* pElement)   {}
    void InsertElement([[maybe_unused]] int nPos, [[maybe_unused]] IOpticsElementBase* pElement) {}
    void Remove([[maybe_unused]] int i)    {}
    void RemoveAll() {}
    int GetElementCount() const {   return 0;   }
    IOpticsElementBase* GetElementAt([[maybe_unused]] int  i) const    {   return NULL;    }

    void GetMemoryUsage(ICrySizer* pSizer) const;
    void Invalidate();

    void Render(SLensFlareRenderParam* pParam, const Vec3& vPos);

    void SetOpticsReference(IOpticsElementBase* pReference)
    {
        if (pReference->GetType() == eFT_Reference)
        {
            m_pOpticsReference = (COpticsReference*)pReference;
        }
    }
    IOpticsElementBase* GetOpticsReference() const
    {
        return m_pOpticsReference;
    }
#if defined(FLARES_SUPPORT_EDITING)
    AZStd::vector<FuncVariableGroup> GetEditorParamGroups();
#endif

public:

    bool m_bEnable;
    string m_name;
    _smart_ptr<COpticsReference> m_pOpticsReference;
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_OPTICSPROXY_H
