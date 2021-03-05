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

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <BuilderSettings/ImageProcessingDefines.h>
#include <BuilderSettings/CubemapSettings.h>
#include <BuilderSettings/MipmapSettings.h>
#include <ImageProcessing/PixelFormats.h>

namespace ImageProcessing
{
    //settings for texture process preset
    class PresetSettings
    {
    public:
        AZ_TYPE_INFO(PresetSettings, "{935BCE3F-9E76-494E-9408-47C5937D7288}");
        AZ_CLASS_ALLOCATOR(PresetSettings, AZ::SystemAllocator, 0);

        PresetSettings();
        PresetSettings(const PresetSettings& other);
        PresetSettings& operator= (const PresetSettings& other);
        bool operator== (const PresetSettings& other) const;
        static void Reflect(AZ::ReflectContext* context);

        //unique id for the preset
        AZ::Uuid m_uuid;

        PresetName m_name;

        //a brief description for the usage of this Preset 
        AZStd::string m_description;

        //misc options
        // "rgbweights". specify preset for weighting of R,G,B channels (used by compressor)
        RGBWeight m_rgbWeight;
        ColorSpace m_srcColorSpace;
        ColorSpace m_destColorSpace;

        // file masks used for helping select default preset and option preset list in texture property dialog
        AZStd::vector<FileMask> m_fileMasks;

        // "ser". Whether to enable supress reduce resolution (m_sizeReduceLevel) during loading, 0(default)
        bool m_suppressEngineReduce;

        //pixel format
        EPixelFormat m_pixelFormat;
        AZStd::string m_pixelFormatName;
        //pixel format for image which only contains alpha channel. this is for if we need to save alpha channel into a seperate image
        EPixelFormat m_pixelFormatAlpha;
        AZStd::string m_pixelFormatAlphaName;
        bool m_discardAlpha;

        // Resolution related settings

        // "maxtexturesize", upper limit of the resolution of generated textures. It should be a power-of-2 number larger than 1
        //  resulting texture will be downscaled if its width or height larger than this value
        // 0 - no upper resolution limit (default)
        unsigned int m_maxTextureSize;

        // "mintexturesize", lower limit of the resolution of generated textures.It should be a power-of-2 number larger than 1
        // resulting texture will be upscaled if its width or height smaller than this value
        // 0 - no lower resolution limit (default)
        unsigned int m_minTextureSize;

        bool m_isPowerOf2;

        //"reduce",  0=no size reduce /1=half resolution /2=quarter resolution, etc"
        unsigned int m_sizeReduceLevel;

        //settings for cubemap generation. it's null if this preset is not for cubemap.
        //"cm" equals 1 to enable cubemap in rc.ini
        AZStd::unique_ptr<CubemapSettings> m_cubemapSetting;

        //settings for mipmap generation. it's null if this preset disable mipmap.
        AZStd::unique_ptr<MipmapSettings> m_mipmapSetting;

        //some specific settings
        // "colorchart". This is to indicate if need to extract color chart from the image and output the color chart data. 
        // This is very specific usage for cryEngine. Check ColorChart.cpp for better explaination.
        bool m_isColorChart;

        //"highpass". Defines which mip level is subtracted when applying the high pass filter 
        //this is only used for terrain asset. we might remove it later since it can be done with source image directly
        AZ::u32 m_highPassMip;

        //"glossfromnormals". Bake normal variance into smoothness stored in alpha channel 
        AZ::u32 m_glossFromNormals;
        
        //"mipnormalize". need normalize the rgb 
        bool m_isMipRenormalize;

        //function to get color's rgb weight in vec3 based on m_rgbWeight enum
        //this is useful for squisher compression
        AZ::Vector3 GetColorWeight();

        //numstreamablemips
        AZ::u32 m_numStreamableMips;

        //legacy options might be removed later
        //"glosslegacydist". If the gloss map use legacy distribution. NW is still using legacy dist
        bool m_isLegacyGloss;

        //"swizzle". need to be 4 character and each character need to be one of "rgba01"
        AZStd::string m_swizzle;

    private:
        void DeepCopyMembers(const PresetSettings& other);
    };
} // namespace ImageProcessing
