/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MaterialNode.h"
#include "AnimTrack.h"
#include <AzCore/Serialization/SerializeContext.h>

#include <IRenderer.h>
#include <IShader.h>
#include <ISystem.h>

#include "AnimSplineTrack.h"
#include "CompoundSplineTrack.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimParamType.h"
#include "Maestro/Types/AnimValueType.h"

// Don't remove or the console builds will break!
#define s_nodeParamsInitialized s_nodeParamsInitializedMat
#define s_nodeParams s_nodeParamsMat
#define AddSupportedParam AddSupportedParamMat

namespace
{
    bool s_nodeParamsInitialized = false;
    StaticInstance<std::vector<CAnimNode::SParamInfo>> s_nodeParams;

    void AddSupportedParam(const char* sName, AnimParamType paramId, AnimValueType valueType)
    {
        CAnimNode::SParamInfo param;
        param.name = sName;
        param.paramType = paramId;
        param.valueType = valueType;
        s_nodeParams.push_back(param);
    }
} // namespace

enum EMaterialNodeParam
{
    MTL_PARAM_SHADER_PARAM1 = static_cast<int>(AnimParamType::User) + 100,
};

//////////////////////////////////////////////////////////////////////////
void CAnimMaterialNode::InitializeTrack(IAnimTrack* pTrack, const CAnimParamType& paramType)
{
    if (paramType == AnimParamType::MaterialOpacity)
    {
        pTrack->SetKeyValueRange(0.0f, 100.f);
    }
    else if (paramType == AnimParamType::MaterialSmoothness)
    {
        pTrack->SetKeyValueRange(0.0f, 255.f);
    }
}

//////////////////////////////////////////////////////////////////////////
CAnimMaterialNode::CAnimMaterialNode()
    : CAnimMaterialNode(0)
{
}

//////////////////////////////////////////////////////////////////////////
CAnimMaterialNode::CAnimMaterialNode(const int id)
    : CAnimNode(id, AnimNodeType::Material)
{
    SetFlags(GetFlags() | eAnimNodeFlags_CanChangeName);

    CAnimMaterialNode::Initialize();
}

//////////////////////////////////////////////////////////////////////////
void CAnimMaterialNode::Initialize()
{
    if (!s_nodeParamsInitialized)
    {
        s_nodeParamsInitialized = true;

        s_nodeParams.reserve(5);
        AddSupportedParam("Diffuse", AnimParamType::MaterialDiffuse, AnimValueType::RGB);
        AddSupportedParam("Emissive Color", AnimParamType::MaterialEmissive, AnimValueType::RGB);
        AddSupportedParam("Emissive Intensity", AnimParamType::MaterialEmissiveIntensity, AnimValueType::Float);
        AddSupportedParam("Glossiness", AnimParamType::MaterialSmoothness, AnimValueType::Float);
        AddSupportedParam("Opacity", AnimParamType::MaterialOpacity, AnimValueType::Float);
        AddSupportedParam("Specular", AnimParamType::MaterialSpecular, AnimValueType::RGB);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimMaterialNode::SetName(const char* name)
{
    CAnimNode::SetName(name);
    UpdateDynamicParams();
}

//////////////////////////////////////////////////////////////////////////
void CAnimMaterialNode::UpdateDynamicParamsInternal()
{
    m_dynamicShaderParamInfos.clear();
    m_nameToDynamicShaderParam.clear();

    const char* pName = GetName();
    IMaterial* pMtl = GetMaterialByName(pName);

    if (!pMtl)
    {
        return;
    }

    const SShaderItem& shaderItem = pMtl->GetShaderItem();
    IRenderShaderResources* pShaderResources = shaderItem.m_pShaderResources;
    if (!pShaderResources)
    {
        return;
    }

    auto& shaderParams = pShaderResources->GetParameters();
    for (int i = 0; i < shaderParams.size(); ++i)
    {
        SShaderParam& shaderParam = shaderParams[i];
        m_nameToDynamicShaderParam[shaderParams[i].m_Name.c_str()] = i;

        CAnimNode::SParamInfo paramInfo;

        switch (shaderParam.m_Type)
        {
        case eType_FLOAT:
        // Fall through
        case eType_HALF:
            paramInfo.valueType = AnimValueType::Float;
            break;
        case eType_VECTOR:
            paramInfo.valueType = AnimValueType::Vector;
            break;
        case eType_FCOLOR:
            paramInfo.valueType = AnimValueType::RGB;
            break;
        case eType_BOOL:
            paramInfo.valueType = AnimValueType::Bool;
            break;
        default:
            continue;
        }

        paramInfo.name = shaderParam.m_Name.c_str();
        paramInfo.paramType = shaderParam.m_Name;
        paramInfo.flags = IAnimNode::ESupportedParamFlags(0);

        m_dynamicShaderParamInfos.push_back(paramInfo);
    }

    // Make sure any color tracks that are animated "ByString"
    // have the track multiplier set.
    int trackCount = NumTracks();
    for (int trackIndex = 0; trackIndex < trackCount; trackIndex++)
    {
        IAnimTrack* track = m_tracks[trackIndex].get();
        if (!(track->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled))
        {
            CAnimParamType param = track->GetParameterType();
            if (param.GetType() == AnimParamType::ByString && track->GetValueType() == AnimValueType::RGB)
            {
                track->SetMultiplier(255.0f);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
unsigned int CAnimMaterialNode::GetParamCount() const
{
    return static_cast<unsigned int>(s_nodeParams.size() + m_dynamicShaderParamInfos.size());
}

//////////////////////////////////////////////////////////////////////////
CAnimParamType CAnimMaterialNode::GetParamType(unsigned int nIndex) const
{
    if (nIndex < s_nodeParams.size())
    {
        return s_nodeParams[nIndex].paramType;
    }
    else if (
        nIndex >= (unsigned int)s_nodeParams.size() &&
        nIndex < ((unsigned int)s_nodeParams.size() + (unsigned int)m_dynamicShaderParamInfos.size()))
    {
        return m_dynamicShaderParamInfos[nIndex - (int)s_nodeParams.size()].paramType;
    }

    return AnimParamType::Invalid;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimMaterialNode::GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const
{
    for (size_t i = 0; i < s_nodeParams.size(); ++i)
    {
        if (s_nodeParams[i].paramType == paramId)
        {
            info = s_nodeParams[i];
            return true;
        }
    }
    for (size_t i = 0; i < m_dynamicShaderParamInfos.size(); ++i)
    {
        if (m_dynamicShaderParamInfos[i].paramType == paramId)
        {
            info = m_dynamicShaderParamInfos[i];
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
AZStd::string CAnimMaterialNode::GetParamName(const CAnimParamType& param) const
{
    if (param.GetType() == AnimParamType::ByString)
    {
        return param.GetName();
    }
    else if ((int)param.GetType() >= (int)MTL_PARAM_SHADER_PARAM1)
    {
        switch ((int)param.GetType())
        {
        case MTL_PARAM_SHADER_PARAM1:
            return "Shader Param 1";
        case MTL_PARAM_SHADER_PARAM1 + 1:
            return "Shader Param 2";
        case MTL_PARAM_SHADER_PARAM1 + 2:
            return "Shader Param 3";
        case MTL_PARAM_SHADER_PARAM1 + 3:
            return "Shader Param 4";
        case MTL_PARAM_SHADER_PARAM1 + 4:
            return "Shader Param 5";
        case MTL_PARAM_SHADER_PARAM1 + 5:
            return "Shader Param 6";
        case MTL_PARAM_SHADER_PARAM1 + 6:
            return "Shader Param 7";
        case MTL_PARAM_SHADER_PARAM1 + 7:
            return "Shader Param 8";
        case MTL_PARAM_SHADER_PARAM1 + 8:
            return "Shader Param 9";
        default:
            return "Unknown Shader Param";
        }
    }

    return CAnimNode::GetParamName(param);
}

//////////////////////////////////////////////////////////////////////////
void CAnimMaterialNode::Animate(SAnimContext& ec)
{
    int paramCount = NumTracks();
    if (paramCount <= 0)
    {
        return;
    }

    // Find material.
    const char* pName = GetName();
    IMaterial* pMtl = GetMaterialByName(pName);

    if (!pMtl)
    {
        return;
    }

    const SShaderItem& shaderItem = pMtl->GetShaderItem();
    IRenderShaderResources* pShaderResources = shaderItem.m_pShaderResources;
    if (!pShaderResources)
    {
        return;
    }

    float fValue;
    Vec3 vValue;

    int trackCount = NumTracks();
    for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
    {
        CAnimParamType paramId = m_tracks[paramIndex]->GetParameterType();
        IAnimTrack* pTrack = m_tracks[paramIndex].get();

        if (pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled)
        {
            continue;
        }

        switch (paramId.GetType())
        {
        case AnimParamType::MaterialOpacity:
            pTrack->GetValue(ec.time, fValue);
            pShaderResources->SetStrengthValue(EFTT_OPACITY, fValue);
            break;
        case AnimParamType::MaterialDiffuse:
            pTrack->GetValue(ec.time, vValue);
            {
                pShaderResources->SetColorValue(EFTT_DIFFUSE, vValue / 255.0f);
            }
            break;
        case AnimParamType::MaterialSpecular:
            pTrack->GetValue(ec.time, vValue);
            {
                pShaderResources->SetColorValue(EFTT_SPECULAR, vValue / 255.0f);
            }
            break;
        case AnimParamType::MaterialEmissive:
            pTrack->GetValue(ec.time, vValue);
            pShaderResources->SetColorValue(EFTT_EMITTANCE, vValue / 255.0f);
            break;
        case AnimParamType::MaterialEmissiveIntensity:
            pTrack->GetValue(ec.time, fValue);
            pShaderResources->SetStrengthValue(EFTT_EMITTANCE, fValue);
            break;
        case AnimParamType::MaterialSmoothness:
            pTrack->GetValue(ec.time, fValue);
            {
                pShaderResources->SetStrengthValue(EFTT_SMOOTHNESS, fValue / 255.0f);
            }
            break;
        case AnimParamType::ByString:
            AnimateNamedParameter(ec, pShaderResources, paramId.GetName(), pTrack);
            break;
        default:
            // Legacy support code
            if (paramId.GetType() >= (AnimParamType)MTL_PARAM_SHADER_PARAM1)
            {
                int id = static_cast<int>(paramId.GetType()) - MTL_PARAM_SHADER_PARAM1;
                if (id < (int)pShaderResources->GetParameters().size())
                {
                    pTrack->GetValue(ec.time, fValue);
                    pShaderResources->GetParameters()[id].m_Value.m_Float = fValue;
                }
            }
        }
    }

    IShader* pShader = shaderItem.m_pShader;
    if (pShader)
    {
        shaderItem.m_pShaderResources->UpdateConstants(pShader);
    }
}

void CAnimMaterialNode::AnimateNamedParameter(
    SAnimContext& ec, IRenderShaderResources* pShaderResources, const char* name, IAnimTrack* pTrack)
{
    TDynamicShaderParamsMap::iterator findIter = m_nameToDynamicShaderParam.find(name);
    if (findIter != m_nameToDynamicShaderParam.end())
    {
        SShaderParam& param = pShaderResources->GetParameters()[findIter->second];

        float fValue;
        Vec3 vecValue;
        Vec3 colorValue;
        bool boolValue;

        switch (pTrack->GetValueType())
        {
        case AnimValueType::Float:
            pTrack->GetValue(ec.time, fValue);
            param.m_Value.m_Float = fValue;
            break;
        case AnimValueType::Vector:
            pTrack->GetValue(ec.time, vecValue);
            param.m_Value.m_Vector[0] = vecValue[0];
            param.m_Value.m_Vector[1] = vecValue[1];
            param.m_Value.m_Vector[2] = vecValue[2];
            break;
        case AnimValueType::RGB:
            pTrack->GetValue(ec.time, colorValue, true);
            param.m_Value.m_Color[0] = colorValue[0];
            param.m_Value.m_Color[1] = colorValue[1];
            param.m_Value.m_Color[2] = colorValue[2];
            param.m_Value.m_Color[3] = 0.0f;
            break;
        case AnimValueType::Bool:
            pTrack->GetValue(ec.time, boolValue);
            param.m_Value.m_Bool = boolValue;
            break;
        }
    }
}

IMaterial* CAnimMaterialNode::GetMaterialByName(const char*)
{
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CAnimMaterialNode::AddTrack(IAnimTrack* track)
{
    CAnimNode::AddTrack(track);
    UpdateDynamicParams();
}

//////////////////////////////////////////////////////////////////////////
void CAnimMaterialNode::Reflect(AZ::ReflectContext* context)
{
    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<CAnimMaterialNode, CAnimNode>()->Version(1);
    }
}

#undef s_nodeParamsInitialized
#undef s_nodeParams
#undef AddSupportedParam
