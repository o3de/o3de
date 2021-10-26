/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/Budget.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/std/function/function_template.h>

#include <NvCloth/Types.h>

AZ_DECLARE_BUDGET(Cloth);

namespace NvCloth
{
    class IClothConfigurator;

    //! Interface to a cloth in the system.
    //! A cloth is formed of particles that are simulated with a series of constraints specified by a fabric.
    //! A cloth must be added to a solver to be simulated.
    //!
    //! @note Use IClothSystem interface to obtain a cloth from the system.
    class ICloth
    {
    public:
        AZ_RTTI(ICloth, "{78817F38-E4A5-4B94-BCD8-3E3B73F66B5A}");

        virtual ~ICloth() = default;

        //! Event signaled before running simulation.
        //! @param (Unnamed) The cloth identifier.
        //! @param (Unnamed) Delta time
        using PreSimulationEvent = AZ::Event<ClothId, float>;

        //! Event signaled after running simulation.
        //! @param (Unnamed) The cloth identifier.
        //! @param (Unnamed) Delta time.
        //! @param (Unnamed) New particles (positions and inverse masses) result of running the simulation.
        using PostSimulationEvent = AZ::Event<ClothId, float, const AZStd::vector<SimParticleFormat>&>;

        //! Returns the cloth identifier.
        virtual ClothId GetId() const = 0;

        //! Returns the list of particles (positions and inverse mass) used when the cloth was created.
        virtual const AZStd::vector<SimParticleFormat>& GetInitialParticles() const = 0;

        //! Returns the list of triangles' indices used when the cloth's fabric was created.
        virtual const AZStd::vector<SimIndexType>& GetInitialIndices() const = 0;

        //! Returns the current list of particles (positions and inverse mass) of cloth.
        virtual const AZStd::vector<SimParticleFormat>& GetParticles() const = 0;

        //! Sets the current particles (positions and inverse mass) for cloth.
        //!
        //! @param particles List of particles to set.
        //!
        //! @note This function results in a copy of all particle data
        //!       to NvCloth and therefore it's not a fast operation.
        virtual void SetParticles(const AZStd::vector<SimParticleFormat>& particles) = 0;

        //! Sets the current particles (positions and inverse mass) for cloth.
        //!
        //! @param particles List of particles to set (rvalue overload).
        //!
        //! @note This function results in a copy of all particle data
        //!       to NvCloth and therefore it's not a fast operation.
        virtual void SetParticles(AZStd::vector<SimParticleFormat>&& particles) = 0;

        //! Makes current and previous particles the same, the next simulation
        //! will not have any effect since delta positions will be zero.
        virtual void DiscardParticleDelta() = 0;

        //! Returns the FabricCookedData used when the cloth was created.
        virtual const FabricCookedData& GetFabricCookedData() const = 0;

        //! Returns the interface to IClothConfigurator to set all cloth
        //! parameters that define its behavior during simulation.
        virtual IClothConfigurator* GetClothConfigurator() = 0;

        //! Connects a handler to the PreSimulationEvent.
        //! Note that the events can be triggered from multiple threads at the same time.
        //! Please make sure the handler is reentrant and thread-safe.
        void ConnectPreSimulationEventHandler(PreSimulationEvent::Handler& handler)
        {
            handler.Connect(m_preSimulationEvent);
        }

        //! Connects a handler to the PostSimulationEvent.
        //! Note that the events can be triggered from multiple threads at the same time.
        //! Please make sure the handler is reentrant and thread-safe.
        void ConnectPostSimulationEventHandler(PostSimulationEvent::Handler& handler)
        {
            handler.Connect(m_postSimulationEvent);
        }

    protected:
        PreSimulationEvent m_preSimulationEvent;
        PostSimulationEvent m_postSimulationEvent;
    };
} // namespace NvCloth
