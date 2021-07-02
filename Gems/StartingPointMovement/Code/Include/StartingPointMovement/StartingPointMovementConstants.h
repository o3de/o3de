/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace Movement
{
    //////////////////////////////////////////////////////////////////////////
    /// These are intended to be used as an index and needs to be implicitly
    /// convertible to int.  See StartingPointMovementUtilities.h for examples
    enum AxisOfRotation
    {
        X_Axis = 0,
        Y_Axis = 1,
        Z_Axis = 2
    };
} //namespace Movement
