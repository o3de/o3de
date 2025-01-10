/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Pass/DownsampleMipChainPassData.h>

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/Shader.h>

namespace AZ
{
    namespace RPI
    {
        //! This pass takes a mip mapped texture as input where the most detailed mip is already written to.
        //! It then recursively downsamples that mip to lower mip levels using the provided Compute Shader.
        //! It does this by recursively creating Compute Passes to write to each mip using the Compute Shader.
        class ATOM_RPI_PUBLIC_API DownsampleMipChainPass
            : public ParentPass
            , private ShaderReloadNotificationBus::Handler
        {
            AZ_RPI_PASS(DownsampleMipChainPass);

        public:
            AZ_RTTI(DownsampleMipChainPass, "{593B0B69-89E4-4DA5-82D2-745FB2E5FFDC}", ParentPass);
            AZ_CLASS_ALLOCATOR(DownsampleMipChainPass, SystemAllocator);
          
            //! Creates a new pass without a PassTemplate
            static Ptr<DownsampleMipChainPass> Create(const PassDescriptor& descriptor);
            virtual ~DownsampleMipChainPass();
            
        protected:
            explicit DownsampleMipChainPass(const PassDescriptor& descriptor);

            // Pass Behaviour Overrides...

            void ResetInternal() override;
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // ShaderReloadNotificationBus::Handler overrides...
            void OnShaderReinitialized(const Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const ShaderVariant& shaderVariant) override;
            
        private:

            // Gets target height, width and mip levels from the input/output image attachment
            void GetInputInfo();

            // Build child passes to downsample each mip level N and output to mip level N+1
            void BuildChildPasses();

            // Updates various settings on the child passes, such as compute thread count to match the image size
            void UpdateChildren();

            // Data driven values for DownsampleMipChainPass
            DownsampleMipChainPassData m_passData;

            // Height and width of the input mip chain texture
            uint32_t m_inputWidth = 0;
            uint32_t m_inputHeight = 0;

            // Number of mip levels in the input mip chain texture
            uint16_t m_mipLevels = 0;

            // Whether we need to rebuild the passes because the number of mips has changes
            bool m_needToRebuildChildren = true;

            // Whether we need to update the children because the input image size has changed
            bool m_needToUpdateChildren = true;
        };
    }   // namespace RPI
}   // namespace AZ
