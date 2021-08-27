/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
