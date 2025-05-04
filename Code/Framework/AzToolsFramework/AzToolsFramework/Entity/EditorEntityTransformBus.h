/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Entity/EntityTypes.h>
#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

namespace AzToolsFramework
{
    //! Notifications about entity transform changes from the editor.
    class EditorTransformChangeNotifications : public AZ::EBusTraits
    {
    public:
        //! A notification that these entities had their transforms changed due to a user interaction in the editor.
        //! @param entityIds Entities that had their transform changed.
        virtual void OnEntityTransformChanged([[maybe_unused]] const AzToolsFramework::EntityIdList& entityIds)
        {
        }

    protected:
        ~EditorTransformChangeNotifications() = default;
    };

    using EditorTransformChangeNotificationBus = AZ::EBus<EditorTransformChangeNotifications>;
} // namespace AzToolsFramework

AZ_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS(AZTF_API, AzToolsFramework::EditorTransformChangeNotifications);
