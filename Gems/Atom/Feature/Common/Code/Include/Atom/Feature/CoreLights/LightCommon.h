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

    // Check if a view is being used by a pipeline that has a GPU culling pass.
    // @param parentScene - Parent Scene associated with a feature processor
    // @param view - Check if gpu culling passes are part of this view
    // @param gpuCullingData - Cached Unordered set of Render Pipelines and views that will contain the LightCullingPass which can be used for querying purposes
    bool HasGPUCulling(
        RPI::Scene* parentScene,
        const RPI::ViewPtr& view,
        AZStd::unordered_set<AZStd::pair<const RPI::RenderPipeline*, const RPI::View*>>& gpuCullingData);

    //! Update gpuCullingData to hold information related to gpu culling passes in order to Check if render pipeline is using GPU culling
    //! @param renderPipeline - Cache data associated with the passed RenderPipeline
    //! @param newView The new view triggered from view changes via OnRenderPipelinePersistentViewChanged
    //! @param previousView The previous view triggered from view changes via OnRenderPipelinePersistentViewChanged
    //! @param gpuCullingData - Update an Unordered set of Render Pipelines and views that will contain the LightCullingPass
    void CacheGPUCullingPipelineInfo(
        RPI::RenderPipeline* renderPipeline,
        RPI::ViewPtr newView,
        RPI::ViewPtr previousView,
        AZStd::unordered_set<AZStd::pair<const RPI::RenderPipeline*, const RPI::View*>>& gpuCullingData);

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

