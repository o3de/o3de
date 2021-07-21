/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>

#include <System/Cloth.h>
#include <System/Fabric.h>
#include <System/Solver.h>

// NvCloth library includes
#include <NvCloth/Cloth.h>
#include <NvClothExt/ClothFabricCooker.h>
#include <foundation/PxVec3.h>
#include <foundation/PxVec4.h>
#include <foundation/PxQuat.h>

namespace NvCloth
{
    namespace Internal
    {
        // Returns AZ::Vector3 as physx::PxVec3 using the same memory.
        // It's safe to reinterpret AZ::Vector3 as physx::PxVec3 because they have the same memory layout
        // and AZ::Vector3 has more memory alignment restrictions than physx::PxVec3.
        // The opposite operation would NOT be safe.
        physx::PxVec3& AsPxVec3(AZ::Vector3& azVec)
        {
            return *reinterpret_cast<physx::PxVec3*>(&azVec);
        }

        const physx::PxVec3& AsPxVec3(const AZ::Vector3& azVec)
        {
            return *reinterpret_cast<const physx::PxVec3*>(&azVec);
        }

        // Returns AZ::Quaternion as physx::PxQuat using the same memory.
        // It's safe to reinterpret AZ::Quaternion as physx::PxQuat because they have the same memory layout
        // and AZ::Quaternion has more memory alignment restrictions than physx::PxQuat.
        // The opposite operation would NOT be safe.
        physx::PxQuat& AsPxQuat(AZ::Quaternion& azQuat)
        {
            return *reinterpret_cast<physx::PxQuat*>(&azQuat);
        }

        const physx::PxQuat& AsPxQuat(const AZ::Quaternion& azQuat)
        {
            return *reinterpret_cast<const physx::PxQuat*>(&azQuat);
        }

        // Copies an AZ vector of AZ::Vector4 elements as a NvCloth Range of physx::PxVec4 elements.
        //
        // It's safe to reinterpret AZ::Vector4 as physx::PxVec4 because they have the same memory layout.
        // Each one has its own memory with their appropriate alignments.
        void FastCopy(const AZStd::vector<AZ::Vector4>& azVector, nv::cloth::Range<physx::PxVec4>& nvRange)
        {
            AZ_Assert(azVector.size() == nvRange.size(),
                "Mismatch in number of elements. AZ vector: %zu Nv Range: %u", azVector.size(), nvRange.size());
            static_assert(sizeof(physx::PxVec4) == sizeof(AZ::Vector4), "physx::PxVec4 and AZ::Vector4 types have different sizes");

            // Reinterpret cast to floats so it does a fast copy.
            AZStd::copy(
                reinterpret_cast<const float*>(azVector.data()),
                reinterpret_cast<const float*>(azVector.data() + azVector.size()),
                reinterpret_cast<float*>(nvRange.begin()));
        }

        // Copies a NvCloth Range of physx::PxVec4 elements as an AZ vector of AZ::Vector4 elements.
        //
        // It's safe to reinterpret AZ::Vector4 as physx::PxVec4 because they have the same memory layout.
        // Each one has its own memory with their appropriate alignments.
        void FastCopy(const nv::cloth::Range<physx::PxVec4>& nvRange, AZStd::vector<AZ::Vector4>& azVector)
        {
            AZ_Assert(azVector.size() == nvRange.size(),
                "Mismatch in number of elements. AZ vector: %zu Nv Range: %u", azVector.size(), nvRange.size());
            static_assert(sizeof(physx::PxVec4) == sizeof(AZ::Vector4), "physx::PxVec4 and AZ::Vector4 types have different sizes");

            // Reinterpret cast to floats so it does a fast copy.
            AZStd::copy(
                reinterpret_cast<const float*>(nvRange.begin()),
                reinterpret_cast<const float*>(nvRange.end()),
                reinterpret_cast<float*>(azVector.begin()));
        }

        // Moves an AZ vector of AZ::Vector4 elements as a NvCloth Range of physx::PxVec4 elements.
        //
        // It's safe to reinterpret AZ::Vector4 as physx::PxVec4 because they have the same memory layout.
        // Each one has its own memory with their appropriate alignments.
        void FastMove(AZStd::vector<AZ::Vector4>&& azVector, nv::cloth::Range<physx::PxVec4>& nvRange)
        {
            AZ_Assert(azVector.size() == nvRange.size(),
                "Mismatch in number of elements. AZ vector: %zu Nv Range: %u", azVector.size(), nvRange.size());
            static_assert(sizeof(physx::PxVec4) == sizeof(AZ::Vector4), "physx::PxVec4 and AZ::Vector4 types have different sizes");

            // Reinterpret cast to floats so it does a fast move.
            AZStd::move(
                reinterpret_cast<float*>(azVector.data()),
                reinterpret_cast<float*>(azVector.data() + azVector.size()),
                reinterpret_cast<float*>(nvRange.begin()));
        }

        // Moves a NvCloth Range of physx::PxVec4 elements as an AZ vector of AZ::Vector4 elements.
        //
        // It's safe to reinterpret AZ::Vector4 as physx::PxVec4 because they have the same memory layout.
        // Each one has its own memory with their appropriate alignments.
        void FastMove(nv::cloth::Range<physx::PxVec4>&& nvRange, AZStd::vector<AZ::Vector4>& azVector)
        {
            AZ_Assert(azVector.size() == nvRange.size(),
                "Mismatch in number of elements. AZ vector: %zu Nv Range: %u", azVector.size(), nvRange.size());
            static_assert(sizeof(physx::PxVec4) == sizeof(AZ::Vector4), "physx::PxVec4 and AZ::Vector4 types have different sizes");

            // Reinterpret cast to floats so it does a fast copy.
            AZStd::move(
                reinterpret_cast<float*>(nvRange.begin()),
                reinterpret_cast<float*>(nvRange.end()),
                reinterpret_cast<float*>(azVector.begin()));
        }
    }

    Cloth::Cloth(
        ClothId id,
        const AZStd::vector<SimParticleFormat>& initialParticles,
        Fabric* fabric,
        NvClothUniquePtr nvCloth)
        : m_id(id)
        , m_nvCloth(AZStd::move(nvCloth))
        , m_fabric(fabric)
        , m_initialParticles(initialParticles)
        , m_initialParticlesWithMassApplied(initialParticles)
    {
        m_simParticles = initialParticles;

        // Construct the default list of phase configurations
        const size_t numPhaseTypes = m_fabric->GetPhaseTypes().size();
        m_nvPhaseConfigs.reserve(numPhaseTypes);
        for (size_t phaseIndex = 0; phaseIndex < numPhaseTypes; phaseIndex++)
        {
            m_nvPhaseConfigs.emplace_back(static_cast<uint16_t>(phaseIndex));
        }
        ApplyPhaseConfigs();

        // Set default gravity
        const AZ::Vector3 gravity(0.0f, 0.0f, -9.81f);
        SetGravity(gravity);

        // One more cloth instance using the fabric
        m_fabric->m_numClothsUsingFabric++;
    }

    Cloth::~Cloth()
    {
        // If cloth is still part of a solver, remove it
        if (m_solver)
        {
            m_solver->RemoveCloth(this);
        }

        // One less cloth instance using the fabric
        m_fabric->m_numClothsUsingFabric--;
    }

    void Cloth::Update()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

        ResolveStaticParticles();

        if (!RetrieveSimulationResults())
        {
            RestoreSimulation();
        }
    }

    ClothId Cloth::GetId() const
    {
        return m_id;
    }

    const AZStd::vector<SimParticleFormat>& Cloth::GetInitialParticles() const
    {
        return m_initialParticles;
    }

    const AZStd::vector<SimIndexType>& Cloth::GetInitialIndices() const
    {
        return m_fabric->m_cookedData.m_indices;
    }

    const AZStd::vector<SimParticleFormat>& Cloth::GetParticles() const
    {
        return m_simParticles;
    }

    void Cloth::SetParticles(const AZStd::vector<SimParticleFormat>& particles)
    {
        if (m_simParticles.size() != particles.size())
        {
            AZ_Warning("Cloth", false, "Unable to set cloth particles as it doesn't match the number of elements. Number of particles passed %zu, expected %zu.",
                particles.size(), m_simParticles.size());
            return;
        }
        m_simParticles = particles;
        CopySimParticlesToNvCloth();
    }

    void Cloth::SetParticles(AZStd::vector<SimParticleFormat>&& particles)
    {
        if (m_simParticles.size() != particles.size())
        {
            AZ_Warning("Cloth", false, "Unable to set cloth particles as it doesn't match the number of elements. Number of particles passed %zu, expected %zu.",
                particles.size(), m_simParticles.size());
            return;
        }
        m_simParticles = AZStd::move(particles);
        CopySimParticlesToNvCloth();
    }

    void Cloth::DiscardParticleDelta()
    {
        const nv::cloth::MappedRange<const physx::PxVec4> currentParticles = nv::cloth::readCurrentParticles(*m_nvCloth);

        nv::cloth::MappedRange<physx::PxVec4> previousParticles = m_nvCloth->getPreviousParticles();
        // Reinterpret cast to floats so it does a fast copy.
        AZStd::copy(
            reinterpret_cast<const float*>(currentParticles.begin()),
            reinterpret_cast<const float*>(currentParticles.end()),
            reinterpret_cast<float*>(previousParticles.begin()));
    }

    const FabricCookedData& Cloth::GetFabricCookedData() const
    {
        return m_fabric->m_cookedData;
    }

    IClothConfigurator* Cloth::GetClothConfigurator()
    {
        return this;
    }

    void Cloth::SetTransform(const AZ::Transform& transformWorld)
    {
        m_nvCloth->setTranslation(Internal::AsPxVec3(transformWorld.GetTranslation()));
        m_nvCloth->setRotation(Internal::AsPxQuat(transformWorld.GetRotation()));
    }

    void Cloth::ClearInertia()
    {
        m_nvCloth->clearInertia();
    }

    void Cloth::SetMass(float mass)
    {
        if (AZ::IsClose(m_mass, mass, std::numeric_limits<float>::epsilon()))
        {
            return;
        }

        m_mass = mass;

        const float inverseMass = (m_mass > 0.0f) ? (1.0f / m_mass) : 0.0f;
        for (size_t i = 0; i < m_simParticles.size(); ++i)
        {
            const float particleInvMass = m_initialParticles[i].GetW() * inverseMass;

            m_simParticles[i].SetW(particleInvMass);
            m_initialParticlesWithMassApplied[i].SetW(particleInvMass);
        }

        CopySimInverseMassesToNvCloth();
    }

    void Cloth::SetGravity(const AZ::Vector3& gravity)
    {
        m_nvCloth->setGravity(Internal::AsPxVec3(gravity));
    }

    void Cloth::SetStiffnessFrequency(float frequency)
    {
        m_nvCloth->setStiffnessFrequency(frequency);
    }

    void Cloth::SetDamping(const AZ::Vector3& damping)
    {
        m_nvCloth->setDamping(Internal::AsPxVec3(damping));
    }

    void Cloth::SetDampingLinearDrag(const AZ::Vector3& linearDrag)
    {
        m_nvCloth->setLinearDrag(Internal::AsPxVec3(linearDrag));
    }

    void Cloth::SetDampingAngularDrag(const AZ::Vector3& angularDrag)
    {
        m_nvCloth->setAngularDrag(Internal::AsPxVec3(angularDrag));
    }

    void Cloth::SetLinearInertia(const AZ::Vector3& linearInertia)
    {
        m_nvCloth->setLinearInertia(Internal::AsPxVec3(linearInertia));
    }

    void Cloth::SetAngularInertia(const AZ::Vector3& angularInertia)
    {
        m_nvCloth->setAngularInertia(Internal::AsPxVec3(angularInertia));
    }

    void Cloth::SetCentrifugalInertia(const AZ::Vector3& centrifugalInertia)
    {
        m_nvCloth->setCentrifugalInertia(Internal::AsPxVec3(centrifugalInertia));
    }

    void Cloth::SetWindVelocity(const AZ::Vector3& velocity)
    {
        m_nvCloth->setWindVelocity(Internal::AsPxVec3(velocity));
    }

    void Cloth::SetWindDragCoefficient(float drag)
    {
        const float airDragPerc = 0.97f; // To improve cloth stability
        m_nvCloth->setDragCoefficient(airDragPerc * drag);
    }

    void Cloth::SetWindLiftCoefficient(float lift)
    {
        const float airLiftPerc = 0.8f; // To improve cloth stability
        m_nvCloth->setLiftCoefficient(airLiftPerc * lift);
    }

    void Cloth::SetWindFluidDensity(float density)
    {
        m_nvCloth->setFluidDensity(density);
    }

    void Cloth::SetCollisionFriction(float friction)
    {
        m_nvCloth->setFriction(friction);
    }

    void Cloth::SetCollisionMassScale(float scale)
    {
        m_nvCloth->setCollisionMassScale(scale);
    }

    void Cloth::EnableContinuousCollision(bool value)
    {
        m_nvCloth->enableContinuousCollision(value);
    }

    void Cloth::SetCollisionAffectsStaticParticles(bool value)
    {
        m_collisionAffectsStaticParticles = value;
    }

    void Cloth::SetSelfCollisionDistance(float distance)
    {
        m_nvCloth->setSelfCollisionDistance(distance);
    }

    void Cloth::SetSelfCollisionStiffness(float stiffness)
    {
        m_nvCloth->setSelfCollisionStiffness(stiffness);
    }

    void Cloth::SetVerticalPhaseConfig(
        float stiffness,
        float stiffnessMultiplier,
        float compressionLimit,
        float stretchLimit)
    {
        SetPhaseConfig(
            nv::cloth::ClothFabricPhaseType::eVERTICAL,
            stiffness,
            stiffnessMultiplier,
            compressionLimit,
            stretchLimit);
    }

    void Cloth::SetHorizontalPhaseConfig(
        float stiffness,
        float stiffnessMultiplier,
        float compressionLimit,
        float stretchLimit)
    {
        SetPhaseConfig(
            nv::cloth::ClothFabricPhaseType::eHORIZONTAL,
            stiffness,
            stiffnessMultiplier,
            compressionLimit,
            stretchLimit);
    }

    void Cloth::SetBendingPhaseConfig(
        float stiffness,
        float stiffnessMultiplier,
        float compressionLimit,
        float stretchLimit)
    {
        SetPhaseConfig(
            nv::cloth::ClothFabricPhaseType::eBENDING,
            stiffness,
            stiffnessMultiplier,
            compressionLimit,
            stretchLimit);
    }

    void Cloth::SetShearingPhaseConfig(
        float stiffness,
        float stiffnessMultiplier,
        float compressionLimit,
        float stretchLimit)
    {
        SetPhaseConfig(
            nv::cloth::ClothFabricPhaseType::eSHEARING,
            stiffness,
            stiffnessMultiplier,
            compressionLimit,
            stretchLimit);
    }

    void Cloth::SetTetherConstraintStiffness(float stiffness)
    {
        m_nvCloth->setTetherConstraintStiffness(stiffness);
    }

    void Cloth::SetTetherConstraintScale(float scale)
    {
        m_nvCloth->setTetherConstraintScale(scale);
    }

    void Cloth::SetSolverFrequency(float frequency)
    {
        m_nvCloth->setSolverFrequency(frequency);
    }

    void Cloth::SetAcceleationFilterWidth(AZ::u32 width)
    {
        m_nvCloth->setAcceleationFilterWidth(width);
    }

    void Cloth::SetSphereColliders(const AZStd::vector<AZ::Vector4>& spheres)
    {
        m_nvCloth->setSpheres(
            ToPxVec4NvRange(spheres),
            0, m_nvCloth->getNumSpheres());
    }

    void Cloth::SetSphereColliders(AZStd::vector<AZ::Vector4>&& spheres)
    {
        SetSphereColliders(spheres); // NvCloth does not offer a move overload for setSpheres, calling the const reference one.
    }

    void Cloth::SetCapsuleColliders(const AZStd::vector<AZ::u32>& capsuleIndices)
    {
        m_nvCloth->setCapsules(
            ToNvRange(capsuleIndices),
            0, m_nvCloth->getNumCapsules());
    }

    void Cloth::SetCapsuleColliders(AZStd::vector<AZ::u32>&& capsuleIndices)
    {
        SetCapsuleColliders(capsuleIndices); // NvCloth does not offer a move overload for setCapsules, calling the const reference one.
    }

    void Cloth::SetMotionConstraints(const AZStd::vector<AZ::Vector4>& constraints)
    {
        if (m_simParticles.size() != constraints.size())
        {
            AZ_Warning("Cloth", false, "Unable to set motions constraints as it doesn't match the number of particles. Numbers of constraints passed %zu, expected %zu.",
                constraints.size(), m_simParticles.size());
            return;
        }
        m_motionConstraints = constraints;
        nv::cloth::Range<physx::PxVec4> motionConstraints = m_nvCloth->getMotionConstraints();
        Internal::FastCopy(m_motionConstraints, motionConstraints);
    }

    void Cloth::SetMotionConstraints(AZStd::vector<AZ::Vector4>&& constraints)
    {
        if (m_simParticles.size() != constraints.size())
        {
            AZ_Warning("Cloth", false, "Unable to set motions constraints as it doesn't match the number of particles. Numbers of constraints passed %zu, expected %zu.",
                constraints.size(), m_simParticles.size());
            return;
        }
        m_motionConstraints = AZStd::move(constraints);
        nv::cloth::Range<physx::PxVec4> motionConstraints = m_nvCloth->getMotionConstraints();
        Internal::FastCopy(m_motionConstraints, motionConstraints);
    }

    void Cloth::ClearMotionConstraints()
    {
        m_motionConstraints.clear();
        m_nvCloth->clearMotionConstraints();
    }

    void Cloth::SetMotionConstraintsScale(float scale)
    {
        m_nvCloth->setMotionConstraintScaleBias(scale, m_nvCloth->getMotionConstraintBias());
    }

    void Cloth::SetMotionConstraintsBias(float bias)
    {
        m_nvCloth->setMotionConstraintScaleBias(m_nvCloth->getMotionConstraintScale(), bias);
    }

    void Cloth::SetMotionConstraintsStiffness(float stiffness)
    {
        m_nvCloth->setMotionConstraintStiffness(stiffness);
    }

    void Cloth::SetSeparationConstraints(const AZStd::vector<AZ::Vector4>& constraints)
    {
        if (m_simParticles.size() != constraints.size())
        {
            AZ_Warning("Cloth", false, "Unable to set separation constraints as it doesn't match the number of particles. Numbers of constraints passed %zu, expected %zu.",
                constraints.size(), m_simParticles.size());
            return;
        }
        nv::cloth::Range<physx::PxVec4> separationConstraints = m_nvCloth->getSeparationConstraints();
        Internal::FastCopy(constraints, separationConstraints);
    }

    void Cloth::SetSeparationConstraints(AZStd::vector<AZ::Vector4>&& constraints)
    {
        if (m_simParticles.size() != constraints.size())
        {
            AZ_Warning("Cloth", false, "Unable to set separation constraints as it doesn't match the number of particles. Numbers of constraints passed %zu, expected %zu.",
                constraints.size(), m_simParticles.size());
            return;
        }
        nv::cloth::Range<physx::PxVec4> separationConstraints = m_nvCloth->getSeparationConstraints();
        Internal::FastMove(AZStd::move(constraints), separationConstraints);
    }

    void Cloth::ClearSeparationConstraints()
    {
        m_nvCloth->clearSeparationConstraints();
    }

    void Cloth::ResolveStaticParticles()
    {
        if (m_collisionAffectsStaticParticles)
        {
            // Nothing to do as in NvCloth colliders affect static particles.
            return;
        }

        // During simulation static particle are always affected by colliders and motion constraints.
        // To remove the effect of colliders on Static Particles we will restore their positions,
        // either with the motion constraints (if existent) or the last simulated particles.

        nv::cloth::MappedRange<physx::PxVec4> particles = m_nvCloth->getCurrentParticles();

        const AZStd::vector<AZ::Vector4>& positions = m_motionConstraints.empty()
            ? m_simParticles
            : m_motionConstraints;

        for (AZ::u32 i = 0; i < particles.size(); ++i)
        {
            // Checking NvCloth current particles is important because their W component will
            // have the result left by the simulation applying both inverse masses and motion constraints.
            if (particles[i].w == 0.0f)
            {
                auto& particle = particles[i];
                const auto& position = positions[i];
                particle.x = position.GetX();
                particle.y = position.GetY();
                particle.z = position.GetZ();
            }
        }
    }

    bool Cloth::RetrieveSimulationResults()
    {
        const nv::cloth::MappedRange<const physx::PxVec4> particles = nv::cloth::readCurrentParticles(*m_nvCloth);

        bool validCloth =
            AZStd::all_of(particles.begin(), particles.end(), [](const physx::PxVec4& particle)
                {
                    return particle.isFinite();
                })
            &&
            // On some platforms when cloth simulation gets corrupted it puts all particles' position to (0,0,0)
            AZStd::any_of(particles.begin(), particles.end(), [](const physx::PxVec4& particle)
                {
                    return particle.x != 0.0f || particle.y != 0.0f || particle.z != 0.0f;
                });

        if (validCloth)
        {
            for (AZ::u32 i = 0; i < particles.size(); ++i)
            {
                m_simParticles[i].SetX(particles[i].x);
                m_simParticles[i].SetY(particles[i].y);
                m_simParticles[i].SetZ(particles[i].z);

                // Not copying inverse masses on purpose since they could be different after running the simulation.
                // This solves a problem when using a value of zero in the motion constraints distance
                // or scale. All inverse masses would go to zero and since we were copying them back,
                // the original data got lost and it was not able to return to a normal state after
                // changing the values back to values other than zero.
            }

            m_numInvalidSimulations = 0; // Reset counter as the results were valid
        }

        return validCloth;
    }

    void Cloth::RestoreSimulation()
    {
        nv::cloth::MappedRange<physx::PxVec4> previousParticles = m_nvCloth->getPreviousParticles();
        nv::cloth::MappedRange<physx::PxVec4> currentParticles = m_nvCloth->getCurrentParticles();

        const AZ::u32 maxAttemptsToRestoreCloth = 15;

        if (m_numInvalidSimulations <= maxAttemptsToRestoreCloth)
        {
            // Leave the NvCloth simulation particles in their last known good position.
            Internal::FastCopy(m_simParticles, previousParticles);
            Internal::FastCopy(m_simParticles, currentParticles);
        }
        else
        {
            // Reset NvCloth simulation particles to their initial position if after a number of
            // attempts cloth has not been restored to a stable state.
            Internal::FastCopy(m_initialParticlesWithMassApplied, previousParticles);
            Internal::FastCopy(m_initialParticlesWithMassApplied, currentParticles);
        }

        m_nvCloth->clearInertia();
        m_nvCloth->clearInterpolation();

        m_numInvalidSimulations++;
    }

    void Cloth::CopySimParticlesToNvCloth()
    {
        // The positions must be copied into the current particles inside NvCloth.
        // Note: Inverse masses are copied as well to do a fast copy,
        //       but inverse masses copied to current particles have no effect.
        nv::cloth::MappedRange<physx::PxVec4> currentParticles = m_nvCloth->getCurrentParticles();
        Internal::FastCopy(m_simParticles, currentParticles);

        CopySimInverseMassesToNvCloth();
    }

    void Cloth::CopySimInverseMassesToNvCloth()
    {
        // The inverse masses must be copied into the previous particles inside NvCloth
        // to take effect for the next simulation update.
        nv::cloth::MappedRange<physx::PxVec4> previousParticles = m_nvCloth->getPreviousParticles();
        for (AZ::u32 i = 0; i < previousParticles.size(); ++i)
        {
            previousParticles[i].w = m_simParticles[i].GetW();
        }
    }

    void Cloth::SetPhaseConfig(
        int32_t phaseType,
        float stiffness,
        float stiffnessMultiplier,
        float compressionLimit,
        float stretchLimit)
    {
        const auto& phaseTypes = m_fabric->GetPhaseTypes();
        for (size_t i = 0; i < phaseTypes.size(); ++i)
        {
            if (phaseTypes[i] == phaseType)
            {
                m_nvPhaseConfigs[i].mStiffness = stiffness;
                m_nvPhaseConfigs[i].mStiffnessMultiplier = 1.0f - AZ::GetClamp(stiffnessMultiplier, 0.0f, 1.0f); // Internally a value of 1 means no scale inside nvcloth.
                m_nvPhaseConfigs[i].mCompressionLimit = 1.0f + compressionLimit; // A value of 1.0f is no compression inside nvcloth. From [0.0, INF] to [1.0, INF].
                m_nvPhaseConfigs[i].mStretchLimit = 1.0f + stretchLimit; // A value of 1.0f is no stretch inside nvcloth. From [0.0, INF] to [1.0, INF].
            }
        }
        ApplyPhaseConfigs();
    }

    void Cloth::ApplyPhaseConfigs()
    {
        m_nvCloth->setPhaseConfig(
            ToNvRange(m_nvPhaseConfigs));
    }

} // namespace NvCloth
