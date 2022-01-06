/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYMOVIE_MATERIALNODE_H
#define CRYINCLUDE_CRYMOVIE_MATERIALNODE_H
#pragma once

#include "AnimNode.h"
#include <CryCommon/IMaterial.h>
#include <CryCommon/StlUtils.h>

class CAnimMaterialNode
    : public CAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CAnimMaterialNode, AZ::SystemAllocator, 0);
    AZ_RTTI(CAnimMaterialNode, "{15B1E5EA-BB12-445E-B937-34CD559347ED}", CAnimNode);

    CAnimMaterialNode();
    CAnimMaterialNode(const int id);
    static void Initialize();

    void SetName(const char* name) override;

    //////////////////////////////////////////////////////////////////////////
    // Overrides from CAnimNode
    //////////////////////////////////////////////////////////////////////////
    void Animate(SAnimContext& ec) override;
    void AddTrack(IAnimTrack* track) override;

    //////////////////////////////////////////////////////////////////////////
    // Supported tracks description.
    //////////////////////////////////////////////////////////////////////////
    unsigned int GetParamCount() const override;
    CAnimParamType GetParamType(unsigned int nIndex) const override;
    AZStd::string GetParamName(const CAnimParamType& paramType) const override;

    virtual void GetKeyValueRange(float& fMin, float& fMax) const { fMin = m_fMinKeyValue; fMax = m_fMaxKeyValue; };
    virtual void SetKeyValueRange(float fMin, float fMax){ m_fMinKeyValue = fMin; m_fMaxKeyValue = fMax; };

    virtual void InitializeTrack(IAnimTrack* pTrack, const CAnimParamType& paramType);


    static void Reflect(AZ::ReflectContext* context);
protected:
    bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const override;

    void UpdateDynamicParamsInternal() override;
private:
    void AnimateNamedParameter(SAnimContext& ec, IRenderShaderResources* pShaderResources, const char* name, IAnimTrack* pTrack);
    IMaterial * GetMaterialByName(const char* pName);

    float m_fMinKeyValue;
    float m_fMaxKeyValue;

    std::vector< CAnimNode::SParamInfo > m_dynamicShaderParamInfos;
    typedef AZStd::unordered_map<AZStd::string, size_t, stl::hash_string_caseless<AZStd::string>, stl::equality_string_caseless<AZStd::string> > TDynamicShaderParamsMap;
    TDynamicShaderParamsMap m_nameToDynamicShaderParam;
};

#endif // CRYINCLUDE_CRYMOVIE_MATERIALNODE_H
