/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class Frustum;
    class Transform;
    struct ViewFrustumAttributes;
} // namespace AZ

namespace AzFramework
{
    class DebugDisplayRequests;

    void DisplayFrustum(const AZ::Frustum& frustum, DebugDisplayRequests& debugDisplay);

    void DisplayFrustum(const AZ::ViewFrustumAttributes& viewFrustumAttribs, DebugDisplayRequests& debugDisplay);

    void DisplayFrustum(
        const AZ::Transform& worldFromView, float aspect, float fovRadians, float nearClip, float farClip,
        DebugDisplayRequests& debugDisplay);

    struct OctreeDebug
    {
        AZStd::vector<AZ::Aabb> m_entryAabbsInBounds;
        AZStd::vector<AZ::Aabb> m_entryAabbsInFrustum;
        AZStd::vector<AZ::Aabb> m_nodeBounds;

        void Clear()
        {
            m_entryAabbsInBounds.clear();
            m_entryAabbsInFrustum.clear();
            m_nodeBounds.clear();
        }
    };

    void DisplayOctreeDebug(const OctreeDebug& octreeDebug, DebugDisplayRequests& debugDisplay);

} // namespace AzFramework
