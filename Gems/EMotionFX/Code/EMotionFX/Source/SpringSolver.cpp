/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <EMotionFX/Source/SpringSolver.h>
#include <EMotionFX/Source/TransformData.h>

namespace EMotionFX
{
    //------------------------------------------------------------------------
    // SpringSolver::CollisionObject
    //------------------------------------------------------------------------
    void SpringSolver::CollisionObject::SetupSphere(const AZ::Vector3& center, float radius)
    {
        m_start = center;
        m_end = center;
        m_radius = radius;
        m_type = CollisionType::Sphere;
    }

    void SpringSolver::CollisionObject::SetupCapsule(const AZ::Vector3& startPos, const AZ::Vector3& endPos, float radius)
    {
        m_start = startPos;
        m_end = endPos;
        m_radius = radius;
        m_type = CollisionType::Capsule;
    }

    //------------------------------------------------------------------------
    // SpringSolver
    //------------------------------------------------------------------------
    SpringSolver::SpringSolver()
    {
        m_springs.reserve(3);
        m_particles.reserve(5);
        m_collisionObjects.reserve(3);
    }

    void SpringSolver::CreateCollider(size_t skeletonJointIndex, const AzPhysics::ShapeColliderPair& shapePair)
    {
        const Physics::ShapeConfiguration* shapeConfig = shapePair.second.get();
        if (!shapeConfig)
        {
            return;
        }

        if (shapeConfig->GetShapeType() != Physics::ShapeType::Sphere && shapeConfig->GetShapeType() != Physics::ShapeType::Capsule)
        {
            AZ_Error("EMotionFX", false, "Unsupported collider shape type in simulated object solver '%s'. Only spheres and capsules are supported.", m_name.c_str());
            return;
        }

        // Add a new collider to the solver and initialize it from the collider setup shape.
        m_collisionObjects.emplace_back();
        CollisionObject& colObject = m_collisionObjects.back();
        colObject.m_jointIndex = skeletonJointIndex;
        colObject.m_shapePair = &shapePair;
        InitColliderFromColliderSetupShape(colObject);

        // Register this collider index in all particle collider joint exclusion list.
        const AZStd::string& colliderTag = shapePair.first->m_tag;
        for (Particle& particle : m_particles)
        {
            const AZStd::vector<AZStd::string>& exclusionList = particle.m_joint->GetColliderExclusionTags();
            for (const AZStd::string& exclusionColliderTag : exclusionList)
            {
                if (exclusionColliderTag == colliderTag)
                {
                    const size_t colliderIndex = m_collisionObjects.size() - 1;
                    particle.m_colliderExclusions.emplace_back(colliderIndex);
                    break;
                }
            }
        }
    }

    void SpringSolver::InitColliders(const InitSettings& initSettings)
    {
        const Actor* actor = m_actorInstance->GetActor();
        const PhysicsSetup* physicsSetup = actor->GetPhysicsSetup().get();
        if (!physicsSetup)
        {
            return;
        }

        const Physics::CharacterColliderConfiguration& colliderSetup = physicsSetup->GetSimulatedObjectColliderConfig();
        for (const AZStd::string& colliderTag : initSettings.m_colliderTags)
        {
            bool colliderFound = false;
            for (const auto& nodeConfig : colliderSetup.m_nodes) 
            {
                for (const AzPhysics::ShapeColliderPair& shapePair : nodeConfig.m_shapes)
                {
                    if (shapePair.first->m_tag == colliderTag)
                    {
                        // Make sure we can find the joint in the skeleton.
                        size_t skeletonJointIndex;
                        if (!actor->GetSkeleton()->FindNodeAndIndexByName(nodeConfig.m_name, skeletonJointIndex))
                        {
                            AZ_Warning("EMotionFX", false, "Cannot find joint '%s' to attach the collider to. Skipping this collider inside simulation '%s'.", nodeConfig.m_name.c_str(), m_name.c_str());
                            continue;
                        }

                        colliderFound = true;
                        CreateCollider(skeletonJointIndex, shapePair);
                    }
                }
            }

            if (!colliderFound)
            {
                AZ_Warning("EMotionFX", false, "Cannot find referenced collider(s) with tag '%s' inside the collider setup for simulation '%s'. Skipping this collider.", colliderTag.c_str(), m_name.c_str());
                continue;
            }
        }
    }

    void SpringSolver::InitColliderFromColliderSetupShape(CollisionObject& collider)
    {
        const Physics::ColliderConfiguration* colliderConfig = collider.m_shapePair->first.get();
        const Physics::ShapeConfiguration* shapeConfig = collider.m_shapePair->second.get();
        const Physics::ShapeType shapeType = shapeConfig->GetShapeType();
        const AZ::Transform colliderOffset = AZ::Transform::CreateFromQuaternionAndTranslation(colliderConfig->m_rotation, colliderConfig->m_position);

        switch (shapeType)
        {
            case Physics::ShapeType::Sphere:
            {
                const Physics::SphereShapeConfiguration* sphereConfig = static_cast<const Physics::SphereShapeConfiguration*>(shapeConfig);
                collider.SetupSphere(colliderConfig->m_position, sphereConfig->m_radius);
            }
            break;

            case Physics::ShapeType::Capsule:
            {
                const Physics::CapsuleShapeConfiguration* capsuleConfig = static_cast<const Physics::CapsuleShapeConfiguration*>(shapeConfig);
                const float halfHeight = capsuleConfig->m_height * 0.5f;
                const AZ::Vector3 start = AZ::Vector3(0.0f, 0.0f, -halfHeight + capsuleConfig->m_radius);
                const AZ::Vector3 end = AZ::Vector3(0.0f, 0.0f, halfHeight - capsuleConfig->m_radius);
                collider.SetupCapsule(colliderOffset.TransformPoint(start), colliderOffset.TransformPoint(end), capsuleConfig->m_radius);
            }
            break;

            default:
                AZ_Assert(false, "Unsupported collider shape type in spring solver.");
        }
    }

    void SpringSolver::InitCollidersFromColliderSetupShapes()
    {
        for (CollisionObject& collider : m_collisionObjects)
        {
            InitColliderFromColliderSetupShape(collider);
        }
    }

    void SpringSolver::InitAutoColliderExclusion()
    {
        const Pose* bindPose = m_actorInstance->GetTransformData()->GetBindPose();
        UpdateCollisionObjectsModelSpace(*bindPose);

        for (SimulatedJoint* joint : m_simulatedObject->GetSimulatedJoints())
        {
            InitAutoColliderExclusion(joint);
        }
    }

    void SpringSolver::CheckAndExcludeCollider(size_t colliderIndex, const SimulatedJoint* joint)
    {
        const size_t particleIndex = FindParticle(joint->GetSkeletonJointIndex());
        AZ_Assert(particleIndex != InvalidIndex, "Expected particle to be found for this joint.");
        if (particleIndex == InvalidIndex)
        {
            return;
        }

        Particle& particle = m_particles[particleIndex];
        const CollisionObject& colObject = m_collisionObjects[static_cast<size_t>(colliderIndex)];

        const bool needsExclusion = joint->IsGeometricAutoExclusion() ? CheckIsJointInsideCollider(colObject, particle) : true;
        if (needsExclusion)
        {
            if (AZStd::find(particle.m_colliderExclusions.begin(), particle.m_colliderExclusions.end(), colliderIndex) == particle.m_colliderExclusions.end())
            {
                particle.m_colliderExclusions.emplace_back(colliderIndex);
            }
        }
    }

    void SpringSolver::InitAutoColliderExclusion(SimulatedJoint* joint)
    {
        switch (joint->GetAutoExcludeMode())
        {
            // All colliders that intersect this joint will be excluded.
            case SimulatedJoint::AutoExcludeMode::All:
            {
                const size_t numColliders = m_collisionObjects.size();
                for (size_t colliderIndex = 0; colliderIndex < numColliders; ++colliderIndex)
                {
                    CheckAndExcludeCollider(colliderIndex, joint);
                }
                break;
            }

            // Only colliders on this joint will be excluded if they intersect this joint.
            case SimulatedJoint::AutoExcludeMode::Self:
            {
                const size_t numColliders = m_collisionObjects.size();
                for (size_t colliderIndex = 0; colliderIndex < numColliders; ++colliderIndex)
                {
                    if (m_collisionObjects[colliderIndex].m_jointIndex == joint->GetSkeletonJointIndex())
                    {
                        CheckAndExcludeCollider(colliderIndex, joint);
                    }
                }
                break;
            }

            // This joint and its parent and child joints colliders are tested for intersection, and excluded if intersecting.
            case SimulatedJoint::AutoExcludeMode::SelfAndNeighbors:
            {
                const size_t numColliders = m_collisionObjects.size();
                for (size_t colliderIndex = 0; colliderIndex < numColliders; ++colliderIndex)
                {
                    if (joint->GetSkeletonJointIndex() == m_collisionObjects[colliderIndex].m_jointIndex)
                    {
                        CheckAndExcludeCollider(colliderIndex, joint);
                    }

                    const SimulatedJoint* parentJoint = joint->FindParentSimulatedJoint();
                    if (parentJoint && parentJoint->GetSkeletonJointIndex() == m_collisionObjects[colliderIndex].m_jointIndex)
                    {
                        CheckAndExcludeCollider(colliderIndex, joint);
                    }

                    const size_t numChildJoints = joint->CalculateNumChildSimulatedJoints();
                    for (size_t childIndex = 0; childIndex < numChildJoints; ++childIndex)
                    {
                        const SimulatedJoint* childJoint = joint->FindChildSimulatedJoint(childIndex);
                        if (m_collisionObjects[colliderIndex].m_jointIndex == childJoint->GetSkeletonJointIndex())
                        {
                            CheckAndExcludeCollider(colliderIndex, joint);
                        }
                    }
                }
                break;
            }

            // We do nothing, pure manual exclusion list setup.
            case SimulatedJoint::AutoExcludeMode::None:
            {
                break;
            }

            default:
                AZ_Assert(false, "Unsupported auto collider exclusion mode.");
        }
    }

    SpringSolver::Particle* SpringSolver::AddJoint(const SimulatedJoint* joint)
    {
        AZ_Assert(joint, "Expected the joint be a valid pointer.");
        const size_t jointIndex = joint->GetSkeletonJointIndex();
        if (jointIndex == InvalidIndex)
        {
            return nullptr;
        }

        size_t particleA = FindParticle(jointIndex);
        if (particleA == InvalidIndex)
        {
            particleA = AddParticle(joint);
        }
        else
        {
            return &m_particles[particleA];
        }

        if (m_parentParticle != InvalidIndex)
        {
            Spring newSpring;
            newSpring.m_particleA = particleA;
            newSpring.m_particleB = m_parentParticle;
            newSpring.m_restLength = (m_particles[particleA].m_pos - m_particles[m_parentParticle].m_pos).GetLength();
            if (newSpring.m_restLength < AZ::Constants::FloatEpsilon)
            {
                return &m_particles[m_parentParticle];
            }
            newSpring.m_allowStretch = false;
            m_springs.emplace_back(newSpring);
        }

        m_parentParticle = particleA;
        return &m_particles[particleA];
    }

    bool SpringSolver::RecursiveAddJoint(const SimulatedJoint* joint, size_t parentParticleIndex)
    {
        SetParentParticle(parentParticleIndex);

        // Register the joint, which creates a particle internally.
        SpringSolver::Particle* particle = AddJoint(joint);
        if (!particle)
        {
            AZ_Error("EMotionFX", false, "Failed to find skeletal joint for simulated joint for simulation '%s'. Disabling simulated object '%s'.", m_name.c_str(), m_simulatedObject->GetName().c_str());
            return false;
        }

        AZ_Assert(joint->GetMass() > AZ::Constants::FloatEpsilon, "Expected mass of the joint to be greater than zero.");

        // Locate the particle index in the sim, and set that as parent.
        // This is done because we create a spring between the current parent and the new joint.
        parentParticleIndex = FindParticle(joint->GetSkeletonJointIndex());
        AZ_Assert(parentParticleIndex != InvalidIndex, "Expected particle index to be a valid index.");

        // Add all child joints.
        const size_t numChildJoints = joint->CalculateNumChildSimulatedJoints();
        for (size_t i = 0; i < numChildJoints; ++i)
        {
            const SimulatedJoint* child = joint->FindChildSimulatedJoint(i);
            AZ_Assert(child, "Expected to find the child simulated joint.");
            if (!RecursiveAddJoint(child, parentParticleIndex))
            {
                return false;
            }
        }

        return true;
    }

    bool SpringSolver::Init(const InitSettings& settings)
    {
        AZ_Assert(settings.m_actorInstance, "Expecting a valid actor instance to initialize the solver for.");
        AZ_Assert(settings.m_simulatedObject, "Expecting a valid simulated object pointer to initialize the solver for.");
        m_actorInstance = settings.m_actorInstance;
        m_simulatedObject = settings.m_simulatedObject;
        m_name = settings.m_name;

        // Don't allow empty objects.
        const size_t numRootJoints = m_simulatedObject->GetNumSimulatedRootJoints();
        if (numRootJoints == 0)
        {
            //AZ_Warning("EMotionFX", false, "Simulated object '%s' in simulation '%s' has a simulated object without any joints. A minimum of two joints per chain is required.", m_simulatedObject->GetName().c_str(), m_name.c_str());
            return false;
        }

        // Make sure we have at least 2 joint chains.
        for (size_t rootIndex = 0; rootIndex < numRootJoints; ++rootIndex)
        {
            const SimulatedJoint* rootJoint = m_simulatedObject->GetSimulatedRootJoint(rootIndex);
            if (rootJoint->CalculateNumChildSimulatedJoints() == 0)
            {
                AZ_Warning("EMotionFX", false, "Simulated object '%s' in simulation '%s' has a chain with just one joint. A minimum of two joints per chain is required.", m_simulatedObject->GetName().c_str(), m_name.c_str());
                return false;
            }
        }

        // Add all simulated joints to the solver for this simulated object.
        for (size_t rootIndex = 0; rootIndex < numRootJoints; ++rootIndex)
        {
            SimulatedJoint* rootJoint = m_simulatedObject->GetSimulatedRootJoint(rootIndex);
            rootJoint->SetPinned(true);
            if (!RecursiveAddJoint(rootJoint, InvalidIndex))
            {
                return false;
            }
        }

        // Add the requested colliders.
        InitColliders(settings);

        // Verify integrity of the collider exclusion list.
        for (const Particle& particle : m_particles)
        {
            const AZStd::vector<AZStd::string>& exclusionList = particle.m_joint->GetColliderExclusionTags();
            for (const AZStd::string& exclusionTag : exclusionList)
            {
                // See if we have a collider with the given tag.
                const auto collisionObjectResult = AZStd::find_if(m_collisionObjects.begin(), m_collisionObjects.end(),
                    [&exclusionTag](const CollisionObject& colObject)
                    {
                        return (exclusionTag == colObject.m_shapePair->first->m_tag);
                    }
                );

                if (collisionObjectResult == m_collisionObjects.end())
                {
                    AZ_Warning("EMotionFX", false, "Simulated object '%s' in simulation '%s' has a joint '%s' that references a non existing collider with the tag '%s' in its collider exclusion list.", 
                        m_simulatedObject->GetName().c_str(), 
                        m_name.c_str(), 
                        m_actorInstance->GetActor()->GetSkeleton()->GetNode(particle.m_joint->GetSkeletonJointIndex())->GetName(),
                        exclusionTag.c_str() );
                }
            }
        }

        // Initialize all rest lengths.
        for (Spring& spring : m_springs)
        {
            const size_t jointIndexA = m_particles[spring.m_particleA].m_joint->GetSkeletonJointIndex();
            const size_t jointIndexB = m_particles[spring.m_particleB].m_joint->GetSkeletonJointIndex();
            const Pose* bindPose = m_actorInstance->GetTransformData()->GetBindPose();
            const float restLength = (bindPose->GetModelSpaceTransform(jointIndexB).m_position - bindPose->GetModelSpaceTransform(jointIndexA).m_position).GetLength();
            if (restLength > AZ::Constants::FloatEpsilon)
            {
                spring.m_restLength = restLength;
            }
            else
            {
                spring.m_restLength = 0.001f;
            }
        }

        // Automatically add colliders to the exclusion list when joints are inside the collider etc.
        InitAutoColliderExclusion();

        return true;
    }

    void SpringSolver::DebugRender(const Pose& pose, bool renderColliders, bool renderLimits, const AZ::Color& color) const
    {
        if (!m_actorInstance)
        {
            return;
        }

        const float scaleFactor = GetScaleFactor();

        // Draw the springs.
        DebugDraw& debugDraw = GetDebugDraw();
        DebugDraw::ActorInstanceData* drawData = debugDraw.GetActorInstanceData(m_actorInstance);
        drawData->Lock();
        for (const Spring& spring : m_springs)
        {
            const Particle& particleA = m_particles[spring.m_particleA];
            const Particle& particleB = m_particles[spring.m_particleB];

            // Output pose lines.
            drawData->DrawLine(particleB.m_pos, particleA.m_pos, color);

            // Cone limits.
            if (renderLimits)
            {
                const float coneAngle = particleB.m_joint->GetConeAngleLimit();
                if (coneAngle < 180.0f)
                {
                    drawData->DrawWireframeJointLimitCone(particleB.m_pos, particleB.m_limitDir, 0.1f * scaleFactor, coneAngle, coneAngle, AZ::Color(0.8f, 0.6f, 0.8f, 1.0f), /*numAngularSubDivs=*/32, /*numRadialSubdivs=*/2);
                }
            }
        }

        // Draw spheres around each joint, representing its collision radius.
        for (const Particle& particle : m_particles)
        {
            drawData->DrawMarker(particle.m_pos, particle.m_joint->IsPinned() ? AZ::Color(0.0f, 1.0f, 1.0f, 1.0f) : AZ::Color(0.0f, 1.0f, 0.0f, 1.0f), 0.015f * scaleFactor);

            // Joint radius.
            if (renderColliders)
            {
                const float radius = particle.m_joint->GetCollisionRadius() * scaleFactor;
                if (radius > 0.0f)
                {
                    const AZ::Quaternion& jointRotation = pose.GetWorldSpaceTransform(particle.m_joint->GetSkeletonJointIndex()).m_rotation;
                    drawData->DrawWireframeSphere(particle.m_pos, radius, AZ::Color(0.3f, 0.3f, 0.3f, 1.0f), jointRotation, 12, 12);
                }
            }
        }

        // Draw the collider shapes.
        if (renderColliders)
        {
            for (const CollisionObject& collider : m_collisionObjects)
            {
                if (collider.GetType() == CollisionObject::CollisionType::Sphere)
                {
                    const AZ::Quaternion& jointRotation = pose.GetWorldSpaceTransform(collider.m_jointIndex).m_rotation;
                    drawData->DrawWireframeSphere(collider.m_globalStart, collider.m_scaledRadius, color * 0.65f, jointRotation, 16, 16);
                }
                else if (collider.GetType() == CollisionObject::CollisionType::Capsule)
                {
                    const float length = (collider.m_globalEnd - collider.m_globalStart).GetLength();
                    AZ::u32 numBodySubDivs = static_cast<AZ::u32>(length * 25.0f);
                    numBodySubDivs = AZ::GetClamp<AZ::u32>(numBodySubDivs, 2, 32);

                    AZ::u32 numSideSubDivs = static_cast<AZ::u32>(collider.m_scaledRadius * 40.0f);
                    numSideSubDivs = AZ::GetClamp<AZ::u32>(numSideSubDivs, 4, 32);
                    drawData->DrawWireframeCapsule(collider.m_globalStart, collider.m_globalEnd, collider.m_scaledRadius, color * 0.7f, numBodySubDivs, numSideSubDivs);
                }
            }
        }

        drawData->Unlock();
    }

    void SpringSolver::SetNumIterations(size_t numIterations)
    {
        AZ_Assert(numIterations > 0, "Number of iterations cannot be zero.");
        AZ_Assert(numIterations <= 50, "Number of iterations in the spring solver shouldn't be set so high.");
        m_numIterations = AZ::GetClamp<size_t>(numIterations, 1, 50);
    }

    size_t SpringSolver::GetNumIterations() const
    {
        return m_numIterations;
    }

    void SpringSolver::SetGravity(const AZ::Vector3& gravity)
    {
        m_gravity = gravity;
    }

    const AZ::Vector3& SpringSolver::GetGravity() const
    {
        return m_gravity;
    }

    size_t SpringSolver::FindParticle(size_t jointIndex) const
    {
        const size_t numParticles = m_particles.size();
        for (size_t i = 0; i < numParticles; ++i)
        {
            if (m_particles[i].m_joint->GetSkeletonJointIndex() == jointIndex)
            {
                return i;
            }
        }

        return InvalidIndex;
    }

    SpringSolver::Particle* SpringSolver::FindParticle(AZStd::string_view nodeName)
    {
        const Actor* actor = m_actorInstance->GetActor();
        const Skeleton* skeleton = actor->GetSkeleton();
        const size_t numParticles = m_particles.size();
        for (size_t i = 0; i < numParticles; ++i)
        {
            if (skeleton->GetNode(m_particles[i].m_joint->GetSkeletonJointIndex())->GetNameString() == nodeName)
            {
                return &m_particles[i];
            }
        }

        return nullptr;
    }

    size_t SpringSolver::AddParticle(const SimulatedJoint* joint)
    {
        AZ_Assert(joint, "Expected a joint that is a valid pointer.");
        AZ_Assert(joint->GetMass() > AZ::Constants::FloatEpsilon, "Expected mass to be larger than zero.");
        Particle particle;
        particle.m_joint = joint;
        particle.m_pos = m_actorInstance->GetTransformData()->GetBindPose()->GetModelSpaceTransform(joint->GetSkeletonJointIndex()).m_position;
        particle.m_oldPos = particle.m_pos;
        particle.m_parentParticleIndex = m_parentParticle;
        m_particles.emplace_back(particle);
        return m_particles.size() - 1;
    }

    bool SpringSolver::AddSupportSpring(size_t nodeA, size_t nodeB, float restLength)
    {
        if (nodeA == InvalidIndex || nodeB == InvalidIndex)
        {
            return false;
        }

        const size_t particleA = FindParticle(nodeA);
        const size_t particleB = FindParticle(nodeB);
        if (particleA == InvalidIndex || particleB == InvalidIndex)
        {
            return false;
        }

        if (restLength < 0.0f)
        {
            const Pose* pose = m_actorInstance->GetTransformData()->GetCurrentPose();
            const AZ::Vector3 posA = pose->GetWorldSpaceTransform(nodeA).m_position;
            const AZ::Vector3 posB = pose->GetWorldSpaceTransform(nodeB).m_position;
            restLength = (posB - posA).GetLength();
        }

        m_springs.emplace_back(particleA, particleB, restLength, true, true);
        return true;
    }

    bool SpringSolver::AddSupportSpring(AZStd::string_view nodeNameA, AZStd::string_view nodeNameB, float restLength)
    {
        const Skeleton* skeleton = m_actorInstance->GetActor()->GetSkeleton();
        const Node* nodeA = skeleton->FindNodeByNameNoCase(nodeNameA.data());
        const Node* nodeB = skeleton->FindNodeByNameNoCase(nodeNameB.data());
        if (!nodeA || !nodeB)
        {
            return false;
        }

        return AddSupportSpring(nodeA->GetNodeIndex(), nodeB->GetNodeIndex(), restLength);
    }

    bool SpringSolver::RemoveJoint(size_t jointIndex)
    {
        const size_t particleIndex = FindParticle(jointIndex);
        if (particleIndex == InvalidIndex)
        {
            AZ_Warning("EMotionFX", false, "SpringSolver::RemoveJoint() - Failed to find any particle that uses the joint index value %d.", jointIndex);
            return false;
        }

        m_particles.erase(m_particles.begin() + particleIndex);

        for (size_t i = 0; i < m_springs.size();)
        {
            if (m_springs[i].m_particleA == particleIndex || m_springs[i].m_particleB == particleIndex)
            {
                m_springs.erase(m_springs.begin() + i);
            }
            else
            {
                i++;
            }
        }

        return true;
    }

    bool SpringSolver::RemoveJoint(AZStd::string_view nodeName)
    {
        const Node* node = m_actorInstance->GetActor()->GetSkeleton()->FindNodeByNameNoCase(nodeName.data());
        if (!node)
        {
            AZ_Warning("EMotionFX", false, "SpringSolver::RemoveJoint() - Failed to locate the joint with the specified name '%s'.", nodeName.data());
            return false;
        }

        return RemoveJoint(node->GetNodeIndex());
    }

    bool SpringSolver::RemoveSupportSpring(size_t jointIndexA, size_t jointIndexB)
    {
        const size_t particleA = FindParticle(jointIndexA);
        if (particleA == InvalidIndex)
        {
            AZ_Warning("EMotionFX", false, "SpringSolver::RemoveSupportSpring() - Cannot find any particle that uses the specified joint index %d (jointIndexA).", jointIndexA);
            return false;
        }

        const size_t particleB = FindParticle(jointIndexB);
        if (particleB == InvalidIndex)
        {
            AZ_Warning("EMotionFX", false, "SpringSolver::RemoveSupportSpring() - Cannot find any particle that uses the specified joint index %d (jointIndexB).", jointIndexB);
            return false;
        }

        for (size_t i = 0; i < m_springs.size();)
        {
            if ((m_springs[i].m_particleA == particleA && m_springs[i].m_particleB == particleB) || (m_springs[i].m_particleA == particleB && m_springs[i].m_particleB == particleA))
            {
                m_springs.erase(m_springs.begin() + i);
            }
            else
            {
                i++;
            }
        }

        return true;
    }

    bool SpringSolver::RemoveSupportSpring(AZStd::string_view nodeNameA, AZStd::string_view nodeNameB)
    {
        const Skeleton* skeleton = m_actorInstance->GetActor()->GetSkeleton();
        const Node* nodeA = skeleton->FindNodeByNameNoCase(nodeNameA.data());
        const Node* nodeB = skeleton->FindNodeByNameNoCase(nodeNameB.data());

        if (!nodeA || !nodeB)
        {
            AZ_Warning("EMotionFX", false, "SpringSolver::RemoveSupportSpring() - Failed to locate one or both of the specified nodes (using a names '%s' and '%s').", nodeNameA.data(), nodeNameB.data());
            return false;
        }

        return RemoveSupportSpring(nodeA->GetNodeIndex(), nodeB->GetNodeIndex());
    }

    float SpringSolver::GetScaleFactor() const
    {
        #ifndef EMFX_SCALE_DISABLED
            float scaleFactor = m_actorInstance->GetWorldSpaceTransform().m_scale.GetX();
            if (AZ::IsClose(scaleFactor, 0.0f, AZ::Constants::FloatEpsilon))
            {
                return AZ::Constants::FloatEpsilon;
            }

            return scaleFactor;

        #else
            return 1.0f;           
        #endif
    }

    void SpringSolver::CalcForces(const Pose& pose, float scaleFactor)
    {
        const float globalStiffnessFactor = m_simulatedObject->GetStiffnessFactor() * m_stiffnessFactor * scaleFactor;
        const float globalGravityFactor = m_simulatedObject->GetGravityFactor() * m_gravityFactor * scaleFactor;

        for (Particle& particle : m_particles)
        {
            particle.m_force = AZ::Vector3::CreateZero();

            // Try to move us towards the current pose, based on a stiffness.
            const SimulatedJoint* joint = particle.m_joint;
            if (!joint->IsPinned())
            {
                const float stiffnessFactor = joint->GetStiffness() * globalStiffnessFactor;
                if (stiffnessFactor > 0.0f)
                {
                    const Transform jointWorldTransform = pose.GetWorldSpaceTransform(joint->GetSkeletonJointIndex());
                    const AZ::Vector3 force = (jointWorldTransform.m_position - particle.m_pos) + particle.m_externalForce;
                    particle.m_force += force * stiffnessFactor;
                }

                // Apply gravity.
                const float gravityFactor = joint->GetGravityFactor() * globalGravityFactor;
                particle.m_force += gravityFactor * m_gravity;
                particle.m_force *= joint->GetMass();
            }
        }
    }

    void SpringSolver::PerformConeLimit(Particle& particleA, Particle& particleB, const AZ::Vector3& inputDir)
    {
        // Calc the vector pointing towards the child in world space as the animation outputs.
        AZ::Vector3 animDir = inputDir;
        const float animDirLength = animDir.GetLength();
        if (animDir.GetLength() <= AZ::Constants::FloatEpsilon)
        {
            return;
        }

        // Now calculate our current spring direction.
        AZ::Vector3 springDir = particleA.m_pos - particleB.m_pos;
        const float springLength = springDir.GetLength();
        if (springLength <= AZ::Constants::FloatEpsilon)
        {
            return;
        }

        animDir /= animDirLength;
        springDir /= springLength;

        // Calculate the angle between the vectors.
        float dotResult = springDir.Dot(animDir);
        dotResult = AZ::GetClamp(dotResult, -1.0f, 1.0f);

        // If it's outside of its limits.
        float angle = acosf(dotResult);
        const float coneLimit = particleB.m_joint->GetConeAngleLimit();
        if (angle > AZ::DegToRad(coneLimit))
        {
            const AZ::Vector3 rotAxis = animDir.Cross(springDir);
            angle = AZ::RadToDeg(angle) - coneLimit;

            const AZ::Quaternion rot = AZ::Quaternion::CreateFromAxisAngle(rotAxis, AZ::DegToRad(-angle));
            springDir = rot.TransformVector(springDir * springLength);

            particleA.m_pos = particleB.m_pos + springDir;
        }
    }

    void SpringSolver::SatisfyConstraints(const Pose& inputPose, Pose& outPose, size_t numIterations, float scaleFactor)
    {
        for (size_t n = 0; n < numIterations; ++n)
        {
            for (const Spring& spring : m_springs)
            {
                Particle& particleA = m_particles[spring.m_particleA];
                Particle& particleB = m_particles[spring.m_particleB];
                const Transform& worldTransformA = inputPose.GetWorldSpaceTransform(particleA.m_joint->GetSkeletonJointIndex());
                const Transform& worldTransformB = inputPose.GetWorldSpaceTransform(particleB.m_joint->GetSkeletonJointIndex());

                // Try to maintain the rest length by applying correctional forces.
                const AZ::Vector3 delta = particleB.m_pos - particleA.m_pos;
                const float deltaLength = delta.GetLength();
                float diff;
                if (deltaLength > AZ::Constants::FloatEpsilon)
                {
                    const SimulatedJoint* jointA = particleA.m_joint;
                    const SimulatedJoint* jointB = particleB.m_joint;
                    const float invMassA = 1.0f / (jointA->GetMass() * scaleFactor);
                    const float invMassB = 1.0f / (jointB->GetMass() * scaleFactor);

                    const float restLength = spring.m_restLength * scaleFactor;
                    diff = (deltaLength - restLength) / (deltaLength * (invMassA + invMassB));
                }
                else
                {
                    diff = 0.0f;
                }

                const bool pinnedA = particleA.m_joint->IsPinned();
                const bool pinnedB = particleB.m_joint->IsPinned();
                if (!pinnedA && !pinnedB)
                {
                    particleA.m_pos += delta * 0.5f * diff;
                    particleB.m_pos -= delta * 0.5f * diff;
                }
                else if (pinnedA && pinnedB)
                {
                    particleA.m_pos = worldTransformA.m_position;
                    particleB.m_pos = worldTransformB.m_position;
                }
                else if (pinnedB)
                {
                    particleB.m_pos = worldTransformB.m_position;
                    particleA.m_pos += delta * diff;
                }
                else // Only particleA is pinned.
                {
                    particleA.m_pos = worldTransformA.m_position;
                    particleB.m_pos -= delta * diff;
                }

                // Apply cone limit when needed.
                if (particleB.m_joint->GetConeAngleLimit() < 180.0f - 0.001f)
                {
                    if (particleB.m_parentParticleIndex != InvalidIndex)
                    {
                        particleB.m_limitDir = particleB.m_pos - m_particles[particleB.m_parentParticleIndex].m_pos;
                    }
                    else
                    {
                        particleB.m_limitDir = worldTransformA.m_position - worldTransformB.m_position;
                    }
                    PerformConeLimit(particleA, particleB, particleB.m_limitDir);
                }
            } // For all springs.

            // Update the joint transforms and colliders.
            // This has to be done before the collision detection, so that the colliders are up to date.
            UpdateJointTransforms(outPose);
            UpdateCollisionObjects(outPose, 1.0f);

            // Perform collision.
            if (m_collisionDetection)
            {
                for (Particle& particle : m_particles)
                {
                    const SimulatedJoint* joint = particle.m_joint;
                    if (joint->IsPinned())
                    {
                        continue;
                    }

                    const float jointRadius = joint->GetCollisionRadius() * scaleFactor;
                    if (PerformCollision(particle.m_pos, jointRadius, particle))
                    {
                        particle.m_oldPos = MCore::LinearInterpolate<AZ::Vector3>(particle.m_oldPos, particle.m_pos, joint->GetFriction());
                    }
                }
            }
        } // For all iterations.
    }

    void SpringSolver::Simulate(float deltaTime, const Pose& inputPose, Pose& outPose, float scaleFactor)
    {
        CalcForces(inputPose, scaleFactor);
        Integrate(deltaTime);
        SatisfyConstraints(inputPose, outPose, m_numIterations, scaleFactor);
        UpdateJointTransforms(outPose);
    }

    void SpringSolver::UpdateFixedParticles(const Pose& pose)
    {
        for (Particle& particle : m_particles)
        {
            const SimulatedJoint* joint = particle.m_joint;
            if (joint->IsPinned())
            {
                particle.m_pos = pose.GetWorldSpaceTransform(joint->GetSkeletonJointIndex()).m_position;
                particle.m_oldPos = particle.m_pos;
                particle.m_force = AZ::Vector3::CreateZero();
            }
        }
    }

    void SpringSolver::Stabilize()
    {
        m_stabilize = true;
    }

    void SpringSolver::Stabilize(const Pose& inputPose, Pose& pose, size_t numFrames)
    {
        InitCollidersFromColliderSetupShapes();
        const float scaleFactor = GetScaleFactor();

        for (size_t i = 0; i < numFrames; ++i)
        {
            Simulate(static_cast<float>(1.0f/60.0f), inputPose, pose, scaleFactor);
        }
    }

    void SpringSolver::Update(const Pose& inputPose, Pose& pose, float timePassedInSeconds)
    {
        if (!m_actorInstance)
        {
            return;
        }

        // Stabilize the simulation first if desired.
        if (m_stabilize)
        {
            Stabilize(inputPose, pose, /*numFrames=*/7);
            m_stabilize = false;
        }

        // Resize the colliders, based on the current collider setup sizes.
        InitCollidersFromColliderSetupShapes();

        // Perform a simulation step, with the number of iterations we want.
        const float scaleFactor = GetScaleFactor();
        const float stepSize = AZ::GetClamp(timePassedInSeconds, 1.0f / 140.0f, 1.0f / 30.0f); // Make sure we don't update with too small stepsize, as that can cause issues.

        Simulate(stepSize, inputPose, pose, scaleFactor);
    }

    void SpringSolver::AdjustParticles(const ParticleAdjustFunction& func)
    {
        for (Particle& particle : m_particles)
        {
            func(particle);
        }
    }

    void SpringSolver::UpdateJointTransforms(Pose& pose)
    {
        for (const Spring& spring : m_springs)
        {
            if (spring.m_isSupportSpring)
            {
                continue;
            }

            const Particle& particleA = m_particles[spring.m_particleA];
            const Particle& particleB = m_particles[spring.m_particleB];
            Transform modelTransformB = pose.GetModelSpaceTransform(particleB.m_joint->GetSkeletonJointIndex());
            const Transform& modelTransformA = pose.GetModelSpaceTransform(particleA.m_joint->GetSkeletonJointIndex());
            const AZ::Vector3 oldDir = (modelTransformA.m_position - modelTransformB.m_position).GetNormalizedSafe();
            const AZ::Vector3 newDir = m_actorInstance->GetWorldSpaceTransformInversed().TransformVector(particleA.m_pos - particleB.m_pos).GetNormalizedSafe();

            modelTransformB.m_rotation = AZ::Quaternion::CreateShortestArc(oldDir, newDir).GetNormalized() * modelTransformB.m_rotation;
            modelTransformB.m_rotation.Normalize();

            if (spring.m_allowStretch)
            {
                modelTransformB.m_position = particleB.m_pos;
            }

            pose.SetModelSpaceTransform(particleB.m_joint->GetSkeletonJointIndex(), modelTransformB);
        }
    }

    void SpringSolver::Integrate(float timeDelta)
    {
        //const float timeDeltaSq = timeDelta * timeDelta;
        const float timeCorrect = (m_lastTimeDelta > MCore::Math::epsilon) ? (timeDelta / static_cast<float>(m_lastTimeDelta)) : 1.0f;    // Used only for time corrected Verlet.
        const float globalDampingFactor = m_simulatedObject->GetDampingFactor() * m_dampingFactor;

        for (Particle& particle : m_particles)
        {
            // Limit the velocity, making things slightly more stable.
            const float maxVelocity = timeDelta * 10.0f; // 10 is the number of units per second.
            AZ::Vector3 direction = particle.m_pos - particle.m_oldPos;
            if (direction.GetLength() > maxVelocity)
            {
                direction.NormalizeSafe();
                particle.m_oldPos = particle.m_pos - (direction * maxVelocity);
            }

            const AZ::Vector3 pos = particle.m_pos;
            const AZ::Vector3 oldPos = particle.m_oldPos;
            particle.m_oldPos = pos;

            // Do the Vertlet integration step.
            // Choose either standard Verlet or Time Corrected Verlet. We comment out one of them.
            const SimulatedJoint* joint = particle.m_joint;
            const float damping = joint->GetDamping() * globalDampingFactor;
            //particle.m_pos = ((2.0f - damping) * pos - (1.0f - damping) * oldPos) + (particle.m_force * timeDeltaSq); // Standard Verlet integration.

            // Corrected time corrected verlet: xi+1 = xi + (xi - xi-1) * (dti / dti-1) + (a * dti) * (dti + dti-1) / 2.0
            // This is a more stable version of verlet when using non-fixed time deltas. It is a corrected version of the time corrected verlet integration.
            particle.m_pos = pos + (pos - oldPos) * timeCorrect * (1.0f - damping) + (particle.m_force * timeDelta) * (timeDelta + static_cast<float>(m_lastTimeDelta)) / 2.0f;
        }

        m_lastTimeDelta = timeDelta;
    }

    void SpringSolver::UpdateCollisionObjects(const Pose& pose, float scaleFactor)
    {
        for (CollisionObject& colObject : m_collisionObjects)
        {
            if (colObject.m_jointIndex != InvalidIndex)
            {
                const Transform jointWorldTransform = pose.GetWorldSpaceTransform(colObject.m_jointIndex);
                colObject.m_globalStart = jointWorldTransform.TransformPoint(colObject.m_start);
                colObject.m_globalEnd = jointWorldTransform.TransformPoint(colObject.m_end);
                colObject.m_scaledRadius = colObject.m_radius * scaleFactor;
            }
            else
            {
                colObject.m_globalStart = colObject.m_start;
                colObject.m_globalEnd = colObject.m_end;
                colObject.m_scaledRadius = colObject.m_radius * scaleFactor;
            }
        }
    }

    void SpringSolver::UpdateCollisionObjectsModelSpace(const Pose& pose)
    {
        for (CollisionObject& colObject : m_collisionObjects)
        {
            if (colObject.m_jointIndex != InvalidIndex)
            {
                const Transform& jointTransform = pose.GetModelSpaceTransform(colObject.m_jointIndex);
                colObject.m_globalStart = jointTransform.TransformPoint(colObject.m_start);
                colObject.m_globalEnd = jointTransform.TransformPoint(colObject.m_end);
                colObject.m_scaledRadius = colObject.m_radius;
            }
            else
            {
                colObject.m_globalStart = colObject.m_start;
                colObject.m_globalEnd = colObject.m_end;
                colObject.m_scaledRadius = colObject.m_radius;
            }
        }
    }

    bool SpringSolver::CheckIsJointInsideCollider(const CollisionObject& colObject, const Particle& particle) const
    {
        switch (colObject.m_type)
        {
            case CollisionObject::CollisionType::Capsule:
            {
                return CheckIsInsideCapsule(particle.m_pos, colObject.m_globalStart, colObject.m_globalEnd, colObject.m_scaledRadius);
            }

            case CollisionObject::CollisionType::Sphere:
            {
                return CheckIsInsideSphere(particle.m_pos, colObject.m_globalStart, colObject.m_scaledRadius);
            }

            default:
                AZ_Assert(false, "Unknown collision object type.");
        };

        return false;
    }

    bool SpringSolver::PerformCollision(AZ::Vector3& inOutPos, float jointRadius, const Particle& particle)
    {
        bool collided = false; 
        const size_t numColliders = m_collisionObjects.size();
        for (size_t colliderIndex = 0; colliderIndex < numColliders; ++colliderIndex)
        {
            // Skip colliders in the exclusion list.
            if (AZStd::find(particle.m_colliderExclusions.begin(), particle.m_colliderExclusions.end(), colliderIndex) != particle.m_colliderExclusions.end())
            {
                continue;
            }

            const CollisionObject& colObject = m_collisionObjects[colliderIndex];
            switch (colObject.m_type)
            {
                case CollisionObject::CollisionType::Capsule:
                {
                    if (CollideWithCapsule(inOutPos, colObject.m_globalStart, colObject.m_globalEnd, colObject.m_scaledRadius + jointRadius))
                    {
                        collided = true;
                    }
                    break;
                }

                case CollisionObject::CollisionType::Sphere:
                {
                    if (CollideWithSphere(inOutPos, colObject.m_globalStart, colObject.m_scaledRadius + jointRadius))
                    {
                        collided = true;
                    }
                    break;
                }

                default:
                    AZ_Assert(false, "Unknown collision object type.");
            };
        }

        return collided;
    }

    bool SpringSolver::CheckIsInsideSphere(const AZ::Vector3& pos, const AZ::Vector3& center, float radius) const
    {
        const AZ::Vector3 centerToPoint = pos - center;
        const float sqDist = centerToPoint.GetLengthSq();
        return (sqDist <= radius * radius);
    }

    bool SpringSolver::CollideWithSphere(AZ::Vector3& pos, const AZ::Vector3& center, float radius)
    {
        const AZ::Vector3 centerToPoint = pos - center;
        const float sqDist = centerToPoint.GetLengthSq();
        if (sqDist >= radius * radius)
        {
            return false;
        }

        if (sqDist > AZ::Constants::FloatEpsilon)
        {
            pos = center + radius * (centerToPoint / MCore::Math::FastSqrt(sqDist));
        }
        else
        {
            pos = center;
        }

        return true;
    }


    bool SpringSolver::CheckIsInsideCapsule(const AZ::Vector3& pos, const AZ::Vector3& lineStart, const AZ::Vector3& lineEnd, float radius) const
    {
        const AZ::Vector3 startToEnd = lineEnd - lineStart;
        const float squareLength = startToEnd.GetLengthSq();
        float t;
        if (squareLength > AZ::Constants::FloatEpsilon)
        {
            t = (pos - lineStart).Dot(startToEnd) / squareLength;
            t = AZ::GetClamp(t, 0.0f, 1.0f);
        }
        else
        {
            t = 0.0f;
        }

        const AZ::Vector3 projected = lineStart + t * startToEnd;      
        const AZ::Vector3 toPos = pos - projected;
        const float sqLen = toPos.GetLengthSq();
        return (sqLen <= radius * radius);
    }


    bool SpringSolver::CollideWithCapsule(AZ::Vector3& pos, const AZ::Vector3& lineStart, const AZ::Vector3& lineEnd, float radius)
    {
        const AZ::Vector3 startToEnd = lineEnd - lineStart;
        const float squareLength = startToEnd.GetLengthSq();
        float t;
        if (squareLength > AZ::Constants::FloatEpsilon)
        {
            t = (pos - lineStart).Dot(startToEnd) / squareLength;
            t = AZ::GetClamp(t, 0.0f, 1.0f);
        }
        else
        {
            t = 0.0f;
        }
    
        // Calculate the position projected on the line.
        const AZ::Vector3 projected = lineStart + t * startToEnd;

        // The vector from the projected position to the point we are testing with.
        AZ::Vector3 toPos = pos - projected;

        // If the distance is within the diameter of the capsule, it is colliding and then
        // the point needs to be pushed outside of the capsule.
        const float sqLen = toPos.GetLengthSq();
        if (sqLen < radius * radius)
        {
            toPos.NormalizeSafe();
            pos = projected + (toPos * radius);
            return true;
        }
        else // The point is not colliding with the capsule, so ignore it.
        {
            return false;
        }
    }
} // namespace EMotionFX
