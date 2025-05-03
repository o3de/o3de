/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

 #include <AzFramework/Asset/AssetSystemBus.h>
  
 AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZF_API, AzFramework::AssetSystem::AssetSystemInfoNotifications)
 AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZF_API, AzFramework::AssetSystem::AssetSystemRequests)
 AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZF_API, AzFramework::AssetSystem::AssetSystemConnectionNotifications)
 AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZF_API, AzFramework::AssetSystem::AssetSystemStatus)
