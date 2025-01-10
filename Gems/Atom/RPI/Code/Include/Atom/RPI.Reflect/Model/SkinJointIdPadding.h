/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
#include <stdint.h>

namespace AZ
{
    namespace RPI
    {
        //! Calculate the count of dummy joint ids that must be used to keep the buffer aligned.
        //! Joint Ids use raw views into the buffer, and raw views
        //! must begin on 16-byte aligned boundaries.
        ATOM_RPI_REFLECT_API uint32_t CalculateJointIdPaddingCount(uint32_t realJointIdCount);
    } // namespace RPI
} // namespace AZ
