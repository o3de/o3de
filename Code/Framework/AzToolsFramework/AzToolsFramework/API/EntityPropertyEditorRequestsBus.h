/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
    };

    using EntityPropertyEditorRequestBus = AZ::EBus<EntityPropertyEditorRequests>;
} // namespace AzToolsFramework
