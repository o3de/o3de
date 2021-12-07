/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>

namespace AzManipulatorTestFramework
{
    class ViewportInteraction;
    class IndirectCallManipulatorManager;

    //! Implementation of manipulator viewport interaction that manipulates the manager indirectly via bus calls.
    class IndirectCallManipulatorViewportInteraction : public ManipulatorViewportInteraction
    {
    public:
        explicit IndirectCallManipulatorViewportInteraction(AZStd::shared_ptr<AzFramework::DebugDisplayRequests> debugDisplayRequests);
        ~IndirectCallManipulatorViewportInteraction();

        // ManipulatorViewportInteractionInterface ...
        const ViewportInteractionInterface& GetViewportInteraction() const override;
        const ManipulatorManagerInterface& GetManipulatorManager() const override;

    private:
        AZStd::unique_ptr<ViewportInteraction> m_viewportInteraction;
        AZStd::unique_ptr<IndirectCallManipulatorManager> m_manipulatorManager;
    };
} // namespace AzManipulatorTestFramework
