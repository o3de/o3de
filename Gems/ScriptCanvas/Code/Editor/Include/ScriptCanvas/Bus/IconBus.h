/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/string/string.h>

namespace ScriptCanvasEditor
{
    class IconRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        
        virtual AZStd::string GetIconPath() const = 0;
    };
    
    using IconBus = AZ::EBus<IconRequests>;
}
