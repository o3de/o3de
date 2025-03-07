/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/Physics/SystemBus.h>

AZF_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(Physics::DefaultWorldRequests)
AZF_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(Physics::EditorWorldRequests)
AZF_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS_WITH_TRAITS(Physics::SystemRequests, Physics::SystemRequestsTraits);
AZF_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(Physics::SystemDebugRequests);
