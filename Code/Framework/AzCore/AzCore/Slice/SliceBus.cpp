/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Slice/SliceBus.h>

AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZCORE_API, AZ::SliceInstanceEvents);
AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZCORE_API, AZ::SliceAssetSerializationNotifications);
AZ_INSTANTIATE_EBUS_MULTI_ADDRESS(AZCORE_API, AZ::SliceEntityHierarchyInterface);
