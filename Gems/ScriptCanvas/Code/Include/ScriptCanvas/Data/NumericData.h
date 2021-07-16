/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ScriptCanvas/Data/Data.h>

namespace ScriptCanvas
{
    namespace Data
    {
        AZ_INLINE NumberType One() { return 1.0; }
        AZ_INLINE NumberType ToleranceEpsilon() { return AZ::Constants::FloatEpsilon; }
        AZ_INLINE NumberType ToleranceSIMD() { return 0.01; }
        AZ_INLINE NumberType Zero() { return 0.0; }
    }
} 
