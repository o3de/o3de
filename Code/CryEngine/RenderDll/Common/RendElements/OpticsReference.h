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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_OPTICSREFERENCE_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_OPTICSREFERENCE_H
#pragma once

#include "IFlares.h"

class COpticsReference
    : public IOpticsElementBase
{
public:

#if defined(FLARES_SUPPORT_EDITING)
    AZStd::vector<FuncVariableGroup> GetEditorParamGroups();
#endif

    COpticsReference(const char* name);
    ~COpticsReference(){}

    EFlareType GetType()    {       return eFT_Reference;   }
    bool IsGroup() const    {       return false;   }

    string GetName() const  {   return m_name;  }
    void SetName(const char* ch_name)   {   m_name = ch_name; }
    void Load(IXmlNode* pNode);

    IOpticsElementBase* GetParent() const   {   return NULL;    }

    bool IsEnabled() const  {       return true;    }
    void SetEnabled([[maybe_unused]] bool b) {}
    void AddElement(IOpticsElementBase* pElement);
    void InsertElement(int nPos, IOpticsElementBase* pElement);
    void Remove(int i);
    void RemoveAll();
    int GetElementCount() const;
    IOpticsElementBase* GetElementAt(int  i) const;

    void GetMemoryUsage(ICrySizer* pSizer) const;
    void Invalidate();

    void Render(SLensFlareRenderParam* pParam, const Vec3& vPos);

public:
    string m_name;
    std::vector<IOpticsElementBasePtr> m_OpticsList;
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_OPTICSREFERENCE_H
