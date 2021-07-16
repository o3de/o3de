/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "Core.h"

#include <AzCore/EBus/EBus.h>

namespace ScriptCanvas
{
    class Contract;
    class Slot;

    namespace Data
    {
        class Type;
    }

    class ContractEvents : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual void OnValidContract(const Contract*, const Slot&, const Slot&) = 0;
        virtual void OnInvalidContract(const Contract*, const Slot&, const Slot&) = 0;
    };

    using ContractEventBus = AZ::EBus<ContractEvents>;

}
