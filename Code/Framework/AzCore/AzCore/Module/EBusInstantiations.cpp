/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/BehaviorContext.h>

// RTTI buses
DECLARE_EBUS_INSTANTIATION_DLL_MULTI_ADDRESS(BehaviorContextEvents);
DECLARE_EBUS_INSTANTIATION_DLL_MULTI_ADDRESS(BehaviorObjectSignalsInterface);
