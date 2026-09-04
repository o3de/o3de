/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "ScriptVarNode.h"
#include "AnimTrack.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimValueType.h"
#include "Maestro/Types/AnimParamType.h"

#include <ISystem.h>

namespace Maestro
{

    CAnimScriptVarNode::CAnimScriptVarNode(const int id)
        : CAnimNode(id, AnimNodeType::ScriptVar)
    {
        SetFlags(GetFlags() | eAnimNodeFlags_CanChangeName);
        m_value = -1e-20f;
    }

    CAnimScriptVarNode::CAnimScriptVarNode()
        : CAnimScriptVarNode(0)
    {
    }

    void CAnimScriptVarNode::OnReset()
    {
        m_value = -1e-20f;
    }

    void CAnimScriptVarNode::OnResume()
    {
        OnReset();
    }

    void CAnimScriptVarNode::CreateDefaultTracks()
    {
        CreateTrack(AnimParamType::Float);
    };

    unsigned int CAnimScriptVarNode::GetParamCount() const
    {
        return 1;
    }

    CAnimParamType CAnimScriptVarNode::GetParamType(unsigned int nIndex) const
    {
        if (nIndex == 0)
        {
            return AnimParamType::Float;
        }

        return AnimParamType::Invalid;
    }

    bool CAnimScriptVarNode::GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const
    {
        if (paramId.GetType() == AnimParamType::Float)
        {
            info.flags = IAnimNode::ESupportedParamFlags(0);
            info.name = "Value";
            info.paramType = AnimParamType::Float;
            info.valueType = AnimValueType::Float;
            return true;
        }
        return false;
    }

    void CAnimScriptVarNode::Animate(SAnimContext& ec)
    {
        float value = m_value;

        IAnimTrack* pValueTrack = GetTrackForParameter(AnimParamType::Float);

        if (pValueTrack)
        {
            if (pValueTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled)
            {
                return;
            }

            pValueTrack->GetValue(ec.time, value);
        }

        if (value != m_value)
        {
            m_value = value;
        }
    }

    void CAnimScriptVarNode::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CAnimScriptVarNode, CAnimNode>()->Version(1);
        }
    }

} // namespace Maestro
