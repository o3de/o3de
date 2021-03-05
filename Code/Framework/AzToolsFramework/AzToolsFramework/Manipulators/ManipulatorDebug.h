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
