/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Physics/ShapeConfiguration.h>

namespace PhysX
{
    /// EntityColliderComponentRequests
    class EditorColliderComponentRequests
        : public AZ::EntityComponentBus
    {
    public:

        /// Sets the offset of the collider relative to the entity position.
        /// @param offset The offset of the collider
        virtual void SetColliderOffset(const AZ::Vector3& offset) = 0;

        /// Gets the offset of the collider relative to the entity position.
        /// @param offset The offset of the collider
        virtual AZ::Vector3 GetColliderOffset() = 0;

        /// Gets the rotation of the collider relative to the entity rotation.
        /// @param rotation The rotation of the collider
        virtual void SetColliderRotation(const AZ::Quaternion& rotation) = 0;

        /// Gets the rotation of the collider relative to the entity rotation.
        /// @param rotation The rotation of the collider
        virtual AZ::Quaternion GetColliderRotation() = 0;

        /// Gets the world transform of the collider.
        /// @return The world transform of the collider.
        virtual AZ::Transform GetColliderWorldTransform() = 0;
        
        /// Set the radius of the sphere collider.
        /// @param radius The radius of the sphere collider.
        virtual void SetSphereRadius(float radius) = 0;

        /// Gets the radius of the sphere collider.
        /// @return The radius of the sphere collider.
        virtual float GetSphereRadius() = 0;

        /// Set the radius of the capsule collider.
        /// @param radius The radius of the capsule collider.
        virtual void SetCapsuleRadius(float radius) = 0;

        /// Gets the radius of the capsule collider.
        /// @return The radius of the capsule collider.
        virtual float GetCapsuleRadius() = 0;

        /// Set the height of the capsule collider.
        /// @param radius The height of the capsule collider.
        virtual void SetCapsuleHeight(float height) = 0;

        /// Gets the height of the capsule collider.
        /// @return The height of the capsule collider.
        virtual float GetCapsuleHeight() = 0;

        /// Sets the scale of the asset collider.
        /// @param The scale of the asset collider.
        virtual void SetAssetScale(const AZ::Vector3&) = 0;

        /// Gets the scale of the asset collider.
        /// @return The scale of the asset collider.
        virtual AZ::Vector3 GetAssetScale() = 0;

        /// Sets the shape type on the collider.
        /// @param The collider's shape type.
        virtual void SetShapeType(Physics::ShapeType) = 0;

        /// Gets the shape type from the collider.
        /// @return The collider's shape type.
        virtual Physics::ShapeType GetShapeType() = 0;
    };

    using EditorColliderComponentRequestBus = AZ::EBus<EditorColliderComponentRequests>;

    /// <EditorColliderValidationRequests>
    /// This is a Bus in order to communicate the status of the meshes of the collider and avoid dependencies with the rigidbody
    /// </EditorColliderValidationRequests>
    class EditorColliderValidationRequests : public AZ::ComponentBus
    {
    public:
        /// Checks if the the mesh in the collider is correct with the current state of the Rigidbody!
        virtual void ValidateRigidBodyMeshGeometryType() = 0;
    };

    using EditorColliderValidationRequestBus = AZ::EBus<EditorColliderValidationRequests>;
}


