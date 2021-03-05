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
#include "CommentNode.h"
#include "AnimSplineTrack.h"
#include "CommentTrack.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimValueType.h"
#include "Maestro/Types/AnimParamType.h"


//////////////////////////////////////////////////////////////////////////
namespace
{
    bool s_nodeParamsInit = false;
    StaticInstance<std::vector<CAnimNode::SParamInfo>> s_nodeParameters;

    void AddSupportedParameters(const char* sName, AnimParamType paramId, AnimValueType valueType)
    {
        CAnimNode::SParamInfo param;
        param.name = sName;
        param.paramType = paramId;
        param.valueType = valueType;
        s_nodeParameters.push_back(param);
    }
};

//-----------------------------------------------------------------------------
CCommentNode::CCommentNode(const int id)
    : CAnimNode(id, AnimNodeType::Comment)
{
    CCommentNode::Initialize();
}

CCommentNode::CCommentNode()
    : CCommentNode(0)
{
}

//-----------------------------------------------------------------------------
void CCommentNode::Initialize()
{
    if (!s_nodeParamsInit)
    {
        s_nodeParamsInit = true;
        s_nodeParameters.reserve(3);
        AddSupportedParameters("Text", AnimParamType::CommentText, AnimValueType::Unknown);
        AddSupportedParameters("Unit Pos X", AnimParamType::PositionX, AnimValueType::Float);
        AddSupportedParameters("Unit Pos Y", AnimParamType::PositionY, AnimValueType::Float);
    }
}

//-----------------------------------------------------------------------------
void CCommentNode::Animate([[maybe_unused]] SAnimContext& ac)
{
    // It's only for valid operation of key time editing.
    // Actual animation process is in the editor side.
    CCommentTrack* pCommentTrack = static_cast<CCommentTrack*>(GetTrackForParameter(AnimParamType::CommentText));
    if (pCommentTrack)
    {
        pCommentTrack->ValidateKeyOrder();
    }
}

//-----------------------------------------------------------------------------
void CCommentNode::CreateDefaultTracks()
{
    CreateTrack(AnimParamType::CommentText);

    C2DSplineTrack* pTrack = 0;

    pTrack = (C2DSplineTrack*)CreateTrack(AnimParamType::PositionX);
    pTrack->SetDefaultValue(Vec2(0, 50));

    pTrack = (C2DSplineTrack*)CreateTrack(AnimParamType::PositionY);
    pTrack->SetDefaultValue(Vec2(0, 50));
}

//-----------------------------------------------------------------------------
void CCommentNode::OnReset()
{
}

//-----------------------------------------------------------------------------
void CCommentNode::Activate(bool bActivate)
{
    CAnimNode::Activate(bActivate);
}

//-----------------------------------------------------------------------------
/// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
void CCommentNode::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    CAnimNode::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
}

//////////////////////////////////////////////////////////////////////////
void CCommentNode::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<CCommentNode, CAnimNode>()
        ->Version(1);
}

//-----------------------------------------------------------------------------
unsigned int CCommentNode::GetParamCount() const
{
    return s_nodeParameters.size();
}

//-----------------------------------------------------------------------------
CAnimParamType CCommentNode::GetParamType(unsigned int nIndex) const
{
    if (nIndex >= 0 && nIndex < (int)s_nodeParameters.size())
    {
        return s_nodeParameters[nIndex].paramType;
    }

    return AnimParamType::Invalid;
}

//-----------------------------------------------------------------------------
bool CCommentNode::GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const
{
    for (size_t i = 0; i < s_nodeParameters.size(); ++i)
    {
        if (s_nodeParameters[i].paramType == paramId)
        {
            info = s_nodeParameters[i];
            return true;
        }
    }
    return false;
}
