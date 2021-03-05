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

#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>

#include <World.h> // Temporary until LYN-438 work is complete

namespace PhysX
{
    //! PhysX implementation of the AzPhysics::Scene.
    class PhysXScene
        : public AzPhysics::Scene
    {
    public:
        AZ_RTTI(PhysXScene, "{B0FCFDE6-8B59-49D8-8819-E8C2F1EDC182}", AzPhysics::Scene);

        PhysXScene(const AzPhysics::SceneConfiguration& config);
        ~PhysXScene() = default;

        // AzPhysics::PhysicsScene ...
        void StartSimulation(float deltatime) override;
        void FinishSimulation() override;
        void Enable(bool enable) override;
        bool IsEnabled() const override;
        const AzPhysics::SceneConfiguration& GetConfiguration() const override;
        void UpdateConfiguration(const AzPhysics::SceneConfiguration& config) override;

        // Temporary until LYN-438 work is complete
        AZStd::shared_ptr<Physics::World> GetLegacyWorld() const override { return m_legacyWorld; }
    private:
        bool m_isEnabled = true;
        AzPhysics::SceneConfiguration m_config;

        AzPhysics::SystemEvents::OnDefaultSceneConfigurationChangedEvent::Handler m_defaultSceneConfigChanged;

        AZStd::shared_ptr<World> m_legacyWorld; // Temporary until LYN-438 work is complete
    };
}
