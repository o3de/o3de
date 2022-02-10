/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/Components/ImageGradientComponent.h>
#include <Atom/ImageProcessing/ImageProcessingDefines.h>
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
    AZ::JsonSerializationResult::Result JsonImageGradientConfigSerializer::Load(
        void* outputValue, [[maybe_unused]] const AZ::Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        // We can distinguish between version 1 and 2 by the presence of the "ImageAsset" field,
        // which is only in version 1.
        // For version 2, we don't need to do any special processing, so just let the base class
        // load the JSON if we don't find the "ImageAsset" field.
        rapidjson::Value::ConstMemberIterator itr = inputValue.FindMember("ImageAsset");
        if (itr == inputValue.MemberEnd())
        {
            return AZ::BaseJsonSerializer::Load(outputValue, outputValueTypeId, inputValue, context);
        }

        namespace JSR = AZ::JsonSerializationResult;

        auto configInstance = reinterpret_cast<ImageGradientConfig*>(outputValue);
        AZ_Assert(configInstance, "Output value for JsonImageGradientConfigSerializer can't be null.");

        JSR::ResultCode result(JSR::Tasks::ReadField);

        result.Combine(ContinueLoadingFromJsonObjectField(
            &configInstance->m_tilingX, azrtti_typeid<decltype(configInstance->m_tilingX)>(), inputValue, "TilingX", context));

        result.Combine(ContinueLoadingFromJsonObjectField(
            &configInstance->m_tilingY, azrtti_typeid<decltype(configInstance->m_tilingY)>(), inputValue, "TilingY", context));

        // Version 1 stored a custom GradientSignal::ImageAsset as the image asset.
        // In Version 2, we changed the image asset to use the generic AZ::RPI::StreamingImageAsset,
        // so they are both AZ::Data::Asset but reference different types.
        // Using the assetHint, which will be something like "my_test_image.gradimage",
        // we need to find the valid streaming image asset product from the same source,
        // which will be something like "my_test_image.png.streamingimage"
        AZStd::string assetHint;
        AZ::Data::AssetId fixedAssetId;
        auto it = itr->value.FindMember("assetHint");
        if (it != itr->value.MemberEnd())
        {
            AZ::ScopedContextPath subPath(context, "assetHint");
            result.Combine(ContinueLoading(&assetHint, azrtti_typeid<AZStd::string>(), it->value, context));

            if (assetHint.ends_with(".gradimage"))
            {
                // We don't know what image format the original source was, so we need to loop through
                // all the supported image extensions to check if they have a valid corresponding
                // streaming image asset
                for (auto& supportedImageExtension : ImageProcessingAtom::s_SupportedImageExtensions)
                {
                    AZStd::string imageExtension(supportedImageExtension);

                    // The image extensions are stored with a wildcard (e.g. *.png) so we need to strip that off first
                    AZ::StringFunc::Replace(imageExtension, "*", "");

                    // Form potential streaming image path (e.g. my_test_image.png.streamingimage)
                    AZStd::string potentialStreamingImagePath(assetHint);
                    AZ::StringFunc::Replace(potentialStreamingImagePath, ".gradimage", "");
                    potentialStreamingImagePath += imageExtension + ".streamingimage";

                    // Check if there is a valid streaming image asset for this path
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(fixedAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, potentialStreamingImagePath.c_str(), azrtti_typeid<AZ::Data::Asset<AZ::RPI::StreamingImageAsset>>(), false);
                    if (fixedAssetId.IsValid())
                    {
                        break;
                    }
                }
            }
        }

        // Replace the old gradimage with new AssetId for streaming image asset
        if (fixedAssetId.IsValid())
        {
            configInstance->m_imageAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::StreamingImageAsset>(fixedAssetId, AZ::Data::AssetLoadBehavior::QueueLoad);
        }

        return context.Report(result,
            result.GetProcessing() != JSR::Processing::Halted ?
            "Successfully loaded ImageGradientConfig information." :
            "Failed to load ImageGradientConfig information.");
    }

    AZ_CLASS_ALLOCATOR_IMPL(JsonImageGradientConfigSerializer, AZ::SystemAllocator, 0);

    void ImageGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto jsonContext = azrtti_cast<AZ::JsonRegistrationContext*>(context))
        {
            jsonContext->Serializer<JsonImageGradientConfigSerializer>()->HandlesType<ImageGradientConfig>();
        }

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ImageGradientConfig, AZ::ComponentConfig>()
                ->Version(2)
                ->Field("TilingX", &ImageGradientConfig::m_tilingX)
                ->Field("TilingY", &ImageGradientConfig::m_tilingY)
                ->Field("StreamingImageAsset", &ImageGradientConfig::m_imageAsset)
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

    void ImageGradientComponent::GetSubImageData()
    {
        if (!m_configuration.m_imageAsset || !m_configuration.m_imageAsset.IsReady())
        {
            return;
        }

        m_imageData = m_configuration.m_imageAsset->GetSubImageData(0, 0);
    }

    void ImageGradientComponent::Activate()
    {
        // This will immediately call OnGradientTransformChanged and initialize m_gradientTransform.
        GradientTransformNotificationBus::Handler::BusConnect(GetEntityId());

        SetupDependencies();

        ImageGradientRequestBus::Handler::BusConnect(GetEntityId());
        GradientRequestBus::Handler::BusConnect(GetEntityId());

        // Invoke the QueueLoad before connecting to the AssetBus, so that
        // if the asset is already ready, then OnAssetReady will be triggered immediately
        m_imageData = AZStd::span<const uint8_t>();
        m_configuration.m_imageAsset.QueueLoad();

        AZ::Data::AssetBus::Handler::BusConnect(m_configuration.m_imageAsset.GetId());
    }

    void ImageGradientComponent::Deactivate()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        GradientRequestBus::Handler::BusDisconnect();
        ImageGradientRequestBus::Handler::BusDisconnect();
        GradientTransformNotificationBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();

        AZStd::unique_lock<decltype(m_imageMutex)> imageLock(m_imageMutex);
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
        AZStd::unique_lock<decltype(m_imageMutex)> imageLock(m_imageMutex);
        m_configuration.m_imageAsset = asset;

        GetSubImageData();
    }

    void ImageGradientComponent::OnAssetMoved(AZ::Data::Asset<AZ::Data::AssetData> asset, [[maybe_unused]] void* oldDataPointer)
    {
        AZStd::unique_lock<decltype(m_imageMutex)> imageLock(m_imageMutex);
        m_configuration.m_imageAsset = asset;

        GetSubImageData();
    }

    void ImageGradientComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZStd::unique_lock<decltype(m_imageMutex)> imageLock(m_imageMutex);
        m_configuration.m_imageAsset = asset;

        GetSubImageData();
    }

    void ImageGradientComponent::OnGradientTransformChanged(const GradientTransform& newTransform)
    {
        AZStd::unique_lock<decltype(m_imageMutex)> lock(m_imageMutex);
        m_gradientTransform = newTransform;
    }

    float ImageGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        AZ::Vector3 uvw = sampleParams.m_position;
        bool wasPointRejected = false;

        // Return immediately if our cached image data hasn't been retrieved yet
        if (m_imageData.empty())
        {
            return 0.0f;
        }

        {
            AZStd::shared_lock<decltype(m_imageMutex)> imageLock(m_imageMutex);

            m_gradientTransform.TransformPositionToUVWNormalized(sampleParams.m_position, uvw, wasPointRejected);

            if (!wasPointRejected)
            {
                return GetValueFromImageAsset(
                    m_imageData, m_configuration.m_imageAsset->GetImageDescriptor(), uvw, m_configuration.m_tilingX, m_configuration.m_tilingY, 0.0f);
            }
        }

        return 0.0f;
    }

    void ImageGradientComponent::GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
    {
        if (positions.size() != outValues.size())
        {
            AZ_Assert(false, "input and output lists are different sizes (%zu vs %zu).", positions.size(), outValues.size());
            return;
        }

        // Return immediately if our cached image data hasn't been retrieved yet
        if (m_imageData.empty())
        {
            return;
        }

        AZ::Vector3 uvw;
        bool wasPointRejected = false;

        AZStd::shared_lock<decltype(m_imageMutex)> imageLock(m_imageMutex);

        for (size_t index = 0; index < positions.size(); index++)
        {
            m_gradientTransform.TransformPositionToUVWNormalized(positions[index], uvw, wasPointRejected);

            if (!wasPointRejected)
            {
                outValues[index] = GetValueFromImageAsset(
                    m_imageData, m_configuration.m_imageAsset->GetImageDescriptor(), uvw, m_configuration.m_tilingX, m_configuration.m_tilingY, 0.0f);
            }
            else
            {
                outValues[index] = 0.0f;
            }
        }
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
                AZStd::unique_lock<decltype(m_imageMutex)> imageLock(m_imageMutex);

                // Clear our cached image data
                m_imageData = AZStd::span<const uint8_t>();

                m_configuration.m_imageAsset = AZ::Data::AssetManager::Instance().FindOrCreateAsset(assetId, azrtti_typeid<AZ::RPI::StreamingImageAsset>(), m_configuration.m_imageAsset.GetAutoLoadBehavior());
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
