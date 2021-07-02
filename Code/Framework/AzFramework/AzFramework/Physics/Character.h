/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>

#include <AzFramework/Physics/Shape.h>

#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/SimulatedBodyConfiguration.h>

namespace Physics
{
    class Character;

    class CharacterColliderNodeConfiguration
    {
    public:
        AZ_RTTI(Physics::CharacterColliderNodeConfiguration, "{C16F3301-0979-400C-B734-692D83755C39}");
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~CharacterColliderNodeConfiguration() = default;

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_name;
        AzPhysics::ShapeColliderPairList m_shapes;
    };

    class CharacterColliderConfiguration
    {
    public:
        AZ_RTTI(Physics::CharacterColliderConfiguration, "{4DFF1434-DF5B-4ED5-BE0F-D3E66F9B331A}");
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~CharacterColliderConfiguration() = default;

        CharacterColliderNodeConfiguration* FindNodeConfigByName(const AZStd::string& nodeName) const;
        AZ::Outcome<size_t> FindNodeConfigIndexByName(const AZStd::string& nodeName) const;

        void RemoveNodeConfigByName(const AZStd::string& nodeName);

        static void Reflect(AZ::ReflectContext* context);

        AZStd::vector<CharacterColliderNodeConfiguration> m_nodes;
    };

    /// Information required to create the basic physics representation of a character.
    class CharacterConfiguration
        : public AzPhysics::SimulatedBodyConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(CharacterConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(Physics::CharacterConfiguration, "{58D5A6CA-113B-4AC3-8D53-239DB0C4E240}", AzPhysics::SimulatedBodyConfiguration);

        virtual ~CharacterConfiguration() = default;

        static void Reflect(AZ::ReflectContext* context);

        AzPhysics::CollisionGroups::Id m_collisionGroupId; //!< Which layers does this character collide with.
        AzPhysics::CollisionLayer m_collisionLayer; //!< Which collision layer is this character on.
        MaterialSelection m_materialSelection; //!< Material selected from library for the body associated with the character.
        AZ::Vector3 m_upDirection = AZ::Vector3::CreateAxisZ(); //!< Up direction for character orientation and step behavior.
        float m_maximumSlopeAngle = 30.0f; //!< The maximum slope on which the character can move, in degrees.
        float m_stepHeight = 0.5f; //!< Affects what size steps the character can climb.
        float m_minimumMovementDistance = 0.001f; //!< To avoid jittering, the controller will not attempt to move distances below this.
        float m_maximumSpeed = 100.0f; //!< If the accumulated requested velocity for a tick exceeds this magnitude, it will be clamped.
        AZStd::string m_colliderTag; //!< Used to identify the collider associated with the character controller.
        AZStd::shared_ptr<Physics::ShapeConfiguration> m_shapeConfig; //!< The shape to use when creating the character controller.
        AZStd::vector<AZStd::shared_ptr<Physics::Shape>> m_colliders; //!< The list of colliders to attach to the character controller.
    };

    /// Basic implementation of common character-style needs as a WorldBody. Is not a full-functional ship-ready
    /// all-purpose character controller implementation. This class just abstracts some common functionality amongst
    /// typical characters, and is take-it-or-leave it style; useful as a starting point or reference.
    class Character
        : public AzPhysics::SimulatedBody
    {
    public:
        AZ_CLASS_ALLOCATOR(Character, AZ::SystemAllocator, 0);
        AZ_RTTI(Physics::Character, "{962E37A1-3401-4672-B896-0A6157CFAC97}", AzPhysics::SimulatedBody);

        ~Character() override = default;

        virtual AZ::Vector3 GetBasePosition() const = 0;
        virtual void SetBasePosition(const AZ::Vector3& position) = 0;
        virtual void SetRotation(const AZ::Quaternion& rotation) = 0;
        virtual AZ::Vector3 GetCenterPosition() const = 0;
        virtual float GetStepHeight() const = 0;
        virtual void SetStepHeight(float stepHeight) = 0;
        virtual AZ::Vector3 GetUpDirection() const = 0;
        virtual void SetUpDirection(const AZ::Vector3& upDirection) = 0;
        virtual float GetSlopeLimitDegrees() const = 0;
        virtual void SetSlopeLimitDegrees(float slopeLimitDegrees) = 0;
        virtual float GetMaximumSpeed() const = 0;
        virtual void SetMaximumSpeed(float maximumSpeed) = 0;
        virtual AZ::Vector3 GetVelocity() const = 0;
        virtual void SetCollisionLayer(const AzPhysics::CollisionLayer& layer) = 0;
        virtual void SetCollisionGroup(const AzPhysics::CollisionGroup& group) = 0;
        virtual AzPhysics::CollisionLayer GetCollisionLayer() const = 0;
        virtual AzPhysics::CollisionGroup GetCollisionGroup() const = 0;
        virtual AZ::Crc32 GetColliderTag() const = 0;

        /// Queues up a request to apply a velocity to the character.
        /// All requests received during a tick are accumulated (so for example, the effects of animation and gravity
        /// can be applied in two separate requests), and a movement with the accumulated velocity is performed once
        /// per tick, prior to the physics update.
        /// Obstacles may prevent the actual movement from exactly matching the requested movement.
        /// @param velocity The velocity to be added to the accumulated requests.
        virtual void AddVelocity(const AZ::Vector3& velocity) = 0;

        /// Applies the queued velocity requests and zeros the accumulated requested velocity.
        /// The expected usage is for this function to be called internally by the physics system once per tick,
        /// so that the cumulative result of multiple movement effects (e.g. animation, gravity, pseudo-impulses etc)
        /// can be combined from separate calls to AddVelocity. Accumulating the requests avoids performing
        /// multiple expensive character updates, and avoids any effects from the order of requests within a tick.
        /// Users who wish to add a new movement effect should generally just be able to use AddVelocity, and
        /// rely on the existing physics system call to ApplyRequestedVelocity.
        /// @param deltaTime The duration over which to apply the accumulated requested velocity.
        virtual void ApplyRequestedVelocity(float deltaTime) = 0;

        virtual void AttachShape(AZStd::shared_ptr<Physics::Shape> shape) = 0;
    };
} // namespace Physics
