/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/AssetCreator.h>
#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>
#include <AtomCore/std/containers/array_view.h>

namespace AZ
{
    namespace RPI
    {
        class ShaderResourceGroup;

        //! Use a ShaderResourceGroupAssetCreator to create and configure a new ShaderResourceGroupAsset.
        //! Can create a ShaderResourceGroupAsset for multiple RHI APIs.
        class ShaderResourceGroupAssetCreator
            : public AssetCreator<ShaderResourceGroupAsset>
        {
        public:
            //! Begins construction of the shader resource group asset.
            //! @param shaderResourceGroupName The friendly name used to identify the SRG at runtime, unique within the parent shader.
            void Begin(const AZ::Data::AssetId& assetId, const Name& shaderResourceGroupName);

            //! Begins the shader resource group layout creation for a specific RHI API.
            //! Begin must be called before the BeginAPI function is called.
            //! @param type The target RHI API type.
            void BeginAPI(RHI::APIType type);

            //! Assigns the binding slot used by all shader resource groups which use this asset.
            void SetBindingSlot(uint32_t bindingSlot);

            //! Designates this SRG as ShaderVariantKey fallback
            void SetShaderVariantKeyFallback(const Name& shaderInputName, uint32_t bitSize);

            //! Adds a static sampler to the shader resource group. Static samplers cannot be changed at runtime.
            void AddStaticSampler(const RHI::ShaderInputStaticSamplerDescriptor& shaderInputStaticSampler);

            //! Adds a shader input to the ShaderResourceGroupLayout.
            void AddShaderInput(const RHI::ShaderInputBufferDescriptor& shaderInputBuffer);
            void AddShaderInput(const RHI::ShaderInputImageDescriptor& shaderInputImage);
            void AddShaderInput(const RHI::ShaderInputBufferUnboundedArrayDescriptor& shaderInputBufferUnboundedArray);
            void AddShaderInput(const RHI::ShaderInputImageUnboundedArrayDescriptor& shaderInputImageUnboundedArray);
            void AddShaderInput(const RHI::ShaderInputSamplerDescriptor& shaderInputSampler);
            void AddShaderInput(const RHI::ShaderInputConstantDescriptor& shaderInputConstant);

            //! Finalizes and assigns ownership of the asset to result, if successful. 
            //! Otherwise false is returned and result is left untouched.
            bool End(Data::Asset<ShaderResourceGroupAsset>& result);

            bool EndAPI();

        private:
            //! Release all temporary resources when building ends
            void Cleanup();

            RHI::APIType m_currentAPIType;

            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_shaderResourceGroupLayout;
        };
    } // namespace RPI
} // namespace AZ
