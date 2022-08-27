/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ::Render
{
    //! The ImGuiFeatureConfig menu provides a convenient developer-facing frontend for
    //! various renderer configs, exposed as cvars or settings registry entries.
    class ImGuiFeatureConfig
    {
    public:
        //! Draws the ImGuiFeatureConfig window
        void Draw(bool& draw);
    };
} // namespace AZ::Render
