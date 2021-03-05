/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/RTTI/RTTI.h>

#include <AzFramework/Physics/World.h> // Temporary until LYN-438 work is complete

namespace AzPhysics
{
    struct SceneConfiguration;

    class Scene
    {
    public:
        AZ_RTTI(Scene, "{52BD8163-BDC4-4B09-ABB2-11DD1F601FFD}");

        Scene() = default;
        virtual ~Scene() = default;

        //! Start the simulation process.
        //! As an example, this is a good place to trigger and queue any long running work in separate threads.
        //! @param deltatime The time in seconds to run the simulation for.
        virtual void StartSimulation(float deltatime) = 0;

        //! Complete the simulation process.
        //! As an example, this is a good place to wait for any work to complete that was triggered in StartSimulation, or swap buffers if double buffering.
        virtual void FinishSimulation() = 0;

        //! Enable or Disable this Scene's Simulation tick.
        //! Default is Enabled.
        //! @param enable When true the Scene will execute its simulation tick when StartSimulation is called. When false, StartSimulation will not execute.
        virtual void Enable(bool enable) = 0;

        //! Check if this Scene is currently Enabled.
        //! @return When true the Scene is enabled and will execute its simulation tick when StartSimulation is called. When false, StartSimulation will not execute.
        virtual bool IsEnabled() const = 0;

        //! Accessor to the Scenes Configuration.
        //! @returns Return the currently used SceneConfiguration.
        virtual const SceneConfiguration& GetConfiguration() const = 0;

        //! Update the SceneConfiguration.
        //! @param config The new configuration to apply.
        virtual void UpdateConfiguration(const SceneConfiguration& config) = 0;

        // Temporary until LYN-438 work is complete
        virtual AZStd::shared_ptr<Physics::World> GetLegacyWorld() const = 0;
    };
    using SceneList = AZStd::vector<Scene*>;
} // namespace AzPhysics
