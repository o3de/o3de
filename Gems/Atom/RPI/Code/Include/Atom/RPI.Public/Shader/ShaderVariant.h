/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Atom/RHI/DrawListTagRegistry.h>

namespace AZ
{
    namespace RPI
    {
        //! Represents the concrete state to configure a PipelineStateDescriptor. ShaderVariant's match
        //! the RHI::PipelineStateType of the parent Shader instance. For shaders on the raster
        //! pipeline, the RHI::DrawFilterTag is also provided.
        class ShaderVariant final
            : public Data::AssetBus::MultiHandler
        {
            friend class Shader;
        public:
            ShaderVariant() = default;
            virtual ~ShaderVariant();
            AZ_DEFAULT_COPY_MOVE(ShaderVariant);

            //! Fills a pipeline state descriptor with settings provided by the ShaderVariant. (Note that
            //! this does not fill the InputStreamLayout or OutputAttachmentLayout as that also requires 
            //! information from the mesh data and pass system and must be done as a separate step).
            void ConfigurePipelineState(RHI::PipelineStateDescriptor& descriptor) const;
            
            //! Returns the ShaderInputContract which describes which inputs the shader requires
            const ShaderInputContract& GetInputContract() const;

            //! Returns the ShaderOutputContract which describes which outputs the shader requires
            const ShaderOutputContract& GetOutputContract() const;

            const ShaderVariantId& GetShaderVariantId() const { return m_shaderVariantAsset->GetShaderVariantId(); }

            //! Returns whether the variant is fully baked variant (all options are static branches), or false if the
            //! variant uses dynamic branches for some shader options.
            //! If the shader variant is not fully baked, the ShaderVariantKeyFallbackValue must be correctly set when drawing.
            bool IsFullyBaked() const { return m_shaderVariantAsset->IsFullyBaked(); }

            //! Return the timestamp when the associated ShaderAsset was built.
            //! This is used to synchronize versions of the ShaderAsset and ShaderVariantAsset, especially during hot-reload.
            AZStd::sys_time_t GetShaderAssetBuildTimestamp() const { return m_shaderVariantAsset->GetShaderAssetBuildTimestamp(); }

            bool IsRootVariant() const { return m_shaderVariantAsset->IsRootVariant(); }

            ShaderVariantStableId GetStableId() const { return m_shaderVariantAsset->GetStableId(); }

            const Data::Asset<ShaderAsset>& GetShaderAsset() const { return m_shaderAsset; }
            const Data::Asset<ShaderVariantAsset>& GetShaderVariantAsset() const { return m_shaderVariantAsset; }

        private:
            // Called by Shader. Initializes runtime data from asset data. Returns whether the call succeeded.
            bool Init(
                const Data::Asset<ShaderAsset>& shaderAsset,
                const Data::Asset<ShaderVariantAsset>& shaderVariantAsset);

            // AssetBus overrides...
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;

            //! A reference to the shader asset that this is a variant of.
            Data::Asset<ShaderAsset> m_shaderAsset;

            // Cached state from the asset to avoid an indirection.
            RHI::PipelineStateType m_pipelineStateType = RHI::PipelineStateType::Count;

            // State assigned to the pipeline state descriptor.
            RHI::ConstPtr<RHI::PipelineLayoutDescriptor> m_pipelineLayoutDescriptor;
            
            Data::Asset<ShaderVariantAsset> m_shaderVariantAsset;
        };
    }
}
