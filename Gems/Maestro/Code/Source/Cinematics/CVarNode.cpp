/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include "CVarNode.h"
#include "AnimTrack.h"

#include <ISystem.h>
#include <IConsole.h>

#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimValueType.h"
#include "Maestro/Types/AnimParamType.h"

namespace Maestro
{

    CAnimCVarNode::CAnimCVarNode(const int id)
        : CAnimNode(id, AnimNodeType::CVar)
    {
        SetFlags(GetFlags() | eAnimNodeFlags_CanChangeName);
        m_value = -1e-20f; //-1e-28;
    }

    CAnimCVarNode::CAnimCVarNode()
        : CAnimCVarNode(0)
    {
    }

    void CAnimCVarNode::CreateDefaultTracks()
    {
        CreateTrack(AnimParamType::Float);
    }

    void CAnimCVarNode::OnReset()
    {
        m_value = -1e-20f;
    }

    void CAnimCVarNode::OnResume()
    {
        OnReset();
    }

    unsigned int CAnimCVarNode::GetParamCount() const
    {
        return 1;
    }

    CAnimParamType CAnimCVarNode::GetParamType(unsigned int nIndex) const
    {
        if (nIndex == 0)
        {
            return AnimParamType::Float;
        }

        return AnimParamType::Invalid;
    }

    int CAnimCVarNode::GetDefaultKeyTangentFlags() const
    {
        int retTangentFlags = SPLINE_KEY_TANGENT_UNIFIED;

        ICVar* var = gEnv->pConsole->GetCVar(GetName());

        // use step in tangents for int cvars
        if (var && var->GetType() == CVAR_INT)
        {
            // clear tangent flags
            retTangentFlags &= ~SPLINE_KEY_TANGENT_IN_MASK;
            retTangentFlags &= ~SPLINE_KEY_TANGENT_OUT_MASK;

            // set in tangents to step
            retTangentFlags |= (SPLINE_KEY_TANGENT_STEP << SPLINE_KEY_TANGENT_IN_SHIFT);
            retTangentFlags |= (SPLINE_KEY_TANGENT_CUSTOM << SPLINE_KEY_TANGENT_OUT_SHIFT);
        }

        return retTangentFlags;
    }

    bool CAnimCVarNode::GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const
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

    void CAnimCVarNode::SetName(const char* name)
    {
        // Name of node is used as a name of console var.
        CAnimNode::SetName(name);
        ICVar* pVar = gEnv->pConsole->GetCVar(GetName());
        if (pVar)
        {
            m_value = pVar->GetFVal();
        }
    }

    void CAnimCVarNode::Animate(SAnimContext& ec)
    {
        if (ec.resetting)
        {
            return;
        }

        float value = m_value;

        IAnimTrack* pValueTrack = GetTrackForParameter(AnimParamType::Float);

        if (!pValueTrack || (pValueTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled))
        {
            return;
        }

        pValueTrack->GetValue(ec.time, value);

        if (value != m_value)
        {
            m_value = value;
            // Change console var value.
            ICVar* pVar = gEnv->pConsole->GetCVar(GetName());

            if (pVar)
            {
                pVar->Set(m_value);
            }
        }
    }

    void CAnimCVarNode::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CAnimCVarNode, CAnimNode>()->Version(1);
        }
    }

} // namespace Maestro
