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

#include <AzFramework/Entity/EntityContext.h>

#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabInstanceUtils.h>
#include <AzToolsFramework/Prefab/Template/Template.h>

namespace AzToolsFramework::Prefab
{
    using PrefabFocusOperationResult = AZ::Outcome<void, AZStd::string>;

    //! Interface to handle internal operations related to the Prefab Focus system.
    class PrefabFocusInterface
    {
    public:
        AZ_RTTI(PrefabFocusInterface, "{F3CFA37B-5FD8-436A-9C30-60EB54E350E1}");

        //! Initializes the editor interfaces for Prefab Focus mode.
        //! If this is not called on initialization, the Prefab Focus Mode functions will still work
        //! but won't trigger the Editor APIs to visualize focus mode on the UI.
        virtual void InitializeEditorInterfaces() = 0;

        //! Sets the focused instance to the owning instance of the entity id provided.
        //! If the entity id is invalid, then focus on the root prefab instance.
        //! @param EntityId The entity id of the entity whose owning instance we want the prefab system to focus on.
        //! @return PrefabFocusOperationResult that shows if the operation succeeds.
        virtual PrefabFocusOperationResult FocusOnPrefabInstanceOwningEntityId(AZ::EntityId entityId) = 0;

        //! Returns the template id of the currently focused instance.
        //! @param entityContextId The entity context id.
        //! @return TemplateId of the focused instance.
        virtual TemplateId GetFocusedPrefabTemplateId(AzFramework::EntityContextId entityContextId) const = 0;

        //! Returns a reference to the currently focused instance.
        //! @param EntityContextId The entity context id.
        //! @return The focused instance.
        virtual InstanceOptionalReference GetFocusedPrefabInstance(AzFramework::EntityContextId entityContextId) const = 0;

        //! Returns whether the currently focused prefab instance is read-only.
        //! @param EntityContextId The entity context id.
        //! @return True if the currently focused prefab instance is read-only, false otherwise.
        virtual bool IsFocusedPrefabInstanceReadOnly(AzFramework::EntityContextId entityContextId) const = 0;

        //! Appends the path from the focused instance to entity id into the provided patch array.
        //! @param providedPatch The provided path array to be appended to.
        //! @return LinkId stored in the instance closest to the focused instance in hierarchy.
        virtual LinkId AppendPathFromFocusedInstanceToPatchPaths(PrefabDom& providedPatch, AZ::EntityId entityId) const = 0;
    };
} // namespace AzToolsFramework::Prefab
