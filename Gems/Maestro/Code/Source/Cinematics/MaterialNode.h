/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYMOVIE_MATERIALNODE_H
#define CRYINCLUDE_CRYMOVIE_MATERIALNODE_H
#pragma once

#include "AnimNode.h"

class CAnimMaterialNode
    : public CAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CAnimMaterialNode, AZ::SystemAllocator, 0);
    AZ_RTTI(CAnimMaterialNode, "{15B1E5EA-BB12-445E-B937-34CD559347ED}", CAnimNode);

    CAnimMaterialNode();
    CAnimMaterialNode(const int id);
    static void Initialize();

    virtual void SetName(const char* name);

    //////////////////////////////////////////////////////////////////////////
    // Overrides from CAnimNode
    //////////////////////////////////////////////////////////////////////////
    void Animate(SAnimContext& ec);
    void AddTrack(IAnimTrack* track) override;

    //////////////////////////////////////////////////////////////////////////
    // Supported tracks description.
    //////////////////////////////////////////////////////////////////////////
    virtual unsigned int GetParamCount() const;
    virtual CAnimParamType GetParamType(unsigned int nIndex) const;
    virtual const char* GetParamName(const CAnimParamType& paramType) const;

    virtual void GetKeyValueRange(float& fMin, float& fMax) const { fMin = m_fMinKeyValue; fMax = m_fMaxKeyValue; };
    virtual void SetKeyValueRange(float fMin, float fMax){ m_fMinKeyValue = fMin; m_fMaxKeyValue = fMax; };

    virtual void InitializeTrack(IAnimTrack* pTrack, const CAnimParamType& paramType);
    

    static void Reflect(AZ::ReflectContext* context);
protected:
    virtual bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;

    void UpdateDynamicParamsInternal() override;
private:
    void AnimateNamedParameter(SAnimContext& ec, IRenderShaderResources* pShaderResources, const char* name, IAnimTrack* pTrack);
    _smart_ptr<IMaterial> GetMaterialByName(const char* pName);

    float m_fMinKeyValue;
    float m_fMaxKeyValue;

    std::vector< CAnimNode::SParamInfo > m_dynamicShaderParamInfos;
    typedef AZStd::unordered_map< string, size_t, stl::hash_string_caseless<string>, stl::equality_string_caseless<string> > TDynamicShaderParamsMap;
    TDynamicShaderParamsMap m_nameToDynamicShaderParam;
};

#endif // CRYINCLUDE_CRYMOVIE_MATERIALNODE_H
