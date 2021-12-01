/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Entity/EntityContextBus.h>

namespace AzToolsFramework
{
    //! An entity registered as read-only cannot be altered in the editor.
    class ReadOnlyEntityInterface
    {
    public:
        AZ_RTTI(ReadOnlyEntityInterface, "{921FE15B-6EBD-47F0-8238-BC63318DEDEA}");

        //! Registers the entity as read-only.
        //! @param entityId The entityId that will be registered as read-only.
        virtual void RegisterEntityAsReadOnly(AZ::EntityId entityId) = 0;

        //! Registers the entities as read-only.
        //! @param entityIds The entityIds that will be registered as read-only.
        virtual void RegisterEntitiesAsReadOnly(EntityIdList entityIds) = 0;

        //! Unregisters the entity as read-only.
        //! @param entityId The entityId that will be unregistered as read-only.
        virtual void UnregisterEntityAsReadOnly(AZ::EntityId entityId) = 0;

        //! Unregisters the entities as read-only.
        //! @param entityId The entityIds that will be unregistered as read-only.
        virtual void UnregisterEntitiesAsReadOnly(EntityIdList entityIds) = 0;

        //! Returns whether the entity id provided is registered as read-only.
        virtual bool IsReadOnly(AZ::EntityId entityId) const = 0;
    };

} // namespace AzToolsFramework
