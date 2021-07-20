/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>

#include <NvCloth/Types.h>

namespace NvCloth
{
    class ISolver;
    class ICloth;
    class IClothTangentSpace;

    //! Interface to the cloth system that allows to create/destroy cloths and solvers.
    //!
    //! A default solver is always present in the system.
    //! @note Use AZ::Interface<IClothSystem>::Get() to call the interface.
    class IClothSystem
    {
    public:
        AZ_RTTI(IClothSystem, "{83C01566-D028-4BE8-BE95-0A9DFE6137CA}");

        virtual ~IClothSystem() = default;

        //! Looks for a solver and if it cannot find it then it creates it.
        //!
        //! @param name Name the solver.
        //!             Empty string is an invalid solver name.
        //! @return The solver created/found or nullptr if unable to create it.
        virtual ISolver* FindOrCreateSolver(const AZStd::string& name) = 0;

        //! Destroys the solver passed as parameter.
        //! Any reference kept to the solver will be invalid.
        //! Any cloth the solver still has will be automatically removed.
        //!
        //! @param solver The solver to be destroyed. Variable will be set to nullptr.
        virtual void DestroySolver(ISolver*& solver) = 0;

        //! Returns a solver from the system, identified by its name.
        //!
        //! @param name Name the solver. Empty string is an invalid name.
        //! @return The solver found or nullptr if it doesn't exists.
        virtual ISolver* GetSolver(const AZStd::string& name) = 0;

        //! Creates a cloth instance from a fabric.
        //!
        //! @param initialParticles Initial simulation positions and inverse masses for the cloth to start the simulation.
        //!                         They do not have to be the same positions and inverse masses used to cook the fabric.
        //! @param fabricCookedData The fabric data used to create the cloth.
        //! @return The cloth instance or nullptr if unable to create it.
        virtual ICloth* CreateCloth(
            const AZStd::vector<SimParticleFormat>& initialParticles,
            const FabricCookedData& fabricCookedData) = 0;

        //! Destroys the cloth instance passed as parameter.
        //! Any reference kept to the cloth will be invalid.
        //! The cloth be automatically removed from a solver, in case it's still added to one.
        //!
        //! @param cloth The cloth to be destroyed. Variable will be set to nullptr.
        virtual void DestroyCloth(ICloth*& cloth) = 0;

        //! Returns a cloth from the system, identified by its id.
        //!
        //! @param clothId The cloth identifier.
        //! @return The cloth found or nullptr if it doesn't exist.
        virtual ICloth* GetCloth(ClothId clothId) = 0;

        //! Adds a cloth instance to a solver.
        //! Once a cloth is part of a solver it will be simulated and its events
        //! will be signaled.
        //! A cloth can only be added to one solver at a time, if the cloth
        //! was previously added to another solver it will be removed from it first.
        //!
        //! @param cloth The cloth instance to add to the solver.
        //! @param solverName Name of the solver to add the cloth into.
        //!                   By default the cloth will be added to the default solver.
        //!                   Empty string is an invalid solver name.
        //! @return Whether it successfully added cloth to the solver or not.
        virtual bool AddCloth(ICloth* cloth, const AZStd::string& solverName = DefaultSolverName) = 0;

        //! Removes a cloth instance from its solver.
        //! Once a cloth is not part of a solver it will not be simulated and its events
        //! will not be signaled.
        //! If the cloth was not previously part of any solver this function doesn't have any effect.
        //!
        //! @param cloth The cloth instance to remove from the solver.
        virtual void RemoveCloth(ICloth* cloth) = 0;
    };
} // namespace NvCloth
