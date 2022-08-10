/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Manipulators/AngularManipulator.h>

namespace AzFramework
{
    struct CameraState;
    class DebugDisplayRequests;
} // namespace AzFramework

namespace AzToolsFramework
{
    //! A visual segment indicator of how much rotation has occurred about an AngularManipulator using a ManipulatorCircleView.
    class AngularManipulatorCircleViewFeedback
    {
    public:
        AngularManipulatorCircleViewFeedback() = default;

        void Display(
            const AngularManipulator* angularManipulator,
            AzFramework::DebugDisplayRequests& debugDisplayRequests,
            const AzFramework::CameraState& cameraState);

         //! The last action that occurred for an AngularManipulator (note: Must be updated in the AngularManipulator callbacks).
        AngularManipulator::Action m_mostRecentAction;
    };
} // namespace AzToolsFramework
