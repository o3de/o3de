/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Template/Template.h>

namespace AzToolsFramework::Prefab
{
    typedef AZ::Outcome<void, AZStd::string> PrefabFocusOperationResult;

    /*!
     * PrefabFocusInterface
     */
    class PrefabFocusInterface
    {
    public:
        AZ_RTTI(PrefabFocusInterface, "{F3CFA37B-5FD8-436A-9C30-60EB54E350E1}");

        //! Set the focused prefab instance to the owning instance of the entityId provided
        //! @param entityId The entityId of the entity whose owning instance we want the prefab system to focus on.
        virtual PrefabFocusOperationResult FocusOnOwningPrefab(AZ::EntityId entityId) = 0;

        //! Returns the template id of the instance the prefab system is focusing on.
        virtual TemplateId GetFocusedPrefabTemplateId() = 0;

        //! Returns a reference to the instance the prefab system is focusing on.
        virtual InstanceOptionalReference GetFocusedPrefabInstance() = 0;

        //! Returns whether the entity belongs to the instance that is being focused on, or one of its descendants.
        //! @param entityId The entityId of the queried entity.
        //! @return true if the entity belongs to the focused instance or one of its descendants, false otherwise.
        virtual bool IsOwningPrefabBeingFocused(AZ::EntityId entityId) = 0;
    };

} // namespace AzToolsFramework::Prefab
