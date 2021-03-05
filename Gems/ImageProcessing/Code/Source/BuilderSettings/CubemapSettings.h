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
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/Memory.h>
#include <BuilderSettings/ImageProcessingDefines.h>

namespace ImageProcessing
{
    //! settings related to cubemap. Part of texture preset setting. only useful when cubemap enabled
    struct CubemapSettings
    {
        AZ_TYPE_INFO(CubemapSettings, "{C6BDEB7B-8E05-4B2D-8F39-8F6275BC84E8}");
        AZ_CLASS_ALLOCATOR(CubemapSettings, AZ::SystemAllocator, 0);
        bool operator!=(const CubemapSettings& other);
        bool operator==(const CubemapSettings& other);
        static void Reflect(AZ::ReflectContext* context);

        // "cm_ftype", cubemap angular filter type: gaussian, cone, disc, cosine, cosine_power, ggx
        CubemapFilterType m_filter;
        
        // "cm_fangle", base filter angle for cubemap filtering(degrees), 0 - disabled
        float m_angle;

        // "cm_fmipangle", initial mip filter angle for cubemap filtering(degrees), 0 - disabled
        float m_mipAngle;

        // "cm_fmipslope", mip filter angle multiplier for cubemap filtering, 1 - default"
        float m_mipSlope;

        // "cm_edgefixup", cubemap edge fix-up width, 0 - disabled
        float m_edgeFixup;

        // "cm_diff", generate a diffuse illumination light-probe in addition
        bool m_generateDiff;

        // "cm_diffpreset", the name of the preset to be used for the diffuse probe
        AZ::Uuid m_diffuseGenPreset;
    };
} // namespace ImageProcessing