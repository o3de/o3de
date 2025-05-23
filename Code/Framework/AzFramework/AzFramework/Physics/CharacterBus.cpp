/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

 #include <AzFramework/Physics/CharacterBus.h>

 AZ_INSTANTIATE_EBUS_MULTI_ADDRESS(AZF_API, Physics::CharacterRequests);
 AZ_INSTANTIATE_EBUS_MULTI_ADDRESS(AZF_API, Physics::CharacterNotifications);
