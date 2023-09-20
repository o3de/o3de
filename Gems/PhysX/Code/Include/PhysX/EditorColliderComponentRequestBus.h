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
        virtual AZ::Vector3 GetColliderOffset() const = 0;

        /// Gets the rotation of the collider relative to the entity rotation.
        /// @param rotation The rotation of the collider
        virtual void SetColliderRotation(const AZ::Quaternion& rotation) = 0;

        /// Gets the rotation of the collider relative to the entity rotation.
        /// @param rotation The rotation of the collider
        virtual AZ::Quaternion GetColliderRotation() const = 0;

        /// Gets the world transform of the collider.
        /// @return The world transform of the collider.
        virtual AZ::Transform GetColliderWorldTransform() const = 0;

        /// O3DE_DEPRECATION_NOTICE(GHI-14717)
        /// Sets the radius of the sphere collider.
        /// @param radius The radius of the sphere collider.
        AZ_DEPRECATED(virtual void SetSphereRadius(float) {}, "Functionality moved to EditorPrimitiveColliderComponentRequests")

        /// O3DE_DEPRECATION_NOTICE(GHI-14717)
        /// Gets the radius of the sphere collider.
        /// @return The radius of the sphere collider.
        AZ_DEPRECATED(virtual float GetSphereRadius() const { return 0.0f; }, "Functionality moved to EditorPrimitiveColliderComponentRequests")

        /// O3DE_DEPRECATION_NOTICE(GHI-14717)
        /// Sets the radius of the capsule collider.
        /// @param radius The radius of the capsule collider.
        AZ_DEPRECATED(virtual void SetCapsuleRadius(float) {}, "Functionality moved to EditorPrimitiveColliderComponentRequests")

        /// O3DE_DEPRECATION_NOTICE(GHI-14717)
        /// Gets the radius of the capsule collider.
        /// @return The radius of the capsule collider.
        AZ_DEPRECATED(virtual float GetCapsuleRadius() const { return 0.0f; }, "Functionality moved to EditorPrimitiveColliderComponentRequests")

        /// O3DE_DEPRECATION_NOTICE(GHI-14717)
        /// Sets the height of the capsule collider.
        /// @param radius The height of the capsule collider.
        AZ_DEPRECATED(virtual void SetCapsuleHeight(float) {}, "Functionality moved to EditorPrimitiveColliderComponentRequests")

        /// O3DE_DEPRECATION_NOTICE(GHI-14717)
        /// Gets the height of the capsule collider.
        /// @return The height of the capsule collider.
        AZ_DEPRECATED(virtual float GetCapsuleHeight() const { return 0.0f; }, "Functionality moved to EditorPrimitiveColliderComponentRequests")

        /// O3DE_DEPRECATION_NOTICE(GHI-14717)
        /// Sets the radius of the cylinder collider.
        /// @param radius The radius of the cylinder collider.
        AZ_DEPRECATED(virtual void SetCylinderRadius(float) {}, "Functionality moved to EditorPrimitiveColliderComponentRequests")

        /// O3DE_DEPRECATION_NOTICE(GHI-14717)
        /// Gets the radius of the cylinder collider.
        /// @return The radius of the cylinder collider.
        AZ_DEPRECATED(virtual float GetCylinderRadius() const { return 0.0f; }, "Functionality moved to EditorPrimitiveColliderComponentRequests")

        /// O3DE_DEPRECATION_NOTICE(GHI-14717)
        /// Sets the height of the cylinder collider.
        /// @param radius The height of the cylinder collider.
        AZ_DEPRECATED(virtual void SetCylinderHeight(float) {}, "Functionality moved to EditorPrimitiveColliderComponentRequests")

        /// O3DE_DEPRECATION_NOTICE(GHI-14717)
        /// Gets the height of the cylinder collider.
        /// @return The height of the cylinder collider.
        AZ_DEPRECATED(virtual float GetCylinderHeight() const { return 0.0f; }, "Functionality moved to EditorPrimitiveColliderComponentRequests")

        /// O3DE_DEPRECATION_NOTICE(GHI-14717)
        /// Sets the subdivision count of the cylinder collider.
        /// @param radius The subdivision count of the cylinder collider.
        AZ_DEPRECATED(virtual void SetCylinderSubdivisionCount(AZ::u8) {}, "Functionality moved to EditorPrimitiveColliderComponentRequests")

        /// O3DE_DEPRECATION_NOTICE(GHI-14717)
        /// Gets the subdivision count of the cylinder collider.
        /// @return The subdivision count of the cylinder collider.
        AZ_DEPRECATED(virtual AZ::u8 GetCylinderSubdivisionCount() const { return 0; }, "Functionality moved to EditorPrimitiveColliderComponentRequests")

        /// O3DE_DEPRECATION_NOTICE(GHI-14717)
        /// Sets the scale of the asset collider.
        /// @param The scale of the asset collider.
        AZ_DEPRECATED(virtual void SetAssetScale(const AZ::Vector3&) {}, "Functionality moved to EditorMeshColliderComponentRequests")

        /// O3DE_DEPRECATION_NOTICE(GHI-14717)
        /// Gets the scale of the asset collider.
        /// @return The scale of the asset collider.
        AZ_DEPRECATED(virtual AZ::Vector3 GetAssetScale() const { return AZ::Vector3::CreateOne(); }, "Functionality moved to EditorMeshColliderComponentRequests")

        /// O3DE_DEPRECATION_NOTICE(GHI-14717)
        /// Sets the shape type on the collider.
        /// @param The collider's shape type.
        AZ_DEPRECATED(virtual void SetShapeType(Physics::ShapeType) {}, "Functionality moved to EditorPrimitiveColliderComponentRequests")

        /// Gets the shape type from the collider.
        /// @return The collider's shape type.
        virtual Physics::ShapeType GetShapeType() const = 0;
    };

    using EditorColliderComponentRequestBus = AZ::EBus<EditorColliderComponentRequests>;
    
    //! Request bus for colliders using primitive shapes.
    class EditorPrimitiveColliderComponentRequests
        : public AZ::EntityComponentBus
    {
    public:
        //! Sets the shape type on the collider.
        //! @param The collider's shape type.
        virtual void SetShapeType(Physics::ShapeType) = 0;

        //! Set the X/Y/Z dimensions of the box collider.
        //! @param dimensions The X/Y/Z dimensions of the box collider.
        virtual void SetBoxDimensions(const AZ::Vector3& dimensions) = 0;

        //! Get the X/Y/Z dimensions of the box collider.
        //! @return The X/Y/Z dimensions of the box collider.
        virtual AZ::Vector3 GetBoxDimensions() const = 0;

        //! Sets the radius of the sphere collider.
        //! @param radius The radius of the sphere collider.
        virtual void SetSphereRadius(float radius) = 0;

        //! Gets the radius of the sphere collider.
        //! @return The radius of the sphere collider.
        virtual float GetSphereRadius() const = 0;

        //! Sets the radius of the capsule collider.
        //! @param radius The radius of the capsule collider.
        virtual void SetCapsuleRadius(float radius) = 0;

        //! Gets the radius of the capsule collider.
        //! @return The radius of the capsule collider.
        virtual float GetCapsuleRadius() const = 0;

        //! Sets the height of the capsule collider.
        //! @param radius The height of the capsule collider.
        virtual void SetCapsuleHeight(float height) = 0;

        //! Gets the height of the capsule collider.
        //! @return The height of the capsule collider.
        virtual float GetCapsuleHeight() const = 0;

        //! Sets the radius of the cylinder collider.
        //! @param radius The radius of the cylinder collider.
        virtual void SetCylinderRadius(float radius) = 0;

        //! Gets the radius of the cylinder collider.
        //! @return The radius of the cylinder collider.
        virtual float GetCylinderRadius() const = 0;

        //! Sets the height of the cylinder collider.
        //! @param radius The height of the cylinder collider.
        virtual void SetCylinderHeight(float height) = 0;

        //! Gets the height of the cylinder collider.
        //! @return The height of the cylinder collider.
        virtual float GetCylinderHeight() const = 0;

        //! Sets the subdivision count of the cylinder collider.
        //! @param radius The subdivision count of the cylinder collider.
        virtual void SetCylinderSubdivisionCount(AZ::u8 subdivisionCount) = 0;

        //! Gets the subdivision count of the cylinder collider.
        //! @return The subdivision count of the cylinder collider.
        virtual AZ::u8 GetCylinderSubdivisionCount() const = 0;
    };

    using EditorPrimitiveColliderComponentRequestBus = AZ::EBus<EditorPrimitiveColliderComponentRequests>;

    //! Request bus for colliders using PhysX mesh assets.
    class EditorMeshColliderComponentRequests
        : public AZ::EntityComponentBus
    {
    public:
        //! Sets the scale of the asset collider.
        //! @param The scale of the asset collider.
        virtual void SetAssetScale(const AZ::Vector3&) = 0;

        //! Gets the scale of the asset collider.
        //! @return The scale of the asset collider.
        virtual AZ::Vector3 GetAssetScale() const = 0;
    };

    using EditorMeshColliderComponentRequestBus = AZ::EBus<EditorMeshColliderComponentRequests>;

    /// O3DE_DEPRECATION_NOTICE(GHI-14717)
    /// <EditorColliderValidationRequests>
    /// Bus used to validate that non-convex meshes are not used with simulation types which do not support them.
    /// </EditorColliderValidationRequests>
    class AZ_DEPRECATED(EditorColliderValidationRequests, "Functionality moved to EditorMeshColliderValidationRequests")
        : public AZ::ComponentBus
    {
    public:
        /// Checks if the the mesh in the collider is correct with the current state of the Rigidbody.
        virtual void ValidateRigidBodyMeshGeometryType() = 0;
    };

    AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations", "-Wdeprecated-declarations");
    using EditorColliderValidationRequestBus = AZ::EBus<EditorColliderValidationRequests>;
    AZ_POP_DISABLE_WARNING;
    
    //! Bus used to validate that non-convex meshes are not used with simulation types which do not support them.
    class EditorMeshColliderValidationRequests : public AZ::ComponentBus
    {
    public:
        //! Checks if the the mesh in the collider is correct with the current state of the Rigidbody.
        virtual void ValidateRigidBodyMeshGeometryType() = 0;
    };

    using EditorMeshColliderValidationRequestBus = AZ::EBus<EditorMeshColliderValidationRequests>;
}


