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

#include <Atom/RPI.Reflect/Shader/ShaderAsset2.h>

#include <Atom/RHI/DrawListTagRegistry.h>

namespace AZ
{
    namespace RPI
    {
        //! Represents the concrete state to configure a PipelineStateDescriptor. ShaderVariant2's match
        //! the RHI::PipelineStateType of the parent Shader instance. For shaders on the raster
        //! pipeline, the RHI::DrawFilterTag is also provided.
        class ShaderVariant2 final
        {
            friend class Shader2;
        public:
            ShaderVariant2() = default;
            AZ_DEFAULT_COPY_MOVE(ShaderVariant2);

            //! Fills a pipeline state descriptor with settings provided by the ShaderVariant2. (Note that
            //! this does not fill the InputStreamLayout or OutputAttachmentLayout as that also requires 
            //! information from the mesh data and pass system and must be done as a separate step).
            void ConfigurePipelineState(RHI::PipelineStateDescriptor& descriptor) const;

            const ShaderVariantId& GetShaderVariantId() const { return m_shaderVariantAsset->GetShaderVariantId(); }

            //! Returns whether the variant is fully baked variant (all options are static branches), or false if the
            //! variant uses dynamic branches for some shader options.
            //! If the shader variant is not fully baked, the ShaderVariantKeyFallbackValue must be correctly set when drawing.
            bool IsFullyBaked() const { return m_shaderVariantAsset->IsFullyBaked(); }

            //! Return the timestamp when the associated ShaderAsset was built.
            //! This is used to synchronize versions of the ShaderAsset and ShaderVariantAsset, especially during hot-reload.
            AZStd::sys_time_t GetBuildTimestamp() const { return m_shaderVariantAsset->GetBuildTimestamp(); }

            bool IsRootVariant() const { return m_shaderVariantAsset->IsRootVariant(); }

            ShaderVariantStableId GetStableId() const { return m_shaderVariantAsset->GetStableId(); }

        private:
            // Called by Shader. Initializes runtime data from asset data. Returns whether the call succeeded.
            bool Init(
                const ShaderAsset2& shaderAsset,
                Data::Asset<ShaderVariantAsset2> shaderVariantAsset,
                SupervariantIndex supervariantIndex);

            // Cached state from the asset to avoid an indirection.
            RHI::PipelineStateType m_pipelineStateType = RHI::PipelineStateType::Count;

            // State assigned to the pipeline state descriptor.
            RHI::ConstPtr<RHI::PipelineLayoutDescriptor> m_pipelineLayoutDescriptor;
            
            Data::Asset<ShaderVariantAsset2> m_shaderVariantAsset;

            const RHI::RenderStates* m_renderStates = nullptr; // Cached from ShaderAsset2.
        };
    }
}
