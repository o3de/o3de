/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Math/Transform.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    class Quaternion;
} // namespace AZ

namespace AzToolsFramework
{
    AZ::Transform ApplySpace(const AZ::Transform& localTransform, const AZ::Transform& space, const AZ::Vector3& nonUniformScale);

    //! Handles location for manipulators which have a global space but no local transformation.
    class ManipulatorSpace
    {
    public:
        AZ_TYPE_INFO(ManipulatorSpace, "{5D4B8974-8F98-4268-8D6B-3214A77C6382}")
        AZ_CLASS_ALLOCATOR(ManipulatorSpace, AZ::SystemAllocator, 0)

        const AZ::Transform& GetSpace() const;
        void SetSpace(const AZ::Transform& space);
        const AZ::Vector3& GetNonUniformScale() const;
        void SetNonUniformScale(const AZ::Vector3& nonUniformScale);

        //! Calculates a transform combining the space and local transform, taking non-uniform scale into account.
        AZ::Transform ApplySpace(const AZ::Transform& localTransform) const;

    private:
        AZ::Transform m_space = AZ::Transform::CreateIdentity(); //!< Space the manipulator is in.
        AZ::Vector3 m_nonUniformScale = AZ::Vector3::CreateOne(); //!< Handles non-uniform scale for the space the manipulator is in.
    };

    //! Handles location for manipulators which have a global space and a local position, but no local rotation.
    class ManipulatorSpaceWithLocalPosition : public ManipulatorSpace
    {
    public:
        AZ_TYPE_INFO(ManipulatorSpaceWithLocalPosition, "{47BE15AF-60A8-436B-8F3F-7DDFB97220E6}")
        AZ_CLASS_ALLOCATOR(ManipulatorSpaceWithLocalPosition, AZ::SystemAllocator, 0)

        const AZ::Vector3& GetLocalPosition() const;
        void SetLocalPosition(const AZ::Vector3& localPosition);

    private:
        AZ::Vector3 m_localPosition = AZ::Vector3::CreateZero(); //!< Position in local space.
    };

    //! Handles location for manipulators which have a global space and a local transform (position and rotation).
    class ManipulatorSpaceWithLocalTransform : public ManipulatorSpace
    {
    public:
        AZ_TYPE_INFO(ManipulatorSpaceWithLocalTransform, "{6D100797-1DD8-45B0-A21C-8893B770C0BC}")
        AZ_CLASS_ALLOCATOR(ManipulatorSpaceWithLocalTransform, AZ::SystemAllocator, 0)

        const AZ::Vector3& GetLocalPosition() const;
        void SetLocalPosition(const AZ::Vector3& localPosition);

        const AZ::Transform& GetLocalTransform() const;
        const AZ::Quaternion& GetLocalOrientation() const;
        void SetLocalTransform(const AZ::Transform& localTransform);
        void SetLocalOrientation(const AZ::Quaternion& localOrientation);

    private:
        AZ::Transform m_localTransform = AZ::Transform::CreateIdentity(); //!< Local transform.
    };
} // namespace AzToolsFramework
