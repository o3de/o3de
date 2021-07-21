/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/AssetCreator.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <AzCore/std/containers/map.h>

namespace AZ
{
    namespace RPI
    {
        class ShaderAssetCreator
            : public AssetCreator<ShaderAsset>
        {
        public:
            //! Begins creation of a shader asset.
            void Begin(const Data::AssetId& assetId);

            //! [Optional] Set the timestamp for when the ShaderAsset build process began.
            //! This is needed to synchronize between the ShaderAsset and ShaderVariantTreeAsset when hot-reloading shaders.
            void SetShaderAssetBuildTimestamp(AZStd::sys_time_t shaderAssetBuildTimestamp);

            //! [Optional] Sets the name of the shader asset from content.
            void SetName(const Name& name);
            
            //! [Optional] Sets the DrawListTag name associated with this shader.
            void SetDrawListName(const Name& name);

            //! [Optional] Adds a shader resource group asset. The order of shader resource group assets must match the same
            //! order supplied to the pipeline layout descriptor. A mismatch will cause End() to fail. This method
            //! requires that the asset be loaded in order to validate contents properly.
            void AddShaderResourceGroupAsset(const Data::Asset<ShaderResourceGroupAsset>& shaderResourceGroupAsset);

            //! [Required] Assigns the layout used to construct and parse shader options packed into shader variant keys.
            //! Requires that the keys assigned to shader variants were constructed using the same layout.
            void SetShaderOptionGroupLayout(const Ptr<ShaderOptionGroupLayout>& shaderOptionGroupLayout);

            //! Begins the shader creation for a specific RHI API.
            //! Begin must be called before the BeginAPI function is called.
            //! @param type The target RHI API type.
            void BeginAPI(RHI::APIType type);

            //! [Required] There's always a root variant for each RHI::APIType.
            void SetRootShaderVariantAsset(Data::Asset<ShaderVariantAsset> shaderVariantAsset);

            //! [Optional] Not all shaders have attributes before functions. Some attributes do not exist for all RHI::APIType either.
            void SetShaderStageAttributeMapList(const RHI::ShaderStageAttributeMapList& shaderStageAttributeMapList);

            //! [Required] Assigns the pipeline layout descriptor shared by all variants in the shader. Shader variants
            //! embedded in a single shader asset are required to use the same pipeline layout. It is not necessary to call
            //! Finalize() on the pipeline layout prior to assignment, but still permitted.
            void SetPipelineLayout(const Ptr<RHI::PipelineLayoutDescriptor>& pipelineLayoutDescriptor);

            bool EndAPI();

            bool End(Data::Asset<ShaderAsset>& shaderAsset);

            //! Clones an existing ShaderAsset and replaces the referenced Srg and Variant assets.
            using ShaderResourceGroupAssets = AZStd::vector<AZ::Data::Asset<ShaderResourceGroupAsset>>;
            using ShaderRootVariantAssets = AZStd::vector<AZStd::pair<AZ::Crc32, Data::Asset<RPI::ShaderVariantAsset>>>;
            void Clone(const Data::AssetId& assetId,
                       const ShaderAsset* sourceShaderAsset,
                       const ShaderResourceGroupAssets& srgAssets,
                       const ShaderRootVariantAssets& rootVariantAssets);

        private:

            // Shader variants will use this draw list when they don't specify one
            Name m_defaultDrawList;
        };
    } // namespace RPI
} // namespace AZ
