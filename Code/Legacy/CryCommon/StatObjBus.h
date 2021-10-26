/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/unordered_set.h>

//////////////////////////////////////////////////////////////////////////
//
// EBUS support for triggering necessary updates when IStatObj instances
// caches should be updated when 3D Engine events happen during level loads,
// shutting down the application, and so forth
//
//////////////////////////////////////////////////////////////////////////
class InstanceStatObjEvents
    : public AZ::EBusTraits
{
public:
    virtual ~InstanceStatObjEvents() = default;

    // AZ::EBusTraits
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
    using MutexType = AZStd::recursive_mutex;

    virtual void ReleaseData()
    {
    }
};

using InstanceStatObjEventBus = AZ::EBus<InstanceStatObjEvents>;
