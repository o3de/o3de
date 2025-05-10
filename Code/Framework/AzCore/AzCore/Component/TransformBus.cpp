/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>

AZCORE_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZ::TransformInterface);
AZCORE_INSTANTIATE_EBUS_MULTI_ADDRESS(AZ::TransformNotification);
AZCORE_INSTANTIATE_EBUS_MULTI_ADDRESS(AZ::TransformHierarchyInformation);
