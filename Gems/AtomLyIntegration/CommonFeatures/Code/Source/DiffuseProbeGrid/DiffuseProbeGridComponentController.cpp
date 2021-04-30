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

#include <DiffuseProbeGrid/DiffuseProbeGridComponentController.h>
#include <DiffuseProbeGrid/DiffuseProbeGridComponentConstants.h>

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
#include <AzFramework/Scene/SceneSystemInterface.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        void DiffuseProbeGridComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DiffuseProbeGridComponentConfig>()
                    ->Version(0)
                    ->Field("ProbeSpacing", &DiffuseProbeGridComponentConfig::m_probeSpacing)
                    ->Field("Extents", &DiffuseProbeGridComponentConfig::m_extents)
                    ->Field("AmbientMultiplier", &DiffuseProbeGridComponentConfig::m_ambientMultiplier)
                    ->Field("ViewBias", &DiffuseProbeGridComponentConfig::m_viewBias)
                    ->Field("NormalBias", &DiffuseProbeGridComponentConfig::m_normalBias)
                ;
            }
        }

        void DiffuseProbeGridComponentController::Reflect(ReflectContext* context)
        {
            DiffuseProbeGridComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DiffuseProbeGridComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &DiffuseProbeGridComponentController::m_configuration);
            }
        }

        void DiffuseProbeGridComponentController::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        void DiffuseProbeGridComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("DiffuseProbeGridService", 0x63d32042));
        }

        void DiffuseProbeGridComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("DiffuseProbeGridService", 0x63d32042));
        }

        void DiffuseProbeGridComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("BoxShapeService", 0x946a0032));
            required.push_back(AZ_CRC("TransformService"));
        }

        DiffuseProbeGridComponentController::DiffuseProbeGridComponentController(const DiffuseProbeGridComponentConfig& config)
            : m_configuration(config)
        {
        }

        void DiffuseProbeGridComponentController::Activate(AZ::EntityId entityId)
        {
            m_entityId = entityId;

            TransformNotificationBus::Handler::BusConnect(m_entityId);

            m_featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<DiffuseProbeGridFeatureProcessorInterface>(entityId);
            AZ_Assert(m_featureProcessor, "DiffuseProbeGridComponentController was unable to find a DiffuseProbeGridFeatureProcessor on the EntityContext provided.");

            m_transformInterface = TransformBus::FindFirstHandler(entityId);
            AZ_Assert(m_transformInterface, "Unable to attach to a TransformBus handler");
            if (!m_transformInterface)
            {
                return;
            }

            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(m_entityId);
            m_shapeBus = LmbrCentral::ShapeComponentRequestsBus::FindFirstHandler(m_entityId);
            AZ_Assert(m_shapeBus, "DiffuseProbeGridComponentController was unable to find ShapeComponentNotificationsBus");

            m_boxShapeInterface = LmbrCentral::BoxShapeComponentRequestsBus::FindFirstHandler(m_entityId);
            AZ_Assert(m_boxShapeInterface, "DiffuseProbeGridComponentController was unable to find box shape component");

            // add this diffuse probe grid to the feature processor
            const AZ::Transform& transform = m_transformInterface->GetWorldTM();
            m_handle = m_featureProcessor->AddProbeGrid(transform, m_configuration.m_extents, m_configuration.m_probeSpacing);

            m_featureProcessor->SetAmbientMultiplier(m_handle, m_configuration.m_ambientMultiplier);
            m_featureProcessor->SetViewBias(m_handle, m_configuration.m_viewBias);
            m_featureProcessor->SetNormalBias(m_handle, m_configuration.m_normalBias);

            // set box shape component dimensions from the configuration
            // this will invoke the OnShapeChanged() handler and set the outer extents on the feature processor
            m_boxShapeInterface->SetBoxDimensions(m_configuration.m_extents);
        }

        void DiffuseProbeGridComponentController::Deactivate()
        {
            if (m_featureProcessor)
            {
                m_featureProcessor->RemoveProbeGrid(m_handle);
            }

            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
            Data::AssetBus::MultiHandler::BusDisconnect();
            TransformNotificationBus::Handler::BusDisconnect();

            m_transformInterface = nullptr;
            m_featureProcessor = nullptr;
            m_shapeBus = nullptr;
            m_boxShapeInterface = nullptr;
        }

        void DiffuseProbeGridComponentController::SetConfiguration(const DiffuseProbeGridComponentConfig& config)
        {
            m_configuration = config;
        }

        const DiffuseProbeGridComponentConfig& DiffuseProbeGridComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void DiffuseProbeGridComponentController::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
        {
            if (!m_featureProcessor)
            {
                return;
            }

            m_featureProcessor->SetTransform(m_handle, world);
        }

        void DiffuseProbeGridComponentController::OnShapeChanged(ShapeChangeReasons changeReason)
        {
            if (!m_featureProcessor)
            {
                return;
            }

            if (m_inShapeChangeHandler)
            {
                return;
            }

            m_inShapeChangeHandler = true;

            AZ_Assert(m_featureProcessor->IsValidProbeGridHandle(m_handle), "OnShapeChanged handler called before probe was registered with feature processor");

            if (changeReason == ShapeChangeReasons::ShapeChanged)
            {
                AZ::Vector3 dimensions = m_boxShapeInterface->GetBoxDimensions();
                if (m_featureProcessor->ValidateExtents(m_handle, dimensions))
                {
                    m_featureProcessor->SetExtents(m_handle, dimensions);
                    m_configuration.m_extents = dimensions;
                }
                else
                {
                    // restore old dimensions
                    m_boxShapeInterface->SetBoxDimensions(m_configuration.m_extents);
                }
            }

            m_inShapeChangeHandler = false;
        }

        AZ::Aabb DiffuseProbeGridComponentController::GetAabb() const
        {
            return m_shapeBus ? m_shapeBus->GetEncompassingAabb() : AZ::Aabb::CreateNull();
        }

        bool DiffuseProbeGridComponentController::ValidateProbeSpacing(const AZ::Vector3& newSpacing)
        {
            return m_featureProcessor->ValidateProbeSpacing(m_handle, newSpacing);
        }

        void DiffuseProbeGridComponentController::SetProbeSpacing(const AZ::Vector3& probeSpacing)
        {
            m_configuration.m_probeSpacing = probeSpacing;
            m_featureProcessor->SetProbeSpacing(m_handle, m_configuration.m_probeSpacing);
        }

        void DiffuseProbeGridComponentController::SetAmbientMultiplier(float ambientMultiplier)
        {
            m_configuration.m_ambientMultiplier = ambientMultiplier;
            m_featureProcessor->SetAmbientMultiplier(m_handle, m_configuration.m_ambientMultiplier);
        }

        void DiffuseProbeGridComponentController::SetViewBias(float viewBias)
        {
            m_configuration.m_viewBias = viewBias;
            m_featureProcessor->SetViewBias(m_handle, m_configuration.m_viewBias);
        }

        void DiffuseProbeGridComponentController::SetNormalBias(float normalBias)
        {
            m_configuration.m_normalBias = normalBias;
            m_featureProcessor->SetNormalBias(m_handle, m_configuration.m_normalBias);
        }
    } // namespace Render
} // namespace AZ
