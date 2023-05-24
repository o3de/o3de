/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzFramework/Entity/EntityContext.h>

namespace AzToolsFramework
{
    /*!
     * Controls whether an entity is locked in the editor
     * Locked entities can be seen but cannot be selected
     */
    class EditorLockComponentRequests
        : public AZ::ComponentBus
    {
    public:
        /// Set whether this entity is set to be locked in the editor (individual state/flag).
        virtual void SetLocked(bool locked) = 0;

        /// Get whether this entity is set to be locked in the editor (individual state/flag).
        virtual bool GetLocked() = 0;
    };

    /// \ref EditorLockRequests
    using EditorLockComponentRequestBus = AZ::EBus<EditorLockComponentRequests>;

    /**
     * Notifications about whether an Entity is locked in the Editor.
     * See \ref EditorLockRequests.
     */
    class EditorEntityLockComponentNotifications
        : public AZ::ComponentBus
    {
    public:
        /// The entity's current internal lock state/flag has changed.
        /// ATTN: Only EditorEntityModelEntry listens to this notification.
        virtual void OnEntityLockFlagChanged(bool /*locked*/) {}

        /// The entity's current lock has changed (in terms of viewport interaction).
        /// Note: The event may be caused by a layer lock changing or an individually entity lock changing.
        virtual void OnEntityLockChanged(bool /*locked*/) {}
    };

    /// \ref EditorEntityLockComponentNotifications
    using EditorEntityLockComponentNotificationBus = AZ::EBus<EditorEntityLockComponentNotifications>;

    /// Alias for EditorEntityLockComponentNotifications - prefer EditorEntityLockComponentNotifications,
    /// EditorLockComponentNotifications is deprecated.
    using EditorLockComponentNotifications = EditorEntityLockComponentNotifications;

    /// Alias for EditorEntityLockComponentNotificationBus - prefer EditorEntityLockComponentNotificationBus,
    /// EditorLockComponentNotificationBus is deprecated.
    using EditorLockComponentNotificationBus = EditorEntityLockComponentNotificationBus;

} // namespace AzToolsFramework

DECLARE_EBUS_EXTERN(AzToolsFramework::EditorEntityLockComponentNotifications);
