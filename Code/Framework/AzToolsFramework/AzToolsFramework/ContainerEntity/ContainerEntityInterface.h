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

namespace AzToolsFramework
{
    using ContainerEntityOperationResult = AZ::Outcome<void, AZStd::string>;

    //! ContainerEntityInterface
    //! Interface to register and query container entities.
    class ContainerEntityInterface
    {
    public:
        AZ_RTTI(ContainerEntityInterface, "{0A877C3A-726C-4FD2-BAFE-A2B9F1DE78E4}");

        //! Registers the entity as a container. The container will be closed by default.
        //! @param entityId The entityId that will be registered as a container.
        virtual ContainerEntityOperationResult RegisterEntityAsContainer(AZ::EntityId entityId) = 0;

        //! Unregisters the entity as a container.
        //! @param entityId The entityId that will be unregistered as a container.
        virtual ContainerEntityOperationResult UnregisterEntityAsContainer(AZ::EntityId entityId) = 0;

        //! Returns whether the entity id provided is registered as a container.
        virtual bool IsContainer(AZ::EntityId entityId) const = 0;

        //! Sets the open state of the container entity provided.
        //! @param entityId The entityId whose open state will be set.
        //! @param open True if the container should be opened, false if it should be closed.
        //! @return An error message if the operation was invalid, success otherwise.
        virtual ContainerEntityOperationResult SetContainerOpenState(AZ::EntityId entityId, bool open) = 0;

        //! If the entity id provided is registered as a container, it returns whether it's open.
        //! Note that the default value for non-containers is true, so this function can be called without
        //! verifying whether the entityId is registered as a container beforehand, since the container's
        //! open behavior is exactly the same as the one of a regular entity.
        //! @return False if the entityId is registered as a container, and its state is closed. True otherwise.
        virtual bool IsContainerOpen(AZ::EntityId entityId) const = 0;
    };

} // namespace AzToolsFramework
