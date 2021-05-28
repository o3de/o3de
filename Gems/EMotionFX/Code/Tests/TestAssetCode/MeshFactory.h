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
