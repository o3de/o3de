/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : CryMovie animation node for shadow settings

#include "Maestro_precompiled.h"
#include <AzCore/Serialization/SerializeContext.h>

#include "ShadowsSetupNode.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimValueType.h"
#include "Maestro/Types/AnimParamType.h"

//////////////////////////////////////////////////////////////////////////
namespace ShadowSetupNode
{
    bool s_shadowSetupParamsInit = false;
    StaticInstance<std::vector<CAnimNode::SParamInfo>> s_shadowSetupParams;

    void AddSupportedParam(const char* sName, AnimParamType paramId, AnimValueType valueType)
    {
        CAnimNode::SParamInfo param;
        param.name = sName;
        param.paramType = paramId;
        param.valueType = valueType;
        s_shadowSetupParams.push_back(param);
    }
};

//-----------------------------------------------------------------------------
CShadowsSetupNode::CShadowsSetupNode()
    : CShadowsSetupNode(0)
{
}

//-----------------------------------------------------------------------------
CShadowsSetupNode::CShadowsSetupNode(const int id)
    : CAnimNode(id, AnimNodeType::ShadowSetup)
{
    CShadowsSetupNode::Initialize();
}

//-----------------------------------------------------------------------------
void CShadowsSetupNode::Initialize()
{
    if (!ShadowSetupNode::s_shadowSetupParamsInit)
    {
        ShadowSetupNode::s_shadowSetupParamsInit = true;
        ShadowSetupNode::s_shadowSetupParams.reserve(1);
        ShadowSetupNode::AddSupportedParam("GSMCache", AnimParamType::GSMCache, AnimValueType::Bool);
    }
}

//-----------------------------------------------------------------------------
void CShadowsSetupNode::Animate(SAnimContext& ac)
{
    IAnimTrack* pGsmCache = GetTrackForParameter(AnimParamType::GSMCache);
    if (pGsmCache && (pGsmCache->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled) == 0)
    {
        bool val(false);
        pGsmCache->GetValue(ac.time, val);
    }
}

//-----------------------------------------------------------------------------
void CShadowsSetupNode::CreateDefaultTracks()
{
    CreateTrack(AnimParamType::GSMCache);
}

//-----------------------------------------------------------------------------
void CShadowsSetupNode::OnReset()
{
}

//-----------------------------------------------------------------------------
unsigned int CShadowsSetupNode::GetParamCount() const
{
    return ShadowSetupNode::s_shadowSetupParams.size();
}

//-----------------------------------------------------------------------------
CAnimParamType CShadowsSetupNode::GetParamType(unsigned int nIndex) const
{
    if (nIndex >= 0 && nIndex < (int)ShadowSetupNode::s_shadowSetupParams.size())
    {
        return ShadowSetupNode::s_shadowSetupParams[nIndex].paramType;
    }

    return AnimParamType::Invalid;
}

//-----------------------------------------------------------------------------
bool CShadowsSetupNode::GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const
{
    for (size_t i = 0; i < ShadowSetupNode::s_shadowSetupParams.size(); ++i)
    {
        if (ShadowSetupNode::s_shadowSetupParams[i].paramType == paramId)
        {
            info = ShadowSetupNode::s_shadowSetupParams[i];
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CShadowsSetupNode::Reflect(AZ::ReflectContext* context)
{
    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<CShadowsSetupNode, CAnimNode>()
            ->Version(1);
    }
}
