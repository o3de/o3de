/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
#include <Atom/RHI/DeviceBufferView.h>

#include <limits>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            HairDispatchItem::~HairDispatchItem()
            {
            }

            // Reference in the code above that tackles handling of the different dispatches possible
            // This one is targeting the per vertex dispatches for now.
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
                m_dispatchItem.SetArguments(dispatchArgs);
                RHI::PipelineStateDescriptorForDispatch pipelineDesc;
                m_shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId).ConfigurePipelineState(pipelineDesc);
                m_dispatchItem.SetPipelineState(m_shader->AcquirePipelineState(pipelineDesc));
                AZStd::array<const RHI::ShaderResourceGroup*, 2> srgs{
                    hairGenerationSrg->GetRHIShaderResourceGroup(), // Static generation data
                    hairSimSrg->GetRHIShaderResourceGroup() // Dynamic data changed between passes
                };
                m_dispatchItem.SetShaderResourceGroups(srgs);
            }

        } // namespace Hair
    } // namespace Render
} // namespace AZ
