/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorSelectionAccentSystemComponent.h>

namespace AzToolsFramework
{
    /**
     * Bus for querying about sandbox data associated with a given Entity.
     */
    class ComponentEntityEditorRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~ComponentEntityEditorRequests() {}

        /// Returns true if the object is highlighted.
        virtual bool IsSandboxObjectHighlighted() = 0;

        // Sets accent for the component entity
        virtual void SetSandboxObjectAccent(EntityAccentType accent) = 0;

        // Set the component entity's isolation flag when the editor is in Isolation Mode
        virtual void SetSandBoxObjectIsolated(bool isIsolated) = 0;

        // Returns if the component entity is isolated when the editor is in Isolation Mode
        virtual bool IsSandBoxObjectIsolated() = 0;

        /// Updates the entity to match the visibility and lock state of its hierarchy.
        /// Necessary because ancestors that are layers can override the current entity's visibility and lock state.
        virtual void RefreshVisibilityAndLock() = 0;
    };

    using ComponentEntityEditorRequestBus = AZ::EBus < ComponentEntityEditorRequests >;

    class ComponentEntityObjectRequests
        : public AZ::EBusTraits
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef void* BusIdType; // ID'd on CComponentEntityObject pointer.
        //////////////////////////////////////////////////////////////////////////

        virtual ~ComponentEntityObjectRequests() {}

        /// Retrieve AZ Entity Id associated with this sandbox object.
        virtual AZ::EntityId GetAssociatedEntityId() = 0;
    };

    using ComponentEntityObjectRequestBus = AZ::EBus < ComponentEntityObjectRequests >;
} // namespace AzToolsFramework

