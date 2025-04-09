/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AzToolsFramework
{
    class ComponentEditor;

    //! Requests to be made of all EntityPropertyEditorRequests
    //! Beware, there may be more than one EntityPropertyEditor that can respond
    //! Broadcast should be used for accessing these functions
    class EntityPropertyEditorRequests
        : public AZ::EBusTraits
    {
    public:
        using VisitComponentEditorsCallback = AZStd::function<bool(const ComponentEditor*)>;

        //! Returns the list of selected entities or if in a pinned window, the list of entities in that window
        //! \param selectedEntityIds the return vector holding the entities required
        virtual void GetSelectedAndPinnedEntities(EntityIdList& selectedEntityIds) = 0;

        //! Returns the list of selected entities
        //! \param selectedEntityIds the return vector holding the entities required
        virtual void GetSelectedEntities(EntityIdList& selectedEntityIds) = 0;

        //! Returns the list of selected components
        //! \param selectedEntityIds the return vector holding the entities required
        virtual void GetSelectedComponents(AZStd::unordered_set<AZ::EntityComponentIdPair>& selectedComponentEntityIds) = 0;

        //! Explicitly sets a component as having been the most recently added.
        //! This means that the next time the UI refreshes, that component will be ensured to be visible.
        virtual void SetNewComponentId(AZ::ComponentId componentId) = 0;

        //! Visits the component editors in an EntityPropertyEditor via a callback.
        //! @param callback The callback that iterates over all the component editors within an EntityPropertyEditor.
        virtual void VisitComponentEditors(const VisitComponentEditorsCallback& callback) const = 0;
    };

    using EntityPropertyEditorRequestBus = AZ::EBus<EntityPropertyEditorRequests>;
} // namespace AzToolsFramework
