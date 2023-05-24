/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzFramework/Viewport/ViewportId.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace SandboxEditor
{
    //! Notifications for changes to the editor modular viewport camera controller.
    class EditorModularViewportCameraComposerNotifications
    {
    public:
        //! Notify any listeners when changes have been made to the modular viewport camera settings.
        //! @note This is used to update any cached input channels when controls are modified.
        virtual void OnEditorModularViewportCameraComposerSettingsChanged() = 0;

    protected:
        ~EditorModularViewportCameraComposerNotifications() = default;
    };

    using EditorModularViewportCameraComposerNotificationBus =
        AZ::EBus<EditorModularViewportCameraComposerNotifications, AzToolsFramework::ViewportInteraction::ViewportNotificationsEBusTraits>;
} // namespace SandboxEditor
