/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/IdleBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorControllerInterface.h>

namespace AtomToolsFramework
{
    IdleBehavior::IdleBehavior(ViewportInputBehaviorControllerInterface* controller)
        : ViewportInputBehavior(controller)
    {
    }
} // namespace AtomToolsFramework
