/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    namespace Render
    {
        static constexpr const char* const DiffuseProbeGridComponentTypeId = "{9B900A04-192F-4F5E-AE31-762605D8159A}";
        static constexpr const char* const EditorDiffuseProbeGridComponentTypeId = "{F80086E1-ECE7-4E8C-B727-A750D10F7D83}";
        static constexpr float DefaultDiffuseProbeGridSpacing = 2.0f;
        static constexpr float DefaultDiffuseProbeGridExtents = 8.0f;
        static constexpr float DefaultDiffuseProbeGridAmbientMultiplier = 1.0f;
        static constexpr float DefaultDiffuseProbeGridViewBias = 0.2f;
        static constexpr float DefaultDiffuseProbeGridNormalBias = 0.1f;
    } // namespace Render
} // namespace AZ
