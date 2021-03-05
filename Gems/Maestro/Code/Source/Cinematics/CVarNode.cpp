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

#include "Maestro_precompiled.h"
#include <AzCore/Serialization/SerializeContext.h>

#include "CVarNode.h"
#include "AnimTrack.h"

#include <ISystem.h>
#include <IConsole.h>

#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimValueType.h"
#include "Maestro/Types/AnimParamType.h"

//////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////
unsigned int CAnimCVarNode::GetParamCount() const
{
    return 1;
}

//////////////////////////////////////////////////////////////////////////
CAnimParamType CAnimCVarNode::GetParamType(unsigned int nIndex) const
{
    if (nIndex == 0)
    {
        return AnimParamType::Float;
    }

    return AnimParamType::Invalid;
}

//////////////////////////////////////////////////////////////////////////
int CAnimCVarNode::GetDefaultKeyTangentFlags() const
{
    int retTangentFlags = SPLINE_KEY_TANGENT_UNIFIED;

    ICVar* var = gEnv->pConsole->GetCVar(GetNameFast());

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

//////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////
void CAnimCVarNode::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<CAnimCVarNode,CAnimNode>()
        ->Version(1);
}
