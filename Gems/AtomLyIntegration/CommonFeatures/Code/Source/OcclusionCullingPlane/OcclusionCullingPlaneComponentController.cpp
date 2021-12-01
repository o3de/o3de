/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OcclusionCullingPlane/OcclusionCullingPlaneComponentController.h>
#include <OcclusionCullingPlane/OcclusionCullingPlaneComponentConstants.h>

#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Scene.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Scene/Scene.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        void OcclusionCullingPlaneComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<OcclusionCullingPlaneComponentConfig>()
                    ->Version(0)
                    ->Field("ShowVisualization", &OcclusionCullingPlaneComponentConfig::m_showVisualization)
                    ->Field("TransparentVisualization", &OcclusionCullingPlaneComponentConfig::m_transparentVisualization)
                    ;
            }
        }

        void OcclusionCullingPlaneComponentController::Reflect(ReflectContext* context)
        {
            OcclusionCullingPlaneComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<OcclusionCullingPlaneComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &OcclusionCullingPlaneComponentController::m_configuration);
            }
        }

        void OcclusionCullingPlaneComponentController::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        void OcclusionCullingPlaneComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("OcclusionCullingPlaneService", 0x7d036c2e));
        }

        void OcclusionCullingPlaneComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("OcclusionCullingPlaneService", 0x7d036c2e));
        }

        void OcclusionCullingPlaneComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService"));
        }

        OcclusionCullingPlaneComponentController::OcclusionCullingPlaneComponentController(const OcclusionCullingPlaneComponentConfig& config)
            : m_configuration(config)
        {
        }

        void OcclusionCullingPlaneComponentController::Activate(AZ::EntityId entityId)
        {
            m_entityId = entityId;

            TransformNotificationBus::Handler::BusConnect(m_entityId);

            m_featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<OcclusionCullingPlaneFeatureProcessorInterface>(entityId);
            AZ_Assert(m_featureProcessor, "OcclusionCullingPlaneComponentController was unable to find a OcclusionCullingPlaneFeatureProcessor on the EntityContext provided.");

            m_transformInterface = TransformBus::FindFirstHandler(entityId);
            AZ_Assert(m_transformInterface, "Unable to attach to a TransformBus handler");
            if (!m_transformInterface)
            {
                return;
            }

            // add this occlusion plane to the feature processor
            const AZ::Transform& transform = m_transformInterface->GetWorldTM();
            m_handle = m_featureProcessor->AddOcclusionCullingPlane(transform);

            // set visualization
            m_featureProcessor->ShowVisualization(m_handle, m_configuration.m_showVisualization);
            m_featureProcessor->SetTransparentVisualization(m_handle, m_configuration.m_transparentVisualization);
        }

        void OcclusionCullingPlaneComponentController::Deactivate()
        {
            if (m_featureProcessor)
            {
                m_featureProcessor->RemoveOcclusionCullingPlane(m_handle);
            }

            Data::AssetBus::MultiHandler::BusDisconnect();
            TransformNotificationBus::Handler::BusDisconnect();

            m_transformInterface = nullptr;
            m_featureProcessor = nullptr;
        }

        void OcclusionCullingPlaneComponentController::SetConfiguration(const OcclusionCullingPlaneComponentConfig& config)
        {
            m_configuration = config;
        }

        const OcclusionCullingPlaneComponentConfig& OcclusionCullingPlaneComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void OcclusionCullingPlaneComponentController::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
        {
            if (!m_featureProcessor)
            {
                return;
            }

            m_featureProcessor->SetTransform(m_handle, world);
        }
    } // namespace Render
} // namespace AZ
