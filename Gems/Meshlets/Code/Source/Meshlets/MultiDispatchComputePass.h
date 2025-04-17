/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI.Reflect/Size.h>

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <MeshletsRenderObject.h>

namespace AZ
{
    namespace RHI
    {
        struct DispatchItem;
    }

    namespace Meshlets
    {
        //! Multi Dispatch Pass - this pass will handle multiple dispatch submission
        //! during each frame - one dispatch per mesh, each represents group of compute
        //! threads that will be working to create meshlets of the given mesh.
        //! This class can be generalized in the future to become a base class for this
        //! dispatch submission.
        //! [To Do] - revisit the 'BuildCommandListInternal' method and refactor to handle
        //! 'under the hood' RHI CPU threads that carries the submissions in parallel
        class MultiDispatchComputePass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(MultiDispatchComputePass);

        public:
            AZ_RTTI(MultiDispatchComputePass, "{13B3BAC7-0F12-4C23-BD9E-F82A7830195E}", RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(MultiDispatchComputePass, SystemAllocator);
            ~MultiDispatchComputePass() = default;

            static RPI::Ptr<MultiDispatchComputePass> Create(const RPI::PassDescriptor& descriptor);

            //! Thread-safe function for adding the frame's dispatch items
            void AddDispatchItems(AZStd::list<RHI::DeviceDispatchItem*>& dispatchItems);

            // Pass behavior overrides
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            //! Returns the shader held by the ComputePass
            Data::Instance<RPI::Shader> GetShader();

        protected:
            MultiDispatchComputePass(const RPI::PassDescriptor& descriptor);

            // Overriding methods
            void BuildInternal() override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // ShaderReloadNotificationBus::Handler overrides...
            void OnShaderReinitialized(const AZ::RPI::Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<AZ::RPI::ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const AZ::RPI::ShaderVariant& shaderVariant) override;

            void BuildShaderAndRenderData();

        private:
            AZStd::unordered_set<const RHI::DeviceDispatchItem*> m_dispatchItems;
        };

    }   // namespace Meshlets
}   // namespace AZ

