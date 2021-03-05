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

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzFramework/Entity/EntityContext.h>

namespace AzToolsFramework
{
    /**
     * Controls whether an Entity is shown or hidden in the Editor
     * The "Visibility Flag" controls whether an entity can ever be shown.
     * "Current Visibility" is the entity's current state, a combination
     * of factors (including the visibility flag) contribute to this.
     * \note "Visibility" here refers to the ability to be shown,
     * it does refer to whether the entity is currently on camera.
     * \note This functionality is editor-only, it is not available in-game.
     */
    class EditorVisibilityRequests
        : public AZ::ComponentBus
    {
    public:
        using MutexType = AZStd::recursive_mutex;

        /// Set whether this entity is set to be visible in the editor (individual state/flag).
        virtual void SetVisibilityFlag(bool flag) = 0;

        /// Get whether this entity is set to be visible in the editor (individual state/flag).
        virtual bool GetVisibilityFlag() = 0;
    };

    /// \ref EditorVisibilityRequests
    using EditorVisibilityRequestBus = AZ::EBus<EditorVisibilityRequests>;

    /**
     * Messages about whether an Entity is shown or hidden in the Editor.
     * See \ref EditorVisibilityRequests.
     */
    class EditorEntityVisibilityNotifications
        : public AZ::ComponentBus
    {
    public:
        /// The entity's current visibility has changed.
        /// \note This does not reflect whether the entity is currently on-camera.
        virtual void OnEntityVisibilityChanged(bool /*visibility*/) {}

        /// The entity's visibility flag has been changed.
        /// Even if the flag is set true, the entity may be hidden for other reasons.
        /// ATTN: Only EditorEntityModelEntry should listen to this notification.
        virtual void OnEntityVisibilityFlagChanged(bool /*flag*/) {}
    };

    /// \ref EditorEntityVisibilityNotifications
    using EditorEntityVisibilityNotificationBus = AZ::EBus<EditorEntityVisibilityNotifications>;

    /// Alias for EditorEntityVisibilityNotifications - prefer EditorEntityVisibilityNotifications,
    /// EditorVisibilityNotifications is deprecated.
    using EditorVisibilityNotifications = EditorEntityVisibilityNotifications;

    /// Alias for EditorEntityVisibilityNotificationBus - prefer EditorEntityVisibilityNotificationBus,
    /// EditorVisibilityNotificationBus is deprecated.
    using EditorVisibilityNotificationBus = EditorEntityVisibilityNotificationBus;

} // namespace AzToolsFramework
