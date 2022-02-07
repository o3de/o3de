/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : IMaterial interface declaration.
#pragma once

#include <smartptr.h>
#include <AzCore/EBus/EBus.h>

struct IShader;
struct SShaderItem;

namespace AZ
{
    class MaterialNotificationEvents : public AZ::EBusTraits
    {
    public:
        virtual ~MaterialNotificationEvents() {}

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const bool EnableEventQueue = true;
        using EventQueueMutexType = AZStd::mutex;


        virtual void OnShaderLoaded([[maybe_unused]] IShader* shader) {}
    };
    using MaterialNotificationEventBus = AZ::EBus<MaterialNotificationEvents>;
}

struct IMaterial
{
    // TODO: Remove it!
    virtual ~IMaterial() {}
    virtual SShaderItem& GetShaderItem() = 0;
    virtual const SShaderItem& GetShaderItem() const = 0;
};
