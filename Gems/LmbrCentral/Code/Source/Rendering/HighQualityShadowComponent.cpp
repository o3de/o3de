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
#include "LmbrCentral_precompiled.h"
#include "HighQualityShadowComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <LmbrCentral/Rendering/RenderNodeBus.h>
#include <I3DEngine.h>


namespace LmbrCentral
{
    //////////////////////////////////////////////////////////////////////////

    HighQualityShadowConfig::HighQualityShadowConfig()
        : m_enabled(true)
        , m_constBias(0.001f)
        , m_slopeBias(0.01f)
        , m_jitter(0.01f)
        , m_bboxScale(1,1,1)
        , m_shadowMapSize(1024)
    {

    }

    void HighQualityShadowConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<HighQualityShadowConfig>()
                ->Version(1)
                ->Field("Enabled", &HighQualityShadowConfig::m_enabled)
                ->Field("ConstBias", &HighQualityShadowConfig::m_constBias)
                ->Field("SlopeBias", &HighQualityShadowConfig::m_slopeBias)
                ->Field("Jitter", &HighQualityShadowConfig::m_jitter)
                ->Field("BBoxScale", &HighQualityShadowConfig::m_bboxScale)
                ->Field("ShadowMapSize", &HighQualityShadowConfig::m_shadowMapSize)
                ;
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void HighQualityShadowComponent::Reflect(AZ::ReflectContext* context)
    {
        HighQualityShadowConfig::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<HighQualityShadowComponent, AZ::Component>()
                ->Version(1)
                ->Field("HighQualityShadowConfig", &HighQualityShadowComponent::m_config);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<HighQualityShadowComponentRequestBus>("HighQualityShadowComponentRequestBus")
                ->Event("SetEnabled", &HighQualityShadowComponentRequestBus::Events::SetEnabled, { { {"Enabled", ""} } })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Enables or disables the High Quality Shadow")
                ->Event("GetEnabled", &HighQualityShadowComponentRequestBus::Events::GetEnabled)
                ->Attribute(AZ::Script::Attributes::ToolTip, "Returns whether the High Quality Shadow is enabled")
                ->VirtualProperty("Enabled", "GetEnabled", "SetEnabled");
        }
    }



    void HighQualityShadowComponent::Activate()
    {
        HighQualityShadowComponentRequestBus::Handler::BusConnect(GetEntityId());
        MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void HighQualityShadowComponent::Deactivate()
    {
        HighQualityShadowComponentUtils::RemoveShadow(GetEntityId());
        HighQualityShadowComponentRequestBus::Handler::BusDisconnect();
        MeshComponentNotificationBus::Handler::BusDisconnect(GetEntityId());
    }

    bool HighQualityShadowComponent::GetEnabled()
    {
        return m_config.m_enabled;
    }

    void HighQualityShadowComponent::SetEnabled(bool enabled)
    {
        m_config.m_enabled = enabled;
        HighQualityShadowComponentUtils::ApplyShadowSettings(GetEntityId(), m_config);
    }

    void HighQualityShadowComponentUtils::ApplyShadowSettings(AZ::EntityId entityId, const HighQualityShadowConfig& config)
    {
        int numRenderNodesApplied = 0;

        AZ::EBusAggregateResults<IRenderNode*> renderNodes;
        LmbrCentral::RenderNodeRequestBus::EventResult(renderNodes, entityId, &LmbrCentral::RenderNodeRequests::GetRenderNode);
        for(IRenderNode* renderNode : renderNodes.values)
        {
            if (renderNode)
            {
                const EERType type = renderNode->GetRenderNodeType();

                const bool typeIsSupported =
                    type == eERType_StaticMeshRenderComponent  || // Mesh Component uses this
                    type == eERType_DynamicMeshRenderComponent || // Mesh Component uses this
                    type == eERType_SkinnedMeshRenderComponent || // Skinned Mesh Component uses this; probably isn't necessary since this is deprecated, but is included for completion's sake
                    type == eERType_RenderComponent;              // Actor Component uses this

                if (typeIsSupported)
                {
                    ++numRenderNodesApplied;

                    if (config.m_enabled && renderNode->IsReady()) // Note that if the mesh isn't ready yet, OnMeshCreated will be called when it is ready, which will call ApplyShadowSettings again
                    {
                        const Vec3 bboxScaleVec3(config.m_bboxScale.GetX(), config.m_bboxScale.GetY(), config.m_bboxScale.GetZ());
                        gEnv->p3DEngine->AddPerObjectShadow(renderNode, config.m_constBias, config.m_slopeBias, config.m_jitter, bboxScaleVec3, config.m_shadowMapSize);
                    }
                    else
                    {
                        gEnv->p3DEngine->RemovePerObjectShadow(renderNode);
                    }
                }

            }
        }

        // This should never fail because "MeshService" components are mutually exclusive. It is possible that numRenderNodesApplied be 0, 
        // because some mesh-based components, like ActorComponent, may return a null IRenderNode when they aren't initialized yet.
        AZ_Assert(numRenderNodesApplied <= 1, "Expected exactly one mesh-based component. Found %d", numRenderNodesApplied);
    }

    void HighQualityShadowComponentUtils::RemoveShadow(AZ::EntityId entityId)
    {
        AZ::EBusAggregateResults<IRenderNode*> renderNodes;
        LmbrCentral::RenderNodeRequestBus::EventResult(renderNodes, entityId, &LmbrCentral::RenderNodeRequests::GetRenderNode);
        for (IRenderNode* renderNode : renderNodes.values)
        {
            if (renderNode)
            {
                gEnv->p3DEngine->RemovePerObjectShadow(renderNode);
            }
        }
    }

    void HighQualityShadowComponent::OnMeshCreated([[maybe_unused]] const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        HighQualityShadowComponentUtils::ApplyShadowSettings(GetEntityId(), m_config);
    }

    void HighQualityShadowComponent::OnMeshDestroyed()
    {
        HighQualityShadowComponentUtils::RemoveShadow(GetEntityId());
    }

} // namespace LmbrCentral

