/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Utils/ImGuiFeatureConfig.h>

#include <imgui/imgui.h>

namespace AZ::Render
{
    void ImGuiFeatureConfig::Draw(bool& draw)
    {
        if (ImGui::Begin("Feature Config", &draw, 0))
        {
        }
    }
} // namespace AZ::Render
