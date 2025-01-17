/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "CommentNode.h"
#include "AnimSplineTrack.h"
#include "CommentTrack.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimValueType.h"
#include "Maestro/Types/AnimParamType.h"

namespace Maestro
{

    namespace
    {
        bool s_nodeParamsInit = false;
        AZStd::vector<CAnimNode::SParamInfo> s_nodeParameters;

        void AddSupportedParameters(const char* sName, AnimParamType paramId, AnimValueType valueType)
        {
            CAnimNode::SParamInfo param;
            param.name = sName;
            param.paramType = paramId;
            param.valueType = valueType;
            s_nodeParameters.push_back(param);
        }
    } // namespace

    CCommentNode::CCommentNode(const int id)
        : CAnimNode(id, AnimNodeType::Comment)
    {
        CCommentNode::Initialize();
    }

    CCommentNode::CCommentNode()
        : CCommentNode(0)
    {
    }

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

    void CCommentNode::CreateDefaultTracks()
    {
        CreateTrack(AnimParamType::CommentText);

        C2DSplineTrack* pTrack = 0;

        pTrack = (C2DSplineTrack*)CreateTrack(AnimParamType::PositionX);
        pTrack->SetDefaultValue(Vec2(0, 50));

        pTrack = (C2DSplineTrack*)CreateTrack(AnimParamType::PositionY);
        pTrack->SetDefaultValue(Vec2(0, 50));
    }

    void CCommentNode::OnReset()
    {
    }

    void CCommentNode::Activate(bool bActivate)
    {
        CAnimNode::Activate(bActivate);
    }

    /// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through
    /// AZ::SerializeContext and the SequenceComponent
    void CCommentNode::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
    {
        CAnimNode::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
    }

    void CCommentNode::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CCommentNode, CAnimNode>()->Version(1);
        }
    }

    unsigned int CCommentNode::GetParamCount() const
    {
        return static_cast<unsigned int>(s_nodeParameters.size());
    }

    CAnimParamType CCommentNode::GetParamType(unsigned int nIndex) const
    {
        if (nIndex < s_nodeParameters.size())
        {
            return s_nodeParameters[nIndex].paramType;
        }

        return AnimParamType::Invalid;
    }

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

} // namespace Maestro
