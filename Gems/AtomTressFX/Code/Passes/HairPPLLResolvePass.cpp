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

#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>

#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Passes/HairPPLLResolvePass.h>
#include <Rendering/HairFeatureProcessor.h>

#pragma optimize("", off)
namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            HairPPLLResolvePass::HairPPLLResolvePass(const RPI::PassDescriptor& descriptor)
                : RPI::FullscreenTrianglePass(descriptor)
            {
            }

            RPI::Ptr<HairPPLLResolvePass> HairPPLLResolvePass::Create(const RPI::PassDescriptor& descriptor)
            {
                RPI::Ptr<HairPPLLResolvePass> pass = aznew HairPPLLResolvePass(descriptor);

                return AZStd::move(pass);
            }

            void HairPPLLResolvePass::Init()
            {
                FullscreenTrianglePass::Init();
            }

            void HairPPLLResolvePass::OnBuildAttachmentsFinishedInternal()
            {
                FullscreenTrianglePass::OnBuildAttachmentsFinishedInternal();

                if (!m_shaderResourceGroup)
                {
                    AZ_Error("Hair Gem", false, "HairPPLLResolvePass: Could not set per pass srg resources: feature processor not ready yet");
                    return;
                }

                if (!m_featureProcessor)
                {
                    return;
                }
            }

            //---------------------------------------------------------------------
            void HairPPLLResolvePass::BuildAttachmentsInternal()
            {
                // No need to attach any buffer / image - it is done in the fill pass
                FullscreenTrianglePass::BuildAttachmentsInternal();

                if (!m_featureProcessor)
                {
                    RPI::Scene* scene = GetScene();
                    if (scene)
                    {
                        m_featureProcessor = scene->GetFeatureProcessor<HairFeatureProcessor>();
                    }
                    if (!m_featureProcessor)
                    {
                        AZ_Error("Hair Gem", false, "HairPPLLResolvePass - Failed to retrieve Hair feature processor from the scene");
                        return;
                    }
                }
            }

            void HairPPLLResolvePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
            {
                FullscreenTrianglePass::SetupFrameGraphDependencies(frameGraph);

                if (!m_shaderResourceGroup || !m_featureProcessor)
                {
                    return;
                }

                AZ_Error("Hair Gem", m_shaderResourceGroup && m_featureProcessor,
                    "HairPPLLResolvePass::SetupFrameGraphDependencies - m_shaderResourceGroup or Feature processor not initialized");
                
                {
                    SrgBufferDescriptor descriptor = SrgBufferDescriptor(
                        RPI::CommonBufferPoolType::ReadWrite, RHI::Format::Unknown,
                        PPLL_NODE_SIZE, RESERVED_PIXELS_FOR_OIT,
                        Name{ "LinkedListNodesPPLL" }, Name{ "m_linkedListNodes" }, 0, 0
                    );
                    if (!UtilityClass::BindBufferToSrg("Hair Gem", m_featureProcessor->GetPerPixelListBuffer(), descriptor, m_shaderResourceGroup))
                    {
                        AZ_Error("Hair Gem", false, "HairPPLLResolvePass: PPLL list data could not be bound.");
                    }
                }
            }

            void HairPPLLResolvePass::CompileResources(const RHI::FrameGraphCompileContext& context)
            {
                if (!m_shaderResourceGroup || !m_featureProcessor)
                {
                    return;
                }

                // Update the material array constant buffer within the per pass srg
                SrgBufferDescriptor descriptor = SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::Constant, RHI::Format::Unknown,
                    sizeof(AMD::TressFXShadeParams), 1,
                    Name{ "HairMaterialsArray" }, Name{ "m_hairParams" }, 0, 0
                );
                m_featureProcessor->GetMaterialsArray().UpdateGPUData(m_shaderResourceGroup, descriptor);

                // All remaining srgs should compile here
                FullscreenTrianglePass::CompileResources(context);
            }

        } // namespace Hair
     }   // namespace Render
}   // namespace AZ

#pragma optimize("", on)
