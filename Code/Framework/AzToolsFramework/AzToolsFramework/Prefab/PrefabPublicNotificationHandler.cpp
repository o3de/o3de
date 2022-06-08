/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabPublicNotificationHandler.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        void PrefabPublicNotificationHandler::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<PrefabPublicNotificationBus>("PrefabPublicNotificationBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Prefab")
                    ->Attribute(AZ::Script::Attributes::Module, "prefab")
                    ->Handler<PrefabPublicNotificationHandler>()
                    ->Event("OnPrefabInstancePropagationBegin", &PrefabPublicNotifications::OnPrefabInstancePropagationBegin)
                    ->Event("OnPrefabInstancePropagationEnd", &PrefabPublicNotifications::OnPrefabInstancePropagationEnd)
                    ->Event("OnRootPrefabInstanceLoaded", &PrefabPublicNotifications::OnRootPrefabInstanceLoaded)
                    ;
            }
        }
    } // namespace Prefab
} // namespace AzToolsFramework
