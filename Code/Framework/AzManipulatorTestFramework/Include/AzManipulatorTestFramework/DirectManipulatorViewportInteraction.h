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
        std::unique_ptr<ViewportInteraction> m_viewportInteraction;
        std::unique_ptr<DirectCallManipulatorManager> m_manipulatorManager;
    };
} // namespace AzManipulatorTestFramework
