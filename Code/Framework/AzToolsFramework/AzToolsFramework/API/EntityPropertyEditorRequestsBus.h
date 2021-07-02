/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AzToolsFramework
{
    //! Requests to be made of all EntityPropertyEditorRequests
    //! Beware, there may be more than one EntityPropertyEditor that can respond
    //! Broadcast should be used for accessing these functions
    class EntityPropertyEditorRequests
        : public AZ::EBusTraits
    {
    public:
        //! Allows a component to get the list of selected entities or if in a pinned window, the list of entities in that window
        //! \param selectedEntityIds the return vector holding the entities required
        virtual void GetSelectedAndPinnedEntities(EntityIdList& selectedEntityIds) = 0;

        //! Allows a component to get the list of selected entities
        //! \param selectedEntityIds the return vector holding the entities required
        virtual void GetSelectedEntities(EntityIdList& selectedEntityIds) = 0;

        //! Explicitly sets a component as having been the most recently added.
        //! This means that the next time the UI refreshes, that component will be ensured to be visible.
        virtual void SetNewComponentId(AZ::ComponentId componentId) = 0;
    };

    using EntityPropertyEditorRequestBus = AZ::EBus<EntityPropertyEditorRequests>;
} // namespace AzToolsFramework
