/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>

#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        //! This class provides common code to use shader option.
        class PostProcessingShaderOptionBase
        {
        public:
            PostProcessingShaderOptionBase() = default;
            ~PostProcessingShaderOptionBase() = default;

        protected:
            //! Creates the PSO for ShaderOption and save it as a cache.
            void PreloadShaderVariant(
                const Data::Instance<AZ::RPI::Shader>& shader,
                const RPI::ShaderOptionGroup& shaderOption,
                const RHI::RenderAttachmentConfiguration& renderAttachmentConfiguration,
                const RHI::MultisampleState& multisampleState);

            //! Update shaderVariant. Used when the shader is switched.
            void UpdateShaderVariant(const AZ::RPI::ShaderOptionGroup& shaderOption);

            //! Set a key in the SRG for dynamic branching for shaders not created in advance.
            void CompileShaderVariant(Data::Instance<AZ::RPI::ShaderResourceGroup>& shaderResourceGroup);

            //! Get precomputed pipeline.
            const AZ::RHI::SingleDevicePipelineState* GetPipelineStateFromShaderVariant() const;

        private:
            struct ShaderVariantInformation
            {
                bool m_isFullyBaked = false;
                const AZ::RHI::SingleDevicePipelineState* m_pipelineState = nullptr;
            };
            AZStd::unordered_map<AZ::u64, ShaderVariantInformation> m_shaderVariantTable;
            AZ::u64 m_currentShaderVariantKeyValue = 0;
            AZ::RPI::ShaderVariantKey m_currentShaderVariantKeyFallbackValue;

            const ShaderVariantInformation* GetShaderVariant(AZ::u64 key) const;
        };
    }   // namespace Render
}   // namespace AZ
