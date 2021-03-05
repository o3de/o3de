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

#include "RenderDll_precompiled.h"
#include "OpticsReference.h"

#if defined(FLARES_SUPPORT_EDITING)
AZStd::vector<FuncVariableGroup> COpticsReference::GetEditorParamGroups()
{
    return AZStd::vector<FuncVariableGroup>();
}
#endif

COpticsReference::COpticsReference(const char* name)
    : m_name(name)
{
}

void COpticsReference::Load([[maybe_unused]] IXmlNode* pNode)
{
}

void COpticsReference::AddElement(IOpticsElementBase* pElement)
{
    m_OpticsList.push_back(pElement);
}

void COpticsReference::InsertElement(int nPos, IOpticsElementBase* pElement)
{
    if (nPos < 0 || nPos >= (int)m_OpticsList.size())
    {
        return;
    }
    m_OpticsList.insert(m_OpticsList.begin() + nPos, pElement);
}

void COpticsReference::Remove(int i)
{
    if (i < 0 || i >= (int)m_OpticsList.size())
    {
        return;
    }
    m_OpticsList.erase(m_OpticsList.begin() + i);
}

void COpticsReference::RemoveAll()
{
    m_OpticsList.clear();
}

int COpticsReference::GetElementCount() const
{
    return m_OpticsList.size();
}

IOpticsElementBase* COpticsReference::GetElementAt(int  i) const
{
    if (i < 0 || i >= (int)m_OpticsList.size())
    {
        return NULL;
    }
    return m_OpticsList[i];
}

void COpticsReference::GetMemoryUsage(ICrySizer* pSizer) const
{
    for (int i = 0, iSize(m_OpticsList.size()); i < iSize; ++i)
    {
        m_OpticsList[i]->GetMemoryUsage(pSizer);
    }
}

void COpticsReference::Invalidate()
{
    for (int i = 0, iSize(m_OpticsList.size()); i < iSize; ++i)
    {
        m_OpticsList[i]->Invalidate();
    }
}

void COpticsReference::Render(SLensFlareRenderParam* pParam, const Vec3& vPos)
{
    for (int i = 0, iSize(m_OpticsList.size()); i < iSize; ++i)
    {
        m_OpticsList[i]->Render(pParam, vPos);
    }
}
