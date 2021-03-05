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
#include <PhysX_precompiled.h>

#include <Scene/PhysXScene.h>

namespace PhysX
{
    PhysXScene::PhysXScene(const AzPhysics::SceneConfiguration& config)
        : m_legacyWorld(AZStd::make_shared<World>(config.m_legacyId, config.m_legacyConfiguration))
        , m_config(config)
    {

    }

    void PhysXScene::StartSimulation(float deltatime)
    {
        if (IsEnabled() && m_legacyWorld)
        {
            m_legacyWorld->StartSimulation(deltatime);
        }
    }

    void PhysXScene::FinishSimulation()
    {
        if (IsEnabled() && m_legacyWorld)
        {
            m_legacyWorld->FinishSimulation();
        }
    }

    void PhysXScene::Enable(bool enable)
    {
        m_isEnabled = enable;
    }

    bool PhysXScene::IsEnabled() const
    {
        return m_isEnabled;
    }

    const AzPhysics::SceneConfiguration& PhysXScene::GetConfiguration() const
    {
        return m_config;
    }

    void PhysXScene::UpdateConfiguration(const AzPhysics::SceneConfiguration& config)
    {
        m_config = config;
    }
}
