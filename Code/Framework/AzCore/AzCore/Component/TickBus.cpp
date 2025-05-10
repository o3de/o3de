/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Component/TickBus.h>

AZCORE_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZ::TickEvents);
AZCORE_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZ::SystemTickEvents);
AZCORE_INSTANTIATE_EBUS_SINGLE_ADDRESS(AZ::TickRequests);
