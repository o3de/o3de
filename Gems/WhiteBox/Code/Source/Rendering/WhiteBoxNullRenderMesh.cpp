/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBox_precompiled.h"

#include "WhiteBoxNullRenderMesh.h"

namespace WhiteBox
{
    // TODO: LYN-786
    void WhiteBoxNullRenderMesh::BuildMesh(
        [[maybe_unused]] const WhiteBoxRenderData& renderData, [[maybe_unused]] const AZ::Transform& worldFromLocal,
        [[maybe_unused]] AZ::EntityId entityId)
    {
        // noop
    }

    void WhiteBoxNullRenderMesh::UpdateTransform([[maybe_unused]] const AZ::Transform& worldFromLocal)
    {
        // noop
    }

    void WhiteBoxNullRenderMesh::UpdateMaterial([[maybe_unused]] const WhiteBoxMaterial& material)
    {
        // noop
    }

    bool WhiteBoxNullRenderMesh::IsVisible() const
    {
        return false;
    }

    void WhiteBoxNullRenderMesh::SetVisiblity([[maybe_unused]] bool visibility)
    {
        // noop
    }
} // namespace WhiteBox
