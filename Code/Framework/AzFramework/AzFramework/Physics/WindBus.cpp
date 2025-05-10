/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/Physics/WindBus.h>

AZF_INSTANTIATE_EBUS_SINGLE_ADDRESS_WITH_TRAITS(Physics::WindRequests, Physics::WindRequestsTraits)
AZF_INSTANTIATE_EBUS_SINGLE_ADDRESS(Physics::WindNotifications)
