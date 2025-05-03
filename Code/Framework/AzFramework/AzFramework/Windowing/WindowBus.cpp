/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/Windowing/WindowBus.h>

AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(AZF_API, AzFramework::WindowRequests)
AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(AZF_API, AzFramework::WindowNotifications)
AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(AZF_API, AzFramework::ExclusiveFullScreenRequests)
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZF_API, AzFramework::WindowSystemRequests)
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZF_API, AzFramework::WindowSystemNotifications)
