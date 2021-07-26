/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

#include <NvCloth/IClothSystem.h>
#include <NvCloth/ICloth.h>
#include <NvCloth/ISolver.h>

#include <System/Factory.h>
#include <System/Solver.h>
#include <System/Fabric.h>
#include <System/Cloth.h>

namespace NvCloth
{
    //! Implementation of the IClothSystem interface.
    //!
    //! This class has the responsibility to initialize and tear down NvCloth library.
    //! It owns all Solvers, Cloths and Fabrics, and it manages their creation and destruction.
    //! It's also the responsible for updating (on Physics Tick) all the solvers that are not flagged as "user simulated".
    class SystemComponent
        : public AZ::Component
        , protected IClothSystem
        , protected AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(SystemComponent, "{89DF5C48-64AC-4B8E-9E61-0D4C7A7B5491}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        static void InitializeNvClothLibrary();
        static void TearDownNvClothLibrary();

        //! Returns true when there is no error reported.
        static bool CheckLastClothError();

        //! Resets the last error reported by NvCloth.
        static void ResetLastClothError();

    protected:
        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // IClothSystem overrides ...
        ISolver* FindOrCreateSolver(const AZStd::string& name) override;
        void DestroySolver(ISolver*& solver) override;
        ISolver* GetSolver(const AZStd::string& name) override;
        ICloth* CreateCloth(
            const AZStd::vector<SimParticleFormat>& initialParticles,
            const FabricCookedData& fabricCookedData) override;
        void DestroyCloth(ICloth*& cloth) override;
        ICloth* GetCloth(ClothId clothId) override;
        bool AddCloth(ICloth* cloth, const AZStd::string& solverName = DefaultSolverName) override;
        void RemoveCloth(ICloth* cloth) override;

        // AZ::TickBus::Handler overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;

    private:
        void InitializeSystem();
        void DestroySystem();

        FabricId FindOrCreateFabric(const FabricCookedData& fabricCookedData);
        void DestroyFabric(FabricId fabricId);

        // Factory that creates all the solvers, fabric and cloths.
        AZStd::unique_ptr<Factory> m_factory;

        // List of all the solvers created.
        AZStd::vector<AZStd::unique_ptr<Solver>> m_solvers;

        // List of all the fabrics created.
        AZStd::unordered_map<FabricId, AZStd::unique_ptr<Fabric>> m_fabrics;

        // List of all the cloths created.
        AZStd::unordered_map<ClothId, AZStd::unique_ptr<Cloth>> m_cloths;
    };
} // namespace NvCloth
