/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Entity/EntityContextBus.h>

namespace AzToolsFramework
{
    //! FocusModeInterface
    //! Interface to handle the Editor Focus Mode.
    class FocusModeInterface
    {
    public:
        AZ_RTTI(FocusModeInterface, "{437243B0-F86B-422F-B7B8-4A21CC000702}");

        //! Sets the root entity the Editor should focus on.
        //! The Editor will only allow the user to select entities that are descendants of the EntityId provided.
        //! @param entityId The entityId that will become the new focus root.
        virtual void SetFocusRoot(AZ::EntityId entityId) = 0;

        //! Clears the Editor focus, allowing the user to select the whole level again.
        virtual void ClearFocusRoot(AzFramework::EntityContextId entityContextId) = 0;

        //! Returns the entity id of the root of the current Editor focus.
        //! @return The entity id of the root of the Editor focus, or an invalid entity id if no focus is set.
        virtual AZ::EntityId GetFocusRoot(AzFramework::EntityContextId entityContextId) = 0;

        //! Returns whether the entity id provided is part of the focused sub-tree.
        virtual bool IsInFocusSubTree(AZ::EntityId entityId) const = 0;
    };

} // namespace AzToolsFramework
