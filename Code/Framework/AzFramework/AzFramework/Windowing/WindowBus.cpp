/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/Windowing/WindowBus.h>

AZF_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(AzFramework::WindowRequests)
AZF_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(AzFramework::WindowNotifications)
AZF_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(AzFramework::ExclusiveFullScreenRequests)
AZF_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AzFramework::WindowSystemRequests)
AZF_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AzFramework::WindowSystemNotifications)
