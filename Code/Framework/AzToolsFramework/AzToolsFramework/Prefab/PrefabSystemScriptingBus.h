/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Link/Link.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>
#include <AzToolsFramework/Prefab/Template/Template.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/EBus/EBus.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        // Bus that exposes a script-friendly interface to the PrefabSystemComponent
        struct PrefabSystemScriptingEbusTraits : AZ::EBusTraits
        {
            using MutexType = AZ::NullMutex;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

            virtual TemplateId CreatePrefabTemplate(
                const AZStd::vector<AZ::EntityId>& entityIds, const AZStd::string& filePath) = 0;
        };
        
        using PrefabSystemScriptingBus = AZ::EBus<PrefabSystemScriptingEbusTraits>;
        
    } // namespace Prefab
} // namespace AzToolsFramework

