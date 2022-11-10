/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/Components/ImageGradientComponent.h>
#include <Atom/ImageProcessing/ImageProcessingDefines.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <GradientSignal/Ebuses/GradientTransformRequestBus.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace GradientSignal
{
    AZ::JsonSerializationResult::Result JsonImageGradientConfigSerializer::Load(
        void* outputValue, [[maybe_unused]] const AZ::Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;

        auto configInstance = reinterpret_cast<ImageGradientConfig*>(outputValue);
        AZ_Assert(configInstance, "Output value for JsonImageGradientConfigSerializer can't be null.");

        JSR::ResultCode result(JSR::Tasks::ReadField);

        // The tiling field was moved from individual float values for X/Y to an AZ::Vector2,
        // so we need to handle migrating these float fields over to the vector field
        rapidjson::Value::ConstMemberIterator tilingXIter = inputValue.FindMember("TilingX");
        if (tilingXIter != inputValue.MemberEnd())
        {
            AZ::ScopedContextPath subPath(context, "TilingX");
            float tilingX;
            result.Combine(ContinueLoading(&tilingX, azrtti_typeid<float>(), tilingXIter->value, context));
            configInstance->m_tiling.SetX(tilingX);
        }

        rapidjson::Value::ConstMemberIterator tilingYIter = inputValue.FindMember("TilingY");
        if (tilingYIter != inputValue.MemberEnd())
        {
            AZ::ScopedContextPath subPath(context, "TilingY");
            float tilingY;
            result.Combine(ContinueLoading(&tilingY, azrtti_typeid<float>(), tilingYIter->value, context));
            configInstance->m_tiling.SetY(tilingY);
        }

        // We can distinguish between version 1 and 2 by the presence of the "ImageAsset" field,
        // which is only in version 1.
        // For version 2, we don't need to do any special processing, so just let the base class
        // load the JSON if we don't find the "ImageAsset" field.
        rapidjson::Value::ConstMemberIterator itr = inputValue.FindMember("ImageAsset");
        if (itr == inputValue.MemberEnd())
        {
            return AZ::BaseJsonSerializer::Load(outputValue, outputValueTypeId, inputValue, context);
        }

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

        // The "AdvancedMode" toggle has been removed, all settings are always active and visible now.
        // If the "AdvancedMode" setting was previously disabled, make sure to set the appropriate settings to their defaults
        rapidjson::Value::ConstMemberIterator advancedModeIter = inputValue.FindMember("AdvancedMode");
        if (advancedModeIter != inputValue.MemberEnd())
        {
            AZ::ScopedContextPath subPath(context, "AdvancedMode");
            bool advancedMode = false;
            result.Combine(ContinueLoading(&advancedMode, azrtti_typeid<bool>(), advancedModeIter->value, context));
            if (!advancedMode)
            {
                configInstance->m_channelToUse = ChannelToUse::Red;
                configInstance->m_customScaleType = CustomScaleType::None;
                configInstance->m_mipIndex = 0;
                configInstance->m_samplingType = SamplingType::Point;
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

    bool DoesFormatSupportTerrarium(AZ::RHI::Format format)
    {
        // The terrarium type is only supported by 8-bit formats that have
        // at least RGB
        switch (format)
        {
        case AZ::RHI::Format::R8G8B8A8_UNORM:
        case AZ::RHI::Format::R8G8B8A8_UNORM_SRGB:
            return true;
        }

        return false;
    }

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
                ->Version(6)
                ->Field("StreamingImageAsset", &ImageGradientConfig::m_imageAsset)
                ->Field("SamplingType", &ImageGradientConfig::m_samplingType)
                ->Field("Tiling", &ImageGradientConfig::m_tiling)
                ->Field("ChannelToUse", &ImageGradientConfig::m_channelToUse)
                ->Field("MipIndex", &ImageGradientConfig::m_mipIndex)
                ->Field("CustomScale", &ImageGradientConfig::m_customScaleType)
                ->Field("ScaleRangeMin", &ImageGradientConfig::m_scaleRangeMin)
                ->Field("ScaleRangeMax", &ImageGradientConfig::m_scaleRangeMax)
                ;

        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<ImageGradientConfig>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Property("tiling", BehaviorValueProperty(&ImageGradientConfig::m_tiling))
                ;
        }
    }

    bool ImageGradientConfig::GetManualScaleVisibility() const
    {
        return (m_customScaleType == CustomScaleType::Manual);
    }

    bool ImageGradientConfig::IsImageAssetReadOnly() const
    {
        return m_imageModificationActive;
    }

    bool ImageGradientConfig::AreImageOptionsReadOnly() const
    {
        return m_imageModificationActive || !(m_imageAsset.GetId().IsValid());
    }

    AZStd::string ImageGradientConfig::GetImageAssetPropertyName() const
    {
        return m_imageAssetPropertyLabel;
    }

    void ImageGradientConfig::SetImageAssetPropertyName(const AZStd::string& imageAssetPropertyName)
    {
        m_imageAssetPropertyLabel = imageAssetPropertyName;
    }


    void ImageGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void ImageGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void ImageGradientComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientTransformService"));
    }

    void ImageGradientComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
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

            behaviorContext->Class<ImageGradientComponent>()
                ->RequestBus("ImageGradientRequestBus")
                ->RequestBus("ImageGradientModificationBus")
                ;

            behaviorContext->EBus<ImageGradientRequestBus>("ImageGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "vegetation")
                ->Event("GetImageAssetPath", &ImageGradientRequestBus::Events::GetImageAssetPath)
                ->Event("GetImageAssetSourcePath", &ImageGradientRequestBus::Events::GetImageAssetSourcePath)
                ->Event("SetImageAssetPath", &ImageGradientRequestBus::Events::SetImageAssetPath)
                ->Event("SetImageAssetSourcePath", &ImageGradientRequestBus::Events::SetImageAssetSourcePath)
                ->VirtualProperty("ImageAssetPath", "GetImageAssetPath", "SetImageAssetPath")
                ->Event("GetTilingX", &ImageGradientRequestBus::Events::GetTilingX)
                ->Event("SetTilingX", &ImageGradientRequestBus::Events::SetTilingX)
                ->VirtualProperty("TilingX", "GetTilingX", "SetTilingX")
                ->Event("GetTilingY", &ImageGradientRequestBus::Events::GetTilingY)
                ->Event("SetTilingY", &ImageGradientRequestBus::Events::SetTilingY)
                ->VirtualProperty("TilingY", "GetTilingY", "SetTilingY")
            ;

            behaviorContext->EBus<ImageGradientModificationBus>("ImageGradientModificationBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "vegetation")
                ->Event("StartImageModification", &ImageGradientModificationBus::Events::StartImageModification)
                ->Event("EndImageModification", &ImageGradientModificationBus::Events::EndImageModification)
            ;
        }
    }

    ImageGradientComponent::ImageGradientComponent(const ImageGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void ImageGradientComponent::GetSubImageData()
    {
        if (!m_configuration.m_imageAsset || !m_configuration.m_imageAsset.IsReady())
        {
            return;
        }

        // If we have loaded in an old image asset with an unsupported pixel format,
        // don't try to access the image data because there will be spam of asserts,
        // so just log an error message and bail out
        AZ::RHI::Format format = m_configuration.m_imageAsset->GetImageDescriptor().m_format;
        bool isFormatSupported = AZ::RPI::IsImageDataPixelAPISupported(format);
        if (!isFormatSupported)
        {
            AZ_Error("GradientSignal", false, "Image asset (%s) has an unsupported pixel format: %s",
                m_configuration.m_imageAsset.GetHint().c_str(), AZ::RHI::ToString(format));
            return;
        }

        // Prevent loading of the image data if an invalid configuration was specified by the user
        const auto numComponents = AZ::RHI::GetFormatComponentCount(format);
        const AZ::u8 channel = aznumeric_cast<AZ::u8>(m_configuration.m_channelToUse);
        if (m_configuration.m_channelToUse == ChannelToUse::Terrarium)
        {
            if (!DoesFormatSupportTerrarium(format))
            {
                AZ_Error("GradientSignal", false, "Unable to interpret image as Terrarium because image asset (%s) has pixel format (%s), which only supports %d channels",
                    m_configuration.m_imageAsset.GetHint().c_str(), AZ::RHI::ToString(format), numComponents);
                return;
            }
        }
        else if (channel >= numComponents)
        {
            AZ_Error("GradientSignal", false, "Unable to use channel %d because image asset (%s) has pixel format (%s), which only supports %d channels",
                channel, m_configuration.m_imageAsset.GetHint().c_str(), AZ::RHI::ToString(format), numComponents);
            return;
        }

        m_currentChannel = m_configuration.m_channelToUse;
        m_currentScaleType = m_configuration.m_customScaleType;
        m_currentSamplingType = m_configuration.m_samplingType;

        // Make sure the custom mip level doesn't exceed the available mip levels in this
        // image asset. If so, then just use the lowest available mip level.
        auto mipLevelCount = m_configuration.m_imageAsset->GetImageDescriptor().m_mipLevels;
        m_currentMipIndex = m_configuration.m_mipIndex;
        if (m_currentMipIndex >= mipLevelCount)
        {
            AZ_Warning("GradientSignal", false, "Mip level index (%d) out of bounds, only %d levels available. Using lowest available mip level",
                m_currentMipIndex, mipLevelCount);

            m_currentMipIndex = aznumeric_cast<AZ::u32>(mipLevelCount) - 1;
        }

        // Update our cached image data
        UpdateCachedImageBufferData(
            m_configuration.m_imageAsset->GetImageDescriptorForMipLevel(m_currentMipIndex),
            m_configuration.m_imageAsset->GetSubImageData(m_currentMipIndex, 0));

        // Calculate the multiplier and offset based on our scale type
        // Make sure we do this last, because the calculation might
        // depend on the image data (e.g. auto scale finds the min/max value
        // from the image data, which might be different based on the mip level)
        switch (m_currentScaleType)
        {
        case CustomScaleType::Auto:
            SetupAutoScaleMultiplierAndOffset();
            break;

        case CustomScaleType::Manual:
            SetupManualScaleMultiplierAndOffset();
            break;

        case CustomScaleType::None:
        default:
            SetupDefaultMultiplierAndOffset();
            break;
        }
    }

    float ImageGradientComponent::GetValueFromImageData(SamplingType samplingType, const AZ::Vector3& uvw, float defaultValue) const
    {
        if (!m_imageData.empty())
        {
            const auto& width = m_imageDescriptor.m_size.m_width;
            const auto& height = m_imageDescriptor.m_size.m_height;

            if (width > 0 && height > 0)
            {
                // When "rasterizing" from uvs, a range of 0-1 has slightly different meanings depending on the sampler state.
                // For repeating states (Unbounded/None, Repeat), a uv value of 1 should wrap around back to our 0th pixel.
                // For clamping states (Clamp to Zero, Clamp to Edge), a uv value of 1 should point to the last pixel.

                // We assume here that the code handling sampler states has handled this for us in the clamping cases
                // by reducing our uv by a small delta value such that anything that wants the last pixel has a value
                // just slightly less than 1.

                // Keeping that in mind, we scale our uv from 0-1 to 0-image size inclusive.  So a 4-pixel image will scale
                // uv values of 0-1 to 0-4, not 0-3 as you might expect.  This is because we want the following range mappings:
                // [0 - 1/4)   = pixel 0
                // [1/4 - 1/2) = pixel 1
                // [1/2 - 3/4) = pixel 2
                // [3/4 - 1)   = pixel 3
                // [1 - 1 1/4) = pixel 0
                // ...

                // Also, based on our tiling settings, we extend the size of our image virtually by a factor of tilingX and tilingY.  
                // A 16x16 pixel image and tilingX = tilingY = 1  maps the uv range of 0-1 to 0-16 pixels.  
                // A 16x16 pixel image and tilingX = tilingY = 1.5 maps the uv range of 0-1 to 0-24 pixels.

                const AZ::Vector2 tiledDimensions(width * GetTilingX(), height * GetTilingY());

                // Convert from uv space back to pixel space
                AZ::Vector2 pixelLookup = (AZ::Vector2(uvw) * tiledDimensions);

                // UVs outside the 0-1 range are treated as infinitely tiling, so that we behave the same as the 
                // other gradient generators.  As mentioned above, if clamping is desired, we expect it to be applied
                // outside of this function.
                float pixelX = pixelLookup.GetX();
                float pixelY = pixelLookup.GetY();
                auto x = aznumeric_cast<AZ::u32>(pixelX) % width;
                auto y = aznumeric_cast<AZ::u32>(pixelY) % height;

                // Retrieve our pixel value based on our sampling type
                const float value = GetValueForSamplingType(samplingType, x, y, pixelX, pixelY);

                // Scale (inverse lerp) the value into a 0 - 1 range. We also clamp it because manual scale values could cause
                // the result to fall outside of the expected output range.
                return AZStd::clamp((value - m_offset) * m_multiplier, 0.0f, 1.0f);
            }
        }

        return defaultValue;
    }

    float ImageGradientComponent::GetPixelValue(AZ::u32 x, AZ::u32 y) const
    {
        // Flip the y because images are stored in reverse of our world axes
        const auto& height = m_imageDescriptor.m_size.m_height;
        y = (height - 1) - y;

        // For terrarium, there is a separate algorithm for retrieving the value
        float value = (m_currentChannel == ChannelToUse::Terrarium)
            ? GetTerrariumPixelValue(x, y)
            : AZ::RPI::GetImageDataPixelValue<float>(
                m_imageData, m_imageDescriptor, x, y, aznumeric_cast<AZ::u8>(m_currentChannel));

        return value;
    }

    float ImageGradientComponent::GetTerrariumPixelValue(AZ::u32 x, AZ::u32 y) const
    {
        float r = AZ::RPI::GetImageDataPixelValue<float>(m_imageData, m_imageDescriptor, x, y, aznumeric_cast<AZ::u8>(ChannelToUse::Red));
        float g = AZ::RPI::GetImageDataPixelValue<float>(m_imageData, m_imageDescriptor, x, y, aznumeric_cast<AZ::u8>(ChannelToUse::Green));
        float b = AZ::RPI::GetImageDataPixelValue<float>(m_imageData, m_imageDescriptor, x, y, aznumeric_cast<AZ::u8>(ChannelToUse::Blue));

        /*
            "Terrarium" is an image-based terrain file format as defined here:  https://www.mapzen.com/blog/terrain-tile-service/
            According to the website:  "Terrarium format PNG tiles contain raw elevation data in meters, in Mercator projection (EPSG:3857).
            All values are positive with a 32,768 offset, split into the red, green, and blue channels, with 16 bits of integer and 8 bits of fraction. To decode:  (red * 256 + green + blue / 256) - 32768"
            This gives a range -32768 to 32768 meters at a constant 1/256 meter resolution. For reference, the lowest point on Earth (Mariana Trench) is at -10911 m, and the highest point (Mt Everest) is at 8848 m.
            The equation of (red * 256 + green + blue / 256) - 32768 is based on red/green/blue being u8 values, but we are getting float values back
            in the range of 0.0f - 1.0f, so the multipliers below have been modified slightly to account for that scaling
        */
        constexpr float redMultiplier = (255.0f * 256.0f) / 65536.0f;
        constexpr float greenMultiplier = 255.0f / 65536.0f;
        constexpr float blueMultiplier = (255.0f / 256.0f) / 65536.0f;
        return (r * redMultiplier) + (g * greenMultiplier) + (b * blueMultiplier);
    }

    void ImageGradientComponent::SetupMultiplierAndOffset(float min, float max)
    {
        // Pre-calculate values for scaling our input range to our output range of 0 - 1. Scaling just uses the standard inverse lerp
        // formula of "output = (input - min) / (max - min)", or "output = (input - offset) * multiplier" where
        // multiplier is 1 / (max - min) and offset is min. Precalculating this way lets us gracefully handle the case where min and
        // max are equal, since we don't want to divide by infinity, without needing to check for that case on every pixel.

        // If our range is equivalent, set our multiplier and offset so that
        // any input value > min goes to 1 and any input value <= min goes to 0. 
        m_multiplier = (min == max) ? AZStd::numeric_limits<float>::max() : (1.0f / (max - min));
        m_offset = min;
    }

    void ImageGradientComponent::SetupDefaultMultiplierAndOffset()
    {
        // By default, don't perform any scaling - assume the input range is from 0 - 1, same as the desired output.
        SetupMultiplierAndOffset(0.0f, 1.0f);
    }

    void ImageGradientComponent::SetupAutoScaleMultiplierAndOffset()
    {
        auto width = m_imageDescriptor.m_size.m_width;
        auto height = m_imageDescriptor.m_size.m_height;

        float min = AZStd::numeric_limits<float>::max();
        float max = AZStd::numeric_limits<float>::min();

        if (m_currentChannel == ChannelToUse::Terrarium)
        {
            for (uint32_t y = 0; y < height; y++)
            {
                for (uint32_t x = 0; x < width; x++)
                {
                    float value = GetTerrariumPixelValue(x, y);

                    if (value < min)
                    {
                        min = value;
                    }
                    if (value > max)
                    {
                        max = value;
                    }
                }
            }
        }
        else
        {
            auto topLeft = AZStd::make_pair<uint32_t, uint32_t>(0, 0);
            auto bottomRight = AZStd::make_pair<uint32_t, uint32_t>(width, height);

            AZ::RPI::GetSubImagePixelValues(
                m_configuration.m_imageAsset, topLeft, bottomRight,
                [&min, &max]([[maybe_unused]] const AZ::u32& x, [[maybe_unused]] const AZ::u32& y, const float& value) {
                if (value < min)
                {
                    min = value;
                }
                if (value > max)
                {
                    max = value;
                }
            }, aznumeric_cast<AZ::u8>(m_currentChannel));
        }

        // Retrieve the min/max values from our image data and set our multiplier and offset based on that
        SetupMultiplierAndOffset(min, max);
    }

    void ImageGradientComponent::SetupManualScaleMultiplierAndOffset()
    {
        m_configuration.m_scaleRangeMin = AZStd::clamp(m_configuration.m_scaleRangeMin, 0.0f, 1.0f);
        m_configuration.m_scaleRangeMax = AZStd::clamp(m_configuration.m_scaleRangeMax, 0.0f, 1.0f);
        // Set our multiplier and offset based on the manual scale range. Note that the manual scale range might be less than the
        // input range and possibly even inverted.
        SetupMultiplierAndOffset(m_configuration.m_scaleRangeMin, m_configuration.m_scaleRangeMax);
    }

    float ImageGradientComponent::GetClampedValue(int32_t x, int32_t y) const
    {
        const auto& width = m_imageDescriptor.m_size.m_width;
        const auto& height = m_imageDescriptor.m_size.m_height;

        switch (m_gradientTransform.GetWrappingType())
        {
        case WrappingType::ClampToZero:
            if (x < 0 || x > m_maxX || y < 0 || y > m_maxY)
            {
                return 0.0f;
            }
        case WrappingType::ClampToEdge:
            x = AZ::GetClamp(x, 0, m_maxX);
            y = AZ::GetClamp(y, 0, m_maxY);
            break;
        case WrappingType::Mirror:
            if (x < 0)
            {
                x = -x;
            }
            if (y < 0)
            {
                y = -y;
            }
            if (x > m_maxX)
            {
                x = m_maxX - (x % width);
            }
            if (y > m_maxY)
            {
                x = m_maxY - (y % height);
            }
        case WrappingType::None:
        case WrappingType::Repeat:
        default:
            x = x % width;
            y = y % height;
            break;
        }
        return  GetPixelValue(x, y);
    }

    void ImageGradientComponent::Get4x4Neighborhood(uint32_t x, uint32_t y, AZStd::array<AZStd::array<float, 4>, 4>& values) const
    {
        for (int32_t yIndex = 0; yIndex < 4; ++yIndex)
        {
            for (int32_t xIndex = 0; xIndex < 4; ++xIndex)
            {
                values[xIndex][yIndex] = GetClampedValue(x + xIndex - 1, y + yIndex - 1);
            }
        }
    }

    float ImageGradientComponent::GetValueForSamplingType(SamplingType samplingType, AZ::u32 x0, AZ::u32 y0, float pixelX, float pixelY) const
    {
        switch (samplingType)
        {
        case SamplingType::Point:
        default:
            // Retrieve the pixel value for the single point
            return GetPixelValue(x0, y0);

        case SamplingType::Bilinear:
        {
            // Bilinear interpolation
            //
            //   |
            //   |
            //   |  (x0,y1) *             * (x1,y1)
            //   |
            //   |                o (x,y)
            //   |
            //   |  (x0,y0) *             * (x1,y0)
            //   |___________________________________
            //
            // The bilinear filtering samples from a grid around a desired pixel (x,y)
            // x0,y0 contains one corner of our grid square, x1,y1 contains the opposite corner, and deltaX/Y is the fractional
            // amount the position exists between those corners.
            // Ex: (3.3, 4.4) would have a x0,y0 of (3, 4), a x1,y1 of (4, 5), and a deltaX/Y of (0.3, 0.4).

            const float valueX0Y0 = GetClampedValue(x0, y0);
            const float valueX1Y0 = GetClampedValue(x0 + 1, y0);
            const float valueX0Y1 = GetClampedValue(x0, y0 + 1);
            const float valueX1Y1 = GetClampedValue(x0 + 1, y0 + 1);

            float deltaX = pixelX - floor(pixelX);
            float deltaY = pixelY - floor(pixelY);
            const float valueXY0 = AZ::Lerp(valueX0Y0, valueX1Y0, deltaX);
            const float valueXY1 = AZ::Lerp(valueX0Y1, valueX1Y1, deltaX);
            return AZ::Lerp(valueXY0, valueXY1, deltaY);
        }
        case SamplingType::Bicubic:
        {
            // Catmull-Rom style bicubic filtering. This uses the neighborhood of 16 samples to calculate a smooth curve for values
            // in between discrete sample locations. See https://en.wikipedia.org/wiki/Bicubic_interpolation

            // Simplified interpolation function
            auto cubicInterpolate = [](float p0, float p1, float p2, float p3, float delta) -> float
            {
                return p1 + 0.5f * delta * (p2 - p0 + delta * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3 + delta * (3.0f * (p1 - p2) + p3 - p0)));
            };

            AZStd::array<AZStd::array<float, 4>, 4> values;
            Get4x4Neighborhood(x0, y0, values);

            float deltaX = pixelX - floor(pixelX);
            float deltaY = pixelY - floor(pixelY);

            const float valueXY0 = cubicInterpolate(values[0][0], values[1][0], values[2][0], values[3][0], deltaX);
            const float valueXY1 = cubicInterpolate(values[0][1], values[1][1], values[2][1], values[3][1], deltaX);
            const float valueXY2 = cubicInterpolate(values[0][2], values[1][2], values[2][2], values[3][2], deltaX);
            const float valueXY3 = cubicInterpolate(values[0][3], values[1][3], values[2][3], values[3][3], deltaX);

            return cubicInterpolate(valueXY0, valueXY1, valueXY2, valueXY3, deltaY);
        }
        }
    }

    void ImageGradientComponent::Activate()
    {
        // This will immediately call OnGradientTransformChanged and initialize m_gradientTransform.
        GradientTransformNotificationBus::Handler::BusConnect(GetEntityId());

        ImageGradientRequestBus::Handler::BusConnect(GetEntityId());
        ImageGradientModificationBus::Handler::BusConnect(GetEntityId());

        // Invoke the QueueLoad before connecting to the AssetBus, so that
        // if the asset is already ready, then OnAssetReady will be triggered immediately
        UpdateCachedImageBufferData({}, {});
        m_configuration.m_imageAsset.QueueLoad(AZ::Data::AssetLoadParameters(nullptr, AZ::Data::AssetDependencyLoadRules::LoadAll));

        AZ::Data::AssetBus::Handler::BusConnect(m_configuration.m_imageAsset.GetId());

        // Connect to GradientRequestBus last so that everything is initialized before listening for gradient queries.
        GradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void ImageGradientComponent::Deactivate()
    {
        // Disconnect from GradientRequestBus first to ensure no queries are in process when deactivating.
        GradientRequestBus::Handler::BusDisconnect();

        AZ::Data::AssetBus::Handler::BusDisconnect();
        ImageGradientModificationBus::Handler::BusDisconnect();
        ImageGradientRequestBus::Handler::BusDisconnect();
        GradientTransformNotificationBus::Handler::BusDisconnect();

        // Make sure we don't keep any cached references to the image asset data or the image modification buffer.
        UpdateCachedImageBufferData({}, {});

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

    void ImageGradientComponent::UpdateCachedImageBufferData(
        const AZ::RHI::ImageDescriptor& imageDescriptor, AZStd::span<const uint8_t> imageData)
    {
        bool shouldRefreshModificationBuffer = false;

        // If we're changing our image data from our modification buffer to something else while it's active,
        // let's refresh the modification buffer with the new data.
        if (ModificationBufferIsActive() && (imageData.data() != m_imageData.data()))
        {
            shouldRefreshModificationBuffer = true;
        }

        m_imageDescriptor = imageDescriptor;
        m_imageData = imageData;

        m_maxX = imageDescriptor.m_size.m_width - 1;
        m_maxY = imageDescriptor.m_size.m_height - 1;

        if (shouldRefreshModificationBuffer)
        {
            m_modifiedImageData.resize(0);
            CreateImageModificationBuffer();
        }
    }

    void ImageGradientComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZStd::unique_lock lock(m_queryMutex);
        m_configuration.m_imageAsset = asset;
        GetSubImageData();
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void ImageGradientComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    void ImageGradientComponent::OnGradientTransformChanged(const GradientTransform& newTransform)
    {
        AZStd::unique_lock lock(m_queryMutex);
        m_gradientTransform = newTransform;
    }

    void ImageGradientComponent::StartImageModification()
    {
        m_configuration.m_imageModificationActive = true;

        if (m_modifiedImageData.empty())
        {
            CreateImageModificationBuffer();
        }
    }

    void ImageGradientComponent::EndImageModification()
    {
        m_configuration.m_imageModificationActive = false;
    }

    AZStd::vector<float>* ImageGradientComponent::GetImageModificationBuffer()
    {
        // This will get replaced with safe/robust methods of modifying the image as paintbrush functionality
        // continues to get added to the Image Gradient component.
        return &m_modifiedImageData;
    }

    void ImageGradientComponent::CreateImageModificationBuffer()
    {
        if (m_imageData.empty())
        {
            AZ_Error("ImageGradientComponent", false,
                "Image data is empty. Make sure the image asset is fully loaded before attempting to modify it.");
            return;
        }

        const auto& width = m_imageDescriptor.m_size.m_width;
        const auto& height = m_imageDescriptor.m_size.m_height;

        if (m_modifiedImageData.empty())
        {
            // Create a memory buffer for holding all of our modified image information.
            // We'll always use a buffer of floats to ensure that we're modifying at the highest precision possible.
            m_modifiedImageData.reserve(width * height);

            // Fill the buffer with all of our existing pixel values.
            for (uint32_t y = 0; y < height; y++)
            {
                for (uint32_t x = 0; x < width; x++)
                {
                    float pixel = AZ::RPI::GetImageDataPixelValue<float>(
                        m_imageData, m_imageDescriptor, x, y, aznumeric_cast<AZ::u8>(m_currentChannel));
                    m_modifiedImageData.emplace_back(pixel);
                }
            }

            // Create an image descriptor describing our new buffer (correct width, height, and single-channel 32-bit float format)
            auto imageDescriptor =
                AZ::RHI::ImageDescriptor::Create2D(AZ::RHI::ImageBindFlags::None, width, height, AZ::RHI::Format::R32_FLOAT);

            // Set our imageData pointer to point to our modified data buffer.
            auto imageData = AZStd::span<const uint8_t>(
                reinterpret_cast<uint8_t*>(m_modifiedImageData.data()), m_modifiedImageData.size() * sizeof(float));

            UpdateCachedImageBufferData(imageDescriptor, imageData);
        }
        else
        {
            // If this triggers, we've somehow gotten our image modification buffer out of sync with the image descriptor information.
            AZ_Assert(m_modifiedImageData.size() == (width * height), "Image modification buffer exists but is the wrong size.");
        }
    }

    void ImageGradientComponent::ClearImageModificationBuffer()
    {
        AZ_Assert(!ModificationBufferIsActive(), "Clearing modified image data while it's still in use as the active asset!");
        AZ_Assert(!m_configuration.m_imageModificationActive, "Clearing modified image data while in modification mode!")
        m_modifiedImageData.resize(0);
    }

    bool ImageGradientComponent::ModificationBufferIsActive() const
    {
        // The modification buffer is considered active if the modification buffer has data in it and
        // our cached imageData pointer is pointing into the modification buffer instead of into an image asset.
        return (m_modifiedImageData.data() != nullptr) &&
            (reinterpret_cast<const void*>(m_imageData.data()) == reinterpret_cast<const void*>(m_modifiedImageData.data()));
    }

    float ImageGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        AZ::Vector3 position(sampleParams.m_position);
        float value = 0.0f;
        GetValuesInternal(m_currentSamplingType, AZStd::span<AZ::Vector3>(&position, 1), AZStd::span<float>(&value, 1));

        return value;
    }

    void ImageGradientComponent::GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
    {
        GetValuesInternal(m_currentSamplingType, positions, outValues);
    }

    void ImageGradientComponent::GetPixelValuesByPosition(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
    {
        GetValuesInternal(SamplingType::Point, positions, outValues);
    }

    void ImageGradientComponent::GetValuesInternal(
        SamplingType samplingType, AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
    {
        if (positions.size() != outValues.size())
        {
            AZ_Assert(false, "input and output lists are different sizes (%zu vs %zu).", positions.size(), outValues.size());
            return;
        }

        AZStd::shared_lock lock(m_queryMutex);

        // Return immediately if our cached image data hasn't been retrieved yet
        if (m_imageData.empty())
        {
            return;
        }

        AZ::Vector3 uvw;
        bool wasPointRejected = false;

        for (size_t index = 0; index < positions.size(); index++)
        {
            m_gradientTransform.TransformPositionToUVWNormalized(positions[index], uvw, wasPointRejected);

            if (!wasPointRejected)
            {
                outValues[index] = GetValueFromImageData(samplingType, uvw, 0.0f);
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

    AZStd::string ImageGradientComponent::GetImageAssetSourcePath() const
    {
        // The m_imageAsset path is to the product, so it will have an additional extension:
        //      e.g. image.png.streamingimage
        // So to provide just the source asset path we need to remove the product extension
        AZStd::string imageAssetPath = GetImageAssetPath();
        AZ::IO::Path imageSourceAssetPath = AZ::IO::Path(imageAssetPath).ReplaceExtension("");

        return imageSourceAssetPath.c_str();
    }

    void ImageGradientComponent::SetImageAssetPath(const AZStd::string& assetPath)
    {
        AZ::Data::AssetId assetId;

        if (!assetPath.empty())
        {
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, assetPath.c_str(), AZ::Data::s_invalidAssetType, false);

            if (!assetId.IsValid())
            {
                // This case can occur either if the asset path is completely wrong, or if it's correct but the asset is still in
                // the process of being created and being processed. Even though the second possibility is valid, 
                AZ_Warning(
                    "GradientSignal", false, "Can't find an Asset ID for %s, SetImageAssetPath() will be ignored.", assetPath.c_str());
                return;
            }
        }

        // If we were given a valid asset, then make sure it is the right type
        if (assetId.IsValid())
        {
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetId);

            if (assetInfo.m_assetType != azrtti_typeid<AZ::RPI::StreamingImageAsset>())
            {
                AZ_Warning("GradientSignal", false, "Asset type for %s is not AZ::RPI::StreamingImageAsset, will be ignored", assetPath.c_str());
                return;
            }
        }

        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> imageAsset;

        if (assetId.IsValid())
        {
            imageAsset = AZ::Data::AssetManager::Instance().FindOrCreateAsset(
                assetId, azrtti_typeid<AZ::RPI::StreamingImageAsset>(), m_configuration.m_imageAsset.GetAutoLoadBehavior());
        }

        SetImageAsset(imageAsset);
    }

    void ImageGradientComponent::SetImageAssetSourcePath(const AZStd::string& assetPath)
    {
        // SetImageAssetPath expects a product asset path, so we need to append the product
        // extension to the source asset path we are given
        AZStd::string productAssetPath(assetPath);
        productAssetPath += ".streamingimage";

        SetImageAssetPath(productAssetPath);
    }

    AZ::Data::Asset<AZ::RPI::StreamingImageAsset> ImageGradientComponent::GetImageAsset() const
    {
        return m_configuration.m_imageAsset;
    }

    void ImageGradientComponent::SetImageAsset(const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& asset)
    {
        // If we're setting the component to the same asset we're already using, then early-out.
        if (asset.GetId() == m_configuration.m_imageAsset.GetId())
        {
            return;
        }

        // Stop listening for the current image asset.
        AZ::Data::AssetBus::Handler::BusDisconnect(m_configuration.m_imageAsset.GetId());

        {
            // Only hold the lock during the actual data changes, to ensure that we aren't mid-query when changing it, but also to
            // minimize the total lock duration. Also, because we've disconnected from the imageAsset Asset bus prior to locking this,
            // we won't get any OnAsset* notifications while we're changing out the asset.
            AZStd::unique_lock lock(m_queryMutex);

            // Clear our cached image data unless we're currently using a modification buffer.
            // If we're using a modification buffer, we want to keep it active until the new image has finished loading in.
            if (!asset.IsReady() && !ModificationBufferIsActive())
            {
                UpdateCachedImageBufferData({}, {});
            }

            m_configuration.m_imageAsset = asset;
        }

        if (m_configuration.m_imageAsset.GetId().IsValid())
        {
            // If we have a valid Asset ID, check to see if it also appears in the AssetCatalog. This might be an Asset ID for an asset
            // that doesn't exist yet if it was just created from the Editor component.
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, m_configuration.m_imageAsset.GetId());

            // Only queue the load if it appears in the Asset Catalog. If it doesn't, we'll get notified when it shows up.
            if (assetInfo.m_assetId.IsValid())
            {
                m_configuration.m_imageAsset.QueueLoad(AZ::Data::AssetLoadParameters(nullptr, AZ::Data::AssetDependencyLoadRules::LoadAll));
            }

            // Start listening for all events for this asset.
            AZ::Data::AssetBus::Handler::BusConnect(m_configuration.m_imageAsset.GetId());
        }

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }


    uint32_t ImageGradientComponent::GetImageHeight() const
    {
        return m_imageDescriptor.m_size.m_height;
    }

    uint32_t ImageGradientComponent::GetImageWidth() const
    {
        return m_imageDescriptor.m_size.m_width;
    }

    AZ::Vector2 ImageGradientComponent::GetImagePixelsPerMeter() const
    {
        // Get the number of pixels in our image that maps to each meter based on the tiling settings.

        const auto& width = m_imageDescriptor.m_size.m_width;
        const auto& height = m_imageDescriptor.m_size.m_height;

        if (width > 0 && height > 0)
        {
            const AZ::Aabb bounds = m_gradientTransform.GetBounds();
            const AZ::Vector2 boundsMeters(bounds.GetExtents());
            const AZ::Vector2 imagePixelsInBounds(width / GetTilingX(), height / GetTilingY());
            return imagePixelsInBounds / boundsMeters;
        }

        return AZ::Vector2::CreateZero();
    }

    float ImageGradientComponent::GetTilingX() const
    {
        return m_configuration.m_tiling.GetX();
    }

    void ImageGradientComponent::SetTilingX(float tilingX)
    {
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.m_tiling.SetX(tilingX);
        }
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float ImageGradientComponent::GetTilingY() const
    {
        return m_configuration.m_tiling.GetY();
    }

    void ImageGradientComponent::SetTilingY(float tilingY)
    {
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.m_tiling.SetY(tilingY);
        }
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void ImageGradientComponent::SetPixelValueByPosition(const AZ::Vector3& position, float value)
    {
        SetPixelValuesByPosition(AZStd::span<const AZ::Vector3>(&position, 1), AZStd::span<float>(&value, 1));
    }

    void ImageGradientComponent::SetPixelValuesByPosition(AZStd::span<const AZ::Vector3> positions, AZStd::span<const float> values)
    {
        AZStd::unique_lock lock(m_queryMutex);

        if (m_modifiedImageData.empty())
        {
            AZ_Error("ImageGradientComponent", false,
                "Image modification mode needs to be started before the image values can be set.");
            return;
        }

        const auto& width = m_imageDescriptor.m_size.m_width;
        const auto& height = m_imageDescriptor.m_size.m_height;

        // No pixels, so nothing to modify.
        if ((width == 0) || (height == 0))
        {
            return;
        }

        const AZ::Vector3 tiledDimensions((width * GetTilingX()), (height * GetTilingY()), 0.0f);

        for (size_t index = 0; index < positions.size(); index++)
        {
            // Use the Gradient Transform to convert from world space to image space.
            AZ::Vector3 uvw = positions[index];
            bool wasPointRejected = true;
            m_gradientTransform.TransformPositionToUVWNormalized(positions[index], uvw, wasPointRejected);

            if (!wasPointRejected)
            {
                // Since the Image Gradient also has a tiling factor, scale the returned image space value
                // by the tiling factor to get to the specific pixel requested.
                AZ::Vector3 pixelLookup = (uvw * tiledDimensions);

                // UVs outside the 0-1 range are treated as infinitely tiling, we mod the values to bring them back into image bounds.
                float pixelX = pixelLookup.GetX();
                float pixelY = pixelLookup.GetY();
                auto x = aznumeric_cast<AZ::u32>(pixelX) % width;
                auto y = aznumeric_cast<AZ::u32>(pixelY) % height;

                // Flip the y because images are stored in reverse of our world axes
                y = (height - 1) - y;

                // Modify the correct pixel in our modification buffer.
                m_modifiedImageData[(y * width) + x] = values[index];
            }
        }
    }

    void ImageGradientComponent::GetPixelIndicesForPositions(
        AZStd::span<const AZ::Vector3> positions, AZStd::span<PixelIndex> outIndices) const
    {
        AZStd::shared_lock lock(m_queryMutex);

        const auto& width = m_imageDescriptor.m_size.m_width;
        const auto& height = m_imageDescriptor.m_size.m_height;

        const AZ::Vector3 tiledDimensions((width * GetTilingX()), (height * GetTilingY()), 0.0f);

        for (size_t index = 0; index < positions.size(); index++)
        {
            // Use the Gradient Transform to convert from world space to image space.
            AZ::Vector3 uvw = positions[index];
            bool wasPointRejected = true;
            m_gradientTransform.TransformPositionToUVWNormalized(positions[index], uvw, wasPointRejected);

            if ((width > 0) && (height > 0) && (!wasPointRejected))
            {
                // Since the Image Gradient also has a tiling factor, scale the returned image space value
                // by the tiling factor to get to the specific pixel requested.
                AZ::Vector3 pixelLookup = (uvw * tiledDimensions);

                // UVs outside the 0-1 range are treated as infinitely tiling, we mod the values to bring them back into image bounds.
                float pixelX = pixelLookup.GetX();
                float pixelY = pixelLookup.GetY();
                auto x = aznumeric_cast<AZ::u32>(pixelX) % width;
                auto y = aznumeric_cast<AZ::u32>(pixelY) % height;

                // Flip the y because images are stored in reverse of our world axes
                y = (height - 1) - y;

                outIndices[index] = PixelIndex(aznumeric_cast<int16_t>(x), aznumeric_cast<int16_t>(y));
            }
            else
            {
                outIndices[index] = PixelIndex(aznumeric_cast<int16_t>(-1), aznumeric_cast<int16_t>(-1));
            }
        }

    }

    void ImageGradientComponent::GetPixelValuesByPixelIndex(AZStd::span<const PixelIndex> positions, AZStd::span<float> outValues) const
    {
        AZStd::shared_lock lock(m_queryMutex);

        const auto& width = m_imageDescriptor.m_size.m_width;
        const auto& height = m_imageDescriptor.m_size.m_height;

        for (size_t index = 0; index < positions.size(); index++)
        {
            const auto& [x, y] = positions[index];

            if ((x >= 0) && (x < aznumeric_cast<int16_t>(width)) && (y >= 0) && (y < aznumeric_cast<int16_t>(height)))
            {
                // For terrarium, there is a separate algorithm for retrieving the value
                outValues[index] = (m_currentChannel == ChannelToUse::Terrarium)
                    ? GetTerrariumPixelValue(x, y)
                    : AZ::RPI::GetImageDataPixelValue<float>(
                          m_imageData, m_imageDescriptor, x, y, aznumeric_cast<AZ::u8>(m_currentChannel));
            }
        }
    }

    void ImageGradientComponent::SetPixelValueByPixelIndex(const PixelIndex& position, float value)
    {
        SetPixelValuesByPixelIndex(AZStd::span<const PixelIndex>(&position, 1), AZStd::span<float>(&value, 1));
    }

    void ImageGradientComponent::SetPixelValuesByPixelIndex(AZStd::span<const PixelIndex> positions, AZStd::span<const float> values)
    {
        AZStd::unique_lock lock(m_queryMutex);

        if (m_modifiedImageData.empty())
        {
            AZ_Error("ImageGradientComponent", false, "Image modification mode needs to be started before the image values can be set.");
            return;
        }

        const auto& width = m_imageDescriptor.m_size.m_width;
        const auto& height = m_imageDescriptor.m_size.m_height;

        // No pixels, so nothing to modify.
        if ((width == 0) || (height == 0))
        {
            return;
        }

        for (size_t index = 0; index < positions.size(); index++)
        {
            const auto& [x, y] = positions[index];

            if ((x >= 0) && (x < aznumeric_cast<int16_t>(width)) && (y >= 0) && (y < aznumeric_cast<int16_t>(height)))
            {
                // Modify the correct pixel in our modification buffer.
                m_modifiedImageData[(y * width) + x] = values[index];
            }
        }
    }


}
