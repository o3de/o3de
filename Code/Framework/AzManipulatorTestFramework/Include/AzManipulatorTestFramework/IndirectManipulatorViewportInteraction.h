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

#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>

namespace AzManipulatorTestFramework
{
    class ViewportInteraction;
    class IndirectCallManipulatorManager;

    //! Implementation of manipulator viewport interaction that manipulates the manager indirectly via bus calls.
    class IndirectCallManipulatorViewportInteraction
        : public ManipulatorViewportInteraction
    {
    public:
        IndirectCallManipulatorViewportInteraction();
        ~IndirectCallManipulatorViewportInteraction();
        
        // ManipulatorViewportInteractionInterface ...
        const ViewportInteractionInterface& GetViewportInteraction() const override;
        const ManipulatorManagerInterface& GetManipulatorManager() const override;

    private:
        AZStd::unique_ptr<ViewportInteraction> m_viewportInteraction;
        AZStd::unique_ptr<IndirectCallManipulatorManager> m_manipulatorManager;
    };
} // namespace AzManipulatorTestFramework
