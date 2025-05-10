/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/Windowing/WindowBus.h>

AZF_INSTANTIATE_EBUS_MULTI_ADDRESS(AzFramework::WindowRequests)
AZF_INSTANTIATE_EBUS_MULTI_ADDRESS(AzFramework::WindowNotifications)
AZF_INSTANTIATE_EBUS_MULTI_ADDRESS(AzFramework::ExclusiveFullScreenRequests)
AZF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzFramework::WindowSystemRequests)
AZF_INSTANTIATE_EBUS_SINGLE_ADDRESS(AzFramework::WindowSystemNotifications)
