/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// carbonated begin (akostin/onframebeginend): Allow custom actions on begin/end frame
#if defined(CARBONATED)

#include <AzCore/EBus/EBus.h>

/*!
 * Requests to CrySystemPostTick
 */
class CrySystemPostTick
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // multi listener
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus

    virtual void OnFrameEnd() = 0;
};
using CrySystemPostTickBus = AZ::EBus<CrySystemPostTick>;

#endif
