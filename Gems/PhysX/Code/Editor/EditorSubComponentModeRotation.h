
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
