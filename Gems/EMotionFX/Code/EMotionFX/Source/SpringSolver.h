/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Pose.h>

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string_view.h>

namespace EMotionFX
{
    class ActorInstance;
    class SimulatedJoint;
    class SimulatedObject;

    class EMFX_API SpringSolver
    {
    public:
        struct EMFX_API Spring
        {
            size_t m_particleA = ~0UL; /**< The first particle index. */
            size_t m_particleB = ~0UL; /**< The second particle index (the parent). */
            float m_restLength = 0.1f; /**< The rest length of the spring. */
            bool m_allowStretch = false; /**< Allow this spring to be stretched or compressed? (default=false). */
            bool m_isSupportSpring = false; /**< Is this spring a support spring? (default=false). */

            Spring() = default;
            Spring(size_t particleA, size_t particleB, float restLength, bool isSupportSpring, bool allowStretch)
                : m_restLength(restLength)
                , m_particleA(particleA)
                , m_particleB(particleB)
                , m_isSupportSpring(isSupportSpring)
                , m_allowStretch(allowStretch)
            {
            }
        };

        struct EMFX_API Particle
        {
            const SimulatedJoint* m_joint = nullptr;
            AZ::Vector3 m_pos = AZ::Vector3::CreateZero(); /**< The current (desired) particle position, in world space. */
            AZ::Vector3 m_oldPos = AZ::Vector3::CreateZero(); /**< The previous position of the particle. */
            AZ::Vector3 m_force = AZ::Vector3::CreateZero(); /**< The internal force, which contains the gravity and other pulling and pushing forces. */
            AZ::Vector3 m_externalForce = AZ::Vector3::CreateZero(); /**< A user defined external force, which is added on top of the internal force. Can be used to simulate wind etc. */
            AZ::Vector3 m_limitDir = AZ::Vector3::CreateZero(); /**< The joint limit direction vector, used for the cone angle limit. This is the center direction of the cone. */
            AZStd::vector<size_t> m_colliderExclusions; /**< Index values inside the collider array. Colliders listed in this list should be ignored durin collision detection. */
            size_t m_parentParticleIndex = InvalidIndex; /**< The parent particle index. */
        };

        class EMFX_API CollisionObject
        {
            friend class SpringSolver;

        public:
            enum class CollisionType
            {
                Sphere, /**< A sphere, which is described by a center position (m_start) and a radius. */
                Capsule /**< A capsule, which is described by a start (m_start) and end (m_end) position, and a diameter (m_radius). */
            };

            void SetupSphere(const AZ::Vector3& center, float radius);
            void SetupCapsule(const AZ::Vector3& startPos, const AZ::Vector3& endPos, float radius);

            AZ_INLINE CollisionType GetType() const { return m_type; }

        private:
            CollisionType m_type = CollisionType::Sphere; /**< The collision primitive type (a sphere, or capsule, etc). */
            size_t m_jointIndex = InvalidIndex; /**< The joint index to attach to, or ~0 for non-attached. */
            AZ::Vector3 m_globalStart = AZ::Vector3::CreateZero(); /**< The world space start position, or the world space center in case of a sphere. */
            AZ::Vector3 m_globalEnd = AZ::Vector3::CreateZero(); /**< The world space end position. This is ignored in case of a sphere. */
            AZ::Vector3 m_start = AZ::Vector3::CreateZero(); /**< The start of the primitive. In case of a sphere the center, in case of a capsule the start of the capsule. */
            AZ::Vector3 m_end = AZ::Vector3::CreateZero(); /**< The end position of the primitive. In case of a sphere this is ignored. */
            float m_radius = 1.0f; /**< The radius or thickness. */
            float m_scaledRadius = 1.0f; /**< The scaled radius value, scaled by the joint's world space transform. */
            const AzPhysics::ShapeColliderPair* m_shapePair = nullptr;
        };

        struct EMFX_API InitSettings
        {
            ActorInstance* m_actorInstance = nullptr; /**< The actor instance to initialize for. */
            const SimulatedObject* m_simulatedObject = nullptr; /**< The simulated object to use inside this solver. */
            AZStd::vector<AZStd::string> m_colliderTags; /**< The list of colliders to collide against. */
            AZStd::string m_name; /**< The name of the simulation, used during error and warning messages. */
        };

        using ParticleAdjustFunction = AZStd::function<void(Particle&)>;

        SpringSolver();

        bool Init(const InitSettings& initSettings);
        void Update(const Pose& inputPose, Pose& pose, float timePassedInSeconds);
        void DebugRender(const Pose& pose, bool renderColliders, bool renderLimits, const AZ::Color& color) const;
        void Stabilize();
        void Log();

        void AdjustParticles(const ParticleAdjustFunction& func);

        AZ_INLINE Particle& GetParticle(size_t index) { return m_particles[index]; }
        AZ_INLINE size_t GetNumParticles() const { return m_particles.size(); }
        AZ_INLINE Spring& GetSpring(size_t index) { return m_springs[index]; }
        AZ_INLINE size_t GetNumSprings() const { return m_springs.size(); }

        void SetParentParticle(size_t parentParticleIndex) { m_parentParticle = parentParticleIndex; }

        void SetGravity(const AZ::Vector3& gravity);
        const AZ::Vector3& GetGravity() const;
        void SetNumIterations(size_t numIterations);
        size_t GetNumIterations() const;

        Particle* AddJoint(const SimulatedJoint* joint);
        bool AddSupportSpring(size_t nodeA, size_t nodeB, float restLength = -1.0f);
        bool AddSupportSpring(AZStd::string_view nodeNameA, AZStd::string_view nodeNameB, float restLength = -1.0f);

        bool RemoveJoint(size_t jointIndex);
        bool RemoveJoint(AZStd::string_view nodeName);
        bool RemoveSupportSpring(size_t jointIndexA, size_t jointIndexB);
        bool RemoveSupportSpring(AZStd::string_view nodeNameA, AZStd::string_view nodeNameB);

        void SetStiffnessFactor(float factor) { m_stiffnessFactor = factor; }
        void SetGravityFactor(float factor) { m_gravityFactor = factor; }
        void SetDampingFactor(float factor) { m_dampingFactor = factor; }

        float GetStiffnessFactor() const { return m_stiffnessFactor; }
        float GetGravityFactor() const { return m_gravityFactor; }
        float GetDampingFactor() const { return m_dampingFactor; }

        size_t FindParticle(size_t jointIndex) const;
        Particle* FindParticle(AZStd::string_view nodeName);

        AZ_INLINE void RemoveCollisionObject(size_t index) { m_collisionObjects.erase(m_collisionObjects.begin() + index); }
        AZ_INLINE void RemoveAllCollisionObjects() { m_collisionObjects.clear(); }
        AZ_INLINE CollisionObject& GetCollisionObject(size_t index) { return m_collisionObjects[index]; }
        AZ_INLINE size_t GetNumCollisionObjects() const { return m_collisionObjects.size(); }
        AZ_INLINE bool GetCollisionEnabled() const { return m_collisionDetection; }
        AZ_INLINE void SetCollisionEnabled(bool enabled) { m_collisionDetection = enabled; }

    private:
        void InitColliders(const InitSettings& initSettings);
        void CreateCollider(size_t skeletonJointIndex, const AzPhysics::ShapeColliderPair& shapePair);
        void InitColliderFromColliderSetupShape(CollisionObject& collider);
        void InitCollidersFromColliderSetupShapes();
        bool RecursiveAddJoint(const SimulatedJoint* joint, size_t parentParticleIndex);
        void Integrate(float timeDelta);
        void CalcForces(const Pose& pose, float scaleFactor);
        void SatisfyConstraints(const Pose& inputPose, Pose& outPose, size_t numIterations, float scaleFactor);
        void Simulate(float deltaTime, const Pose& inputPose, Pose& outPose, float scaleFactor);
        void UpdateJointTransforms(Pose& pose);
        size_t AddParticle(const SimulatedJoint* joint);
        bool CollideWithSphere(AZ::Vector3& pos, const AZ::Vector3& center, float radius);
        bool CollideWithCapsule(AZ::Vector3& pos, const AZ::Vector3& lineStart, const AZ::Vector3& lineEnd, float radius);
        bool CheckIsInsideSphere(const AZ::Vector3& pos, const AZ::Vector3& center, float radius) const;    
        bool CheckIsInsideCapsule(const AZ::Vector3& pos, const AZ::Vector3& lineStart, const AZ::Vector3& lineEnd, float radius) const;
        void UpdateCollisionObjects(const Pose& pose, float scaleFactor);
        void UpdateCollisionObjectsModelSpace(const Pose& pose);
        bool PerformCollision(AZ::Vector3& inOutPos, float jointRadius, const Particle& particle);
        void PerformConeLimit(Particle& particleA, Particle& particleB, const AZ::Vector3& inputDir);
        bool CheckIsJointInsideCollider(const CollisionObject& colObject, const Particle& particle) const;
        void CheckAndExcludeCollider(size_t colliderIndex, const SimulatedJoint* joint);
        void UpdateFixedParticles(const Pose& pose);
        void Stabilize(const Pose& inputPose, Pose& pose, size_t numFrames=5);
        void InitAutoColliderExclusion();
        void InitAutoColliderExclusion(SimulatedJoint* joint);
        float GetScaleFactor() const;

        AZStd::vector<Spring> m_springs; /**< The collection of springs in the system. */
        AZStd::vector<Particle> m_particles; /**< The particles, which are connected by springs. */
        AZStd::vector<CollisionObject> m_collisionObjects; /**< The collection of collision objects. */
        AZStd::string m_name; /**< The name of the simulation. */
        AZ::Vector3 m_gravity = AZ::Vector3(0.0f, 0.0f, -9.81f); /**< The gravity force vector, which is (0.0f, 0.0f, -9.81f) on default. */
        ActorInstance* m_actorInstance = nullptr; /**< The actor instance we work on. */
        const SimulatedObject* m_simulatedObject = nullptr; /**< The simulated object we are simulating. */
        size_t m_numIterations = 2; /**< The number of iterations to run the constraint solving routines. The default is 2. */
        size_t m_parentParticle = InvalidIndex; /**< The parent particle of the one you add next. Set ot ~0 when there is no parent particle yet. */
        double m_lastTimeDelta = 0.0; /**< The previous time delta. */
        float m_stiffnessFactor = 1.0; /**< The factor that is applied to the stiffness of all joints. A value of 2 would make everything twice as stiff. */
        float m_gravityFactor = 1.0; /**< The factor that is applied to the gravity. A value of 2 would make the gravity twice as large. */
        float m_dampingFactor = 1.0; /**< The factor that is applied to the damping. A value of 2 would make the damping twice as large. */
        bool m_collisionDetection = true; /**< Perform collision detection? Default is true. */
        bool m_stabilize = true; /**< When set to true this will stabilize/warmup the simulation. */
    };
} // namespace EMotionFX
