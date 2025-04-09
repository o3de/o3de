/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : CryMovie animation node for shadow settings

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/vector.h>

#include "ShadowsSetupNode.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimValueType.h"
#include "Maestro/Types/AnimParamType.h"

namespace Maestro
{

    namespace ShadowSetupNodeHelper
    {
        static bool s_shadowSetupParamsInit = false;
        static AZStd::vector<CAnimNode::SParamInfo> s_shadowSetupParams;

        static void AddSupportedParam(const char* sName, AnimParamType paramId, AnimValueType valueType)
        {
            CAnimNode::SParamInfo param;
            param.name = sName;
            param.paramType = paramId;
            param.valueType = valueType;
            s_shadowSetupParams.push_back(param);
        }
    } // namespace ShadowSetupNodeHelper

    CShadowsSetupNode::CShadowsSetupNode()
        : CShadowsSetupNode(0)
    {
    }

    CShadowsSetupNode::CShadowsSetupNode(const int id)
        : CAnimNode(id, AnimNodeType::ShadowSetup)
    {
        CShadowsSetupNode::Initialize();
    }

    void CShadowsSetupNode::Initialize()
    {
        using namespace ShadowSetupNodeHelper;
        if (!s_shadowSetupParamsInit)
        {
            s_shadowSetupParamsInit = true;
            s_shadowSetupParams.reserve(1);
            AddSupportedParam("GSMCache", AnimParamType::GSMCache, AnimValueType::Bool);
        }
    }

    void CShadowsSetupNode::Animate(SAnimContext& ac)
    {
        IAnimTrack* pGsmCache = GetTrackForParameter(AnimParamType::GSMCache);
        if (pGsmCache && (pGsmCache->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled) == 0)
        {
            bool val(false);
            pGsmCache->GetValue(ac.time, val);
        }
    }

    void CShadowsSetupNode::CreateDefaultTracks()
    {
        CreateTrack(AnimParamType::GSMCache);
    }

    void CShadowsSetupNode::OnReset()
    {
    }

    unsigned int CShadowsSetupNode::GetParamCount() const
    {
        return static_cast<int>(ShadowSetupNodeHelper::s_shadowSetupParams.size());
    }

    CAnimParamType CShadowsSetupNode::GetParamType(unsigned int nIndex) const
    {
        using namespace ShadowSetupNodeHelper;
        if (nIndex < s_shadowSetupParams.size())
        {
            return s_shadowSetupParams[nIndex].paramType;
        }

        return AnimParamType::Invalid;
    }

    bool CShadowsSetupNode::GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const
    {
        using namespace ShadowSetupNodeHelper;
        for (size_t i = 0; i < s_shadowSetupParams.size(); ++i)
        {
            if (s_shadowSetupParams[i].paramType == paramId)
            {
                info = s_shadowSetupParams[i];
                return true;
            }
        }
        return false;
    }

    void CShadowsSetupNode::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CShadowsSetupNode, CAnimNode>()->Version(1);
        }
    }

} // namespace Maestro
