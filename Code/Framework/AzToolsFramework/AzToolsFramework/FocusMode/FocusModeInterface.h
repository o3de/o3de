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


namespace AzToolsFramework::FocusModeFramework
{
    /*!
     * FocusModeInterface
     * Interface to handle the Editor Focus Mode.
     */
    class FocusModeInterface
    {
    public:
        AZ_RTTI(FocusModeInterface, "{437243B0-F86B-422F-B7B8-4A21CC000702}");

        //! Sets the root entity the Editor should focus on.
        //! The Editor will only allow the user to select entities belonging to the sub-tree that has root in the entityId provided.
        //! @param entityId The entityId that will become the new focus root.
        virtual void SetFocusRoot(AZ::EntityId entityId) = 0;

        //! Returns the entity id of the root of the current Editor focus.
        //! @return The id of the entity that is the root of the Editor focus.
        virtual AZ::EntityId GetFocusRoot() = 0;

        //! Returns whether the entity id provided is part of the focused sub-tree.
        //! @return True if entityId belongs to the focused sub-tree, false otherwise.
        virtual bool IsInFocusSubTree(AZ::EntityId entityId) = 0;
    };

} // namespace AzToolsFramework::FocusModeFrameworkclassPrefabEditPublicInterface
