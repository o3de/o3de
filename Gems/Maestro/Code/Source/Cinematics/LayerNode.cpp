/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "LayerNode.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimValueType.h"
#include "Maestro/Types/AnimParamType.h"

namespace Maestro
{

    namespace LayerNodeHelper
    {
        static bool s_nodeParamsInitialized = false;
        static AZStd::vector<CAnimNode::SParamInfo> s_nodeParams;

        static void AddSupportedParam(const char* sName, AnimParamType paramId, AnimValueType valueType)
        {
            CAnimNode::SParamInfo param;
            param.name = sName;
            param.paramType = paramId;
            param.valueType = valueType;
            s_nodeParams.push_back(param);
        }
    } // namespace LayerNodeHelper

    CLayerNode::CLayerNode()
        : CLayerNode(0)
    {
    }

    CLayerNode::CLayerNode(const int id)
        : CAnimNode(id, AnimNodeType::Layer)
        , m_bInit(false)
        , m_bPreVisibility(true)
    {
        CLayerNode::Initialize();
    }

    void CLayerNode::Initialize()
    {
        using namespace LayerNodeHelper;
        if (!s_nodeParamsInitialized)
        {
            s_nodeParamsInitialized = true;
            s_nodeParams.reserve(1);
            AddSupportedParam("Visibility", AnimParamType::Visibility, AnimValueType::Bool);
        }
    }

    void CLayerNode::Animate(SAnimContext& ec)
    {
        int trackCount = NumTracks();
        for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
        {
            CAnimParamType paramType = m_tracks[paramIndex]->GetParameterType();
            IAnimTrack* pTrack = m_tracks[paramIndex].get();
            if (pTrack->GetNumKeys() == 0)
            {
                continue;
            }

            if (pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled)
            {
                continue;
            }

            if (pTrack->IsMasked(ec.trackMask))
            {
                continue;
            }

            switch (paramType.GetType())
            {
            case AnimParamType::Visibility:
                if (!ec.resetting)
                {
                    IAnimTrack* visTrack = pTrack;
                    bool visible = true;
                    visTrack->GetValue(ec.time, visible);

                    if (m_bInit)
                    {
                        if (visible != m_bPreVisibility)
                        {
                            m_bPreVisibility = visible;
                        }
                    }
                    else
                    {
                        m_bInit = true;
                        m_bPreVisibility = visible;
                    }
                }
                break;
            }
        }
    }

    void CLayerNode::CreateDefaultTracks()
    {
        CreateTrack(AnimParamType::Visibility);
    }

    void CLayerNode::OnReset()
    {
        m_bInit = false;
    }

    void CLayerNode::Activate(bool bActivate)
    {
        CAnimNode::Activate(bActivate);
    }

    /// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence
    /// Component
    void CLayerNode::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
    {
        CAnimNode::Serialize(xmlNode, bLoading, bLoadEmptyTracks);

        // Nothing to be serialized at this moment.
    }

    unsigned int CLayerNode::GetParamCount() const
    {
        return static_cast<unsigned int>(LayerNodeHelper::s_nodeParams.size());
    }

    CAnimParamType CLayerNode::GetParamType(unsigned int nIndex) const
    {
        using namespace LayerNodeHelper;
        if (nIndex < (int)s_nodeParams.size())
        {
            return s_nodeParams[nIndex].paramType;
        }

        return AnimParamType::Invalid;
    }

    bool CLayerNode::GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const
    {
        using namespace LayerNodeHelper;
        for (unsigned int i = 0; i < s_nodeParams.size(); i++)
        {
            if (s_nodeParams[i].paramType == paramId)
            {
                info = s_nodeParams[i];
                return true;
            }
        }
        return false;
    }

    void CLayerNode::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CLayerNode, CAnimNode>()->Version(1);
        }
    }

} // namespace Maestro
