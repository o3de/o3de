/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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