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
#include <AzCore/Component/Entity.h>

#include <AzCore/Component/ComponentBus.h>

namespace AzToolsFramework
{
    /**
     * This bus will notify handlers when an entity's "start active" flag has changed
     */
    class EditorEntityRuntimeActivationChangeNotifications 
        : public AZ::EBusTraits
    {
    public:
        virtual ~EditorEntityRuntimeActivationChangeNotifications() = default;
        
        virtual void OnEntityRuntimeActivationChanged(AZ::EntityId entityId, bool activeOnStart) = 0;
    };

    using EditorEntityRuntimeActivationChangeNotificationBus = AZ::EBus<EditorEntityRuntimeActivationChangeNotifications>;
}
