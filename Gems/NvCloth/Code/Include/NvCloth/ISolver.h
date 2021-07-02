/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/std/function/function_template.h>
#include <AzCore/std/string/string.h>

namespace NvCloth
{
    //! Interface to a solver in the system.
    //! A solver contains cloth instances and they run simulation to all of them.
    //!
    //! @note Use IClothSystem interface to obtain a solver from the system.
    class ISolver
    {
    public:
        AZ_RTTI(ISolver, "{4077FEB2-78E3-4A8F-AA33-67446E6ECD1F}");

        virtual ~ISolver() = default;

        //! Event signaled before running simulation in the solver.
        //! @param (Unnamed) Name of the solver.
        //! @param (Unnamed) Delta time
        using PreSimulationEvent = AZ::Event<const AZStd::string&, float>;

        //! Event signaled after running simulation in the solver.
        //! @param (Unnamed) Name of the solver.
        //! @param (Unnamed) Delta time.
        using PostSimulationEvent = AZ::Event<const AZStd::string&, float>;

        //! Returns name of the solver.
        virtual const AZStd::string& GetName() const = 0;

        //! Enable or disable running simulation on the solver.
        //! When the solver is disabled it won't run simulation and its events
        //! will not be signaled.
        virtual void Enable(bool value) = 0;

        //! Returns whether the solver is enabled or not.
        virtual bool IsEnabled() const = 0;

        //! Set the solver into user-simulated mode.
        //! When the solver is user-simulated the user will be responsible to call StartSimulation and FinishSimulation
        //! functions, otherwise they will be called by the cloth system.
        virtual void SetUserSimulated(bool value) = 0;

        //! Returns whether the solver StartSimulation and FinishSimulation functions will be called by the user or the cloth system.
        virtual bool IsUserSimulated() const = 0;

        //! Start simulation of all the cloths that are part of this solver. This will setup and start cloth simulation jobs.
        //! If the solver is in user-simulated mode the user is responsible for calling this function.
        //! Note: This is a non-blocking call.
        virtual void StartSimulation(float deltaTime) = 0;

        //! Complete the simulation process.
        //! If the solver is in user-simulated mode the user is responsible for calling this function.
        //! Note: This is a blocking call that will wait for the simulation jobs to complete.
        virtual void FinishSimulation() = 0;

        //! Specifies the distance (meters) that cloths' particles need to be separated from each other.
        //! Inter-collision refers to collisions between different cloth instances in the solver,
        //! do not confuse with self-collision, which is available per cloth through IClothConfigurator.
        //! When distance is 0 inter-collision is disabled (default).
        //!
        //! @note Using inter-collision with more than 32 cloths added to the solver will cause undefined behavior.
        virtual void SetInterCollisionDistance(float distance) = 0;

        //! Sets the stiffness for inter-collision constraints.
        //! Stiffness range is [0.0, 1.0]. Default value is 1.0.
        virtual void SetInterCollisionStiffness(float stiffness) = 0;

        //! Sets the number of iterations the solver will do during inter-collision.
        //! Default value is 1.
        virtual void SetInterCollisionIterations(AZ::u32 iterations) = 0;

        //! Connects a handler to the PreSimulationEvent.
        void ConnectPreSimulationEventHandler(PreSimulationEvent::Handler& handler)
        {
            handler.Connect(m_preSimulationEvent);
        }

        //! Connects a handler to the PostSimulationEvent.
        void ConnectPostSimulationEventHandler(PostSimulationEvent::Handler& handler)
        {
            handler.Connect(m_postSimulationEvent);
        }

    protected:
        PreSimulationEvent m_preSimulationEvent;
        PostSimulationEvent m_postSimulationEvent;
    };
} // namespace NvCloth
