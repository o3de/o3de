#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            class SkinMeshData
                : public MeshData
            {
            public:
                AZ_RTTI(SkinMeshData, "{F765B68B-101E-4CED-879A-663AEDE6AE89}", MeshData);

                virtual ~SkinMeshData() override = default;
            };
        } // GraphData
    } // SceneData
} // AZ
