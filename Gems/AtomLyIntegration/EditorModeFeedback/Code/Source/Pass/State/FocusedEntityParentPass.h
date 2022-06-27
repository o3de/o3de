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
    class FocusedEntityParentPass
        : public EditorStateParentPassBase
            
        , private AzToolsFramework::ViewportEditorModeNotificationsBus::Handler
        //, private EditorModeFocusModeStateNotificationBus::Handler
        //, private EditorModeFocusModeStateRequestBus::Handler
    {
    public:
        FocusedEntityParentPass();
        ~FocusedEntityParentPass();

        // ViewportEditorModeNotificationsBus overrides ...
        void OnEditorModeActivated(
            const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;
        void OnEditorModeDeactivated(
            const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;

        // EditorModeFocusModeStateRequestBus overrides ...
        //void SetUnfocusDesaturation(float intensity);
        //void SetUnfocusTint(const AZ::Color& tint);
        //void SetUnfocusBlur(float intensity);

        bool IsEnabled() const override;

        // EditorStateParentPassBase overrides ...
        AzToolsFramework::EntityIdList GetMaskedEntities() const override;
    private:
        void InitPassData(RPI::ParentPass* parentPass) override;

        bool m_enabled = false;
    };
} // namespace AZ::Render
