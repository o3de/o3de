/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/Editor/EditorContextMenuBus.h>

//! Provides additional functionality for the editor's context menu.
class EditorContextMenuHandler
    : private AzToolsFramework::EditorContextMenuBus::Handler
    , private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
{
public:
    void Setup();
    void Teardown();

private:
    // EditorContextMenu overrides ...
    void PopulateEditorGlobalContextMenu(QMenu* menu, const AZStd::optional<AzFramework::ScreenPoint>& point, int flags) override;
    int GetMenuPosition() const override;

    // ActionManagerRegistrationNotificationBus overrides ...
    void OnMenuBindingHook() override;
    void OnActionRegistrationHook() override;
};
