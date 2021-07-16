/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>

#include <NvCloth/ICloth.h>
#include <NvCloth/IClothConfigurator.h>

#include <System/NvTypes.h>

// NvCloth library includes
#include <NvCloth/PhaseConfig.h>

namespace NvCloth
{
    class Solver;
    class Fabric;

    //! Implementation of the ICloth and IClothConfigurator interfaces.
    class Cloth
        : public ICloth
        , public IClothConfigurator
    {
    public:
        AZ_RTTI(Cloth, "{D9DEED18-FEF2-440B-8639-A080F8C1F6DB}", ICloth);

        Cloth(
            ClothId id,
            const AZStd::vector<SimParticleFormat>& initialParticles,
            Fabric* fabric,
            NvClothUniquePtr nvCloth);
        ~Cloth();

        //! Returns the fabric used to create this cloth.
        Fabric* GetFabric() { return m_fabric; }

        //! Returns the solver this cloth is added to or nullptr if it's not part of any solver.
        Solver* GetSolver() { return m_solver; }

        //! Retrieves the latest simulation data from NvCloth and updates the particles.
        void Update();

        // ICloth overrides ...
        ClothId GetId() const override;
        const AZStd::vector<SimParticleFormat>& GetInitialParticles() const override;
        const AZStd::vector<SimIndexType>& GetInitialIndices() const override;
        const AZStd::vector<SimParticleFormat>& GetParticles() const override;
        void SetParticles(const AZStd::vector<SimParticleFormat>& particles) override;
        void SetParticles(AZStd::vector<SimParticleFormat>&& particles) override;
        void DiscardParticleDelta() override;
        const FabricCookedData& GetFabricCookedData() const override;
        IClothConfigurator* GetClothConfigurator() override;

        // IClothConfigurator overrides ...
        void SetTransform(const AZ::Transform& transformWorld) override;
        void ClearInertia() override;
        void SetMass(float mass) override;
        void SetGravity(const AZ::Vector3& gravity) override;
        void SetStiffnessFrequency(float frequency) override;
        void SetDamping(const AZ::Vector3& damping) override;
        void SetDampingLinearDrag(const AZ::Vector3& linearDrag) override;
        void SetDampingAngularDrag(const AZ::Vector3& angularDrag) override;
        void SetLinearInertia(const AZ::Vector3& linearInertia) override;
        void SetAngularInertia(const AZ::Vector3& angularInertia) override;
        void SetCentrifugalInertia(const AZ::Vector3& centrifugalInertia) override;
        void SetWindVelocity(const AZ::Vector3& velocity) override;
        void SetWindDragCoefficient(float drag) override;
        void SetWindLiftCoefficient(float lift) override;
        void SetWindFluidDensity(float density) override;
        void SetCollisionFriction(float friction) override;
        void SetCollisionMassScale(float scale) override;
        void EnableContinuousCollision(bool value) override;
        void SetCollisionAffectsStaticParticles(bool value) override;
        void SetSelfCollisionDistance(float distance) override;
        void SetSelfCollisionStiffness(float stiffness) override;
        void SetVerticalPhaseConfig(
            float stiffness,
            float stiffnessMultiplier,
            float compressionLimit,
            float stretchLimit) override;
        void SetHorizontalPhaseConfig(
            float stiffness,
            float stiffnessMultiplier,
            float compressionLimit,
            float stretchLimit) override;
        void SetBendingPhaseConfig(
            float stiffness,
            float stiffnessMultiplier,
            float compressionLimit,
            float stretchLimit) override;
        void SetShearingPhaseConfig(
            float stiffness,
            float stiffnessMultiplier,
            float compressionLimit,
            float stretchLimit) override;
        void SetTetherConstraintStiffness(float stiffness) override;
        void SetTetherConstraintScale(float scale) override;
        void SetSolverFrequency(float frequency) override;
        void SetAcceleationFilterWidth(AZ::u32 width) override;
        void SetSphereColliders(const AZStd::vector<AZ::Vector4>& spheres) override;
        void SetSphereColliders(AZStd::vector<AZ::Vector4>&& spheres) override;
        void SetCapsuleColliders(const AZStd::vector<AZ::u32>& capsuleIndices) override;
        void SetCapsuleColliders(AZStd::vector<AZ::u32>&& capsuleIndices) override;
        void SetMotionConstraints(const AZStd::vector<AZ::Vector4>& constraints) override;
        void SetMotionConstraints(AZStd::vector<AZ::Vector4>&& constraints) override;
        void ClearMotionConstraints() override;
        void SetMotionConstraintsScale(float scale) override;
        void SetMotionConstraintsBias(float bias) override;
        void SetMotionConstraintsStiffness(float stiffness) override;
        void SetSeparationConstraints(const AZStd::vector<AZ::Vector4>& constraints) override;
        void SetSeparationConstraints(AZStd::vector<AZ::Vector4>&& constraints) override;
        void ClearSeparationConstraints() override;

    private:
        void ResolveStaticParticles();
        bool RetrieveSimulationResults();
        void RestoreSimulation();

        // Copies up current particles to NvCloth.
        void CopySimParticlesToNvCloth();

        // Copies up current inverse masses to NvCloth.
        void CopySimInverseMassesToNvCloth();

        void SetPhaseConfig(
            int32_t phaseType,
            float stiffness,
            float stiffnessMultiplier,
            float compressionLimit,
            float stretchLimit);
        void ApplyPhaseConfigs();

        // Cloth unique identifier.
        ClothId m_id;

        // NvCloth cloth object.
        NvClothUniquePtr m_nvCloth;

        // Fabric used to create this cloth.
        Fabric* m_fabric = nullptr;

        // Current solver this cloth is added to.
        Solver* m_solver = nullptr;

        // Initial data from cloth creation.
        AZStd::vector<SimParticleFormat> m_initialParticles;
        AZStd::vector<SimParticleFormat> m_initialParticlesWithMassApplied; // Needed by RestoreSimulation

        // Current simulation particles (positions + inverse masses).
        AZStd::vector<SimParticleFormat> m_simParticles;

        // Current mass value applied to all particles.
        float m_mass = 1.0f;

        // When true, colliders affect static particles.
        bool m_collisionAffectsStaticParticles = false;

        // Current phases configuration data.
        AZStd::vector<nv::cloth::PhaseConfig> m_nvPhaseConfigs;

        // Current motion constraints.
        // Caching it to be used in ResolveStaticParticles(), having it available avoids
        // having to call m_nvCloth->getMotionConstraints(), which there is no const version
        // and would wake the simulation.
        AZStd::vector<AZ::Vector4> m_motionConstraints;

        // Number of continuous invalid simulations.
        // That's when NvCloth provided invalid data when retrieving simulation results.
        AZ::u32 m_numInvalidSimulations = 0;

        // Solver has the responsibility of adding/removing cloths to solvers,
        // so it needs exclusive access to m_solver and m_nvCloth members.
        friend class Solver;
    };
} // namespace NvCloth
