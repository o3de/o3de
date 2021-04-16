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

/*
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/PipelineState.h>

#include <Atom/RPI.Public/Base.h>
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

                //pass->SetSrgBindIndices();

                return AZStd::move(pass);
            }


            void HairPPLLResolvePass::Init()
            {
                FullscreenTrianglePass::Init();
            }

            bool HairPPLLResolvePass::IsEnabled() const
            {
                return m_featureProcessor ? true : false;
            }

            //! Set the binding indices of all members of the SRG
            void HairPPLLResolvePass::SetSrgsBindIndices()
            {
            }

            //! Bind SRG constants - done via macro reflection
            void HairPPLLResolvePass::SetSrgsData()
            {
            }

            void HairPPLLResolvePass::SetFeatureProcessor(HairFeatureProcessor* featureeProcessor)
            {
                m_featureProcessor = featureeProcessor;
                if (m_featureProcessor)
                {
                    m_shaderResourceGroup = featureeProcessor->GetPerPassSrg();
                }
            }

            //---------------------------------------------------------------------
            void HairPPLLResolvePass::BuildAttachmentsInternal()
            {
                // No need to attach any buffer / image as it is done in the fill pass
                /*
                if (!m_featureProcessor)
                {
                    RPI::Scene* scene = GetScene();
                    if (!scene)
                    {
                        return;
                    }

                    m_featureProcessor = scene->GetFeatureProcessor<HairFeatureProcessor>();
                    if (!m_featureProcessor)
                    {
                        return;
                    }
                }

                // Inputs
                AttachBufferToSlot(Name{"PerPixelLinkedList"}, m_featureProcessor->GetPerPixelListBuffer());
                AttachImageToSlot(Name{"PerPixelListHead"}, m_featureProcessor->GetPerPixelHeadImage());
                */
            }

            void HairPPLLResolvePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
            {
                FullscreenTrianglePass::SetupFrameGraphDependencies(frameGraph);

                SetSrgsData();
            }

            void HairPPLLResolvePass::CompileResources(const RHI::FrameGraphCompileContext& context)
            {
                if (m_shaderResourceGroup != nullptr)
                {
                    BindPassSrg(context, m_shaderResourceGroup);
//                    m_shaderResourceGroup->Compile();
                }
                // {To Do] Adi: add srgs compilation here
//                FullscreenTrianglePass::CompileResources(context);
            }

        } // namespace Hair
     }   // namespace Render
}   // namespace AZ

