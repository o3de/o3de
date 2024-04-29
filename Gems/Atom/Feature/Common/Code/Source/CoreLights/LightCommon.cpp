/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Atom/Feature/CoreLights/LightCommon.h>

namespace AZ::Render::LightCommon
{
    bool HasGPUCulling(
        RPI::Scene* parentScene,
        const RPI::ViewPtr& view,
        AZStd::unordered_set<AZStd::pair<const RPI::RenderPipeline*, const RPI::View*>>& gpuCullingData)
    {
        for (const auto& renderPipeline : parentScene->GetRenderPipelines())
        {
            if (renderPipeline->NeedsRender() && gpuCullingData.contains(AZStd::pair(renderPipeline.get(), view.get())))
            {
                return true;
            }
        }
        return false;
    }

    void CacheGPUCullingPipelineInfo(
        RPI::RenderPipeline* renderPipeline,
        RPI::ViewPtr newView,
        RPI::ViewPtr previousView,
        AZStd::unordered_set<AZStd::pair<const RPI::RenderPipeline*, const RPI::View*>>& gpuCullingData)
    {
        // Check if render pipeline is using GPU culling
        if (!renderPipeline->FindFirstPass(AZ::Name("LightCullingPass")))
        {
            return;
        }

        if (previousView)
        {
            gpuCullingData.erase(AZStd::make_pair(renderPipeline, previousView.get()));
        }

        if (newView)
        {
            gpuCullingData.insert(AZStd::make_pair(renderPipeline, newView.get()));
        }
    }

    void UpdateVisibleBuffers(
        const AZStd::string_view& inputBufferName,
        const AZStd::string_view& inputBufferSrgName,
        const AZStd::string_view& inputElementCountSrgName,
        uint32_t inputVisibleBufferUsedCount,
        AZStd::vector<GpuBufferHandler>& outputVisibleBufferHandlers)
    {
        while (inputVisibleBufferUsedCount >= outputVisibleBufferHandlers.size())
        {
            GpuBufferHandler::Descriptor desc;
            desc.m_bufferName = inputBufferName;
            desc.m_bufferSrgName = inputBufferSrgName;
            desc.m_elementCountSrgName = inputElementCountSrgName;
            desc.m_elementFormat = AZ::RHI::Format::R32_UINT;
            desc.m_srgLayout = RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();

            outputVisibleBufferHandlers.emplace_back(GpuBufferHandler(desc));
        }
    }

} // namespace AZ::Render::LightCommon

