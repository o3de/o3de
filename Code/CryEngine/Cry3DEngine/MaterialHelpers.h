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

#ifndef __MaterialHelpers_h__
#define __MaterialHelpers_h__
#pragma once

#include <IMaterial.h>

// Description:
//   Namespace "implementation", not a class "implementation", no member-variables, only const functions;
//   Used to encapsulate the material-definition/io into Cry3DEngine (and make it plugable that way).
struct MaterialHelpers
    : public IMaterialHelpers
{
    //////////////////////////////////////////////////////////////////////////
    virtual EEfResTextures FindTexSlot(const char* texName) const final;
    virtual const char* FindTexName(EEfResTextures texSlot) const final;
    virtual const char* LookupTexName(EEfResTextures texSlot) const final;
    virtual const char* LookupTexDesc(EEfResTextures texSlot) const final;
    virtual const char* LookupTexEnum(EEfResTextures texSlot) const final;
    virtual const char* LookupTexSuffix(EEfResTextures texSlot) const final;
    virtual bool IsAdjustableTexSlot(EEfResTextures texSlot) const final;

    //////////////////////////////////////////////////////////////////////////
    virtual bool SetGetMaterialParamFloat(IRenderShaderResources& pShaderResources, const char* sParamName, float& v, bool bGet) const final;
    virtual bool SetGetMaterialParamVec3(IRenderShaderResources& pShaderResources, const char* sParamName, Vec3& v, bool bGet) const final;

    //////////////////////////////////////////////////////////////////////////
    virtual void SetTexModFromXml(SEfTexModificator& pShaderResources, const XmlNodeRef& modNode) const final;
    virtual void SetXmlFromTexMod(const SEfTexModificator& pShaderResources, XmlNodeRef& node) const final;

    //////////////////////////////////////////////////////////////////////////
    virtual void SetTexturesFromXml(SInputShaderResources& pShaderResources, const XmlNodeRef& node) const final;
    virtual void SetXmlFromTextures( SInputShaderResources& pShaderResources, XmlNodeRef& node) const final;

    //////////////////////////////////////////////////////////////////////////
    virtual void SetVertexDeformFromXml(SInputShaderResources& pShaderResources, const XmlNodeRef& node) const final;
    virtual void SetXmlFromVertexDeform(const SInputShaderResources& pShaderResources, XmlNodeRef& node) const final;

    //////////////////////////////////////////////////////////////////////////
    virtual void SetLightingFromXml(SInputShaderResources& pShaderResources, const XmlNodeRef& node) const final;
    virtual void SetXmlFromLighting(const SInputShaderResources& pShaderResources, XmlNodeRef& node) const final;

    //////////////////////////////////////////////////////////////////////////
    virtual void SetShaderParamsFromXml(SInputShaderResources& pShaderResources, const XmlNodeRef& node) const final;
    virtual void SetXmlFromShaderParams(const SInputShaderResources& pShaderResources, XmlNodeRef& node) const final;

    //////////////////////////////////////////////////////////////////////////
    virtual void MigrateXmlLegacyData(SInputShaderResources& pShaderResources, const XmlNodeRef& node) const final;
};

#endif

