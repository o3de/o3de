/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
