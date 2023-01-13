/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ComponentModes/ViewportEditUtilities.h>

namespace AzToolsFramework
{
    template <typename T>
    AZ::Vector3 GetPositionInManipulatorFrame(float worldUniformScale, const AZ::Transform& manipulatorLocalTransform,
        const T& action)
    {
        return manipulatorLocalTransform.GetInverse().TransformPoint(
            action.m_start.m_localPosition +
            action.LocalPositionOffset() / AZ::GetClamp(worldUniformScale, AZ::MinTransformScale, AZ::MaxTransformScale));
    }
} // namespace AzToolsFramework
