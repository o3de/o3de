/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ReflectionProbe/ReflectionProbeComponentController.h>
#include <ReflectionProbe/ReflectionProbeComponentConstants.h>

#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Scene.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetSerializer.h>
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
                    ->Version(1)
                    ->Field("OuterHeight", &ReflectionProbeComponentConfig::m_outerHeight)
                    ->Field("OuterLength", &ReflectionProbeComponentConfig::m_outerLength)
                    ->Field("OuterWidth", &ReflectionProbeComponentConfig::m_outerWidth)
                    ->Field("InnerHeight", &ReflectionProbeComponentConfig::m_innerHeight)
                    ->Field("InnerLength", &ReflectionProbeComponentConfig::m_innerLength)
                    ->Field("InnerWidth", &ReflectionProbeComponentConfig::m_innerWidth)
                    ->Field("UseBakedCubemap", &ReflectionProbeComponentConfig::m_useBakedCubemap)
                    ->Field("BakedCubemapQualityLevel", &ReflectionProbeComponentConfig::m_bakedCubeMapQualityLevel)
                    ->Field("BakedCubeMapRelativePath", &ReflectionProbeComponentConfig::m_bakedCubeMapRelativePath)
                    ->Field("BakedCubeMapAsset", &ReflectionProbeComponentConfig::m_bakedCubeMapAsset)
                    ->Field("AuthoredCubeMapAsset", &ReflectionProbeComponentConfig::m_authoredCubeMapAsset)
                    ->Field("EntityId", &ReflectionProbeComponentConfig::m_entityId)
                    ->Field("UseParallaxCorrection", &ReflectionProbeComponentConfig::m_useParallaxCorrection)
                    ->Field("ShowVisualization", &ReflectionProbeComponentConfig::m_showVisualization)
                    ->Field("RenderExposure", &ReflectionProbeComponentConfig::m_renderExposure)
                    ->Field("BakeExposure", &ReflectionProbeComponentConfig::m_bakeExposure);
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
            dependent.push_back(AZ_CRC_CE("TransformService"));
        }

        void ReflectionProbeComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ReflectionProbeService"));
        }

        void ReflectionProbeComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("ReflectionProbeService"));
            incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
        }

        void ReflectionProbeComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("BoxShapeService"));
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
            m_handle = m_featureProcessor->AddReflectionProbe(
                ComputeOverallTransform(m_transformInterface->GetWorldTM()), m_configuration.m_useParallaxCorrection);

            // set the visualization sphere option
            m_featureProcessor->ShowVisualization(m_handle, m_configuration.m_showVisualization);

            // if this is a new ReflectionProbe entity and the box shape has not been changed (i.e., it's still unit sized)
            // then set the shape to the default extents
            AZ::Vector3 boxDimensions = m_boxShapeInterface->GetBoxDimensions();
            if (m_configuration.m_entityId == EntityId::InvalidEntityId && boxDimensions == AZ::Vector3(1.0f))
            {
                AZ::Vector3 extents(m_configuration.m_outerWidth, m_configuration.m_outerLength, m_configuration.m_outerHeight);

                // resize the box shape, this will invoke OnShapeChanged
                m_boxShapeInterface->SetBoxDimensions(extents);
            }
            else
            {
                // update the outer extents from the box shape
                UpdateOuterExtents();
            }

            // set the inner extents
            m_featureProcessor->SetInnerExtents(m_handle, AZ::Vector3(m_configuration.m_innerWidth, m_configuration.m_innerLength, m_configuration.m_innerHeight));

            // load cubemap
            Data::Asset<RPI::StreamingImageAsset>& cubeMapAsset =
                m_configuration.m_useBakedCubemap ? m_configuration.m_bakedCubeMapAsset : m_configuration.m_authoredCubeMapAsset;

            if (cubeMapAsset.GetId().IsValid())
            {
                cubeMapAsset.QueueLoad();
                Data::AssetBus::MultiHandler::BusConnect(cubeMapAsset.GetId());
            }

            // set cubemap render exposure
            m_featureProcessor->SetRenderExposure(m_handle, m_configuration.m_renderExposure);
        }

        void ReflectionProbeComponentController::Deactivate()
        {
            if (m_featureProcessor)
            {
                m_featureProcessor->RemoveReflectionProbe(m_handle);
                m_handle = ReflectionProbeHandle{};
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
            m_featureProcessor->SetCubeMap(m_handle, image, relativePath);
        }

        void ReflectionProbeComponentController::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            if (m_configuration.m_useBakedCubemap)
            {
                m_configuration.m_bakedCubeMapAsset = asset;
            }
            else
            {
                m_configuration.m_authoredCubeMapAsset = asset;
            }
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
            // disconnect the asset bus since we might reconnect with a different asset
            Data::AssetBus::MultiHandler::BusDisconnect();

            Data::Asset<RPI::StreamingImageAsset>& cubeMapAsset =
                m_configuration.m_useBakedCubemap ? m_configuration.m_bakedCubeMapAsset : m_configuration.m_authoredCubeMapAsset;

            if (cubeMapAsset.GetId().IsValid())
            {
                cubeMapAsset.QueueLoad();

                // this will invoke OnAssetReady()
                Data::AssetBus::MultiHandler::BusConnect(cubeMapAsset.GetId());
            }
            else
            {
                // clear the current cubemap
                Data::Instance<RPI::Image> image = nullptr;
                m_featureProcessor->SetCubeMap(m_handle, image, {});
            }
        }

        void ReflectionProbeComponentController::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
        {
            if (!m_featureProcessor)
            {
                return;
            }

            m_featureProcessor->SetTransform(m_handle, ComputeOverallTransform(world));
        }

        void ReflectionProbeComponentController::OnShapeChanged(ShapeChangeReasons changeReason)
        {
            if (!m_featureProcessor)
            {
                return;
            }

            AZ_Assert(m_featureProcessor->IsValidHandle(m_handle), "OnShapeChanged handler called before probe was registered with feature processor");

            if (changeReason == ShapeChangeReasons::ShapeChanged)
            {
                UpdateOuterExtents();
                // the shape translation offset may have changed, which would affect the overall transform
                m_featureProcessor->SetTransform(m_handle, ComputeOverallTransform(m_transformInterface->GetWorldTM()));
            }
        }

        void ReflectionProbeComponentController::UpdateOuterExtents()
        {
            if (!m_featureProcessor)
            {
                return;
            }

            AZ::Vector3 dimensions = m_boxShapeInterface->GetBoxDimensions();
            m_featureProcessor->SetOuterExtents(m_handle, dimensions);

            m_configuration.m_outerWidth = dimensions.GetX();
            m_configuration.m_outerLength = dimensions.GetY();
            m_configuration.m_outerHeight = dimensions.GetZ();

            AZ::Interface<AzFramework::IEntityBoundsUnion>::Get()->RefreshEntityLocalBoundsUnion(m_entityId);

            // clamp the inner extents to the outer extents
            m_configuration.m_innerWidth = AZStd::min(m_configuration.m_innerWidth, m_configuration.m_outerWidth);
            m_configuration.m_innerLength = AZStd::min(m_configuration.m_innerLength, m_configuration.m_outerLength);
            m_configuration.m_innerHeight = AZStd::min(m_configuration.m_innerHeight, m_configuration.m_outerHeight);

            m_innerExtentsChangedEvent.Signal(true);
        }

        void ReflectionProbeComponentController::SetBakeExposure(float bakeExposure)
        {
            if (!m_featureProcessor)
            {
                return;
            }

            m_featureProcessor->SetBakeExposure(m_handle, bakeExposure);
        }

        void ReflectionProbeComponentController::BakeReflectionProbe(BuildCubeMapCallback callback, const AZStd::string& relativePath)
        {
            if (!m_featureProcessor)
            {
                return;
            }

            m_featureProcessor->Bake(m_handle, callback, relativePath);
        }

        AZ::Aabb ReflectionProbeComponentController::GetAabb() const
        {
            return m_shapeBus ? m_shapeBus->GetEncompassingAabb() : AZ::Aabb();
        }

        AZ::Aabb ReflectionProbeComponentController::GetWorldBounds() const
        {
            return GetAabb();
        }

        AZ::Aabb ReflectionProbeComponentController::GetLocalBounds() const
        {
            if (!m_shapeBus)
            {
                return Aabb::CreateNull();
            }

            AZ::Transform unused;
            AZ::Aabb localBounds = AZ::Aabb::CreateNull();
            m_shapeBus->GetTransformAndLocalBounds(unused, localBounds);
            if (!m_boxShapeInterface->IsTypeAxisAligned())
            {
                return localBounds;
            }
            else
            {
                return localBounds.GetTransformedAabb(
                    AZ::Transform::CreateFromQuaternion(m_transformInterface->GetWorldTM().GetRotation().GetInverseFast()));
            }
        }

        void ReflectionProbeComponentController::RegisterInnerExtentsChangedHandler(AZ::Event<bool>::Handler& handler)
        {
            handler.Connect(m_innerExtentsChangedEvent);
        }

        AZ::Transform ReflectionProbeComponentController::ComputeOverallTransform(const AZ::Transform& entityTransform) const
        {
            const bool isTypeAxisAligned = m_boxShapeInterface ? m_boxShapeInterface->IsTypeAxisAligned() : false;
            const AZ::Vector3 translationOffset = m_shapeBus ? m_shapeBus->GetTranslationOffset() : AZ::Vector3::CreateZero();
            const AZ::Transform translationOffsetTransform = AZ::Transform::CreateTranslation(translationOffset);

            if (isTypeAxisAligned)
            {
                AZ::Transform entityTransformNoRotation = entityTransform;
                entityTransformNoRotation.SetRotation(AZ::Quaternion::CreateIdentity());
                return entityTransformNoRotation * translationOffsetTransform;
            }

            return entityTransform * translationOffsetTransform;
        }
    } // namespace Render
} // namespace AZ
