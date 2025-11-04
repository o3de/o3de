/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>

//! Provides additional functionality for the editor's context menu.
class EditorContextMenuHandler:
    private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
{
public:
    void Setup();
    void Teardown();

private:

    // ActionManagerRegistrationNotificationBus overrides ...
    void OnMenuBindingHook() override;
    void OnActionRegistrationHook() override;
};
