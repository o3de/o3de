/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Asset/AssetReference.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Pass/RenderPassData.h>

namespace AZ
{
    namespace RPI
    {
        //! Custom data for the ComputePass. Should be specified in the PassRequest.
        struct ATOM_RPI_REFLECT_API ComputePassData
            : public RenderPassData
        {
            AZ_RTTI(ComputePassData, "{C9ADBF8D-34A3-4406-9067-DD17D0564FD8}", RenderPassData);
            AZ_CLASS_ALLOCATOR(ComputePassData, SystemAllocator);

            ComputePassData() = default;
            virtual ~ComputePassData() = default;

            static void Reflect(ReflectContext* context);

            AssetReference m_shaderReference;

            uint32_t m_totalNumberOfThreadsX = 0;
            uint32_t m_totalNumberOfThreadsY = 0;
            uint32_t m_totalNumberOfThreadsZ = 0;

            bool m_fullscreenDispatch = false;
            AZ::Name m_fullscreenSizeSourceSlotName;

            bool m_indirectDispatch = false;
            AZ::Name m_indirectDispatchBufferSlotName;

            // Whether the pass should use async compute and run on the compute hardware queue.
            bool m_useAsyncCompute = false;
        };
    } // namespace RPI
} // namespace AZ

