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
