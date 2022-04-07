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

    AZStd::vector<AZ::Edit::EnumConstant<ChannelToUse>> SupportedChannelOptions()
    {
        AZStd::vector<AZ::Edit::EnumConstant<ChannelToUse>> options;

        options.push_back(AZ::Edit::EnumConstant<ChannelToUse>(ChannelToUse::Red, "Red"));
        options.push_back(AZ::Edit::EnumConstant<ChannelToUse>(ChannelToUse::Green, "Green"));
        options.push_back(AZ::Edit::EnumConstant<ChannelToUse>(ChannelToUse::Blue, "Blue"));
        options.push_back(AZ::Edit::EnumConstant<ChannelToUse>(ChannelToUse::Alpha, "Alpha"));
        options.push_back(AZ::Edit::EnumConstant<ChannelToUse>(ChannelToUse::Terrarium, "Terrarium"));

        return options;
    }

    AZStd::vector<AZ::Edit::EnumConstant<CustomScaleType>> SupportedScaleOptions()
    {
        AZStd::vector<AZ::Edit::EnumConstant<CustomScaleType>> options;

        options.push_back(AZ::Edit::EnumConstant<CustomScaleType>(CustomScaleType::None, "None"));
        options.push_back(AZ::Edit::EnumConstant<CustomScaleType>(CustomScaleType::Auto, "Auto"));
        options.push_back(AZ::Edit::EnumConstant<CustomScaleType>(CustomScaleType::Manual, "Manual"));

        return options;
    }

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
                ->Version(5)
                ->Field("Tiling", &ImageGradientConfig::m_tiling)
                ->Field("StreamingImageAsset", &ImageGradientConfig::m_imageAsset)
                ->Field("AdvancedMode", &ImageGradientConfig::m_advancedMode)
                ->Field("ChannelToUse", &ImageGradientConfig::m_channelToUse)
                ->Field("CustomScale", &ImageGradientConfig::m_customScaleType)
                ->Field("ScaleRangeMin", &ImageGradientConfig::m_scaleRangeMin)
                ->Field("ScaleRangeMax", &ImageGradientConfig::m_scaleRangeMax)
                ->Field("MipIndex", &ImageGradientConfig::m_mipIndex)
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
                    ->Attribute(AZ::Edit::Attributes::Handler, AZ_CRC_CE("GradientSignalStreamingImageAsset"))
                    ->DataElement(AZ::Edit::UIHandlers::Vector2, &ImageGradientConfig::m_tiling, "Tiling", "Number of times to tile horizontally/vertically.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 1024.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.25f)

                    ->GroupElementToggle("Advanced", &ImageGradientConfig::m_advancedMode)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ImageGradientConfig::m_channelToUse, "Channel To Use", "The channel to use from the image.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::IsAdvancedModeReadOnly)
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &SupportedChannelOptions)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ImageGradientConfig::m_customScaleType, "Custom Scale", "Choose a type of scaling to be applied to the image data.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::IsAdvancedModeReadOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &SupportedScaleOptions)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImageGradientConfig::m_scaleRangeMin, "Range Minimum", "The minimum range each value from the image data is scaled against.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::IsAdvancedModeReadOnly)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ImageGradientConfig::GetManualScaleVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImageGradientConfig::m_scaleRangeMax, "Range Maximum", "The maximum range each value from the image data is scaled against.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::IsAdvancedModeReadOnly)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &ImageGradientConfig::GetManualScaleVisibility)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ImageGradientConfig::m_mipIndex, "Mip Index", "Mip index to sample from.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::IsAdvancedModeReadOnly)
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, AZ::RHI::Limits::Image::MipCountMax)
                    ;
            }
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

    bool ImageGradientConfig::IsAdvancedModeReadOnly() const
    {
        return !m_advancedMode;
    }

    AZ::Crc32 ImageGradientConfig::GetManualScaleVisibility() const
    {
        return (m_customScaleType == CustomScaleType::Manual) ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
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
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "vegetation")
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

        // Prevent loading of the image data if an invalid advanced configuration
        // was specified by the user
        if (m_configuration.m_advancedMode)
        {
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
                const auto channelOptions = SupportedChannelOptions();
                AZ_Error("GradientSignal", false, "Unable to use channel (%s) because image asset (%s) has pixel format (%s), which only supports %d channels",
                    channelOptions[channel].m_description.c_str(), m_configuration.m_imageAsset.GetHint().c_str(), AZ::RHI::ToString(format), numComponents);
                return;
            }

            m_currentChannel = m_configuration.m_channelToUse;
            m_currentScaleType = m_configuration.m_customScaleType;

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
        }
        else
        {
            m_currentChannel = ChannelToUse::Red;
            m_currentScaleType = CustomScaleType::None;
            m_currentMipIndex = 0;
        }

        // Update our cached image data
        m_imageDescriptor = m_configuration.m_imageAsset->GetImageDescriptorForMipLevel(m_currentMipIndex);
        m_imageData = m_configuration.m_imageAsset->GetSubImageData(m_currentMipIndex, 0);

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

    float ImageGradientComponent::GetValueFromImageData(const AZ::Vector3& uvw, float defaultValue) const
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

                const AZ::Vector3 tiledDimensions((width * GetTilingX()),
                    (height * GetTilingY()),
                    0.0f);

                // Convert from uv space back to pixel space
                AZ::Vector3 pixelLookup = (uvw * tiledDimensions);

                // UVs outside the 0-1 range are treated as infinitely tiling, so that we behave the same as the 
                // other gradient generators.  As mentioned above, if clamping is desired, we expect it to be applied
                // outside of this function.
                auto x = aznumeric_cast<AZ::u32>(pixelLookup.GetX()) % width;
                auto y = aznumeric_cast<AZ::u32>(pixelLookup.GetY()) % height;

                // Flip the y because images are stored in reverse of our world axes
                y = (height - 1) - y;

                // For terrarium, there is a separate algorithm for retrieving the value
                const float value = (m_currentChannel == ChannelToUse::Terrarium)
                    ? GetTerrariumPixelValue(x, y)
                    : AZ::RPI::GetImageDataPixelValue<float>(
                        m_imageData, m_imageDescriptor, x, y, aznumeric_cast<AZ::u8>(m_currentChannel));

                // Scale (inverse lerp) the value into a 0 - 1 range. We also clamp it because manual scale values could cause
                // the result to fall outside of the expected output range.
                return AZStd::clamp((value - m_offset) * m_multiplier, 0.0f, 1.0f);
            }
        }

        return defaultValue;
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
                return GetValueFromImageData(uvw, 0.0f);
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
                outValues[index] = GetValueFromImageData(uvw, 0.0f);
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
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, assetPath.c_str(), AZ::Data::s_invalidAssetType, false);
        if (assetId.IsValid() || assetPath.empty())
        {
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

            AZ::Data::AssetBus::Handler::BusDisconnect(m_configuration.m_imageAsset.GetId());

            {
                AZStd::unique_lock<decltype(m_imageMutex)> imageLock(m_imageMutex);

                // Clear our cached image data
                m_imageData = AZStd::span<const uint8_t>();

                if (assetPath.empty())
                {
                    m_configuration.m_imageAsset.Reset();
                }
                else
                {
                    m_configuration.m_imageAsset = AZ::Data::AssetManager::Instance().FindOrCreateAsset(assetId, azrtti_typeid<AZ::RPI::StreamingImageAsset>(), m_configuration.m_imageAsset.GetAutoLoadBehavior());
                }
            }

            SetupDependencies();
            m_configuration.m_imageAsset.QueueLoad();
            AZ::Data::AssetBus::Handler::BusConnect(m_configuration.m_imageAsset.GetId());
            LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
        }
    }

    float ImageGradientComponent::GetTilingX() const
    {
        return m_configuration.m_tiling.GetX();
    }

    void ImageGradientComponent::SetTilingX(float tilingX)
    {
        m_configuration.m_tiling.SetX(tilingX);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float ImageGradientComponent::GetTilingY() const
    {
        return m_configuration.m_tiling.GetY();
    }

    void ImageGradientComponent::SetTilingY(float tilingY)
    {
        m_configuration.m_tiling.SetY(tilingY);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}
