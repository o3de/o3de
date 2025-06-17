/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <ACES/Aces.h>
#include <LookupTable/LookupTableAsset.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

#include <AzCore/Debug/Trace.h>

namespace
{
    static const AZ::RHI::Format LutFormat = AZ::RHI::Format::R16G16B16A16_FLOAT;

    uint16_t ConvertFloatToHalf(const float Value)
    {
        uint32_t result;

        uint32_t uiValue = ((uint32_t*)(&Value))[0];
        uint32_t sign = (uiValue & 0x80000000U) >> 16U; // Sign shifted two bytes right for combining with return
        uiValue = uiValue & 0x7FFFFFFFU; // Hack off the sign

        if (uiValue > 0x47FFEFFFU)
        {
            // The number is too large to be represented as a half.  Saturate to infinity.
            result = 0x7FFFU;
        }
        else
        {
            if (uiValue < 0x38800000U)
            {
                // The number is too small to be represented as a normalized half.
                // Convert it to a denormalized value.
                uint32_t shift = 113U - (uiValue >> 23U);
                uiValue = (0x800000U | (uiValue & 0x7FFFFFU)) >> shift;
            }
            else
            {
                // Rebias the exponent to represent the value as a normalized half.
                uiValue += 0xC8000000U;
            }
            result = ((uiValue + 0x0FFFU + ((uiValue >> 13U) & 1U)) >> 13U) & 0x7FFFU;
        }
        // Add back sign and return
        return static_cast<uint16_t>(result | sign);
    }
}

namespace AZ::Render
{
    void AcesDisplayMapperFeatureProcessor::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext
                ->Class<AcesDisplayMapperFeatureProcessor, FeatureProcessor>()
                ->Version(0);
        }
    }

    void AcesDisplayMapperFeatureProcessor::Activate()
    {
    }

    void AcesDisplayMapperFeatureProcessor::Deactivate()
    {
        m_ownedLuts.clear();
    }

    void AcesDisplayMapperFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
    {
        AZ_PROFILE_FUNCTION(AzRender);
        AZ_UNUSED(packet);
    }

    void AcesDisplayMapperFeatureProcessor::Render([[maybe_unused]] const FeatureProcessor::RenderPacket& packet)
    {
    }

    void AcesDisplayMapperFeatureProcessor::ApplyLdrOdtParameters(DisplayMapperParameters* displayMapperParameters)
    {
        AZ_Assert(displayMapperParameters != nullptr, "The pOutParameters must not to be null pointer.");
        if (displayMapperParameters == nullptr)
        {
            return;
        }

        // These values in the ODT parameter are taken from the reference ACES transform. 
        //  
        // The original ACES references.
        // Common:
        // https://github.com/ampas/aces-dev/blob/master/transforms/ctl/lib/ACESlib.ODT_Common.ctl
        // For sRGB:
        // https://github.com/ampas/aces-dev/tree/master/transforms/ctl/odt/sRGB
        displayMapperParameters->m_cinemaLimits[0] = 0.02f;
        displayMapperParameters->m_cinemaLimits[1] = 48.0f;
        displayMapperParameters->m_acesSplineParams = GetAcesODTParameters(OutputDeviceTransformType_48Nits);
        displayMapperParameters->m_OutputDisplayTransformFlags = AlterSurround | ApplyDesaturation | ApplyCATD60toD65;
        displayMapperParameters->m_OutputDisplayTransformMode = Srgb;
        ColorConvertionMatrixType   colorMatrixType = XYZ_To_Rec709;
        switch (displayMapperParameters->m_OutputDisplayTransformMode)
        {
        case Srgb:
            colorMatrixType = XYZ_To_Rec709;
            break;
        case PerceptualQuantizer:
        case Ldr:
            colorMatrixType = XYZ_To_Bt2020;
            break;
        default:
            break;
        }
        displayMapperParameters->m_XYZtoDisplayPrimaries = GetColorConvertionMatrix(colorMatrixType);

        displayMapperParameters->m_surroundGamma = 0.9811f;
        displayMapperParameters->m_gamma = 2.2f;
    }

    void AcesDisplayMapperFeatureProcessor::ApplyHdrOdtParameters(DisplayMapperParameters* displayMapperParameters, const OutputDeviceTransformType& odtType)
    {
        AZ_Assert(displayMapperParameters != nullptr, "The pOutParameters must not to be null pointer.");
        if (displayMapperParameters == nullptr)
        {
            return;
        }

        // Dynamic range limit values taken from NVIDIA HDR sample. 
        // These values represent and low and high end of the dynamic range in terms of stops from middle grey (0.18)
        float lowerDynamicRangeInStops = -12.f;
        float higherDynamicRangeInStops = 10.f;
        const float MIDDLE_GREY = 0.18f;

        switch (odtType)
        {
        case OutputDeviceTransformType_1000Nits:
            higherDynamicRangeInStops = 10.f;
            break;
        case OutputDeviceTransformType_2000Nits:
            higherDynamicRangeInStops = 11.f;
            break;
        case OutputDeviceTransformType_4000Nits:
            higherDynamicRangeInStops = 12.f;
            break;
        default:
            AZ_Assert(false, "Invalid output device transform type.");
            break;
        }

        displayMapperParameters->m_cinemaLimits[0] = MIDDLE_GREY * exp2(lowerDynamicRangeInStops);
        displayMapperParameters->m_cinemaLimits[1] = MIDDLE_GREY * exp2(higherDynamicRangeInStops);
        displayMapperParameters->m_acesSplineParams = GetAcesODTParameters(odtType);
        displayMapperParameters->m_OutputDisplayTransformFlags = AlterSurround | ApplyDesaturation | ApplyCATD60toD65;
        displayMapperParameters->m_OutputDisplayTransformMode = PerceptualQuantizer;
        ColorConvertionMatrixType   colorMatrixType = XYZ_To_Bt2020;
        displayMapperParameters->m_XYZtoDisplayPrimaries = GetColorConvertionMatrix(colorMatrixType);

        // Surround gamma value is from the dim surround gamma from the ACES reference transforms.
        // https://github.com/ampas/aces-dev/blob/master/transforms/ctl/lib/ACESlib.ODT_Common.ctl
        displayMapperParameters->m_surroundGamma = 0.9811f;
        displayMapperParameters->m_gamma = 1.0f; // gamma not used with perceptual quantizer, but just set to 1.0 anyways
    }

    OutputDeviceTransformType AcesDisplayMapperFeatureProcessor::GetOutputDeviceTransformType(RHI::Format bufferFormat)
    {
        OutputDeviceTransformType outputDeviceTransformType = OutputDeviceTransformType_48Nits;
        if (bufferFormat == RHI::Format::R8G8B8A8_UNORM ||
            bufferFormat == RHI::Format::B8G8R8A8_UNORM)
        {
            outputDeviceTransformType = OutputDeviceTransformType_48Nits;
        }
        else if (bufferFormat == RHI::Format::R10G10B10A2_UNORM)
        {
            outputDeviceTransformType = OutputDeviceTransformType_1000Nits;
        }
        else
        {
            AZ_Assert(false, "Not yet supported.");
            // To work normally on unsupported environment, initialize the display parameters by OutputDeviceTransformType_48Nits.
            outputDeviceTransformType = OutputDeviceTransformType_48Nits;
        }
        return outputDeviceTransformType;
    }

    void AcesDisplayMapperFeatureProcessor::GetAcesDisplayMapperParameters(DisplayMapperParameters* displayMapperParameters, OutputDeviceTransformType odtType)
    {
        switch (odtType)
        {
        case OutputDeviceTransformType_48Nits:
            ApplyLdrOdtParameters(displayMapperParameters);
            break;
        case OutputDeviceTransformType_1000Nits:
        case OutputDeviceTransformType_2000Nits:
        case OutputDeviceTransformType_4000Nits:
            ApplyHdrOdtParameters(displayMapperParameters, odtType);
            break;
        default:
            AZ_Assert(false, "This ODT type[%d] is not supported.", odtType);
            break;
        }
    }

    void AcesDisplayMapperFeatureProcessor::GetOwnedLut(DisplayMapperLut& displayMapperLut, const AZ::Name& lutName)
    {
        auto it = m_ownedLuts.find(lutName);
        if (it == m_ownedLuts.end())
        {
            InitializeLutImage(lutName);
            it = m_ownedLuts.find(lutName);
            AZ_Assert(it != m_ownedLuts.end(), "AcesDisplayMapperFeatureProcessor unable to create LUT %s", lutName.GetCStr());
        }
        displayMapperLut = it->second;
    }

    void AcesDisplayMapperFeatureProcessor::GetDisplayMapperLut(DisplayMapperLut& displayMapperLut)
    {
        const AZ::Name acesLutName("AcesLutImage");
        auto it = m_ownedLuts.find(acesLutName);
        if (it == m_ownedLuts.end())
        {
            InitializeLutImage(acesLutName);

            it = m_ownedLuts.find(acesLutName);
            AZ_Assert(it != m_ownedLuts.end(), "AcesDisplayMapperFeatureProcessor unable to create ACES LUT image");
        }
        displayMapperLut = it->second;
    }

    void AcesDisplayMapperFeatureProcessor::GetLutFromAssetLocation(DisplayMapperAssetLut& displayMapperAssetLut, const AZStd::string& assetPath)
    {
        Data::AssetId assetId = RPI::AssetUtils::GetAssetIdForProductPath(assetPath.c_str(), RPI::AssetUtils::TraceLevel::Error);
        GetLutFromAssetId(displayMapperAssetLut, assetId);
    }

    void AcesDisplayMapperFeatureProcessor::GetLutFromAssetId(DisplayMapperAssetLut& displayMapperAssetLut, const AZ::Data::AssetId assetId)
    {
        if (!assetId.IsValid())
        {
            return;
        }

        // Check first if this already exists
        auto it = m_assetLuts.find(assetId.ToString<AZStd::string>());
        if (it != m_assetLuts.end())
        {
            displayMapperAssetLut = it->second;
            return;
        }

        // Read the lut which is a .3dl file embedded within an azasset file.
        Data::Asset<RPI::AnyAsset> asset = RPI::AssetUtils::LoadAssetById<RPI::AnyAsset>(assetId, RPI::AssetUtils::TraceLevel::Error);
        const LookupTableAsset* lutAsset = RPI::GetDataFromAnyAsset<LookupTableAsset>(asset);

        if (lutAsset == nullptr)
        {
            AZ_Error("AcesDisplayMapperFeatureProcessor", false, "Unable to read LUT from asset.");
            asset.Release();
            return;
        }

        // The first row of numbers in a 3dl file is a number of vertices that partition the space from [0,..1023]
        // This assumes that the vertices are evenly spaced apart. Non-uniform spacing is supported by the format,
        // but haven't been encountered yet.
        const size_t lutSize = lutAsset->m_intervals.size();

        if (lutSize == 0)
        {
            AZ_Error("AcesDisplayMapperFeatureProcessor", false, "Lut asset has invalid size.");
            asset.Release();
            return;
        }

        // Create a buffer of half floats from the LUT and use it to initialize a 3d texture.

        const size_t kChannels = 4;
        const size_t kChannelBytes = 2;
        const size_t bytesPerRow = lutSize * kChannels * kChannelBytes;
        const size_t bytesPerSlice = bytesPerRow * lutSize;

        AZStd::vector<uint16_t> u16Buffer;
        const size_t bufferSize = lutSize * lutSize * lutSize * kChannels;
        u16Buffer.resize(bufferSize);

        for (size_t slice = 0; slice < lutSize; slice++)
        {
            for (size_t column = 0; column < lutSize; column++)
            {
                for (size_t row = 0; row < lutSize; row++)
                {
                    // Index in the LUT texture data
                    size_t idx = (column * kChannels) +
                        ((bytesPerRow * row) / kChannelBytes) +
                        ((bytesPerSlice * slice) / kChannelBytes);

                    // Vertices the .3dl file are listed first by increasing slice, then row, and finally column coordinate
                    // This corresponds to blue, green, and red channels, respectively.
                    size_t assetIdx = slice + lutSize * row + (lutSize * lutSize * column);

                    AZ::u64 red = lutAsset->m_values[assetIdx * 3 + 0];
                    AZ::u64 green = lutAsset->m_values[assetIdx * 3 + 1];
                    AZ::u64 blue = lutAsset->m_values[assetIdx * 3 + 2];
                    
                    // The vertices in the file are given as a positive integer value in [0,..4095] and need to be normalized
                    constexpr float NormalizeValue = 4095.0f;

                    u16Buffer[idx + 0] = ConvertFloatToHalf(static_cast<float>(red) / NormalizeValue);
                    u16Buffer[idx + 1] = ConvertFloatToHalf(static_cast<float>(green) / NormalizeValue);
                    u16Buffer[idx + 2] = ConvertFloatToHalf(static_cast<float>(blue) / NormalizeValue);
                    u16Buffer[idx + 3] = 0x3b00; // 1.0 in half
                }
            }
        }

        asset.Release();

        Data::Instance<RPI::StreamingImagePool> streamingImagePool = RPI::ImageSystemInterface::Get()->GetSystemStreamingPool();

        RHI::Size imageSize;
        imageSize.m_width = static_cast<uint32_t>(lutSize);
        imageSize.m_height = static_cast<uint32_t>(lutSize);
        imageSize.m_depth = static_cast<uint32_t>(lutSize);
        size_t imageDataSize = bytesPerSlice * lutSize;

        Data::Instance<RPI::StreamingImage> lutStreamingImage = RPI::StreamingImage::CreateFromCpuData(
            *streamingImagePool, RHI::ImageDimension::Image3D, imageSize, LutFormat, u16Buffer.data(), imageDataSize);

        AZ_Error("AcesDisplayMapperFeatureProcessor", lutStreamingImage, "Failed to initialize the lut assetId %s.", assetId.ToString<AZStd::string>().c_str());

        DisplayMapperAssetLut assetLut;
        assetLut.m_lutStreamingImage = lutStreamingImage;

        // Add to the list of LUT asset resources
        m_assetLuts.insert(AZStd::pair<AZStd::string, DisplayMapperAssetLut>(assetId.ToString<AZStd::string>(), assetLut));
        displayMapperAssetLut = assetLut;
    }

    void AcesDisplayMapperFeatureProcessor::InitializeImagePool()
    {
        m_displayMapperImagePool = aznew RHI::ImagePool;
        m_displayMapperImagePool->SetName(Name("DisplayMapperImagePool"));

        RHI::ImagePoolDescriptor   imagePoolDesc = {};
        imagePoolDesc.m_bindFlags = RHI::ImageBindFlags::ShaderReadWrite;
        imagePoolDesc.m_budgetInBytes = ImagePoolBudget;

        RHI::ResultCode resultCode = m_displayMapperImagePool->Init(imagePoolDesc);
        if (resultCode != RHI::ResultCode::Success)
        {
            AZ_Error("AcesDisplayMapperFeatureProcessor", false, "Failed to initialize image pool.");
            return;
        }
    }

    void AcesDisplayMapperFeatureProcessor::InitializeLutImage(const AZ::Name& lutName)
    {
        if (!m_displayMapperImagePool)
        {
            InitializeImagePool();
        }

        DisplayMapperLut lutResource;
        lutResource.m_lutImage = aznew RHI::Image;
        lutResource.m_lutImage->SetName(lutName);

        RHI::ImageInitRequest imageRequest;
        imageRequest.m_image = lutResource.m_lutImage.get();
        static const int LutSize = 32;
        imageRequest.m_descriptor = RHI::ImageDescriptor::Create3D(RHI::ImageBindFlags::ShaderReadWrite, LutSize, LutSize, LutSize, LutFormat);
        RHI::ResultCode resultCode = m_displayMapperImagePool->InitImage(imageRequest);

        if (resultCode != RHI::ResultCode::Success)
        {
            AZ_Error("AcesDisplayMapperFeatureProcessor", false, "Failed to initialize LUT image.");
            return;
        }

        lutResource.m_lutImageViewDescriptor = RHI::ImageViewDescriptor::Create(LutFormat, 0, 0);
        lutResource.m_lutImageView = lutResource.m_lutImage->BuildImageView(lutResource.m_lutImageViewDescriptor);
        if (!lutResource.m_lutImageView.get())
        {
            AZ_Error("AcesDisplayMapperFeatureProcessor", false, "Failed to initialize LUT image view.");
            return;
        }

        // Add to the list of lut resources
        lutResource.m_lutImageView->SetName(lutName);
        m_ownedLuts[lutName] = lutResource;
    }

    ShaperParams AcesDisplayMapperFeatureProcessor::GetShaperParameters(ShaperPresetType shaperPreset, float customMinEv, float customMaxEv)
    {
        // Default is a linear shaper with bias 0.0 and scale 1.0. That is, fx = x*1.0 + 0.0
        ShaperParams shaperParams = { ShaperType::Linear, 0.0, 1.f };
        switch (shaperPreset)
        {
        case ShaperPresetType::None:
            break;
        case ShaperPresetType::Log2_48Nits:
            shaperParams = GetAcesShaperParameters(OutputDeviceTransformType::OutputDeviceTransformType_48Nits);
            break;
        case ShaperPresetType::Log2_1000Nits:
            shaperParams = GetAcesShaperParameters(OutputDeviceTransformType::OutputDeviceTransformType_1000Nits);
            break;
        case ShaperPresetType::Log2_2000Nits:
            shaperParams = GetAcesShaperParameters(OutputDeviceTransformType::OutputDeviceTransformType_2000Nits);
            break;
        case ShaperPresetType::Log2_4000Nits:
            shaperParams = GetAcesShaperParameters(OutputDeviceTransformType::OutputDeviceTransformType_4000Nits);
            break;
        case ShaperPresetType::LinearCustomRange:
        {
            // Map the range min exposure - max exposure to 0-1. Convert EV values to linear values here to avoid that work in the shader.
            // Shader equation becomes (x - bias) / scale;
            constexpr float MediumGray = 0.18f;
            const float minValue = MediumGray * powf(2, customMinEv);
            const float maxValue = MediumGray * powf(2, customMaxEv);
            shaperParams.m_type = ShaperType::Linear;
            shaperParams.m_scale = 1.0f / (maxValue - minValue);
            shaperParams.m_bias = -minValue * shaperParams.m_scale;
            break;
        }
        case ShaperPresetType::Log2CustomRange:
            shaperParams = GetLog2ShaperParameters(customMinEv, customMaxEv);
            break;
        case ShaperPresetType::PqSmpteSt2084:
            shaperParams.m_type = ShaperType::PqSmpteSt2084;
            break;
        default:
            AZ_Error("DisplayMapperPass", false, "Invalid shaper preset type.");
            break;
        }
        return shaperParams;
    }

    void AcesDisplayMapperFeatureProcessor::GetDefaultDisplayMapperConfiguration(DisplayMapperConfigurationDescriptor& config)
    {
        // Default configuration is ACES with LDR color grading LUT disabled.
        config.m_operationType = DisplayMapperOperationType::Aces;
        config.m_ldrGradingLutEnabled = false;
        config.m_ldrColorGradingLut.Release();
    }

    void AcesDisplayMapperFeatureProcessor::RegisterDisplayMapperConfiguration(const DisplayMapperConfigurationDescriptor& config)
    {
        m_displayMapperConfiguration = config;
    }

    void AcesDisplayMapperFeatureProcessor::UnregisterDisplayMapperConfiguration()
    {
        m_displayMapperConfiguration.reset();
    }

    const DisplayMapperConfigurationDescriptor* AcesDisplayMapperFeatureProcessor::GetDisplayMapperConfiguration()
    {
        return m_displayMapperConfiguration.has_value() ? &m_displayMapperConfiguration.value() : nullptr;
    }
} // namespace AZ::Render
