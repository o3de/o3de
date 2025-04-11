/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/QueryPoolDescriptor.h>
#include <Atom/RHI.Reflect/IndirectBufferLayout.h>
#include <Atom/RHI.Reflect/VariableRateShadingEnums.h>

#include <AzCore/std/containers/array.h>

namespace AZ::RHI
{
    struct DeviceFeatures
    {
        //! Whether the adapter supports geometry shaders.
        bool m_geometryShader;

        //! Whether the adapter supports compute shaders.
        bool m_computeShader;

        //! Whether color attachments can utilize independent blend modes.
        bool m_independentBlend;

        //! Whether the adapter supports dual source blending.
        bool m_dualSourceBlending = false;

        //! Whether the adapter supports custom sample positions.
        bool m_customSamplePositions = false;

        //! Supported query types
        AZStd::array<QueryTypeFlags, HardwareQueueClassCount> m_queryTypesMask = { { QueryTypeFlags::None, QueryTypeFlags::None, QueryTypeFlags::None } };

        //! Whether the adapter supports precise occlusion query.
        bool m_occlusionQueryPrecise = false;

        //! Whether the adapter supports predication
        bool m_predication = false;

        //! Whether the adapter supports indirect draw
        bool m_indirectDrawSupport = true;
            
        //! Whether the adapter supports a count buffer when doing indirect drawing.
        bool m_indirectDrawCountBufferSupported = false;

        //! Whether the adapter supports a count buffer when doing indirect dispatching.
        bool m_indirectDispatchCountBufferSupported = false;

        //! Whether the adapter supports supplying a start instance location when doing indirect drawing.
        bool m_indirectDrawStartInstanceLocationSupported = false;

        //! Tier of supported indirect commands.
        RHI::IndirectCommandTiers m_indirectCommandTier = RHI::IndirectCommandTiers::Tier0;

        //! The type of support for subpass inputs of render target attachments.
        SubpassInputSupportType m_subpassInputSupport = SubpassInputSupportType::None;

        //! Whether Ray Tracing support is available.
        bool m_rayTracing = false;

        //! Whether Unbounded Array support is available.
        bool m_unboundedArrays = false;

        //! Whether Unbounded Array support is simulated for Bindless Srg.
        bool m_simulateBindlessUA = false;

        //! Whether PipelineLibrary related serialized data needs to be loaded/saved explicitly as drivers (like dx12/vk) do not support it internally
        bool m_isPsoCacheFileOperationsNeeded = true;

        //! Whether supports tile resource.
        bool m_tiledResource = true;

        //! Supported modes for setting the variable shading rate.
        RHI::ShadingRateTypeFlags m_shadingRateTypeMask = RHI::ShadingRateTypeFlags::None;

        //! Supported values for setting the shading rate.
        RHI::ShadingRateFlags m_shadingRateMask = RHI::ShadingRateFlags::None;

        //! Whether the shading rate attachment is read by the GPU during execution.
        //! Otherwise, the image is read by the CPU during scope execution (so it must not be modified by either CPU or GPU at that time).
        bool m_dynamicShadingRateImage = false;
            
        //! Whether the adapter supports wave/subgroup operation
        bool m_waveOperation = false;
                        
        //! Whether swapchain scaling support is available.
        RHI::ScalingFlags m_swapchainScalingFlags = RHI::ScalingFlags::None;

        //! Whether fences can be signalled from the CPU
        //! If this is false, the Fence::SignalOnCpu inserts the signal command into a queue
        bool m_signalFenceFromCPU = false;

        //! Whether float16 (half-precision floating-point format) support is available.
        bool m_float16 = false;

        /// Additional features here.
    };
}
