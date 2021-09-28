/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelectionRequestBus.h>

namespace AZ
{
    class Aabb;
}

namespace AzFramework
{
    struct CameraState;
}

namespace AzToolsFramework
{
    //! Is the pivot at the center of the object (middle of extents) or at the
    //! exported authored object root position.
    inline bool Centered(const EditorTransformComponentSelectionRequests::Pivot pivot)
    {
        return pivot == EditorTransformComponentSelectionRequests::Pivot::Center;
    }

    //! Return offset from object pivot to center if center is true, otherwise Vector3::Zero.
    AZ::Vector3 CalculateCenterOffset(AZ::EntityId entityId, EditorTransformComponentSelectionRequests::Pivot pivot);

    //! Calculate scale factor based on distance from camera
    float CalculateScreenToWorldMultiplier(const AZ::Vector3& worldPosition, const AzFramework::CameraState& cameraState);

    //! Given a mouse interaction, determine if the pick ray from its position
    //! in screen space intersected an aabb in world space.
    bool AabbIntersectMouseRay(const ViewportInteraction::MouseInteraction& mouseInteraction, const AZ::Aabb& aabb);

    //! Wrapper to perform an intersection between a ray and an aabb.
    //! Note: direction should be normalized (it is scaled internally by the editor pick distance).
    bool AabbIntersectRay(const AZ::Vector3& origin, const AZ::Vector3& direction, const AZ::Aabb& aabb, float& distance);

    //! Return if a mouse interaction (pick ray) did intersect the tested EntityId.
    bool PickEntity(
        AZ::EntityId entityId, const ViewportInteraction::MouseInteraction& mouseInteraction, float& closestDistance, int viewportId);

    //! Wrapper for EBus call to return the CameraState for a given viewport.
    AzFramework::CameraState GetCameraState(int viewportId);

    //! Wrapper for EBus call to return the DPI scaling for a given viewport.
    float GetScreenDisplayScaling(int viewportId);

    //! A utility to return the center of several points.
    //! Take several positions and store the min and max of each in
    //! turn - when all points have been added return the center/midpoint.
    class MidpointCalculator
    {
    public:
        //! Default constructed with min and max initialized to opposites.
        MidpointCalculator() = default;

        //! Call this for all positions you want to be considered.
        void AddPosition(const AZ::Vector3& position)
        {
            m_minPosition = position.GetMin(m_minPosition);
            m_maxPosition = position.GetMax(m_maxPosition);
        }

        //! Once all positions have been added, call this to return the midpoint.
        AZ::Vector3 CalculateMidpoint() const
        {
            return m_minPosition + (m_maxPosition - m_minPosition) * 0.5f;
        }

    private:
        AZ::Vector3 m_minPosition = AZ::Vector3(std::numeric_limits<float>::max());
        AZ::Vector3 m_maxPosition = AZ::Vector3(-std::numeric_limits<float>::max());
    };
} // namespace AzToolsFramework
