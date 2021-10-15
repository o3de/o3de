/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <BuilderSettings/PresetSettings.h>
#include <Processing/PixelFormatInfo.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace ImageProcessingAtom
{
    PresetSettings::PresetSettings()
    {
    }

    PresetSettings::PresetSettings(const PresetSettings& other)
    {
        DeepCopyMembers(other);
    }

    void PresetSettings::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<PresetSettings>()
                ->Version(2)
                ->Field("UUID", &PresetSettings::m_uuid)
                ->Field("Name", &PresetSettings::m_name)
                ->Field("Description", &PresetSettings::m_description)
                ->Field("GenerateIBLOnly", &PresetSettings::m_generateIBLOnly)
                ->Field("RGB_Weight", &PresetSettings::m_rgbWeight)
                ->Field("SourceColor", &PresetSettings::m_srcColorSpace)
                ->Field("DestColor", &PresetSettings::m_destColorSpace)
                ->Field("FileMasks", &PresetSettings::m_fileMasks)
                ->Field("SuppressEngineReduce", &PresetSettings::m_suppressEngineReduce)
                ->Field("PixelFormat", &PresetSettings::m_pixelFormat)
                ->Field("PixelFormatAlpha", &PresetSettings::m_pixelFormatAlpha)
                ->Field("DiscardAlpha", &PresetSettings::m_discardAlpha)
                ->Field("MaxTextureSize", &PresetSettings::m_maxTextureSize)
                ->Field("MinTextureSize", &PresetSettings::m_minTextureSize)
                ->Field("IsPowerOf2", &PresetSettings::m_isPowerOf2)
                ->Field("SizeReduceLevel", &PresetSettings::m_sizeReduceLevel)
                ->Field("IsColorChart", &PresetSettings::m_isColorChart)
                ->Field("HighPassMip", &PresetSettings::m_highPassMip)
                ->Field("GlossFromNormal", &PresetSettings::m_glossFromNormals)
                ->Field("UseLegacyGloss", &PresetSettings::m_isLegacyGloss)
                ->Field("MipRenormalize", &PresetSettings::m_isMipRenormalize)
                ->Field("NumberResidentMips", &PresetSettings::m_numResidentMips)
                ->Field("Swizzle", &PresetSettings::m_swizzle)
                ->Field("CubemapSettings", &PresetSettings::m_cubemapSetting)
                ->Field("MipMapSetting", &PresetSettings::m_mipmapSetting);

            serialize->Enum<RGBWeight>()
                ->Value("Uniform", RGBWeight::uniform)
                ->Value("Luminance", RGBWeight::luminance)
                ->Value("CIEXYZ", RGBWeight::ciexyz)
                ;

            serialize->Enum<ColorSpace>()
                ->Value("Linear", ColorSpace::linear)
                ->Value("sRGB", ColorSpace::sRGB)
                ->Value("Auto", ColorSpace::autoSelect)
                ;

            serialize->Enum<CubemapFilterType>()
                ->Value("Disc", CubemapFilterType::disc)
                ->Value("Cone", CubemapFilterType::cone)
                ->Value("Cosine", CubemapFilterType::cosine)
                ->Value("Gaussian", CubemapFilterType::gaussian)
                ->Value("CosinePower", CubemapFilterType::cosine_power)
                ->Value("GGX", CubemapFilterType::ggx)
                ;

            serialize->Enum<MipGenType>()
                ->Value("Point", MipGenType::point)
                ->Value("Box", MipGenType::box)
                ->Value("Triangle", MipGenType::triangle)
                ->Value("Quadratic", MipGenType::quadratic)
                ->Value("Gaussian", MipGenType::gaussian)
                ->Value("BlackmanHarris", MipGenType::blackmanHarris)
                ->Value("KaiserSinc", MipGenType::kaiserSinc)
                ;

            serialize->Enum<EPixelFormat>()
                ->Value("R8G8B8A8", EPixelFormat::ePixelFormat_R8G8B8A8)
                ->Value("R8G8B8X8", EPixelFormat::ePixelFormat_R8G8B8X8)
                ->Value("R8G8", EPixelFormat::ePixelFormat_R8G8)
                ->Value("R8", EPixelFormat::ePixelFormat_R8)
                ->Value("A8", EPixelFormat::ePixelFormat_A8)
                ->Value("R16G16B16A16", EPixelFormat::ePixelFormat_R16G16B16A16)
                ->Value("R16G16", EPixelFormat::ePixelFormat_R16G16)
                ->Value("R16", EPixelFormat::ePixelFormat_R16)                    
                ->Value("ASTC_4x4", EPixelFormat::ePixelFormat_ASTC_4x4)
                ->Value("ASTC_5x4", EPixelFormat::ePixelFormat_ASTC_5x4)
                ->Value("ASTC_5x5", EPixelFormat::ePixelFormat_ASTC_5x5)
                ->Value("ASTC_6x5", EPixelFormat::ePixelFormat_ASTC_6x5)
                ->Value("ASTC_6x6", EPixelFormat::ePixelFormat_ASTC_6x6)
                ->Value("ASTC_8x5", EPixelFormat::ePixelFormat_ASTC_8x5)
                ->Value("ASTC_8x6", EPixelFormat::ePixelFormat_ASTC_8x6)
                ->Value("ASTC_8x8", EPixelFormat::ePixelFormat_ASTC_8x8)
                ->Value("ASTC_10x5", EPixelFormat::ePixelFormat_ASTC_10x5)
                ->Value("ASTC_10x6", EPixelFormat::ePixelFormat_ASTC_10x6)
                ->Value("ASTC_10x8", EPixelFormat::ePixelFormat_ASTC_10x8)
                ->Value("ASTC_10x10", EPixelFormat::ePixelFormat_ASTC_10x10)
                ->Value("ASTC_12x10", EPixelFormat::ePixelFormat_ASTC_12x10)
                ->Value("ASTC_12x12", EPixelFormat::ePixelFormat_ASTC_12x12)
                ->Value("BC1", EPixelFormat::ePixelFormat_BC1)
                ->Value("BC1a", EPixelFormat::ePixelFormat_BC1a)
                ->Value("BC3", EPixelFormat::ePixelFormat_BC3)
                ->Value("BC3t", EPixelFormat::ePixelFormat_BC3t)
                ->Value("BC4", EPixelFormat::ePixelFormat_BC4)
                ->Value("BC4s", EPixelFormat::ePixelFormat_BC4s)
                ->Value("BC5", EPixelFormat::ePixelFormat_BC5)
                ->Value("BC5s", EPixelFormat::ePixelFormat_BC5s)
                ->Value("BC6UH", EPixelFormat::ePixelFormat_BC6UH)
                ->Value("BC7", EPixelFormat::ePixelFormat_BC7)
                ->Value("BC7t", EPixelFormat::ePixelFormat_BC7t)
                ->Value("R9G9B9E5", EPixelFormat::ePixelFormat_R9G9B9E5)
                ->Value("R32G32B32A32F", EPixelFormat::ePixelFormat_R32G32B32A32F)
                ->Value("R32G32F", EPixelFormat::ePixelFormat_R32G32F)
                ->Value("R32F", EPixelFormat::ePixelFormat_R32F)
                ->Value("R16G16B16A16F", EPixelFormat::ePixelFormat_R16G16B16A16F)
                ->Value("R16G16F", EPixelFormat::ePixelFormat_R16G16F)
                ->Value("R16F", EPixelFormat::ePixelFormat_R16F)
                ->Value("B8G8R8A8", EPixelFormat::ePixelFormat_B8G8R8A8)
                ->Value("R8G8B8", EPixelFormat::ePixelFormat_R8G8B8)
                ->Value("B8G8R8", EPixelFormat::ePixelFormat_B8G8R8)
                ->Value("R32", EPixelFormat::ePixelFormat_R32)
                ->Value("Unknown", EPixelFormat::ePixelFormat_Unknown)
                ;
        }
    }

    PresetSettings& PresetSettings::operator= (const PresetSettings& other)
    {
        DeepCopyMembers(other);
        return *this;
    }

    bool PresetSettings::operator==(const PresetSettings& other) const
    {
        bool arePointersEqual = true;

        ///////
        // MipMap Settings
        //////
        // If both pointers are allocated...
        if (m_mipmapSetting && other.m_mipmapSetting)
        {
            // If the allocated values are different...
            if (*m_mipmapSetting != *other.m_mipmapSetting)
            {
                arePointersEqual = false;
            }
        }
        // Otherwise, one or both pointers are un-allocated.
        // If only one pointer is allocated (via unequivalency)...
        else if (m_mipmapSetting != other.m_mipmapSetting)
        {
            arePointersEqual = false;
        }
        ///////
        // CubeMap Settings
        //////
        // If both pointers are allocated...
        if (m_cubemapSetting && other.m_cubemapSetting)
        {
            // If the allocated values are different...
            if (*m_cubemapSetting != *other.m_cubemapSetting)
            {
                arePointersEqual = false;
            }
        }
        // Otherwise, one or both pointers are un-allocated.
        // If only one pointer is allocated (via unequivalency)...
        else if (m_cubemapSetting != other.m_cubemapSetting)
        {
            arePointersEqual = false;
        }
        return
            arePointersEqual &&
            m_uuid == other.m_uuid &&
            m_name == other.m_name &&
            m_description == other.m_description &&
            m_generateIBLOnly == other.m_generateIBLOnly &&
            m_rgbWeight == other.m_rgbWeight &&
            m_srcColorSpace == other.m_srcColorSpace &&
            m_destColorSpace == other.m_destColorSpace &&
            m_fileMasks == other.m_fileMasks &&
            m_suppressEngineReduce == other.m_suppressEngineReduce &&
            m_pixelFormat == other.m_pixelFormat &&
            m_pixelFormatAlpha == other.m_pixelFormatAlpha &&
            m_discardAlpha == other.m_discardAlpha &&
            m_minTextureSize == other.m_minTextureSize &&
            m_maxTextureSize == other.m_maxTextureSize &&
            m_isPowerOf2 == other.m_isPowerOf2 &&
            m_sizeReduceLevel == other.m_sizeReduceLevel &&
            m_isColorChart == other.m_isColorChart &&
            m_highPassMip == other.m_highPassMip &&
            m_glossFromNormals == other.m_glossFromNormals &&
            m_isLegacyGloss == other.m_isLegacyGloss &&
            m_swizzle == other.m_swizzle &&
            m_isMipRenormalize == other.m_isMipRenormalize &&
            m_numResidentMips == other.m_numResidentMips;
    }

    void PresetSettings::DeepCopyMembers(const PresetSettings& other)
    {
        if (this != &other)
        {
            if (other.m_mipmapSetting)
            {
                m_mipmapSetting = AZStd::make_unique<MipmapSettings>(*other.m_mipmapSetting);
            }

            if (other.m_cubemapSetting)
            {
                m_cubemapSetting = AZStd::make_unique<CubemapSettings>(*other.m_cubemapSetting);
            }

            m_uuid = other.m_uuid;
            m_name = other.m_name;
            m_description = other.m_description;
            m_generateIBLOnly = other.m_generateIBLOnly;
            m_rgbWeight = other.m_rgbWeight;
            m_srcColorSpace = other.m_srcColorSpace;
            m_destColorSpace = other.m_destColorSpace;
            m_fileMasks = other.m_fileMasks;
            m_suppressEngineReduce = other.m_suppressEngineReduce;
            m_pixelFormat = other.m_pixelFormat;
            m_pixelFormatAlpha = other.m_pixelFormatAlpha;
            m_discardAlpha = other.m_discardAlpha;
            m_minTextureSize = other.m_minTextureSize;
            m_maxTextureSize = other.m_maxTextureSize;
            m_isPowerOf2 = other.m_isPowerOf2;
            m_sizeReduceLevel = other.m_sizeReduceLevel;
            m_isColorChart = other.m_isColorChart;
            m_highPassMip = other.m_highPassMip;
            m_glossFromNormals = other.m_glossFromNormals;
            m_isLegacyGloss = other.m_isLegacyGloss;
            m_swizzle = other.m_swizzle;
            m_isMipRenormalize = other.m_isMipRenormalize;
            m_numResidentMips = other.m_numResidentMips;
        }
    }

    AZ::Vector3 PresetSettings::GetColorWeight()
    {
        switch (m_rgbWeight)
        {
        case RGBWeight::uniform:
            return AZ::Vector3(0.3333f, 0.3334f, 0.3333f);
        case RGBWeight::ciexyz:
            return AZ::Vector3(0.2126f, 0.7152f, 0.0722f);
        case RGBWeight::luminance:
            return AZ::Vector3(0.3086f, 0.6094f, 0.0820f);
        default:
            AZ_Assert(false, "color weight value need to be added to new rgbWeight enum");
            return AZ::Vector3(0.3333f, 0.3334f, 0.3333f);
        }
    }

    void MultiplatformPresetSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MultiplatformPresetSettings>()
                ->Version(1)
                ->Field("DefaultPreset", &MultiplatformPresetSettings::m_defaultPreset)
                ->Field("PlatformsPresets", &MultiplatformPresetSettings::m_presets)
                ;
        }
    }
    
    const PresetSettings* MultiplatformPresetSettings::GetPreset(const PlatformName& platform) const
    {
        auto itr = m_presets.find(platform);
        if (itr != m_presets.end())
        {
            return &itr->second;
        }
        return &m_defaultPreset;
    }
        
    const PresetSettings& MultiplatformPresetSettings::GetDefaultPreset() const
    {
        return m_defaultPreset;
    }
    
    void MultiplatformPresetSettings::ClearPlatformPresets()
    {
        m_presets.clear();
    }

    void MultiplatformPresetSettings::SetDefaultPreset(const PresetSettings& preset)
    {
        m_defaultPreset = preset;
    }
    
    void MultiplatformPresetSettings::SetPresetForPlatform(const PresetSettings& preset, const PlatformName& platform)
    {
        AZ_Assert(!platform.empty(), "Platform string shouldn't be empty");
        if (!platform.empty())
        {
            m_presets[platform] = preset;
        }
    }
    
    void MultiplatformPresetSettings::SetPresetName(const PresetName& name)
    {
        m_defaultPreset.m_name = name;
    }

    const PresetName& MultiplatformPresetSettings::GetPresetName() const
    {
        return m_defaultPreset.m_name;
    }

    AZ::Uuid MultiplatformPresetSettings::GetPresetId() const
    {
        return m_defaultPreset.m_uuid;
    }

} // namespace ImageProcessingAtom
