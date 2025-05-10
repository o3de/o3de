/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/AzCoreAPI.h>

// RTTI buses
AZCORE_INSTANTIATE_EBUS_MULTI_ADDRESS(AZ::BehaviorContextEvents);
AZCORE_INSTANTIATE_EBUS_MULTI_ADDRESS(AZ::BehaviorObjectSignalsInterface);
