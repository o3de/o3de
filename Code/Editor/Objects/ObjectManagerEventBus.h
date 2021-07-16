/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
