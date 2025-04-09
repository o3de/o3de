/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImageBasedLights/ImageBasedLightComponentController.h>
#include <AtomLyIntegration/CommonFeatures/ImageBasedLights/ImageBasedLightComponentConstants.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/Utils/Utils.h>

namespace AZ
{
    namespace Render
    {
        void ImageBasedLightComponentController::Reflect(ReflectContext* context)
        {
            ImageBasedLightComponentConfig::Reflect(context);

            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ImageBasedLightComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &ImageBasedLightComponentController::m_configuration)
                    ;
            }

            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->EBus<ImageBasedLightComponentRequestBus>("ImageBasedLightComponentRequestBus")
                    ->Event("SetSpecularImageAssetId", &ImageBasedLightComponentRequestBus::Events::SetSpecularImageAssetId)
                    ->Event("GetSpecularImageAssetId", &ImageBasedLightComponentRequestBus::Events::GetSpecularImageAssetId)
                    ->Event("SetDiffuseImageAssetId", &ImageBasedLightComponentRequestBus::Events::SetDiffuseImageAssetId)
                    ->Event("GetDiffuseImageAssetId", &ImageBasedLightComponentRequestBus::Events::GetDiffuseImageAssetId)
                    ->Event("SetSpecularImageAssetPath", &ImageBasedLightComponentRequestBus::Events::SetSpecularImageAssetPath)
                    ->Event("GetSpecularImageAssetPath", &ImageBasedLightComponentRequestBus::Events::GetSpecularImageAssetPath)
                    ->Event("SetDiffuseImageAssetPath", &ImageBasedLightComponentRequestBus::Events::SetDiffuseImageAssetPath)
                    ->Event("GetDiffuseImageAssetPath", &ImageBasedLightComponentRequestBus::Events::GetDiffuseImageAssetPath)
                    ->VirtualProperty("SpecularImageAssetId", "GetSpecularImageAssetId", "SetSpecularImageAssetId")
                    ->VirtualProperty("DiffuseImageAssetId", "GetDiffuseImageAssetId", "SetDiffuseImageAssetId")
                    ->VirtualProperty("SpecularImageAssetPath", "GetSpecularImageAssetPath", "SetSpecularImageAssetPath")
                    ->VirtualProperty("DiffuseImageAssetPath", "GetDiffuseImageAssetPath", "SetDiffuseImageAssetPath")
                    ;
            }
        }

        void ImageBasedLightComponentController::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ImageBasedLightService"));
        }

        void ImageBasedLightComponentController::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("ImageBasedLightService"));
        }

        ImageBasedLightComponentController::ImageBasedLightComponentController(const ImageBasedLightComponentConfig& config)
            : m_configuration(config)
        {
        }

        void ImageBasedLightComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;

            m_featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<ImageBasedLightFeatureProcessorInterface>(m_entityId);
            AZ_Error("ImageBasedLightComponentController", m_featureProcessor, "Unable to find a ImageBasedLightFeatureProcessorInterface on this entity's scene.");

            if (m_featureProcessor)
            {
                LoadImage(m_configuration.m_specularImageAsset);
                LoadImage(m_configuration.m_diffuseImageAsset);
                m_featureProcessor->SetExposure(m_configuration.m_exposure);

                TransformInterface* transformInterface = TransformBus::FindFirstHandler(m_entityId);
                AZ_Assert(transformInterface, "Unable to attach to a TransformBus handler. Entity transform will not affect IBL.");

                const AZ::Transform& transform = transformInterface ? transformInterface->GetWorldTM() : Transform::Identity();
                m_featureProcessor->SetOrientation(transform.GetRotation());

                TransformNotificationBus::Handler::BusConnect(m_entityId);
                ImageBasedLightComponentRequestBus::Handler::BusConnect(m_entityId);
                transformInterface = nullptr;
            }
        }

        void ImageBasedLightComponentController::Deactivate()
        {
            ImageBasedLightComponentRequestBus::Handler::BusDisconnect();
            TransformNotificationBus::Handler::BusDisconnect();

            ReleaseImages();

            if (m_featureProcessor)
            {
                m_featureProcessor->Reset();
                m_featureProcessor = nullptr;
            }

            m_entityId = EntityId(EntityId::InvalidEntityId);
        }

        void ImageBasedLightComponentController::SetConfiguration(const ImageBasedLightComponentConfig& config)
        {
            m_configuration = config;
        }

        const ImageBasedLightComponentConfig& ImageBasedLightComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void ImageBasedLightComponentController::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            UpdateWithAsset(asset);
        }

        void ImageBasedLightComponentController::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            UpdateWithAsset(asset);
        }

        void ImageBasedLightComponentController::OnAssetError(Data::Asset<Data::AssetData> asset)
        {
            UpdateWithAsset(asset);
        }

        void ImageBasedLightComponentController::UpdateWithAsset(Data::Asset<Data::AssetData> updatedAsset)
        {
            // REMARK: This function is typically invoked within the context of one of the AssetBus::OnAssetXXX functions,
            // and a deadlock may occur according to the following sequence:
            // 1. Starting from Main thread, AssetBus locks a mutex.
            // 2. AssetBus calls OnAssetReady and it enters in this function.
            // 3. Start the instantiation of a new StreamingImage.
            // 4. StreamingImage asynchronously queues work in the "Seconday Copy Queue".
            // 5. StreamingImage waits until the work completes.
            // 6. The thread of "Seconday Copy Queue" gets a new work item, which may hold a reference
            //    to an old StreamingImage.
            // 7. The old StreamingImage gets destroyed and it calls AssetBus::MultiHandler::BusDisconnect(GetAssetId());
            // 8. When calling AssetBus::MultiHandler::BusDisconnect(GetAssetId()); it tries to lock the same mutex
            //    from step 1. But the mutex is already locked on Main Thread in step 1.
            // 9. The "Seconday Copy Queue" thread deadlocks and never completes the work.
            // 10. Main thread is also deadlocked waiting for "Seconday Copy Queue" to complete.
            // The solution is to enqueue texture update on the next tick.
            auto postTickLambda = [=]()
            {
                if (m_configuration.m_specularImageAsset.GetId() == updatedAsset.GetId())
                {
                    if (m_featureProcessor && HandleAssetUpdate(updatedAsset, m_configuration.m_specularImageAsset))
                    {
                        m_featureProcessor->SetSpecularImage(m_configuration.m_specularImageAsset);
                        ImageBasedLightComponentNotificationBus::Event(
                            m_entityId, &ImageBasedLightComponentNotifications::OnSpecularImageUpdated);
                    }
                }
                else if (m_configuration.m_diffuseImageAsset.GetId() == updatedAsset.GetId())
                {
                    if (m_featureProcessor && HandleAssetUpdate(updatedAsset, m_configuration.m_diffuseImageAsset))
                    {
                        m_featureProcessor->SetDiffuseImage(m_configuration.m_diffuseImageAsset);
                        ImageBasedLightComponentNotificationBus::Event(
                            m_entityId, &ImageBasedLightComponentNotifications::OnDiffuseImageUpdated);
                    }
                }
            };
            AZ::TickBus::QueueFunction(AZStd::move(postTickLambda));
        }

        bool ImageBasedLightComponentController::HandleAssetUpdate(Data::Asset<Data::AssetData> updatedAsset, Data::Asset<RPI::StreamingImageAsset>& configAsset)
        {
            configAsset = updatedAsset;

            if (updatedAsset.IsReady())
            {
                auto& descriptor = configAsset->GetImageDescriptor();
                bool isCubemap = descriptor.m_isCubemap || descriptor.m_arraySize == 6;

                if (isCubemap)
                {
                    return true;
                }
            }
            return false;
        }

        void ImageBasedLightComponentController::SetSpecularImageAsset(const Data::Asset<RPI::StreamingImageAsset>& imageAsset)
        {
            Data::AssetBus::MultiHandler::BusDisconnect(m_configuration.m_specularImageAsset.GetId());
            m_configuration.m_specularImageAsset = imageAsset;
            if (imageAsset.GetId().IsValid())
            {
                LoadImage(m_configuration.m_specularImageAsset);
            }
            else if (m_featureProcessor)
            {
                // Clear out current image asset
                m_featureProcessor->SetSpecularImage(m_configuration.m_specularImageAsset);
            }
        }

        void ImageBasedLightComponentController::SetDiffuseImageAsset(const Data::Asset<RPI::StreamingImageAsset>& imageAsset)
        {
            Data::AssetBus::MultiHandler::BusDisconnect(m_configuration.m_diffuseImageAsset.GetId());
            m_configuration.m_diffuseImageAsset = imageAsset;
            if (imageAsset.GetId().IsValid())
            {
                LoadImage(m_configuration.m_diffuseImageAsset);
            }
            else if (m_featureProcessor)
            {
                // Clear out current image asset
                m_featureProcessor->SetDiffuseImage(m_configuration.m_diffuseImageAsset);
            }
        }

        Data::Asset<RPI::StreamingImageAsset> ImageBasedLightComponentController::GetSpecularImageAsset() const
        {
            return m_configuration.m_specularImageAsset;
        }

        Data::Asset<RPI::StreamingImageAsset> ImageBasedLightComponentController::GetDiffuseImageAsset() const
        {
            return m_configuration.m_diffuseImageAsset;
        }

        void ImageBasedLightComponentController::SetSpecularImageAssetId(const Data::AssetId imageAssetId)
        {
            SetSpecularImageAsset(GetAssetFromId<RPI::StreamingImageAsset>(imageAssetId, m_configuration.m_specularImageAsset.GetAutoLoadBehavior()));
        }

        void ImageBasedLightComponentController::SetDiffuseImageAssetId(const Data::AssetId imageAssetId)
        {
            SetDiffuseImageAsset(GetAssetFromId<RPI::StreamingImageAsset>(imageAssetId, m_configuration.m_diffuseImageAsset.GetAutoLoadBehavior()));
        }

        void ImageBasedLightComponentController::SetSpecularImageAssetPath(const AZStd::string path)
        {
            SetSpecularImageAsset(GetAssetFromPath<RPI::StreamingImageAsset>(path, m_configuration.m_specularImageAsset.GetAutoLoadBehavior()));
        }

        void ImageBasedLightComponentController::SetDiffuseImageAssetPath(const AZStd::string path)
        {
            SetDiffuseImageAsset(GetAssetFromPath<RPI::StreamingImageAsset>(path, m_configuration.m_diffuseImageAsset.GetAutoLoadBehavior()));
        }

        AZStd::string ImageBasedLightComponentController::GetSpecularImageAssetPath() const
        {
            AZStd::string assetPathString;
            Data::AssetCatalogRequestBus::BroadcastResult(assetPathString, &Data::AssetCatalogRequests::GetAssetPathById, m_configuration.m_specularImageAsset.GetId());
            return assetPathString;
        }

        AZStd::string ImageBasedLightComponentController::GetDiffuseImageAssetPath() const
        {
            AZStd::string assetPathString;
            Data::AssetCatalogRequestBus::BroadcastResult(assetPathString, &Data::AssetCatalogRequests::GetAssetPathById, m_configuration.m_diffuseImageAsset.GetId());
            return assetPathString;
        }

        Data::AssetId ImageBasedLightComponentController::GetSpecularImageAssetId() const
        {
            return m_configuration.m_specularImageAsset.GetId();
        }

        Data::AssetId ImageBasedLightComponentController::GetDiffuseImageAssetId() const
        {
            return m_configuration.m_diffuseImageAsset.GetId();
        }

        void ImageBasedLightComponentController::SetExposure(float exposure)
        {
            m_configuration.m_exposure = exposure;
            if (m_featureProcessor)
            {
                m_featureProcessor->SetExposure(exposure);
            }
        }

        float ImageBasedLightComponentController::GetExposure() const
        {
            return m_configuration.m_exposure;
        }

        void ImageBasedLightComponentController::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
        {
            const Quaternion& rotation = world.GetRotation();
            if (m_featureProcessor)
            {
                m_featureProcessor->SetOrientation(rotation);
            }
        }

        void ImageBasedLightComponentController::LoadImage(Data::Asset<RPI::StreamingImageAsset>& imageAsset)
        {
            Data::AssetBus::MultiHandler::BusDisconnect(imageAsset.GetId());

            if (imageAsset.GetId().IsValid())
            {
                // If the asset is already loaded it'll call OnAssetReady() immediately on BusConnect().
                Data::AssetBus::MultiHandler::BusConnect(imageAsset.GetId());
                imageAsset.QueueLoad();
            }
            else
            {
                // Call update for invalid assets so current assets can be cleared if necessary.
                UpdateWithAsset(imageAsset);
            }
        }

        void ImageBasedLightComponentController::ReleaseImages()
        {
            Data::AssetBus::MultiHandler::BusDisconnect();
            m_configuration.m_specularImageAsset.Release();
            m_configuration.m_diffuseImageAsset.Release();
        }

    } // namespace Render
} // namespace AZ
