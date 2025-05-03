/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/Physics/WindBus.h>

AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS_WITH_TRAITS(AZF_API,Physics::WindRequests, Physics::WindRequestsTraits)
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZF_API,Physics::WindNotifications)
