/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ImageGradientComponent.h"
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <GradientSignal/Ebuses/GradientTransformRequestBus.h>

namespace GradientSignal
{
    void ImageGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ImageGradientConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("ImageAsset", &ImageGradientConfig::m_imageAsset)
                ->Field("TilingX", &ImageGradientConfig::m_tilingX)
                ->Field("TilingY", &ImageGradientConfig::m_tilingY)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<ImageGradientConfig>(
                    "Image Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &ImageGradientConfig::m_imageAsset, "Image Asset", "Image asset whose values will be mapped as gradient output.")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ImageGradientConfig::m_tilingX, "Tiling X", "Number of times to tile horizontally.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 1024.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.25f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ImageGradientConfig::m_tilingY, "Tiling Y", "Number of times to tile vertically.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 1024.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.25f)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<ImageGradientConfig>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Property("tilingX", BehaviorValueProperty(&ImageGradientConfig::m_tilingX))
                ->Property("tilingY", BehaviorValueProperty(&ImageGradientConfig::m_tilingY))
                ;
        }
    }

    void ImageGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void ImageGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void ImageGradientComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientTransformService", 0x8c8c5ecc));
    }

    void ImageGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        ImageGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ImageGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &ImageGradientComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("ImageGradientComponentTypeId", BehaviorConstant(ImageGradientComponentTypeId));

            behaviorContext->Class<ImageGradientComponent>()->RequestBus("ImageGradientRequestBus");

            behaviorContext->EBus<ImageGradientRequestBus>("ImageGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetImageAssetPath", &ImageGradientRequestBus::Events::GetImageAssetPath)
                ->Event("SetImageAssetPath", &ImageGradientRequestBus::Events::SetImageAssetPath)
                ->VirtualProperty("ImageAssetPath", "GetImageAssetPath", "SetImageAssetPath")
                ->Event("GetTilingX", &ImageGradientRequestBus::Events::GetTilingX)
                ->Event("SetTilingX", &ImageGradientRequestBus::Events::SetTilingX)
                ->VirtualProperty("TilingX", "GetTilingX", "SetTilingX")
                ->Event("GetTilingY", &ImageGradientRequestBus::Events::GetTilingY)
                ->Event("SetTilingY", &ImageGradientRequestBus::Events::SetTilingY)
                ->VirtualProperty("TilingY", "GetTilingY", "SetTilingY")
                ;
        }
    }

    ImageGradientComponent::ImageGradientComponent(const ImageGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void ImageGradientComponent::SetupDependencies()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(m_configuration.m_imageAsset.GetId());
    }

    void ImageGradientComponent::Activate()
    {
        SetupDependencies();

        ImageGradientRequestBus::Handler::BusConnect(GetEntityId());
        GradientRequestBus::Handler::BusConnect(GetEntityId());
        AZ::Data::AssetBus::Handler::BusConnect(m_configuration.m_imageAsset.GetId());

        AZStd::lock_guard<decltype(m_imageMutex)> imageLock(m_imageMutex);
        m_configuration.m_imageAsset.QueueLoad();
    }

    void ImageGradientComponent::Deactivate()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        GradientRequestBus::Handler::BusDisconnect();
        ImageGradientRequestBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();

        AZStd::lock_guard<decltype(m_imageMutex)> imageLock(m_imageMutex);
        m_configuration.m_imageAsset.Release();
    }

    bool ImageGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const ImageGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool ImageGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<ImageGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void ImageGradientComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZStd::lock_guard<decltype(m_imageMutex)> imageLock(m_imageMutex);
        m_configuration.m_imageAsset = asset;
    }

    void ImageGradientComponent::OnAssetMoved(AZ::Data::Asset<AZ::Data::AssetData> asset, [[maybe_unused]] void* oldDataPointer)
    {
        AZStd::lock_guard<decltype(m_imageMutex)> imageLock(m_imageMutex);
        m_configuration.m_imageAsset = asset;
    }

    void ImageGradientComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZStd::lock_guard<decltype(m_imageMutex)> imageLock(m_imageMutex);
        m_configuration.m_imageAsset = asset;
    }

    float ImageGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        AZ::Vector3 uvw = sampleParams.m_position;

        bool wasPointRejected = false;
        const bool shouldNormalizeOutput = true;
        GradientTransformRequestBus::Event(
            GetEntityId(), &GradientTransformRequestBus::Events::TransformPositionToUVW, sampleParams.m_position, uvw, shouldNormalizeOutput, wasPointRejected);

        if (!wasPointRejected)
        {
            AZStd::lock_guard<decltype(m_imageMutex)> imageLock(m_imageMutex);
            return GetValueFromImageAsset(m_configuration.m_imageAsset, uvw, m_configuration.m_tilingX, m_configuration.m_tilingY, 0.0f);
        }

        return 0.0f;
    }

    AZStd::string ImageGradientComponent::GetImageAssetPath() const
    {
        AZStd::string assetPathString;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPathString, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_configuration.m_imageAsset.GetId());
        return assetPathString;
    }

    void ImageGradientComponent::SetImageAssetPath(const AZStd::string& assetPath)
    {
        AZ::Data::AssetId assetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, assetPath.c_str(), AZ::Data::s_invalidAssetType, false);
        if (assetId.IsValid())
        {
            AZ::Data::AssetBus::Handler::BusDisconnect(m_configuration.m_imageAsset.GetId());

            {
                AZStd::lock_guard<decltype(m_imageMutex)> imageLock(m_imageMutex);
                m_configuration.m_imageAsset = AZ::Data::AssetManager::Instance().FindOrCreateAsset(assetId, azrtti_typeid<ImageAsset>(), m_configuration.m_imageAsset.GetAutoLoadBehavior());
            }

            SetupDependencies();
            AZ::Data::AssetBus::Handler::BusConnect(m_configuration.m_imageAsset.GetId());
            LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
        }
    }

    float ImageGradientComponent::GetTilingX() const
    {
        return m_configuration.m_tilingX;
    }

    void ImageGradientComponent::SetTilingX(float tilingX)
    {
        m_configuration.m_tilingX = tilingX;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float ImageGradientComponent::GetTilingY() const
    {
        return m_configuration.m_tilingY;
    }

    void ImageGradientComponent::SetTilingY(float tilingY)
    {
        m_configuration.m_tilingY = tilingY;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}
