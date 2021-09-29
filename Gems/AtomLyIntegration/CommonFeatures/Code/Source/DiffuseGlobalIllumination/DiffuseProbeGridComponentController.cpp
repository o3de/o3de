/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DiffuseGlobalIllumination/DiffuseProbeGridComponentController.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGridComponentConstants.h>

#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Scene.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetSerializer.h>
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
                    ->Field("EditorMode", &DiffuseProbeGridComponentConfig::m_editorMode)
                    ->Field("RuntimeMode", &DiffuseProbeGridComponentConfig::m_runtimeMode)
                    ->Field("BakedIrradianceTextureRelativePath", &DiffuseProbeGridComponentConfig::m_bakedIrradianceTextureRelativePath)
                    ->Field("BakedDistanceTextureRelativePath", &DiffuseProbeGridComponentConfig::m_bakedDistanceTextureRelativePath)
                    ->Field("BakedRelocationTextureRelativePath", &DiffuseProbeGridComponentConfig::m_bakedRelocationTextureRelativePath)
                    ->Field("BakedClassificationTextureRelativePath", &DiffuseProbeGridComponentConfig::m_bakedClassificationTextureRelativePath)
                    ->Field("BakedIrradianceTextureAsset", &DiffuseProbeGridComponentConfig::m_bakedIrradianceTextureAsset)
                    ->Field("BakedDistanceTextureAsset", &DiffuseProbeGridComponentConfig::m_bakedDistanceTextureAsset)
                    ->Field("BakedRelocationTextureAsset", &DiffuseProbeGridComponentConfig::m_bakedRelocationTextureAsset)
                    ->Field("BakedClassificationTextureAsset", &DiffuseProbeGridComponentConfig::m_bakedClassificationTextureAsset)
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
            incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
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

            // special handling is required if this component is being cloned in the editor:
            // check to see if the baked textures are already referenced by another DiffuseProbeGrid
            if (m_featureProcessor->AreBakedTexturesReferenced(
                    m_configuration.m_bakedIrradianceTextureRelativePath,
                    m_configuration.m_bakedDistanceTextureRelativePath,
                    m_configuration.m_bakedRelocationTextureRelativePath,
                    m_configuration.m_bakedClassificationTextureRelativePath))
            {
                // clear the baked texture paths and assets, since they belong to the original entity (not the clone)
                m_configuration.m_bakedIrradianceTextureRelativePath.clear();
                m_configuration.m_bakedDistanceTextureRelativePath.clear();
                m_configuration.m_bakedRelocationTextureRelativePath.clear();
                m_configuration.m_bakedClassificationTextureRelativePath.clear();

                m_configuration.m_bakedIrradianceTextureAsset.Reset();
                m_configuration.m_bakedDistanceTextureAsset.Reset();
                m_configuration.m_bakedRelocationTextureAsset.Reset();
                m_configuration.m_bakedClassificationTextureAsset.Reset();
            }

            // add this diffuse probe grid to the feature processor
            const AZ::Transform& transform = m_transformInterface->GetWorldTM();
            m_handle = m_featureProcessor->AddProbeGrid(transform, m_configuration.m_extents, m_configuration.m_probeSpacing);

            m_featureProcessor->SetAmbientMultiplier(m_handle, m_configuration.m_ambientMultiplier);
            m_featureProcessor->SetViewBias(m_handle, m_configuration.m_viewBias);
            m_featureProcessor->SetNormalBias(m_handle, m_configuration.m_normalBias);

            // load the baked texture assets, but only if they are all valid
            if (m_configuration.m_bakedIrradianceTextureAsset.GetId().IsValid() &&
                m_configuration.m_bakedDistanceTextureAsset.GetId().IsValid() &&
                m_configuration.m_bakedRelocationTextureAsset.GetId().IsValid() &&
                m_configuration.m_bakedClassificationTextureAsset.GetId().IsValid())
            {
                Data::AssetBus::MultiHandler::BusConnect(m_configuration.m_bakedIrradianceTextureAsset.GetId());
                Data::AssetBus::MultiHandler::BusConnect(m_configuration.m_bakedDistanceTextureAsset.GetId());
                Data::AssetBus::MultiHandler::BusConnect(m_configuration.m_bakedRelocationTextureAsset.GetId());
                Data::AssetBus::MultiHandler::BusConnect(m_configuration.m_bakedClassificationTextureAsset.GetId());

                m_configuration.m_bakedIrradianceTextureAsset.QueueLoad();
                m_configuration.m_bakedDistanceTextureAsset.QueueLoad();
                m_configuration.m_bakedRelocationTextureAsset.QueueLoad();
                m_configuration.m_bakedClassificationTextureAsset.QueueLoad();
            }
            else if (m_configuration.m_runtimeMode == DiffuseProbeGridMode::Baked ||
                     m_configuration.m_runtimeMode == DiffuseProbeGridMode::AutoSelect ||
                     m_configuration.m_editorMode == DiffuseProbeGridMode::Baked ||
                     m_configuration.m_editorMode == DiffuseProbeGridMode::AutoSelect)
            {
                AZ_Error("DiffuseProbeGrid", false, "DiffuseProbeGrid mdoe is set to Baked or Auto-Select, but it does not have baked texture assets. Please re-bake this DiffuseProbeGrid.");
            }

            m_featureProcessor->SetMode(m_handle, m_configuration.m_runtimeMode);

            // if this is a new DiffuseProbeGrid entity and the box shape has not been changed (i.e., it's still unit sized)
            // then use the default extents, otherwise use the current box shape extents
            AZ::Vector3 extents(0.0f);
            AZ::Vector3 boxDimensions = m_boxShapeInterface->GetBoxDimensions();
            if (m_configuration.m_entityId == EntityId::InvalidEntityId && boxDimensions == AZ::Vector3(1.0f))
            {
                extents = m_configuration.m_extents;
            }
            else
            {
                extents = boxDimensions;
            }

            m_boxShapeInterface->SetBoxDimensions(extents);
        }

        void DiffuseProbeGridComponentController::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            // if all assets are ready we can set the baked texture images
            if (m_configuration.m_bakedIrradianceTextureAsset.IsReady() &&
                m_configuration.m_bakedDistanceTextureAsset.IsReady() &&
                m_configuration.m_bakedRelocationTextureAsset.IsReady() &&
                m_configuration.m_bakedClassificationTextureAsset.IsReady())
            {
                Data::AssetBus::MultiHandler::BusDisconnect(m_configuration.m_bakedIrradianceTextureAsset.GetId());
                Data::AssetBus::MultiHandler::BusDisconnect(m_configuration.m_bakedDistanceTextureAsset.GetId());
                Data::AssetBus::MultiHandler::BusDisconnect(m_configuration.m_bakedRelocationTextureAsset.GetId());
                Data::AssetBus::MultiHandler::BusDisconnect(m_configuration.m_bakedClassificationTextureAsset.GetId());

                UpdateBakedTextures();
            }
        }

        void DiffuseProbeGridComponentController::OnAssetError(Data::Asset<Data::AssetData> asset)
        {
            Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

            AZ_Error("DiffuseProbeGrid", false, "Failed to load baked texture [%s], please re-bake this DiffuseProbeGrid.", asset.GetId().ToString<AZStd::string>().c_str());
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
            if (!m_featureProcessor)
            {
                return;
            }

            m_configuration.m_ambientMultiplier = ambientMultiplier;
            m_featureProcessor->SetAmbientMultiplier(m_handle, m_configuration.m_ambientMultiplier);
        }

        void DiffuseProbeGridComponentController::SetViewBias(float viewBias)
        {
            if (!m_featureProcessor)
            {
                return;
            }

            m_configuration.m_viewBias = viewBias;
            m_featureProcessor->SetViewBias(m_handle, m_configuration.m_viewBias);
        }

        void DiffuseProbeGridComponentController::SetNormalBias(float normalBias)
        {
            if (!m_featureProcessor)
            {
                return;
            }

            m_configuration.m_normalBias = normalBias;
            m_featureProcessor->SetNormalBias(m_handle, m_configuration.m_normalBias);
        }

        void DiffuseProbeGridComponentController::SetEditorMode(DiffuseProbeGridMode editorMode)
        {
            if (!m_featureProcessor)
            {
                return;
            }

            // update the configuration and change the DiffuseProbeGrid mode
            m_configuration.m_editorMode = editorMode;
            m_featureProcessor->SetMode(m_handle, m_configuration.m_editorMode);
        }

        void DiffuseProbeGridComponentController::SetRuntimeMode(DiffuseProbeGridMode runtimeMode)
        {
            if (!m_featureProcessor)
            {
                return;
            }

            // only update the configuration
            m_configuration.m_runtimeMode = runtimeMode;
        }

        void DiffuseProbeGridComponentController::BakeTextures(DiffuseProbeGridBakeTexturesCallback callback)
        {
            if (!m_featureProcessor)
            {
                return;
            }

            m_featureProcessor->BakeTextures(
                m_handle,
                callback,
                m_configuration.m_bakedIrradianceTextureRelativePath,
                m_configuration.m_bakedDistanceTextureRelativePath,
                m_configuration.m_bakedRelocationTextureRelativePath,
                m_configuration.m_bakedClassificationTextureRelativePath);
        }

        void DiffuseProbeGridComponentController::UpdateBakedTextures()
        {
           if (!m_featureProcessor)
           {
               return;
           }
           
           DiffuseProbeGridBakedTextures bakedTextures;
           bakedTextures.m_irradianceImage = RPI::StreamingImage::FindOrCreate(m_configuration.m_bakedIrradianceTextureAsset);
           bakedTextures.m_irradianceImageRelativePath = m_configuration.m_bakedIrradianceTextureRelativePath;
           bakedTextures.m_distanceImage = RPI::StreamingImage::FindOrCreate(m_configuration.m_bakedDistanceTextureAsset);
           bakedTextures.m_distanceImageRelativePath = m_configuration.m_bakedDistanceTextureRelativePath;
           bakedTextures.m_relocationImageDescriptor = m_configuration.m_bakedRelocationTextureAsset->GetImageDescriptor();
           bakedTextures.m_relocationImageData = m_configuration.m_bakedRelocationTextureAsset->GetSubImageData(0, 0);
           bakedTextures.m_relocationImageRelativePath = m_configuration.m_bakedRelocationTextureRelativePath;
           bakedTextures.m_classificationImageDescriptor = m_configuration.m_bakedClassificationTextureAsset->GetImageDescriptor();
           bakedTextures.m_classificationImageData = m_configuration.m_bakedClassificationTextureAsset->GetSubImageData(0, 0);
           bakedTextures.m_classificationImageRelativePath = m_configuration.m_bakedClassificationTextureRelativePath;

           m_featureProcessor->SetBakedTextures(m_handle, bakedTextures);
        }
    } // namespace Render
} // namespace AZ
