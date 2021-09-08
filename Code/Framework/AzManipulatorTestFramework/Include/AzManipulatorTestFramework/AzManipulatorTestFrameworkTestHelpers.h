/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkUtils.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>
#include <AzManipulatorTestFramework/IndirectManipulatorViewportInteraction.h>
#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <type_traits>

namespace UnitTest
{
    //! Fixture to provide the indirect call viewport interaction that is dependent on AzToolsFramework::ToolsApplication.
    //! \tparam ToolsApplicationFixtureT The fixture that provides the AzToolsFramework::ToolsApplication functionality.
    template<typename ToolsApplicationFixtureT>
    class IndirectCallManipulatorViewportInteractionFixtureMixin : public ToolsApplicationFixtureT
    {
        using IndirectCallManipulatorViewportInteraction = AzManipulatorTestFramework::IndirectCallManipulatorViewportInteraction;
        using ImmediateModeActionDispatcher = AzManipulatorTestFramework::ImmediateModeActionDispatcher;

        void SetUpEditorFixtureImpl() override
        {
            ToolsApplicationFixtureT::SetUpEditorFixtureImpl();
            m_viewportManipulatorInteraction = AZStd::make_unique<IndirectCallManipulatorViewportInteraction>();
            m_actionDispatcher = AZStd::make_unique<ImmediateModeActionDispatcher>(*m_viewportManipulatorInteraction);
            m_cameraState =
                AzFramework::CreateIdentityDefaultCamera(AZ::Vector3::CreateZero(), AzManipulatorTestFramework::DefaultViewportSize);
        }

        void TearDownEditorFixtureImpl() override
        {
            m_actionDispatcher.reset();
            m_viewportManipulatorInteraction.reset();
            ToolsApplicationFixtureT::TearDownEditorFixtureImpl();
        }

    public:
        AzFramework::CameraState m_cameraState;
        AZStd::unique_ptr<ImmediateModeActionDispatcher> m_actionDispatcher;
        AZStd::unique_ptr<IndirectCallManipulatorViewportInteraction> m_viewportManipulatorInteraction;
    };

    //! Fixture to provide the indirect call viewport interaction that inherits from ToolsApplicationFixture for the
    //! dependent on AzToolsFramework::ToolsApplication.
    using IndirectCallManipulatorViewportInteractionFixture =
        IndirectCallManipulatorViewportInteractionFixtureMixin<ToolsApplicationFixture>;
} // namespace UnitTest
