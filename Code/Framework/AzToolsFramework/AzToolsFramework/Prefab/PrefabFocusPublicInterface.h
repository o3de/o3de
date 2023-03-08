/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <AzCore/std/string/string.h>

#include <AzFramework/Entity/EntityContext.h>

#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Template/Template.h>

namespace AzToolsFramework::Prefab
{
    using PrefabFocusOperationResult = AZ::Outcome<void, AZStd::string>;

    enum class PrefabEditScope
    {
        HIDE_NESTED_INSTANCES_CONTENT = 0,
        SHOW_NESTED_INSTANCES_CONTENT
    };

    //! Public Interface for external systems to utilize the Prefab Focus system. This interface shouldn't expose
    //! internal prefab constructs like TemplateIds and prefab instances. For such cases, use the PrefabFocusInterface
    //! instead.
    class PrefabFocusPublicInterface
    {
    public:
        AZ_RTTI(PrefabFocusPublicInterface, "{53EE1D18-A41F-4DB1-9B73-9448F425722E}");

        //! Set the focused prefab instance to the owning instance of the entityId provided. Supports undo/redo.
        //! @param entityId The entityId of the entity whose owning instance we want the prefab system to focus on.
        virtual PrefabFocusOperationResult FocusOnOwningPrefab(AZ::EntityId entityId) = 0;

        //! Set the focused prefab instance to the parent of the currently focused prefab instance. Supports undo/redo.
        virtual PrefabFocusOperationResult FocusOnParentOfFocusedPrefab(AzFramework::EntityContextId entityContextId) = 0;

        //! Set the focused prefab instance to the instance at position index of the current path. Supports undo/redo.
        //! @param index The index of the instance in the current path that we want the prefab system to focus on.
        virtual PrefabFocusOperationResult FocusOnPathIndex(AzFramework::EntityContextId entityContextId, int index) = 0;

        //! Opens the prefab instance owning the entityId provided.Supports undo/redo.
        //! @param entityId The entityId of the entity whose owning instance we want to open for overrides.
        virtual PrefabFocusOperationResult SetOwningPrefabInstanceOpenState(AZ::EntityId entityId, bool openState) = 0;

        //! Returns the entity id of the container entity for the instance the prefab system is focusing on.
        virtual AZ::EntityId GetFocusedPrefabContainerEntityId(AzFramework::EntityContextId entityContextId) const = 0;

        //! Returns whether the entity belongs to the instance that is being focused on.
        //! @param entityId The entityId of the queried entity.
        //! @return true if the entity belongs to the focused instance, false otherwise.
        virtual bool IsOwningPrefabBeingFocused(AZ::EntityId entityId) const = 0;

        //! Returns whether the entity belongs to the instance that is being focused on, or one of its descendants.
        //! @param entityId The entityId of the queried entity.
        //! @return true if the entity belongs to the focused instance or one of its descendants, false otherwise.
        virtual bool IsOwningPrefabInFocusHierarchy(AZ::EntityId entityId) const = 0;

        //! Returns the path from the root instance to the currently focused instance.
        //! @return A path composed from the names of the container entities for the instance path.
        virtual const AZ::IO::Path& GetPrefabFocusPath(AzFramework::EntityContextId entityContextId) const = 0;

        //! Returns the size of the path to the currently focused instance.
        virtual const int GetPrefabFocusPathLength(AzFramework::EntityContextId entityContextId) const = 0;

        //! Sets the currently using prefab edit scope in editor.
        virtual void SetPrefabEditScope(AzFramework::EntityContextId entityContextId, PrefabEditScope prefabEditScope) = 0;
    };

    /**
     * The primary purpose of this bus is to facilitate writing automated tests for prefab focus mode.
     * If you would like to integrate prefabs focus mode into your system, please call PrefabFocusPublicInterface
     * for better performance.
     */
    class PrefabFocusPublicRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    };

    using PrefabFocusPublicRequestBus = AZ::EBus<PrefabFocusPublicInterface, PrefabFocusPublicRequests>;

} // namespace AzToolsFramework::Prefab
