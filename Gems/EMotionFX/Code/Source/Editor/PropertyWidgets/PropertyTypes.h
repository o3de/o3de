/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>

namespace AzToolsFramework
{
    class PropertyHandlerBase;
}

namespace EMotionFX
{
    AZStd::vector<AzToolsFramework::PropertyHandlerBase*> RegisterPropertyTypes();
    void UnregisterPropertyTypes(const AZStd::vector<AzToolsFramework::PropertyHandlerBase*>& handlers);
} // namespace EMotionFX
