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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_OBJECTMANAGER_BUS_H
#define CRYINCLUDE_EDITOR_OBJECTS_OBJECTMANAGER_BUS_H
#pragma once

#include <AzCore/EBus/EBus.h>

//////////////////////////////////////////////////////////////////////////
// EBus to handle object manager events
//////////////////////////////////////////////////////////////////////////
namespace AZ
{
    class ObjectManagerEvents
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        virtual ~ObjectManagerEvents() = default;

        virtual void OnExportingStarting() {}
        virtual void OnExportingFinished() {}
    };

    using ObjectManagerEventBus = AZ::EBus<ObjectManagerEvents>;
}

#endif // CRYINCLUDE_EDITOR_OBJECTS_OBJECTMANAGER_BUS_H
