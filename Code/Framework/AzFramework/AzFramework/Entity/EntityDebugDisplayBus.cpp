/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(AZF_API, AzFramework::DebugDisplayRequests)
AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(AZF_API, AzFramework::EntityDebugDisplayEvents)
AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(AZF_API, AzFramework::ViewportDebugDisplayEvents)
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZF_API, AzFramework::DebugDisplayEvents)
