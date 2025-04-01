/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/DispatchItem.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>

namespace AZ
{
    namespace RPI
    {
        class ShaderResourceGroup;

        //! A ComputePass is a leaf pass (pass with no children) that is used for GPU compute
        class ATOM_RPI_PUBLIC_API ComputePass
            : public RenderPass
            , private ShaderReloadNotificationBus::Handler
        {
            AZ_RPI_PASS(ComputePass);

        public:
            AZ_RTTI(ComputePass, "{61464A74-BD35-4954-AB27-492644EA6C2A}", RenderPass);
            AZ_CLASS_ALLOCATOR(ComputePass, SystemAllocator);
            virtual ~ComputePass();

            //! Creates a ComputePass
            static Ptr<ComputePass> Create(const PassDescriptor& descriptor);

            //! Sets the target total number of threads to dispatch in each dimension
            void SetTargetThreadCounts(uint32_t targetThreadCountX, uint32_t targetThreadCountY, uint32_t targetThreadCountZ);

            //! Returns the shader resource group
            Data::Instance<ShaderResourceGroup> GetShaderResourceGroup() const;

            //! Return the shader
            Data::Instance<Shader> GetShader() const;

            // When the compute shader is reloaded, this callback will be called. Subclasses of ComputePass
            // should override OnShaderReloadedInternal() instead. By default  OnShaderReloadedInternal() will simply call
            // the callback.
            using ComputeShaderReloadedCallback = AZStd::function<void(ComputePass* computePass)>;
            void SetComputeShaderReloadedCallback(ComputeShaderReloadedCallback callback);

            //! Updates the shader variant being used by the pass
            void UpdateShaderOptions(const ShaderVariantId& shaderVariantId);

        protected:
            ComputePass(const PassDescriptor& descriptor, AZ::Name supervariant = AZ::Name(""));

            // Pass behavior overrides...
            void BuildInternal() override;

            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // Calculates the group counts for the dispatch item using the target image dimensions
            // and the number of threads per group (group size)
            void MatchDimensionsToOutput();

            // The compute shader that will be used by the pass
            Data::Instance<Shader> m_shader = nullptr;

            // Default draw SRG for using the shader option system's variant fallback key
            Data::Instance<RPI::ShaderResourceGroup> m_drawSrg = nullptr;

            // The draw item submitted by this pass
            RHI::DispatchItem m_dispatchItem;

            // Whether or not to make the pass a full-screen compute pass, and which slot to use as the size source
            AZ::Name m_fullscreenSizeSourceSlotName;
            bool m_fullscreenDispatch = false;
            AZ::RPI::PassAttachmentBinding* m_fullscreenSizeSourceBinding = nullptr;

            // Wheter or not to make the pass a indirect dispatch compute pass, and which slot to use as indirect dispatch
            // command buffer
            bool m_indirectDispatch = false;
            AZ::Name m_indirectDispatchBufferSlotName;
            AZ::RPI::PassAttachmentBinding* m_indirectDispatchBufferBinding = nullptr;
            AZ::RHI::Ptr<AZ::RHI::IndirectBufferSignature> m_indirectDispatchBufferSignature;
            AZ::RHI::IndirectBufferView m_indirectDispatchBufferView;

            // ShaderReloadNotificationBus::Handler overrides...
            void OnShaderReinitialized(const Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const ShaderVariant& shaderVariant) override;

            void LoadShader(AZ::Name supervariant = AZ::Name(""));
            PassDescriptor m_passDescriptor;

            // At the end of LoadShader(), this function is called. By default it executes
            // the callback function. This gives an opportunity to subclasses and owners of compute passes
            // to call SetTargetThreadCounts(), update shader constants, etc.
            virtual void OnShaderReloadedInternal();
            ComputeShaderReloadedCallback m_shaderReloadedCallback = nullptr;
        };
    }   // namespace RPI
}   // namespace AZ
