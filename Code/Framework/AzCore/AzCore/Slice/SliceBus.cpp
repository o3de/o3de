/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Slice/SliceBus.h>

AZCORE_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZ::SliceInstanceEvents);
AZCORE_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZ::SliceAssetSerializationNotifications);
AZCORE_INSTANTIATE_EBUS_MULTI_ADDRESS(AZ::SliceEntityHierarchyInterface);
