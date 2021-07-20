/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once


#include <Atom_Feature_Traits_Platform.h>
#if AZ_TRAIT_LUXCORE_SUPPORTED

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <Atom/Feature/LuxCore/LuxCoreBus.h>
#include <luxcore/luxcore.h>

namespace AZ
{
    namespace Render
    {
        // Build a pipeline to get raw data from runtime texture
        class LuxCoreTexture final
        {
        public:
            LuxCoreTexture() = default;
            LuxCoreTexture( AZ::Data::Instance<AZ::RPI::Image> image, LuxCoreTextureType type);
            LuxCoreTexture(const LuxCoreTexture &texture);
            ~LuxCoreTexture();

            void* GetRawDataPointer();

            unsigned int GetTextureWidth();
            unsigned int GetTextureHeight();
            unsigned int GetTextureChannels();

            void Init(AZ::Data::Instance<AZ::RPI::Image> image, LuxCoreTextureType type);

            void AddRenderTargetPipeline();
            luxrays::Properties GetLuxCoreTextureProperties();
            bool IsIBLTexture();
            bool IsTextureReady();

        private:
            AZStd::string m_luxCoreTextureName;
            luxrays::Properties m_luxCoreTexture;
            
            AZ::RPI::RenderPipelinePtr m_rtPipeline = nullptr;

            AZStd::vector<uint8_t> m_textureData;
            AZ::Data::Instance<AZ::RPI::Image> m_texture;
            unsigned int m_textureChannels = 4;

            Data::AssetId m_textureAssetId;
            LuxCoreTextureType m_type = LuxCoreTextureType::Default;

            bool m_textureReadbackComplete = false;
        };
    }
}
#endif
