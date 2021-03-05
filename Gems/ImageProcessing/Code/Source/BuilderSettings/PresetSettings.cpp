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

#include "ImageProcessing_precompiled.h"
#include <BuilderSettings/PresetSettings.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace ImageProcessing
{
    PresetSettings::PresetSettings()
        : m_uuid(0)
        , m_rgbWeight(RGBWeight::uniform)
        , m_srcColorSpace(ColorSpace::sRGB)
        , m_destColorSpace(ColorSpace::autoSelect)
        , m_suppressEngineReduce(false)
        , m_pixelFormat(ePixelFormat_R8G8B8A8)
        , m_pixelFormatName("R8G8B8A8")
        , m_pixelFormatAlpha(ePixelFormat_Unknown)
        , m_pixelFormatAlphaName("")
        , m_discardAlpha(false)
        , m_maxTextureSize(0)
        , m_minTextureSize(0)
        , m_isPowerOf2(false)
        , m_sizeReduceLevel(0)
        , m_isColorChart(0)
        , m_highPassMip(0)
        , m_glossFromNormals(false)
        , m_isMipRenormalize(false)
        , m_numStreamableMips(100)
        , m_isLegacyGloss(false)
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
                ->Version(1)
                ->Field("UUID", &PresetSettings::m_uuid)
                ->Field("Name", &PresetSettings::m_name)
                ->Field("Description", &PresetSettings::m_description)
                ->Field("RGB_Weight", &PresetSettings::m_rgbWeight)
                ->Field("SourceColor", &PresetSettings::m_srcColorSpace)
                ->Field("DestColor", &PresetSettings::m_destColorSpace)
                ->Field("FileMasks", &PresetSettings::m_fileMasks)
                ->Field("SuppressEngineReduce", &PresetSettings::m_suppressEngineReduce)
                ->Field("PixelFormat", &PresetSettings::m_pixelFormatName)
                ->Field("PixelFormatAlpha", &PresetSettings::m_pixelFormatAlphaName)
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
                ->Field("NumberStreamableMips", &PresetSettings::m_numStreamableMips)
                ->Field("Swizzle", &PresetSettings::m_swizzle)
                ->Field("CubemapSettings", &PresetSettings::m_cubemapSetting)
                ->Field("MipMapSetting", &PresetSettings::m_mipmapSetting);
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
            m_rgbWeight == other.m_rgbWeight &&
            m_srcColorSpace == other.m_srcColorSpace &&
            m_destColorSpace == other.m_destColorSpace &&
            m_fileMasks == other.m_fileMasks &&
            m_suppressEngineReduce == other.m_suppressEngineReduce &&
            m_pixelFormat == other.m_pixelFormat &&
            m_pixelFormatName == other.m_pixelFormatName &&
            m_pixelFormatAlpha == other.m_pixelFormatAlpha &&
            m_pixelFormatAlphaName == other.m_pixelFormatAlphaName &&
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
            m_numStreamableMips == other.m_numStreamableMips;
    }

    void PresetSettings::DeepCopyMembers(const PresetSettings & other)
    {
        if (this != &other)
        {
            if(other.m_mipmapSetting)
            {
                m_mipmapSetting = AZStd::make_unique<MipmapSettings>(*other.m_mipmapSetting);
            }

            if(other.m_cubemapSetting)
            {
                m_cubemapSetting = AZStd::make_unique<CubemapSettings>(*other.m_cubemapSetting);
            }

            m_uuid = other.m_uuid;
            m_name = other.m_name;
            m_description = other.m_description;
            m_rgbWeight = other.m_rgbWeight;
            m_srcColorSpace = other.m_srcColorSpace;
            m_destColorSpace = other.m_destColorSpace;
            m_fileMasks = other.m_fileMasks;
            m_suppressEngineReduce = other.m_suppressEngineReduce;
            m_pixelFormat = other.m_pixelFormat;
            m_pixelFormatAlpha = other.m_pixelFormatAlpha;
            m_pixelFormatName = other.m_pixelFormatName;
            m_pixelFormatAlphaName = other.m_pixelFormatAlphaName;
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
            m_numStreamableMips = other.m_numStreamableMips;
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
} // namespace ImageProcessing
