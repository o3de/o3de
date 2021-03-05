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
#include <AzCore/EBus/EBus.h>

#pragma once

namespace AzToolsFramework
{
    using EntityIdList = AZStd::vector<AZ::EntityId>;

    /*!
     * Bus for notifications about entity transform changes from the editor viewport
     */
    class EditorTransformChangeNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~EditorTransformChangeNotifications() = default;

        /*!
         * Notification that the specified entities are about to have their transforms changed due to user interaction in the editor viewport
         *
         * \param entityIds Entities about to be changed
         */
        virtual void OnEntityTransformChanging(const AzToolsFramework::EntityIdList& /*entityIds*/) {};

        /*!
         * Notification that the specified entities had their transforms changed due to user interaction in the editor viewport
         *
         * \param entityIds Entities changed
         */
        virtual void OnEntityTransformChanged(const AzToolsFramework::EntityIdList& /*entityIds*/) {};
    };

    using EditorTransformChangeNotificationBus = AZ::EBus<EditorTransformChangeNotifications>;

} // namespace AzToolsFramework

