/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Component/TickBus.h>

AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZCORE_API, AZ::TickEvents);
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZCORE_API, AZ::SystemTickEvents);
AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(AZCORE_API, AZ::TickRequests);
