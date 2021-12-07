/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ManipulatorSpace.h"

namespace AzToolsFramework
{
    AZ::Transform ApplySpace(const AZ::Transform& localTransform, const AZ::Transform& space, const AZ::Vector3& nonUniformScale)
    {
        AZ::Transform result;
        result.SetRotation(space.GetRotation() * localTransform.GetRotation());
        result.SetTranslation(space.TransformPoint(nonUniformScale * localTransform.GetTranslation()));
        result.SetUniformScale(space.GetUniformScale() * localTransform.GetUniformScale());
        return result;
    }

    const AZ::Transform& ManipulatorSpace::GetSpace() const
    {
        return m_space;
    }

    void ManipulatorSpace::SetSpace(const AZ::Transform& space)
    {
        m_space = space;
    }

    const AZ::Vector3& ManipulatorSpace::GetNonUniformScale() const
    {
        return m_nonUniformScale;
    }

    void ManipulatorSpace::SetNonUniformScale(const AZ::Vector3& nonUniformScale)
    {
        m_nonUniformScale = nonUniformScale;
    }

    AZ::Transform ManipulatorSpace::ApplySpace(const AZ::Transform& localTransform) const
    {
        return AzToolsFramework::ApplySpace(localTransform, m_space, m_nonUniformScale);
    }

    const AZ::Vector3& ManipulatorSpaceWithLocalPosition::GetLocalPosition() const
    {
        return m_localPosition;
    }

    void ManipulatorSpaceWithLocalPosition::SetLocalPosition(const AZ::Vector3& localPosition)
    {
        m_localPosition = localPosition;
    }

    const AZ::Vector3& ManipulatorSpaceWithLocalTransform::GetLocalPosition() const
    {
        return m_localTransform.GetTranslation();
    }

    void ManipulatorSpaceWithLocalTransform::SetLocalPosition(const AZ::Vector3& localPosition)
    {
        m_localTransform.SetTranslation(localPosition);
    }

    const AZ::Transform& ManipulatorSpaceWithLocalTransform::GetLocalTransform() const
    {
        return m_localTransform;
    }

    const AZ::Quaternion& ManipulatorSpaceWithLocalTransform::GetLocalOrientation() const
    {
        return m_localTransform.GetRotation();
    }

    void ManipulatorSpaceWithLocalTransform::SetLocalTransform(const AZ::Transform& localTransform)
    {
        m_localTransform = localTransform;
    }

    void ManipulatorSpaceWithLocalTransform::SetLocalOrientation(const AZ::Quaternion& localOrientation)
    {
        m_localTransform.SetRotation(localOrientation);
    }
} // namespace AzToolsFramework
