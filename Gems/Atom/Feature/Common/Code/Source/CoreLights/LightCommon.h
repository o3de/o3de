/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Internal/MathTypes.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ::Render::LightCommon
{
    inline float GetRadiusFromInvRadiusSquared(float invRadiusSqaured)
    {
        return (invRadiusSqaured <= 0.0f) ? 1.0f : Sqrt(1.0f / invRadiusSqaured);
    }

    // Check if a view has a pipeline that needs CPU culling (i.e. doesn't have a GPU culling pass).
    // @param view - The view we are checking against
    // @param cpuCulledPipelinesPerView - Map of views -> pipelines in that view that need CPU culling (i.e. no GPU culling pass)
    bool NeedsCPUCulling(
        const RPI::ViewPtr& view,
        const AZStd::unordered_map<const RPI::View*, AZStd::vector<const RPI::RenderPipeline*>>& cpuCulledPipelinesPerView);

    //! Cache pipelines that need CPU culling (i.e. no GPU culling pass) with their respective view, store in cpuCulledPipelinesPerView
    //! @param renderPipeline - Cache data associated with the passed RenderPipeline
    //! @param newView The new view triggered from view changes via OnRenderPipelinePersistentViewChanged
    //! @param previousView The previous view triggered from view changes via OnRenderPipelinePersistentViewChanged
    // @param cpuCulledPipelinesPerView - Map of views -> pipelines in that view that need CPU culling (i.e. no GPU culling pass)
    void CacheCPUCulledPipelineInfo(
        RPI::RenderPipeline* renderPipeline,
        RPI::ViewPtr newView,
        RPI::ViewPtr previousView,
        AZStd::unordered_map<const RPI::View*, AZStd::vector<const RPI::RenderPipeline*>>& cpuCulledPipelinesPerView);

    //! Populate and cache multiple buffer handlers (one per view) that will be used to hold visibility data from cpu culling
    //! @param inputBufferName - Gpu Buffer name
    //! @param inputBufferSrgName - Gpu buffer srg name
    //! @param inputElementCountSrgName - Count SRG name related to the number of objects represented by the buffer
    //! @param inputVisibleBufferUsedCount - Number of buffers already cached per view
    //! @param outputVisibleBufferHandlers - Handle to the vector gpu buffer handlers that needs to be populated
    void UpdateVisibleBuffers(
         const AZStd::string_view& inputBufferName,
         const AZStd::string_view& inputBufferSrgName,
         const AZStd::string_view& inputElementCountSrgName,
         uint32_t inputVisibleBufferUsedCount,
         AZStd::vector<GpuBufferHandler>& outputVisibleBufferHandlers);

} // namespace AZ::Render::LightCommon

