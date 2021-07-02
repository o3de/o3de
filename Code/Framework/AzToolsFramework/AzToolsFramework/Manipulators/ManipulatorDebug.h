/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    class Vector3;
    class Transform;
} // namespace AZ

namespace AzFramework
{
    class DebugDisplayRequests;
} // namespace AzFramework

namespace AzToolsFramework
{
    void DrawAxis(
        AzFramework::DebugDisplayRequests& display, const AZ::Vector3& position, const AZ::Vector3& direction);

    void DrawTransformAxes(
        AzFramework::DebugDisplayRequests& display, const AZ::Transform& transform);
} // namespace AzToolsFramework
