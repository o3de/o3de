/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
