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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzRTT
{
    typedef AZ::Uuid RenderContextId;

    // various post screen effects will fail if we attempt to render the scene to very
    // small render target sizes so provide a reasonable minimum (tile/icon size)
    constexpr uint32_t MinRenderTargetWidth = 32;
    constexpr uint32_t MinRenderTargetHeight = 32;

    // this maximum recommended texture size applies to width and height
    // using sizes larger than this can lead to performance issues and instability
    constexpr uint32_t MaxRecommendedRenderTargetSize = 2048;

    enum class AlphaMode {
        ALPHA_DISABLED = 0,
        ALPHA_OPAQUE,
        ALPHA_DEPTH_BASED
    };

    // RenderContextConfig stores the render settings to use when rendering to texture.
    // It also provides a more developer-friendly interface to deal with by exposing
    // the most commonly used properties in one place. 
    struct RenderContextConfig
    {
        AZ_CLASS_ALLOCATOR(RenderContextConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(RenderContextConfig, "{6114F930-CBE4-4373-AF9D-3B5319471C8F}");
        virtual ~RenderContextConfig() = default;
        static void Reflect(AZ::ReflectContext* context);

        //! render target width
        uint32_t m_width = 256;

        //! render target height
        uint32_t m_height = 256;

        //! write srgb or linear output
        bool m_sRGBWrite = false;

        //! alpha mode to use for the render target 
        AlphaMode m_alphaMode = AlphaMode::ALPHA_OPAQUE;

        //! scene settings 
        bool m_oceanEnabled = true;
        bool m_terrainEnabled = true;
        bool m_vegetationEnabled = true;

        //! shadow settings
        bool m_shadowsEnabled = true;
        int32_t m_shadowsNumCascades = -1;
        float m_shadowsGSMRange = -1.f;
        float m_shadowsGSMRangeStep = -1.f;

        //! post-effects settings
        bool m_depthOfFieldEnabled = false;
        bool m_motionBlurEnabled = false;
        int m_aaMode = 0;

        //! visiblity for shadow settings
        AZ::Crc32 GetShadowSettingsVisible();

        //! confirm if user wants to use texture size larger than MaxRecommendedRenderTargetSize
        bool ValidateTextureSize(void* newValue, const AZ::Uuid& valueType);
    };
}