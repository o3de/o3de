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
    class AngularManipulator;

    class AngularManipulatorCircleViewFeedback
    {
    public:
        AngularManipulatorCircleViewFeedback() = default;

        void Display(
            const AngularManipulator* angularManipulator,
            AzFramework::DebugDisplayRequests& debugDisplayRequests,
            const AzFramework::CameraState& cameraState);

        AngularManipulator::Action m_mostRecentAction;
    };
} // namespace AzToolsFramework
