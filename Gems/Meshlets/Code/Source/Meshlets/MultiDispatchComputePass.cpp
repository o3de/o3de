/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/CommandList.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/DevicePipelineState.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <MultiDispatchComputePass.h>

namespace AZ
{
    namespace Meshlets
    {
        Data::Instance<RPI::Shader> MultiDispatchComputePass::GetShader()
        {
            return m_shader;
        }

        RPI::Ptr<MultiDispatchComputePass> MultiDispatchComputePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<MultiDispatchComputePass> pass = aznew MultiDispatchComputePass(descriptor);
            return pass;
        }

        MultiDispatchComputePass::MultiDispatchComputePass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

        void MultiDispatchComputePass::BuildInternal()
        {
            ComputePass::BuildInternal();

            // Output
            // This is the buffer that is shared between all objects and dispatches and contains
            // the dynamic data that can be changed between passes.
            Name bufferName = Name{ "MeshletsSharedBuffer" };
            RPI::PassAttachmentBinding* localBinding = FindAttachmentBinding(bufferName);
            if (localBinding && !localBinding->GetAttachment() && Meshlets::SharedBufferInterface::Get())
            {
                AttachBufferToSlot(bufferName, Meshlets::SharedBufferInterface::Get()->GetBuffer());
            }
        }

        void MultiDispatchComputePass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
            // DON'T call the ComputePass:CompileResources as it will try to compile perDraw srg
            // under the assumption that this is a single dispatch compute.  Here we have dispatch
            // per hair object and each has its own perDraw srg.
            if (m_shaderResourceGroup != nullptr)
            {
                BindPassSrg(context, m_shaderResourceGroup);
                m_shaderResourceGroup->Compile();


            }

            // Instead of compiling per frame, have everything compiled only once after data initialization!
            // Below is an example of compiling the dispatch if change is required.
            /*
            for (auto& dispatchItem : m_dispatchItems)
            {
                if (dispatchItem)
                {
                    for (RHI::ShaderResourceGroup* srgInDispatch : dispatchItem->m_shaderResourceGroups)
                    {
                        srgInDispatch->Compile()
                    }
                }
            }
            */
        }

        void MultiDispatchComputePass::AddDispatchItems(AZStd::list<RHI::DeviceDispatchItem*>& dispatchItems)
        {
            for (auto& dispatchItem : dispatchItems)
            {
                if (dispatchItem)
                {
                    m_dispatchItems.insert(dispatchItem);
                }
            }
        }

        // [To Do] Important remark
        //-------------------------
        // When the work load / amount of dispatches is high, the RHI will split work and distribute it
        // between several threads.
        // To avoid repeating the work or possibly corrupting data in such a case, split the work
        // as per Github issue #9899 (https://github.com/o3de/o3de/pull/9899) as an example of how to
        // prevent multiple threads trying to submit the same work.
        // This was not done here yet due to the very limited work required but shuold be changed!
        void MultiDispatchComputePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();

            for (const RHI::DeviceDispatchItem* dispatchItem : m_dispatchItems)
            {
                // The following will bind all registered Srgs set in m_shaderResourceGroupsToBind
                // and sends them to the command list ahead of the dispatch.
                // This includes the PerView, PerScene and PerPass srgs.
                SetSrgsForDispatch(context);

                // In a similar way, add the dispatch high frequencies srgs.
                for (uint32_t srg = 0; srg < dispatchItem->m_shaderResourceGroupCount; ++srg)
                {
                    const RHI::ShaderResourceGroup* shaderResourceGroup = dispatchItem->m_shaderResourceGroups[srg];
                    commandList->SetShaderResourceGroupForDispatch(*shaderResourceGroup);
                }

                // submit the dispatch
                commandList->Submit(*dispatchItem);
            }

            // Clear the dispatch items. They will need to be re-populated next frame
            m_dispatchItems.clear();
        }

        // [To Do] - implement in order to support hot reloading of the shaders
        void MultiDispatchComputePass::BuildShaderAndRenderData()
        {
        }

        // Before reloading shaders, we want to wait for existing dispatches to finish
        // so shader reloading does not interfere in any way. Because AP reloads are async, there might
        // be a case where dispatch resources are destructed and will most certainly cause a GPU crash.
        // If we flag the need for rebuild, the build will be made at the start of the next frame - at
        // this stage the dispatch items should have been cleared - now we can load the shader and data.
        void MultiDispatchComputePass::OnShaderReinitialized([[maybe_unused]] const AZ::RPI::Shader& shader)
        {
            BuildShaderAndRenderData();
        }

        void MultiDispatchComputePass::OnShaderAssetReinitialized([[maybe_unused]] const Data::Asset<AZ::RPI::ShaderAsset>& shaderAsset)
        {
            BuildShaderAndRenderData();
        }

        void MultiDispatchComputePass::OnShaderVariantReinitialized([[maybe_unused]] const AZ::RPI::ShaderVariant& shaderVariant)
        {
            BuildShaderAndRenderData();
        }
    } // namespace Meshlets
}   // namespace AZ
