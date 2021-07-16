/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/tuple.h>

namespace EMotionFX
{
    class Mesh;

    class MeshFactory
    {
    public:
        using SkinInfluence = AZStd::tuple<size_t, float>;
        using VertexSkinInfluences = AZStd::vector<SkinInfluence>;

        static EMotionFX::Mesh* Create(
            const AZStd::vector<AZ::u32>& indices,
            const AZStd::vector<AZ::Vector3>& vertices,
            const AZStd::vector<AZ::Vector3>& normals,
            const AZStd::vector<AZ::Vector2>& uvs = {},
            const AZStd::vector<VertexSkinInfluences>& skinningInfo = {}
        );
    };
} // namespace EMotionFX
