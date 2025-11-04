/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/AssetCreator.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <AzCore/std/containers/map.h>

namespace AZ
{
    namespace RHI
    {
        class ShaderPlatformInterface;
    }

    namespace RPI
    {
        class ATOM_RPI_REFLECT_API ShaderAssetCreator
            : public AssetCreator<ShaderAsset>
        {
        public:
            //! Begins creation of a shader asset.
            void Begin(const Data::AssetId& assetId);

            //! [Optional] Sets the name of the shader asset from content.
            void SetName(const Name& name);
            
            //! [Optional] Sets the DrawListTag name associated with this shader.
            void SetDrawListName(const Name& name);

            //! [Required] Assigns the layout used to construct and parse shader options packed into shader variant keys.
            //! Requires that the keys assigned to shader variants were constructed using the same layout.
            void SetShaderOptionGroupLayout(const Ptr<ShaderOptionGroupLayout>& shaderOptionGroupLayout);

            //! [Optional] Sets the default value for one shader option, overriding any default that was specified in the shader code.
            void SetShaderOptionDefaultValue(const Name& optionName, const Name& optionValue);

            //! Begins the shader creation for a specific RHI API.
            //! Begin must be called before the BeginAPI function is called.
            //! @param type The target RHI API type.
            void BeginAPI(RHI::APIType type);

            //! Begins the creation of a Supervariant for the current RHI::APIType.
            //! If this is the first supervariant its name must be empty. The first
            //! supervariant is always the default, nameless, supervariant.
            void BeginSupervariant(const Name& name);

            void SetSrgLayoutList(const ShaderResourceGroupLayoutList& srgLayoutList);

            //! [Required] Assigns the pipeline layout descriptor shared by all variants in the shader. Shader variants
            //! embedded in a single shader asset are required to use the same pipeline layout. It is not necessary to call
            //! Finalize() on the pipeline layout prior to assignment, but still permitted.
            void SetPipelineLayout(RHI::Ptr<RHI::PipelineLayoutDescriptor> m_pipelineLayoutDescriptor);

            //! Assigns the contract for inputs required by the shader.
            void SetInputContract(const ShaderInputContract& contract);

            //! Assigns the contract for outputs required by the shader.
            void SetOutputContract(const ShaderOutputContract& contract);

            //! Assigns the render states for the draw pipeline. Ignored for non-draw pipelines.
            void SetRenderStates(const RHI::RenderStates& renderStates);

            //! [Optional] Not all shaders have attributes before functions. Some attributes do not exist for all RHI::APIType either.
            void SetShaderStageAttributeMapList(const RHI::ShaderStageAttributeMapList& shaderStageAttributeMapList);

            //! [Required] There's always a root variant for each supervariant.
            void SetRootShaderVariantAsset(Data::Asset<ShaderVariantAsset> shaderVariantAsset);

            //! Set if the supervariant uses specialization constants for shader options.
            void SetUseSpecializationConstants(bool value);

            bool EndSupervariant();

            bool EndAPI();

            bool End(Data::Asset<ShaderAsset>& shaderAsset);

            //! Clones an existing ShaderAsset and replaces the ShaderVariant assets
            using ShaderRootVariantAssetPair = AZStd::pair<AZ::Crc32, Data::Asset<RPI::ShaderVariantAsset>>;
            using ShaderRootVariantAssets = AZStd::vector<ShaderRootVariantAssetPair>;

            struct ShaderSupervariant
            {
                AZ::Name m_name;
                ShaderRootVariantAssets m_rootVariantAssets;
            };
            using ShaderSupervariants = AZStd::vector<ShaderSupervariant>;

            void Clone(const Data::AssetId& assetId,
                       const ShaderAsset& sourceShaderAsset,
                       const ShaderSupervariants& supervariants,
                       const AZStd::vector<RHI::ShaderPlatformInterface*>& platformInterfaces);

        private:

            // Shader variants will use this draw list when they don't specify one.
            Name m_defaultDrawList;

            // The current supervariant is cached here to facilitate asset
            // construction. Additionally, prevents BeginSupervariant to be called more than once before calling EndSupervariant.
            ShaderAsset::Supervariant* m_currentSupervariant = nullptr;

            ShaderOptionGroup m_defaultShaderOptionGroup;
        };
    } // namespace RPI
} // namespace AZ
