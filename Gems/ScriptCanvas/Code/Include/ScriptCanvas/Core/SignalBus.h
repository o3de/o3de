/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Core.h"

#include <AzCore/EBus/EBus.h>

namespace ScriptCanvas
{
    enum class ExecuteMode
    {
        Normal,
        UntilNodeIsFoundInStack
    };

    class SignalInterface : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef ID BusIdType;
        //////////////////////////////////////////////////////////////////////////
                
        virtual void SignalInput(const SlotId& slot) = 0;
        virtual void SignalOutput(const SlotId& slot, ExecuteMode mode = ExecuteMode::Normal) = 0;
    };

    using SignalBus = AZ::EBus<SignalInterface>;
}
