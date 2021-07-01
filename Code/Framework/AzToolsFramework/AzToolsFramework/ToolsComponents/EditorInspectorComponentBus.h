/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/Component.h>

namespace AzToolsFramework
{
    using ComponentOrderArray = AZStd::vector<AZ::ComponentId>;

    class EditorInspectorComponentRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Gets the current component sort order array
         * @return Container of component ids which is in the proper sorted order of components
         */
        virtual ComponentOrderArray GetComponentOrderArray() = 0;

        /**
         * Sets the current component sort order array
         * Emits EditorInspectorComponentNotifications::OnComponentOrderChanged if the new sort order differs from the current ordering.
         * @param componentOrderArray Container of component ids which is sorted in the desired sort order of components
         */
        virtual void SetComponentOrderArray(const ComponentOrderArray& componentOrderArray) = 0;
    };

    using EditorInspectorComponentRequestBus = AZ::EBus<EditorInspectorComponentRequests>;

    class EditorInspectorComponentNotifications
        : public AZ::ComponentBus
    {
    public:

        /**
        * Event fired when the order of components in the inspector has been changed
        */
        virtual void OnComponentOrderChanged() = 0;
    };

    using EditorInspectorComponentNotificationBus = AZ::EBus<EditorInspectorComponentNotifications>;
} // namespace AzToolsFramework
