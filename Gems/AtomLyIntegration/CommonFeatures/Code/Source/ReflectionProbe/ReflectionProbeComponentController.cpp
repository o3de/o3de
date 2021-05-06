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

#include <ReflectionProbe/ReflectionProbeComponentController.h>
#include <ReflectionProbe/ReflectionProbeComponentConstants.h>

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
        void ReflectionProbeComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ReflectionProbeComponentConfig>()
                    ->Version(0)
                    ->Field("OuterHeight", &ReflectionProbeComponentConfig::m_outerHeight)
                    ->Field("OuterLength", &ReflectionProbeComponentConfig::m_outerLength)
                    ->Field("OuterWidth", &ReflectionProbeComponentConfig::m_outerWidth)
                    ->Field("InnerHeight", &ReflectionProbeComponentConfig::m_innerHeight)
                    ->Field("InnerLength", &ReflectionProbeComponentConfig::m_innerLength)
                    ->Field("InnerWidth", &ReflectionProbeComponentConfig::m_innerWidth)
                    ->Field("UseBakedCubemap", &ReflectionProbeComponentConfig::m_useBakedCubemap)
                    ->Field("BakedCubeMapRelativePath", &ReflectionProbeComponentConfig::m_bakedCubeMapRelativePath)
                    ->Field("BakedCubeMapAsset", &ReflectionProbeComponentConfig::m_bakedCubeMapAsset)
                    ->Field("AuthoredCubeMapAsset", &ReflectionProbeComponentConfig::m_authoredCubeMapAsset)
                    ->Field("EntityId", &ReflectionProbeComponentConfig::m_entityId)
                    ->Field("UseParallaxCorrection", &ReflectionProbeComponentConfig::m_useParallaxCorrection)
                    ->Field("ShowVisualization", &ReflectionProbeComponentConfig::m_showVisualization);
            }
        }

        void ReflectionProbeComponentController::Reflect(ReflectContext* context)
        {
            ReflectionProbeComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ReflectionProbeComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &ReflectionProbeComponentController::m_configuration);
            }
        }

        void ReflectionProbeComponentController::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        void ReflectionProbeComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ReflectionProbeService", 0xa5b919ce));
        }

        void ReflectionProbeComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ReflectionProbeService", 0xa5b919ce));
        }

        void ReflectionProbeComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("BoxShapeService", 0x946a0032));
        }

        ReflectionProbeComponentController::ReflectionProbeComponentController(const ReflectionProbeComponentConfig& config)
            : m_configuration(config)
        {
        }

        void ReflectionProbeComponentController::Activate(AZ::EntityId entityId)
        {
            m_entityId = entityId;

            TransformNotificationBus::Handler::BusConnect(m_entityId);
            AzFramework::BoundsRequestBus::Handler::BusConnect(entityId);

            m_featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<ReflectionProbeFeatureProcessorInterface>(entityId);
            AZ_Assert(m_featureProcessor, "ReflectionProbeComponentController was unable to find a ReflectionProbeFeatureProcessor on the EntityContext provided.");

            m_transformInterface = TransformBus::FindFirstHandler(entityId);
            AZ_Warning("ReflectionProbeComponentController", m_transformInterface, "Unable to attach to a TransformBus handler. This mesh will always be rendered at the origin.");

            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(m_entityId);
            m_shapeBus = LmbrCentral::ShapeComponentRequestsBus::FindFirstHandler(m_entityId);

            m_boxShapeInterface = LmbrCentral::BoxShapeComponentRequestsBus::FindFirstHandler(m_entityId);
            AZ_Assert(m_boxShapeInterface, "ReflectionProbeComponentController was unable to find box shape component");

            // special handling is required if this component is being cloned in the editor:
            // if this probe is using a baked cubemap, check to see if it is already referenced by another probe
            if (m_configuration.m_useBakedCubemap)
            {
                if (m_featureProcessor->IsCubeMapReferenced(m_configuration.m_bakedCubeMapRelativePath))
                {
                    // clear the cubeMapRelativePath to prevent the newly cloned reflection probe
                    // from using the same cubemap path as the original reflection probe
                    m_configuration.m_bakedCubeMapRelativePath = "";
                }
            }

            // add this reflection probe to the feature processor
            const AZ::Transform& transform = m_transformInterface->GetWorldTM();
            m_handle = m_featureProcessor->AddProbe(transform, m_configuration.m_useParallaxCorrection);

            // set the visualization sphere option
            m_featureProcessor->ShowProbeVisualization(m_handle, m_configuration.m_showVisualization);

            // update the outer extents from the box shape
            // if the user already resized the box shape on this entity it will inherit those extents
            UpdateOuterExtents();

            // set the inner extents
            m_featureProcessor->SetProbeInnerExtents(m_handle, AZ::Vector3(m_configuration.m_innerWidth, m_configuration.m_innerLength, m_configuration.m_innerHeight));

            // load cubemap
            Data::Asset<RPI::StreamingImageAsset>& cubeMapAsset =
                m_configuration.m_useBakedCubemap ? m_configuration.m_bakedCubeMapAsset : m_configuration.m_authoredCubeMapAsset;
            Data::AssetBus::MultiHandler::BusConnect(cubeMapAsset.GetId());

            const AZStd::string& relativePath =
                m_configuration.m_useBakedCubemap ? m_configuration.m_bakedCubeMapRelativePath : m_configuration.m_authoredCubeMapAsset.GetHint();

            if (cubeMapAsset.GetId().IsValid())
            {
                if (cubeMapAsset.IsReady())
                {
                    Data::Instance<RPI::Image> image = RPI::StreamingImage::FindOrCreate(cubeMapAsset);
                    m_featureProcessor->SetProbeCubeMap(m_handle, image, relativePath);
                }
                else
                {
                    cubeMapAsset.QueueLoad();
                }
            }
        }

        void ReflectionProbeComponentController::Deactivate()
        {
            if (m_featureProcessor)
            {
                m_featureProcessor->RemoveProbe(m_handle);
            }

            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
            Data::AssetBus::MultiHandler::BusDisconnect();
            AzFramework::BoundsRequestBus::Handler::BusDisconnect();
            TransformNotificationBus::Handler::BusDisconnect();

            m_transformInterface = nullptr;
            m_featureProcessor = nullptr;
            m_shapeBus = nullptr;
            m_boxShapeInterface = nullptr;
        }

        void ReflectionProbeComponentController::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            if (!m_featureProcessor)
            {
                return;
            }

            const AZStd::string& relativePath =
                m_configuration.m_useBakedCubemap ? m_configuration.m_bakedCubeMapRelativePath : m_configuration.m_authoredCubeMapAsset.GetHint();

            Data::Instance<RPI::Image> image = RPI::StreamingImage::FindOrCreate(asset);
            m_featureProcessor->SetProbeCubeMap(m_handle, image, relativePath);
        }

        void ReflectionProbeComponentController::SetConfiguration(const ReflectionProbeComponentConfig& config)
        {
            m_configuration = config;
        }

        const ReflectionProbeComponentConfig& ReflectionProbeComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void ReflectionProbeComponentController::UpdateCubeMap()
        {
            Data::Asset<RPI::StreamingImageAsset>& cubeMapAsset =
                m_configuration.m_useBakedCubemap ? m_configuration.m_bakedCubeMapAsset : m_configuration.m_authoredCubeMapAsset;

            const AZStd::string& relativePath =
                m_configuration.m_useBakedCubemap ? m_configuration.m_bakedCubeMapRelativePath : m_configuration.m_authoredCubeMapAsset.GetHint();

            Data::Instance<RPI::Image> image = RPI::StreamingImage::FindOrCreate(cubeMapAsset);
            m_featureProcessor->SetProbeCubeMap(m_handle, image, relativePath);
        }

        void ReflectionProbeComponentController::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
        {
            if (!m_featureProcessor)
            {
                return;
            }

            m_featureProcessor->SetProbeTransform(m_handle, world);
        }

        void ReflectionProbeComponentController::OnShapeChanged(ShapeChangeReasons changeReason)
        {
            if (!m_featureProcessor)
            {
                return;
            }

            AZ_Assert(m_featureProcessor->IsValidProbeHandle(m_handle), "OnShapeChanged handler called before probe was registered with feature processor");

            if (changeReason == ShapeChangeReasons::ShapeChanged)
            {
                UpdateOuterExtents();
            }
        }

        void ReflectionProbeComponentController::UpdateOuterExtents()
        {
            if (!m_featureProcessor)
            {
                return;
            }

            AZ::Vector3 dimensions = m_boxShapeInterface->GetBoxDimensions();
            m_featureProcessor->SetProbeOuterExtents(m_handle, dimensions);

            m_configuration.m_outerWidth = dimensions.GetX();
            m_configuration.m_outerLength = dimensions.GetY();
            m_configuration.m_outerHeight = dimensions.GetZ();

            AzFramework::EntityBoundsUnionRequestBus::Broadcast(
                &AzFramework::EntityBoundsUnionRequestBus::Events::RefreshEntityLocalBoundsUnion, m_entityId);

            // clamp the inner extents to the outer extents
            m_configuration.m_innerWidth = AZStd::min(m_configuration.m_innerWidth, m_configuration.m_outerWidth);
            m_configuration.m_innerLength = AZStd::min(m_configuration.m_innerLength, m_configuration.m_outerLength);
            m_configuration.m_innerHeight = AZStd::min(m_configuration.m_innerHeight, m_configuration.m_outerHeight);
        }

        void ReflectionProbeComponentController::BakeReflectionProbe(BuildCubeMapCallback callback, const AZStd::string& relativePath)
        {
            if (!m_featureProcessor)
            {
                return;
            }

            m_featureProcessor->BakeProbe(m_handle, callback, relativePath);
        }

        AZ::Aabb ReflectionProbeComponentController::GetAabb() const
        {
            return m_shapeBus ? m_shapeBus->GetEncompassingAabb() : AZ::Aabb();
        }

        AZ::Aabb ReflectionProbeComponentController::GetWorldBounds()
        {
            return GetAabb();
        }

        AZ::Aabb ReflectionProbeComponentController::GetLocalBounds()
        {
            if (!m_shapeBus)
            {
                return Aabb::CreateNull();
            }

            AZ::Transform unused;
            AZ::Aabb localBounds = AZ::Aabb::CreateNull();
            m_shapeBus->GetTransformAndLocalBounds(unused, localBounds);
            return localBounds;
        }
    } // namespace Render
} // namespace AZ
