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
    class ReadOnlyEntityPublicInterface
    {
    public:
        AZ_RTTI(ReadOnlyEntityPublicInterface, "{921FE15B-6EBD-47F0-8238-BC63318DEDEA}");

        //! Returns whether the entity id provided is registered as read-only.
        virtual bool IsReadOnly(const AZ::EntityId& entityId) = 0;
    };

    //! An entity registered as read-only cannot be altered in the editor.
    class ReadOnlyEntityQueryInterface
    {
    public:
        AZ_RTTI(ReadOnlyEntityQueryInterface, "{2ACD63C5-1F3E-4DE8-880E-8115F857D329}");

        //! Refreshes the cached read-only status for the entities provided.
        //! @param entityIds The entityIds whose read-only state will be queried again.
        virtual void RefreshReadOnlyState(const EntityIdList& entityIds) = 0;

        //! Refreshes the cached read-only status for all entities.
        //! Useful when disconnecting a handler at runtime.
        virtual void RefreshReadOnlyStateForAllEntities() = 0;
    };

} // namespace AzToolsFramework
