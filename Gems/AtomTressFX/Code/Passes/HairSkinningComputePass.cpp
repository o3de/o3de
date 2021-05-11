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

#include <Atom/RHI/CommandList.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/PipelineState.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

// Hair Specific
#include <Rendering/HairCommon.h>
#include <Rendering/SharedBufferInterface.h>
#include <Rendering/HairDispatchItem.h>
#include <Rendering/HairFeatureProcessor.h>
#include <Rendering/HairRenderObject.h>

#include <Passes/HairSkinningComputePass.h>

#pragma optimize("", off)

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            //        static const char* const HairSkinningBufferBaseName = "HairSkinningBuffer";
            Data::Instance<RPI::Shader> HairSkinningComputePass::GetShader()
            {
                return m_shader;
            }

            RPI::Ptr<HairSkinningComputePass> HairSkinningComputePass::Create(const RPI::PassDescriptor& descriptor)
            {
                RPI::Ptr<HairSkinningComputePass> pass = aznew HairSkinningComputePass(descriptor);
                return pass;
            }

            HairSkinningComputePass::HairSkinningComputePass(const RPI::PassDescriptor& descriptor)
                : RPI::ComputePass(descriptor)
            {
            }

            void HairSkinningComputePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
            {
                ComputePass::SetupFrameGraphDependencies(frameGraph);

                AZ_Error("Hair Gem", m_shaderResourceGroup && m_featureProcessor,
                    "HairSkinningComputePass::SetupFrameGraphDependencies - m_shaderResourceGroup or Feature processor not initialized");
            }

            void HairSkinningComputePass::BuildAttachmentsInternal()
            {
                ComputePass::BuildAttachmentsInternal();

                if (!m_featureProcessor)
                {
                    RPI::Scene* scene = GetScene();
                    if (scene)
                    {
                        m_featureProcessor = scene->GetFeatureProcessor<HairFeatureProcessor>();
                    }
                    if (!m_featureProcessor)
                    {
                        AZ_Error("Hair Gem", false, "HairSkinningComputePass - Failed to retrieve Hair feature processor from the scene");
                        return;
                    }
                }

                // Output
                // This is the buffer that is shared between all objects and dispatches and contains
                // the dynamic data that can be changed between passes.
                Name bufferName = Name{ "SkinnedHairSharedBuffer" };
                RPI::PassAttachmentBinding* localBinding = FindAttachmentBinding(bufferName);
                if (localBinding && !localBinding->m_attachment)
                {
                    AttachBufferToSlot(Name{ "SkinnedHairSharedBuffer" }, m_featureProcessor->GetSharedBuffer());
                }
             }

            void HairSkinningComputePass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
            {
                if (!m_featureProcessor)
                {
                    return;
                }

                // DON'T call the ComputePass:CompileResources as it will try to compile perDraw srg
                // under the assumption that this is a single dispatch compute.  Here we have dispatch
                // per hair object and each has its own perDraw srg.
                if (m_shaderResourceGroup != nullptr)
                {
                    BindPassSrg(context, m_shaderResourceGroup);
                    m_shaderResourceGroup->Compile();
                }
            }

            bool HairSkinningComputePass::IsEnabled() const
            {
                return (m_dispatchItems.size() ? true : false);
            }

            bool HairSkinningComputePass::BuildDispatchItem(HairRenderObject* hairObject, DispatchLevel dispatchLevel)
            {
                return hairObject->BuildDispatchItem(m_shader.get(), m_shaderResourceGroup.get(), dispatchLevel);
            }

            void HairSkinningComputePass::AddDispatchItem(HairRenderObject* hairObject)
            {
                if (!hairObject)
                {
                    return;
                }

                const RHI::DispatchItem* dispatchItem = hairObject->GetDispatchItem(m_shaderResourceGroup.get());
                if (!dispatchItem)
                {
                    return;
                }

                // [To Do] Adi: this was tested and is working properly, however, it is highly
                // dependent on having the group lock properly set and enough between dispatches.
                // Fro dispatches that change the same base data this might be dangerous so
                // monitoring it is crucial and moving towards iterating over passes in the future
                // is highly recommended to verify that a proper barrier is put in place.
                // To that affect, iterating over the entire simulation pass chain rather than iterating
                // over each one separately is much better for improving simulation stability
                // with minimal amount of steps.
                // This will also allow us to avoid doing it to the simple follow hair copy at the end.
                uint32_t iterations = m_allowSimIterations ? AZ::GetMax(hairObject->GetCPULocalShapeIterations(), 1) : 1;
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

                for (int j = 0; j < iterations; ++j)
                {
                    m_dispatchItems.insert(dispatchItem);
                }
            }

            void HairSkinningComputePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
            {
                RHI::CommandList* commandList = context.GetCommandList();

                // The following will bind all registered Srgs set in m_shaderResourceGroupsToBind
                // and sends them to the command list ahead of the dispatch.
                // This includes the PerView, PerScene and PerPass srgs (what about per draw?)

                SetSrgsForDispatch(commandList);

                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                for (const RHI::DispatchItem* dispatchItem : m_dispatchItems)
                {
                    commandList->Submit(*dispatchItem);
                }

                // Clear the dispatch items. They will need to be re-populated next frame
                m_dispatchItems.clear();
            }

            // Before reloading shaders, we want to wait for existing dispatches to finish
            // so shader reloading does not interfere in any way. Because AP reloads are async, there might
            // be a case where dispatch resources are destructed and will most certainly cause a GPU crash.
            // Once m_dispatchItems are empty, we hold the thread lock, reload shaders, then allow things to
            // resume back to normal.
            void HairSkinningComputePass::OnShaderReinitialized([[maybe_unused]] const AZ::RPI::Shader& shader)
            {
                while (true)
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                    if (m_dispatchItems.empty())
                    {
                        m_featureProcessor->SetAddDispatchEnable(false);
                        LoadShader();
                        m_featureProcessor->ForceRebuildRenderData();
                        break;
                    }
                }
            }

            void HairSkinningComputePass::OnShaderAssetReinitialized([[maybe_unused]] const Data::Asset<AZ::RPI::ShaderAsset>& shaderAsset)
            {
                while (true)
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                    if (m_dispatchItems.empty())
                    {
                        m_featureProcessor->SetAddDispatchEnable(false);
                        LoadShader();
                        m_featureProcessor->ForceRebuildRenderData();
                        break;
                    }
                }
            }

            void HairSkinningComputePass::OnShaderVariantReinitialized([[maybe_unused]] const AZ::RPI::Shader& shader, [[maybe_unused]] const AZ::RPI::ShaderVariantId& shaderVariantId, [[maybe_unused]] AZ::RPI::ShaderVariantStableId shaderVariantStableId)
            {
                while (true)
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                    if (m_dispatchItems.empty())
                    {
                        m_featureProcessor->SetAddDispatchEnable(false);
                        LoadShader();
                        m_featureProcessor->ForceRebuildRenderData();
                        break;
                    }
                }
            }
        } // namespace Hair
    }   // namespace Render
}   // namespace AZ

#pragma optimize("", on)

