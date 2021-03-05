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

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    /// Pass this as the second argument when constructing an AZ::Crc32 to force
    /// it to use the lower cased version of your string
    static const bool s_forceCrcLowerCase = true;

    //////////////////////////////////////////////////////////////////////////
    /// These are intended to be used as an index and needs to be implicitly
    /// convertible to int.  See StartingPointCameraUtilities.h for examples
    enum AxisOfRotation : int
    {
        X_Axis = 0,
        Y_Axis = 1,
        Z_Axis = 2
    };

    //////////////////////////////////////////////////////////////////////////
    /// These are intended to be used as an index and needs to be implicitly
    /// convertible to int.  See StartingPointCameraUtilities.h for examples
    enum VectorComponentType : int
    {
        X_Component = 0,
        Y_Component = 1,
        Z_Component = 2,
        None = 3,
    };

    //////////////////////////////////////////////////////////////////////////
    /// These are intended to be used as an index and needs to be implicitly
    /// convertible to int.  See StartingPointCameraUtilities.h for examples
    enum RelativeAxisType : int
    {
        LeftRight = 0,
        ForwardBackward = 1,
        UpDown = 2,
    };

    //////////////////////////////////////////////////////////////////////////
    /// These are intended to be used as an index and needs to be implicitly
    /// convertible to int.  See StartingPointCameraUtilities.h for examples
    enum EulerAngleType : int
    {
        Pitch = 0,
        Roll = 1,
        Yaw = 2,
    };
} //namespace Camera
