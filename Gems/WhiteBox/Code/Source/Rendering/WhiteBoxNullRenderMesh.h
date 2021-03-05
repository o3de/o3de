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

#include "WhiteBoxRenderMeshInterface.h"

namespace WhiteBox
{
    class WhiteBoxNullRenderMesh : public RenderMeshInterface
    {
    public:
        AZ_RTTI(WhiteBoxNullRenderMesh, "{99230D4D-5592-4A41-8BAB-60C1B7C1785D}", RenderMeshInterface);

        // RenderMeshInterface ...
        void BuildMesh(const WhiteBoxRenderData& renderData, const AZ::Transform& worldFromLocal, AZ::EntityId entityId)
            override; // TODO: LYN-786
        void UpdateTransform(const AZ::Transform& worldFromLocal) override;
        void UpdateMaterial(const WhiteBoxMaterial& material) override;
        bool IsVisible() const override;
        void SetVisiblity(bool visibility) override;
    };
} // namespace WhiteBox
