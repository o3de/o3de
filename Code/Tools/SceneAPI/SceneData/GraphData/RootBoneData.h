/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneData/GraphData/BoneData.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneData
    {
        namespace GraphData
        {
            class RootBoneData
                : public AZ::SceneData::GraphData::BoneData
            {
            public:
                AZ_RTTI(RootBoneData, "{EB1FCB42-77A2-4EBA-B70B-8BB1B6948355}", AZ::SceneData::GraphData::BoneData);

                virtual ~RootBoneData() override = default;

                static void Reflect(ReflectContext* context);
            };
        } // namespace GraphData
    } // namespace SceneData
} // namespace AZ
