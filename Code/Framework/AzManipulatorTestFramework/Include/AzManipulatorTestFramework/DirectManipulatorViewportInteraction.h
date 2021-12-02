/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>

namespace AzManipulatorTestFramework
{
    class CustomManipulatorManager;
    class DirectCallManipulatorManager;
    class ViewportInteraction;

    //! Implementation of manipulator viewport interaction that manipulates the manager directly.
    class DirectCallManipulatorViewportInteraction
        : public ManipulatorViewportInteraction
    {
    public:
        DirectCallManipulatorViewportInteraction();
        ~DirectCallManipulatorViewportInteraction();

        // ManipulatorViewportInteractionInterface ...
        const ViewportInteractionInterface& GetViewportInteraction() const override;
        const ManipulatorManagerInterface& GetManipulatorManager() const override;

    private:
        AZStd::shared_ptr<CustomManipulatorManager> m_customManager;
        AZStd::unique_ptr<ViewportInteraction> m_viewportInteraction;
        AZStd::unique_ptr<DirectCallManipulatorManager> m_manipulatorManager;
    };
} // namespace AzManipulatorTestFramework
