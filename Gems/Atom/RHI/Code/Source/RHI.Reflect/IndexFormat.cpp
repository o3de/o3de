/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/IndexFormat.h>

namespace AZ::RHI
{
    uint32_t GetIndexFormatSize(IndexFormat indexFormat)
    {
        uint32_t indexFormatSize{ Internal::GetIndexFormatSize(indexFormat) };
        AZ_Assert(indexFormatSize != 0, "Failed to get size of index format %d", indexFormat);
        return indexFormatSize;
    }
} // namespace AZ::RHI
