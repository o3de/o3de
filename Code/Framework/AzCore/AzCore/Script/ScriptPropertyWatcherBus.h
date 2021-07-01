/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace AZ
{
    // Dummy interface to allow the keying to a be a bit more specific then void*
    class ScriptPropertyWatcher
    {
    public:
        ScriptPropertyWatcher() = default;
        ~ScriptPropertyWatcher() = default;
    };

    class ScriptPropertyWatcherInterface
        : public EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;        
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef ScriptPropertyWatcher* BusIdType;
        
        // Way of the ScriptProperty signalling it was modified.
        //
        // Only used on Properties that are complex and mutable(i.e. ScriptPropertyGenericClass, and ScriptPropertyTable)
        virtual void OnObjectModified() = 0;
    };
    
    typedef AZ::EBus<ScriptPropertyWatcherInterface> ScriptPropertyWatcherBus;
}
