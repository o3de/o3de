/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Pass/State/EditorStateParentPassBase.h>

#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>

namespace AZ::Render
{
    //! Class for the Focused Entity editor state effect.
    class FocusedEntityParentPass
        : public EditorStateParentPassBase
        , private AzToolsFramework::ViewportEditorModeNotificationsBus::Handler
    {
    public:
        FocusedEntityParentPass();
        ~FocusedEntityParentPass();

        // ViewportEditorModeNotificationsBus overrides ...
        void OnEditorModeActivated(
            const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;
        void OnEditorModeDeactivated(
            const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;

        // EditorStateParentPassBase overrides ...
        bool IsEnabled() const override;
        AzToolsFramework::EntityIdList GetMaskedEntities() const override;

    private:
        bool m_inFocusMode = false; //!< True if Focus Mode is active, otherwise false.
    };
} // namespace AZ::Render
