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
#include "OpticsProxy.h"

#if defined(FLARES_SUPPORT_EDITING)
AZStd::vector<FuncVariableGroup> COpticsProxy::GetEditorParamGroups()
{
    return AZStd::vector<FuncVariableGroup>();
}
#endif

COpticsProxy::COpticsProxy(const char* name)
    : m_bEnable(false)
    , m_name(name)
    , m_pOpticsReference(NULL)
{
}

void COpticsProxy::Load(IXmlNode* pNode)
{
    XmlNodeRef pProxy = pNode->findChild("Proxy");
    if (pProxy)
    {
        const char* referenceName(NULL);
        if (pProxy->getAttr("Reference", &referenceName))
        {
            if (referenceName && referenceName[0])
            {
                int nReferenceIndex(-1);
                if (gEnv->pOpticsManager->Load(referenceName, nReferenceIndex))
                {
                    IOpticsElementBase* pOptics = gEnv->pOpticsManager->GetOptics(nReferenceIndex);
                    if (pOptics->GetType() == eFT_Reference)
                    {
                        m_pOpticsReference = (COpticsReference*)gEnv->pOpticsManager->GetOptics(nReferenceIndex);
                    }
                }
            }
        }
    }
}

void COpticsProxy::GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const
{
}

void COpticsProxy::Invalidate()
{
}

void COpticsProxy::Render(SLensFlareRenderParam* pParam, const Vec3& vPos)
{
    if (m_pOpticsReference)
    {
        m_pOpticsReference->Render(pParam, vPos);
    }
}
