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

#pragma once

#include "Material.h"

namespace MaterialHelpers
{
    //////////////////////////////////////////////////////////////////////////
    //! Get public parameters of material in variable block.
    CVarBlock* GetPublicVars(SInputShaderResources& pShaderResources);

    //! Sets variable block of public shader parameters.
    //! VarBlock must be in same format as returned by GetPublicVars().
    void SetPublicVars(CVarBlock* pPublicVars, SInputShaderResources& pInputShaderResources);
    void SetPublicVars(CVarBlock* pPublicVars, SInputShaderResources& pInputShaderResources, IRenderShaderResources* pRenderShaderResources, IShader* pShader);

    //////////////////////////////////////////////////////////////////////////
    CVarBlock* GetShaderGenParamsVars(IShader* pShader, uint64 nShaderGenMask);
    uint64 SetShaderGenParamsVars(IShader* pShader, CVarBlock* pBlock);

    //////////////////////////////////////////////////////////////////////////
    // [Shader System TO DO] change the usage of these functions to retrieve by slot name
    inline EEfResTextures FindTexSlot(const char* texName) { return GetIEditor()->Get3DEngine()->GetMaterialHelpers().FindTexSlot(texName); }
    inline const char* FindTexName(EEfResTextures texSlot) { return GetIEditor()->Get3DEngine()->GetMaterialHelpers().FindTexName(texSlot); }
    inline const char* LookupTexName(EEfResTextures texSlot) { return GetIEditor()->Get3DEngine()->GetMaterialHelpers().LookupTexName(texSlot); }
    inline const char* LookupTexDesc(EEfResTextures texSlot) { return GetIEditor()->Get3DEngine()->GetMaterialHelpers().LookupTexDesc(texSlot); }

    //--------------------------------------------------------------------------
    // Adjustable means that the slot is not virtual, i.e. using a sub-channel from another 
    // slot (for example - smoothness that uses the normal's alpha)
    inline bool IsAdjustableTexSlot(EEfResTextures texSlot) { return GetIEditor()->Get3DEngine()->GetMaterialHelpers().IsAdjustableTexSlot(texSlot); }

    //////////////////////////////////////////////////////////////////////////
    inline void SetTexModFromXml(SEfTexModificator& pShaderResources, const XmlNodeRef& node) { GetIEditor()->Get3DEngine()->GetMaterialHelpers().SetTexModFromXml(pShaderResources, node); }
    inline void SetXmlFromTexMod(const SEfTexModificator& pShaderResources, XmlNodeRef& node) { GetIEditor()->Get3DEngine()->GetMaterialHelpers().SetXmlFromTexMod(pShaderResources, node); }

    //////////////////////////////////////////////////////////////////////////
    inline void SetTexturesFromXml(SInputShaderResources& pShaderResources, const XmlNodeRef& node) { GetIEditor()->Get3DEngine()->GetMaterialHelpers().SetTexturesFromXml(pShaderResources, node); }
    inline void SetXmlFromTextures( SInputShaderResources& pShaderResources, XmlNodeRef& node) { GetIEditor()->Get3DEngine()->GetMaterialHelpers().SetXmlFromTextures(pShaderResources, node); }

    //////////////////////////////////////////////////////////////////////////
    inline void SetVertexDeformFromXml(SInputShaderResources& pShaderResources, const XmlNodeRef& node) { GetIEditor()->Get3DEngine()->GetMaterialHelpers().SetVertexDeformFromXml(pShaderResources, node); }
    inline void SetXmlFromVertexDeform(const SInputShaderResources& pShaderResources, XmlNodeRef& node) { GetIEditor()->Get3DEngine()->GetMaterialHelpers().SetXmlFromVertexDeform(pShaderResources, node); }

    //////////////////////////////////////////////////////////////////////////
    inline void SetLightingFromXml(SInputShaderResources& pShaderResources, const XmlNodeRef& node) { GetIEditor()->Get3DEngine()->GetMaterialHelpers().SetLightingFromXml(pShaderResources, node); }
    inline void SetXmlFromLighting(const SInputShaderResources& pShaderResources, XmlNodeRef& node) { GetIEditor()->Get3DEngine()->GetMaterialHelpers().SetXmlFromLighting(pShaderResources, node); }

    //////////////////////////////////////////////////////////////////////////
    inline void SetShaderParamsFromXml(SInputShaderResources& pShaderResources, const XmlNodeRef& node) { GetIEditor()->Get3DEngine()->GetMaterialHelpers().SetShaderParamsFromXml(pShaderResources, node); }
    inline void SetXmlFromShaderParams(const SInputShaderResources& pShaderResources, XmlNodeRef& node) { GetIEditor()->Get3DEngine()->GetMaterialHelpers().SetXmlFromShaderParams(pShaderResources, node); }

    //////////////////////////////////////////////////////////////////////////
    inline void MigrateXmlLegacyData(SInputShaderResources& pShaderResources, const XmlNodeRef& node) { GetIEditor()->Get3DEngine()->GetMaterialHelpers().MigrateXmlLegacyData(pShaderResources, node); }
}
