/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Model/SkinJointIdPadding.h>
#include <AzCore/base.h>

namespace AZ
{
    namespace RPI
    {
        uint32_t CalculateJointIdPaddingCount(uint32_t realJointIdCount)
        {
            // Raw buffer views must begin on 16-byte aligned boundaries.
            // For a two byte unit16_t joint id, we need to round up to a multiple of 8 elements
            uint32_t paddedNewJointIdCount = AZ::SizeAlignUp(realJointIdCount, 8);
            return paddedNewJointIdCount - realJointIdCount;
        }
    } //namespace RPI
} // namespace AZ
