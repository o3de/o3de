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
    //! Outcome object that returns an error message in case of failure to allow caller to handle internal errors.
    using ContainerEntityOperationResult = AZ::Outcome<void, AZStd::string>;

    //! ContainerEntityInterface
    //! An entity registered as Container is just like a regular entity when open. If its state is changed
    //! to closed, all descendants of the entity will be treated as part of the entity itself. Selecting any
    //! descendant will result in the container being selected, and descendants will be hidden until the
    //! container is opened.
    class ContainerEntityInterface
    {
    public:
        AZ_RTTI(ContainerEntityInterface, "{0A877C3A-726C-4FD2-BAFE-A2B9F1DE78E4}");

        //! Registers the entity as a container. The container will be closed by default.
        //! @param entityId The entityId that will be registered as a container.
        virtual ContainerEntityOperationResult RegisterEntityAsContainer(AZ::EntityId entityId) = 0;

        //! Unregisters the entity as a container.
        //! The system will retain the closed state in case the entity is registered again later, but
        //! if queried the entity will no longer behave as a container.
        //! @param entityId The entityId that will be unregistered as a container.
        virtual ContainerEntityOperationResult UnregisterEntityAsContainer(AZ::EntityId entityId) = 0;

        //! Returns whether the entity id provided is registered as a container.
        virtual bool IsContainer(AZ::EntityId entityId) const = 0;

        //! Sets the open state of the container entity provided.
        //! @param entityId The entityId whose open state will be set.
        //! @param open True if the container should be opened, false if it should be closed.
        //! @return An error message if the operation was invalid, success otherwise.
        virtual ContainerEntityOperationResult SetContainerOpen(AZ::EntityId entityId, bool open) = 0;

        //! If the entity id provided is registered as a container, it returns whether it's open.
        //! @note the default value for non-containers is true, so this function can be called without
        //! verifying whether the entityId is registered as a container beforehand, since the container's
        //! open behavior is exactly the same as the one of a regular entity.
        //! @return False if the entityId is registered as a container, and its state is closed. True otherwise.
        virtual bool IsContainerOpen(AZ::EntityId entityId) const = 0;

        //! Detects if one of the ancestors of entityId is a closed container entity.
        //! @return The highest closed entity container id if any, or entityId otherwise.
        virtual AZ::EntityId FindHighestSelectableEntity(AZ::EntityId entityId) const = 0;

        //! Clears all open state information for Container Entities for the EntityContextId provided.
        //! Used when context is switched, for example in the case of a new root prefab being loaded
        //! in place of an old one.
        //! @note Clear is meant to be called when no container is registered for the context provided.
        //! @return An error message if any container was registered for the context, success otherwise.
        virtual ContainerEntityOperationResult Clear(AzFramework::EntityContextId entityContextId) = 0;

        //! Returns true if one of the ancestors of entityId is a closed container entity.
        virtual bool IsUnderClosedContainerEntity(AZ::EntityId entityId) const = 0;

    };

} // namespace AzToolsFramework
