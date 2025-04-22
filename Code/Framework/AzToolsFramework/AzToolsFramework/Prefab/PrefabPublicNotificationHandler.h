/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ReflectContext.h>

#include <AzToolsFramework/Prefab/PrefabPublicNotificationBus.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabPublicNotificationHandler final
            : public PrefabPublicNotificationBus::Handler
            , public AZ::BehaviorEBusHandler
        {
        public:

            AZ_EBUS_BEHAVIOR_BINDER(PrefabPublicNotificationHandler, "{F6F8C610-F780-45FA-8DC2-742E3FA427B5}", AZ::SystemAllocator,
                OnPrefabInstancePropagationBegin,
                OnPrefabInstancePropagationEnd,
                OnRootPrefabInstanceLoaded);

            static void Reflect(AZ::ReflectContext* context);

            void OnPrefabInstancePropagationBegin() override
            {
                Call(FN_OnPrefabInstancePropagationBegin);
            }

            void OnPrefabInstancePropagationEnd() override
            {
                Call(FN_OnPrefabInstancePropagationEnd);
            }

            void OnRootPrefabInstanceLoaded() override
            {
                Call(FN_OnRootPrefabInstanceLoaded);
            }
        };
    } // namespace Prefab
} // namespace AzToolsFramework
