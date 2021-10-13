/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Asset/AssetCommon.h>

#include <BuilderSettings/ImageProcessingDefines.h>
#include <BuilderSettings/CubemapSettings.h>
#include <BuilderSettings/MipmapSettings.h>
#include <Atom/ImageProcessing/PixelFormats.h>

namespace ImageProcessingAtom
{
    //settings for texture process preset
    class PresetSettings
    {
    public:
        AZ_TYPE_INFO(PresetSettings, "{4F4DEC5C-48DD-40FD-97B4-5FB6FC7242E9}");
        AZ_CLASS_ALLOCATOR(PresetSettings, AZ::SystemAllocator, 0);

        PresetSettings();
        PresetSettings(const PresetSettings& other);
        PresetSettings& operator= (const PresetSettings& other);
        bool operator== (const PresetSettings& other) const;
        static void Reflect(AZ::ReflectContext* context);
        
        // unique id for the preset
        // this uuid will be deprecated. The preset name will be used as an unique id for the preset
        AZ::Uuid m_uuid = 0;

        PresetName m_name;

        //a brief description for the usage of this Preset
        AZStd::string m_description;

        // Controls whether this preset only invokes IBL presets and does not generate its own output product
        bool m_generateIBLOnly = false;

        //misc options
        // "rgbweights". specify preset for weighting of R,G,B channels (used by compressor)
        RGBWeight m_rgbWeight = RGBWeight::uniform;
        ColorSpace m_srcColorSpace = ColorSpace::sRGB;
        ColorSpace m_destColorSpace = ColorSpace::autoSelect;

        // file masks used for helping select default preset and option preset list in texture property dialog
        AZStd::vector<FileMask> m_fileMasks;

        // "ser". Whether to enable suppress reduce resolution (m_sizeReduceLevel) during loading, 0(default)
        bool m_suppressEngineReduce = false;

        //pixel format
        EPixelFormat m_pixelFormat = EPixelFormat::ePixelFormat_R8G8B8A8;
        //pixel format for image which only contains alpha channel. this is for if we need to save alpha channel into a separate image
        EPixelFormat m_pixelFormatAlpha = EPixelFormat::ePixelFormat_A8;
        bool m_discardAlpha = false;

        // Resolution related settings

        // "maxtexturesize", upper limit of the resolution of generated textures. It should be a power-of-2 number larger than 1
        //  resulting texture will be downscaled if its width or height larger than this value
        // 0 - no upper resolution limit (default)
        AZ::u32 m_maxTextureSize = 0;

        // "mintexturesize", lower limit of the resolution of generated textures.It should be a power-of-2 number larger than 1
        // resulting texture will be upscaled if its width or height smaller than this value
        // 0 - no lower resolution limit (default)
        AZ::u32 m_minTextureSize = 0;

        bool m_isPowerOf2 = false;

        //"reduce",  0=no size reduce /1=half resolution /2=quarter resolution, etc"
        AZ::u32 m_sizeReduceLevel = 0;

        //settings for cubemap generation. it's null if this preset is not for cubemap.
        //"cm" equals 1 to enable cubemap in rc.ini
        AZStd::unique_ptr<CubemapSettings> m_cubemapSetting;

        //settings for mipmap generation. it's null if this preset disable mipmap.
        AZStd::unique_ptr<MipmapSettings> m_mipmapSetting;

        //some specific settings
        // "colorchart". This is to indicate if need to extract color chart from the image and output the color chart data.
        // This is very specific usage for cryEngine. Check ColorChart.cpp for better explanation.
        bool m_isColorChart = false;

        //"highpass". Defines which mip level is subtracted when applying the high pass filter
        //this is only used for terrain asset. we might remove it later since it can be done with source image directly
        AZ::u32 m_highPassMip = 0;

        //"glossfromnormals". Bake normal variance into smoothness stored in alpha channel
        AZ::u32 m_glossFromNormals = 0;

        //"mipnormalize". need normalize the rgb
        bool m_isMipRenormalize = false;

        //function to get color's rgb weight in vec3 based on m_rgbWeight enum
        //this is useful for squisher compression
        AZ::Vector3 GetColorWeight();

        //the number of resident mips within the StreamingImageAsset's tail mip chain. When the m_numResidentMips is
        //set to 0, the StreamingImageAsset will contain as many mips as possible (starting from the lowest resolution)
        //that add up to 64K or lower
        AZ::u8 m_numResidentMips = 0;

        //legacy options might be removed later
        //"glosslegacydist". If the gloss map use legacy distribution. NW is still using legacy dist
        bool m_isLegacyGloss = false;

        //"swizzle". need to be 4 character and each character need to be one of "rgba01"
        AZStd::string m_swizzle;

    protected:
        void DeepCopyMembers(const PresetSettings& other);
    };
    
    class MultiplatformPresetSettings
    {
    public:
        AZ_TYPE_INFO(MultiplatformPresetSettings, "{05603AB1-FFC2-48F2-8322-BD265D6FB321}");
        AZ_CLASS_ALLOCATOR(MultiplatformPresetSettings, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        const PresetSettings* GetPreset(const PlatformName& platform) const;
        const PresetSettings& GetDefaultPreset() const;

        //! Clear the preset data for each platform
        void ClearPlatformPresets();

        // functions to set the presets.
        void SetDefaultPreset(const PresetSettings& preset);
        void SetPresetForPlatform(const PresetSettings& preset, const PlatformName& platform);

        void SetPresetName(const PresetName& name);

        const PresetName& GetPresetName() const;
        AZ::Uuid GetPresetId() const;

    private:
        PresetSettings m_defaultPreset;
        AZStd::unordered_map<PlatformName, PresetSettings> m_presets;
    };
    
} // namespace ImageProcessingAtom
