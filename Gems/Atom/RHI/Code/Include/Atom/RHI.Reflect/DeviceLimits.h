/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/Size.h>

namespace AZ::RHI
{
    struct DeviceLimits
    {
        /// The maximum pixel size of a 1d image.
        uint32_t m_maxImageDimension1D = 0;

        /// The maximum pixel size along one axis of a 2d image.
        uint32_t m_maxImageDimension2D = 0;

        /// The maximum pixel size along one axis of a 3d image.
        uint32_t m_maxImageDimension3D = 0;

        /// The maximum pixel size along one axis of a cube image.
        uint32_t m_maxImageDimensionCube = 0;

        /// The maximum size of an image array.
        uint32_t m_maxImageArraySize = 0;

        /// The maximum number of valid memory allocations that can exist simultaneously within a device
        uint32_t m_maxMemoryAllocationCount = 0;
            
        /// The alignment required when creating constant buffer views
        uint32_t m_minConstantBufferViewOffset = 0;

        /// The alignment required when creating typed buffer views
        uint32_t m_minTexelBufferOffsetAlignment = 0;

        /// The alignment required when creating storage buffer views
        uint32_t m_minStorageBufferOffsetAlignment = 0;

        /// The maximum number of draws when doing indirect drawing.
        uint32_t m_maxIndirectDrawCount = 1;

        /// The maximum number of dispatches when doing indirect dispaching.
        uint32_t m_maxIndirectDispatchCount = 1;

        /// The maximum size in bytes of a constant buffer (BufferBindFlags::Constant).
        uint64_t m_maxConstantBufferSize = 0;

        /// The maximum size in bytes of a buffer (BufferBindFlags::ShaderRead, BufferBindFlags::ShaderWrite or BufferBindFlags::ShaderReadWrite).
        uint64_t m_maxBufferSize = 0;
                        
        //! Size of the tile when specifying shading rate through an image. The size refers to the tile's width and
        //! height in texels.
        RHI::Size m_shadingRateTileSize;

        /// Additional limits here.
    };
}
