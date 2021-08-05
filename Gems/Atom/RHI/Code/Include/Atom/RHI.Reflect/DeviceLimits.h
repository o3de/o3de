/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Base.h>

namespace AZ
{
    namespace RHI
    {
        struct DeviceLimits
        {
            /// The maximum pixel size of a 1d image.
            uint32_t m_maxImageDimension1D;

            /// The maximum pixel size along one axis of a 2d image.
            uint32_t m_maxImageDimension2D;

            /// The maximum pixel size along one axis of a 3d image.
            uint32_t m_maxImageDimension3D;

            /// The maximum pixel size along one axis of a cube image.
            uint32_t m_maxImageDimensionCube;

            /// The maximum size of an image array.
            uint32_t m_maxImageArraySize;

            /// The alignment required when creating buffer views
            uint32_t m_minConstantBufferViewOffset;

            /// The maximum number of draws when doing indirect drawing.
            uint32_t m_maxIndirectDrawCount = 1;

            /// The maximum number of dispatches when doing indirect dispaching.
            uint32_t m_maxIndirectDispatchCount = 1;

            /// Additional limits here.
        };
    }
}
