/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformId/PlatformDefaults.h>

// As the Platform defaults is needed within AzCore,
// those structures have been moved to AzCore and brought into
// The AzFramework namespace for backwards compatibility
namespace AzFramework
{
    using namespace AZ::PlatformDefaults;
}
