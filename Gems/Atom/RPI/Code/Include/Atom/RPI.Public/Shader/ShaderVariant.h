/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RHI/DrawListTagRegistry.h>

namespace AZ
{
    namespace RPI
    {
        //! Represents the concrete state to configure a PipelineStateDescriptor. ShaderVariant's match
        //! the RHI::PipelineStateType of the parent Shader instance. For shaders on the raster
        //! pipeline, the RHI::DrawFilterTag is also provided.
        class ATOM_RPI_PUBLIC_API ShaderVariant final
        {
            friend class Shader;
        public:
            ShaderVariant() = default;
            virtual ~ShaderVariant();
            AZ_DEFAULT_COPY_MOVE(ShaderVariant);

            //! Fills a pipeline state descriptor with settings provided by the ShaderVariant. 
            //! It also configures the specialization constants if they are being used by the shader variant.
            //! (Note that this does not fill the InputStreamLayout or OutputAttachmentLayout as that also requires 
            //! information from the mesh data and pass system and must be done as a separate step).
            void ConfigurePipelineState(RHI::PipelineStateDescriptor& descriptor, const ShaderVariantId& specialization) const;
            //! Fills a pipeline state descriptor with settings provided by the ShaderVariant.
            //! It also configures the specialization constants if they are being used by the shader variant.
            void ConfigurePipelineState(RHI::PipelineStateDescriptor& descriptor, const ShaderOptionGroup& specialization) const;
            //! Fills a pipeline state descriptor with settings provided by the ShaderVariant.
            //! Only use this function if the shader variant is not using ANY specialization constant. Otherwise
            //! an error will be raised and the default values will be used.
            void ConfigurePipelineState(RHI::PipelineStateDescriptor& descriptor) const;

            const ShaderVariantId& GetShaderVariantId() const { return m_shaderVariantAsset->GetShaderVariantId(); }

            //! Returns whether the variant is fully baked variant (all options are static branches), or false if the
            //! variant uses dynamic branches for some shader options.
            //! If the shader variant is not fully baked, the ShaderVariantKeyFallbackValue must be correctly set when drawing.
            bool IsFullyBaked() const { return m_shaderVariantAsset->IsFullyBaked(); }

            //! Returns whether the variant is using specialization constants for all of the options.
            bool IsFullySpecialized() const;

            //! Return true if this shader variant has at least one shader option using specialization constant.
            bool UseSpecializationConstants() const;

            //! Return true if this variant needs the ShaderVariantKeyFallbackValue to be correctly set when drawing.
            bool UseKeyFallback() const;

            bool IsRootVariant() const { return m_shaderVariantAsset->IsRootVariant(); }

            ShaderVariantStableId GetStableId() const { return m_shaderVariantAsset->GetStableId(); }

            const Data::Asset<ShaderAsset>& GetShaderAsset() const { return m_shaderAsset; }
            const Data::Asset<ShaderVariantAsset>& GetShaderVariantAsset() const { return m_shaderVariantAsset; }

        private:
            // Called by Shader. Initializes runtime data from asset data. Returns whether the call succeeded.
            bool Init(
                const Data::Asset<ShaderAsset>& shaderAsset,
                const Data::Asset<ShaderVariantAsset>& shaderVariantAsset,
                SupervariantIndex supervariantIndex);

            //! A reference to the shader asset that this is a variant of.
            Data::Asset<ShaderAsset> m_shaderAsset;

            // Cached state from the asset to avoid an indirection.
            RHI::PipelineStateType m_pipelineStateType = RHI::PipelineStateType::Count;

            // State assigned to the pipeline state descriptor.
            RHI::ConstPtr<RHI::PipelineLayoutDescriptor> m_pipelineLayoutDescriptor;
            
            Data::Asset<ShaderVariantAsset> m_shaderVariantAsset;

            const RHI::RenderStates* m_renderStates = nullptr; // Cached from ShaderAsset.
            SupervariantIndex m_supervariantIndex;

            // True if there's at least one shader option that is using a specialization constant.
            bool m_useSpecializationConstants = false;
        };
    }
}
