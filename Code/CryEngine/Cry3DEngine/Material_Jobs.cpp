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

// Description : Material Manager Implementation


#include "Cry3DEngine_precompiled.h"
#include "MatMan.h"
#include "3dEngine.h"
#include "ObjMan.h"
#include "IRenderer.h"
#include "SurfaceTypeManager.h"
#include "CGFContent.h"
#include <IResourceManager.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CMatMan::GetDefaultMaterial()
{
    return m_pDefaultMtl;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CMatInfo::GetSafeSubMtl(int nSubMtlSlot)
{
    if (m_subMtls.empty() || !(m_Flags & MTL_FLAG_MULTI_SUBMTL))
    {
        return this; // Not Multi material.
    }
    if (nSubMtlSlot >= 0 && nSubMtlSlot < (int)m_subMtls.size() && m_subMtls[nSubMtlSlot] != NULL)
    {
        return m_subMtls[nSubMtlSlot];
    }
    else
    {
        return GetMatMan()->GetDefaultMaterial();
    }
}

///////////////////////////////////////////////////////////////////////////////
SShaderItem& CMatInfo::GetShaderItem()
{
    return m_shaderItem;
}


///////////////////////////////////////////////////////////////////////////////
const SShaderItem& CMatInfo::GetShaderItem() const
{
    return m_shaderItem;
}

//////////////////////////////////////////////////////////////////////////
SShaderItem& CMatInfo::GetShaderItem(int nSubMtlSlot)
{
    SShaderItem* pShaderItem = NULL;
    if (m_subMtls.empty() || !(m_Flags & MTL_FLAG_MULTI_SUBMTL))
    {
        pShaderItem = &m_shaderItem; // Not Multi material.
    }
    else if (nSubMtlSlot >= 0 && nSubMtlSlot < (int)m_subMtls.size() && m_subMtls[nSubMtlSlot] != NULL)
    {
        pShaderItem = &(m_subMtls[nSubMtlSlot]->m_shaderItem);
    }
    else
    {
        _smart_ptr<IMaterial> pDefaultMaterial = GetMatMan()->GetDefaultMaterial();
        pShaderItem = &(static_cast<CMatInfo*>(pDefaultMaterial.get())->m_shaderItem);
    }

    return *pShaderItem;
}

///////////////////////////////////////////////////////////////////////////////
const SShaderItem& CMatInfo::GetShaderItem(int nSubMtlSlot) const
{
    const SShaderItem* pShaderItem = NULL;
    if (m_subMtls.empty() || !(m_Flags & MTL_FLAG_MULTI_SUBMTL))
    {
        pShaderItem = &m_shaderItem; // Not Multi material.
    }
    else if (nSubMtlSlot >= 0 && nSubMtlSlot < (int)m_subMtls.size() && m_subMtls[nSubMtlSlot] != NULL)
    {
        pShaderItem = &(m_subMtls[nSubMtlSlot]->m_shaderItem);
    }
    else
    {
        _smart_ptr<IMaterial> pDefaultMaterial = GetMatMan()->GetDefaultMaterial();
        pShaderItem = &(static_cast<CMatInfo*>(pDefaultMaterial.get())->m_shaderItem);
    }

    return *pShaderItem;
}

///////////////////////////////////////////////////////////////////////////////
bool CMatInfo::IsForwardRenderingRequired()
{
    bool bRequireForwardRendering = (m_Flags & MTL_FLAG_REQUIRE_FORWARD_RENDERING) != 0;

    if (!bRequireForwardRendering)
    {
        for (int i = 0; i < (int)m_subMtls.size(); ++i)
        {
            if (m_subMtls[i] != 0 && m_subMtls[i]->m_Flags & MTL_FLAG_REQUIRE_FORWARD_RENDERING)
            {
                bRequireForwardRendering = true;
                break;
            }
        }
    }

    return bRequireForwardRendering;
}

///////////////////////////////////////////////////////////////////////////////
bool CMatInfo::IsNearestCubemapRequired()
{
    bool bRequireNearestCubemap = (m_Flags & MTL_FLAG_REQUIRE_NEAREST_CUBEMAP) != 0;

    if (!bRequireNearestCubemap)
    {
        for (int i = 0; i < (int)m_subMtls.size(); ++i)
        {
            if (m_subMtls[i] != 0 && m_subMtls[i]->m_Flags & MTL_FLAG_REQUIRE_NEAREST_CUBEMAP)
            {
                bRequireNearestCubemap = true;
                break;
            }
        }
    }

    return bRequireNearestCubemap;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
