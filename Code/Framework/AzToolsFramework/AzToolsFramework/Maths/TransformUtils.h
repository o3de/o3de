/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>

namespace AzToolsFramework
{
    /**
     * @brief Take a transform and return it with normalized scale.
     */
    inline AZ::Transform TransformNormalizedScale(const AZ::Transform& transform)
    {
        AZ::Transform transformNormalizedScale = transform;
        transformNormalizedScale.SetUniformScale(1.0f);
        return transformNormalizedScale;
    }

    /**
     * @brief Take a transform and return it with uniform scale - choose the largest element.
     */
    inline AZ::Transform TransformUniformScale(const AZ::Transform& transform)
    {
        AZ::Transform transformUniformScale = transform;
        transformUniformScale.SetUniformScale(transformUniformScale.GetUniformScale());
        return transformUniformScale;
    }

    /**
     * @brief Retrieve the orientation/rotation of a transform after removing any scaling from the transform
     */
    inline AZ::Quaternion QuaternionFromTransformNoScaling(const AZ::Transform& transform)
    {
        return transform.GetRotation().GetNormalized();
    }

    /**
     * @brief Transform a direction by a transform, removing all scaling (if any) before applying
     * the rotation and translation.
     */
    inline AZ::Vector3 TransformDirectionNoScaling(const AZ::Transform& transform, const AZ::Vector3& direction)
    {
        return (transform.GetRotation().TransformVector(direction)).GetNormalizedSafe();
    }

    /**
     * @brief Transform a position by a transform, removing all scaling (if any) before applying
     * the rotation and translation.
     */
    inline AZ::Vector3 TransformPositionNoScaling(const AZ::Transform& transform, const AZ::Vector3& position)
    {
        return transform.GetTranslation() + QuaternionFromTransformNoScaling(transform).TransformVector(position);
    }
} // namespace AzToolsFramework
