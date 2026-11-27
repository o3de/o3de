/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Matrix4x4.h>

namespace XR
{
    //! Creates an off-center projection matrix suitable for VR.
    //! It does the following in order to provide a stereoscopic projection:
    //! Stretch more horizontally and vertically
    //! Generate asymmetric or off-center projection matrix
    //! Right-handed coord system as OpenXR provides data in that system
    //! Provides support for reverse depth in case we want better depth precision
    //! Handles the case where farDist is less than nearDist whereby it will place far plane at infinity.
    AZ::Matrix4x4 CreateStereoscopicProjection(float angleLeft, float angleRight, float angleBottom, float angleTop, float nearDist, float farDist, bool reverseDepth);

    //! Check is OpenXR is enable via command line option or settings registry.
    bool IsOpenXREnabled();
}
