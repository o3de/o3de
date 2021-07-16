/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Crc.h>
#include <AzCore/std/containers/vector.h>

namespace AzToolsFramework
{
    class PropertyHandlerBase;
}

namespace NvCloth
{
    namespace Editor
    {
        const static AZ::Crc32 MeshNodeSelector = AZ_CRC("MeshNodeSelector", 0x50f06073);

        AZStd::vector<AzToolsFramework::PropertyHandlerBase*> RegisterPropertyTypes();
        void UnregisterPropertyTypes(AZStd::vector<AzToolsFramework::PropertyHandlerBase*>& handlers);
    } // namespace Editor
} // namespace NvCloth
