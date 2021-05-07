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

#include <Code/src/TressFX/TressFXCommon.h>

#include <Rendering/HairDispatchItem.h>
#include <Rendering/HairRenderObject.h>
#include <Passes/HairSkinningComputePass.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/BufferView.h>

#include <limits>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            /*
            HairDispatchItem::HairDispatchItem(
            AZStd::intrusive_ptr<HairInputBuffers> inputBuffers,
            const AZStd::vector<RPI::BufferAssetView>& outputBufferViews,
            Data::Instance<RPI::Buffer> boneTransforms,
            RPI::Ptr<HairSkinningPass> HairSkinningPass)
            : m_inputBuffers(inputBuffers)
            , m_outputBufferViews(outputBufferViews)
            , m_boneTransforms(AZStd::move(boneTransforms))
            {
                m_skinningShader = HairSkinningPass->GetShader();
                // To Do] Adi: CreateShaderOptionGroup will also connect to the HairShaderOptionNotificationBus
                m_shaderOptionGroup = HairSkinningPass->CreateShaderOptionGroup(m_shaderOptions, *this);
            }
            */

            HairDispatchItem::~HairDispatchItem()
            {
            }

            /*
            void DispatchComputeShader(EI_CommandContext& ctx, EI_PSO* pso, DispatchLevel level, std::vector<TressFXHairObject*>& hairObjects, const bool iterate = false)
            {
                ctx.BindPSO(pso);
                for (int i = 0; i < hairObjects.size(); ++i)
                {
                    int numGroups = (int)((float)hairObjects[i]->GetNumTotalHairVertices() / (float)TRESSFX_SIM_THREAD_GROUP_SIZE);
                    if (level == DISPATCHLEVEL_STRAND)
                    {
                        numGroups = (int)(((float)(hairObjects[i]->GetNumTotalHairStrands()) / (float)TRESSFX_SIM_THREAD_GROUP_SIZE));
                    }
                    EI_BindSet* bindSets[] = { hairObjects[i]->GetSimBindSet(), &hairObjects[i]->GetDynamicHairData().GetSimBindSet() };
                    ctx.BindSets(pso, 2, bindSets);

                    int iterations = 1;
                    if (iterate)
                    {
                        iterations = hairObjects[i]->GetCPULocalShapeIterations();
                    }
                    for (int j = 0; j < iterations; ++j)
                    {
                        ctx.Dispatch(numGroups);
                    }
                    hairObjects[i]->GetDynamicHairData().UAVBarrier(ctx);
                }
            }
            */

            // Reference in the code above that tackles handling of the different dispatches possible
            // This one is targeting the per vertex dispatches fro now.
            void HairDispatchItem::InitSkinningDispatch(
                RPI::Shader* shader,
                RPI::ShaderResourceGroup* hairGenerationSrg,
                RPI::ShaderResourceGroup* hairSimSrg,
                uint32_t elementsAmount )
            {
                m_shader = shader;
                RHI::DispatchDirect dispatchArgs(
                    elementsAmount, 1, 1,
                    TRESSFX_SIM_THREAD_GROUP_SIZE, 1, 1
                );
                m_dispatchItem.m_arguments = dispatchArgs;
                RHI::PipelineStateDescriptorForDispatch pipelineDesc;
                m_shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId).ConfigurePipelineState(pipelineDesc);
                m_dispatchItem.m_pipelineState = m_shader->AcquirePipelineState(pipelineDesc);
                m_dispatchItem.m_shaderResourceGroupCount = 2;  // the per pass will be added by each pass.
                m_dispatchItem.m_shaderResourceGroups = {
                    hairGenerationSrg->GetRHIShaderResourceGroup(), // Static generation data
                    hairSimSrg->GetRHIShaderResourceGroup()        // Dynamic data changed between passes
                };
            }


            /*
            Data::Instance<RPI::Buffer> HairDispatchItem::GetBoneTransforms() const
            {
                return m_boneTransforms;
            }


            void HairDispatchItem::OnShaderReinitialized(const CachedHairShaderOptions* cachedShaderOptions)
            {
                m_shaderOptionGroup = cachedShaderOptions->CreateShaderOptionGroup(m_shaderOptions);

                if (!Init())
                {
                    AZ_Error("HairDispatchItem", false, "Failed to re-initialize after the shader was re-loaded.");
                }
            }
            */

        } // namespace Hair
    } // namespace Render
} // namespace AZ
