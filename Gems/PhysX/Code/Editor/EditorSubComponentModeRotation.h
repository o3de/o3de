
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Editor/EditorSubComponentModeBase.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace AzToolsFramework
{
    class AngularManipulator;
}

namespace PhysX
{
    class EditorSubComponentModeRotation
        : public PhysX::EditorSubComponentModeBase
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        EditorSubComponentModeRotation(
            const AZ::EntityComponentIdPair& entityComponentIdPair
            , const AZ::Uuid& componentType
            , const AZStd::string& name);
        ~EditorSubComponentModeRotation();

        // PhysX::EditorSubComponentModeBase
        void Refresh() override;

    private:
        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        void CreateManipulators();
        void InstallManipulatorMouseCallbacks();
        void RegisterManipulators();
        void UnregisterManipulators();

        AZStd::array<AZStd::shared_ptr<AzToolsFramework::AngularManipulator>, 3> m_rotationManipulators;
    };
} // namespace PhysX
