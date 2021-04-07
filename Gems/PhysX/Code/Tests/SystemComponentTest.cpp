/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "PhysX_precompiled.h"

#include <Tests/EditorTestUtilities.h>

namespace PhysXEditorTests
{
    TEST_F(PhysXEditorFixture, SetDefaultSceneConfiguration_TriggersHandler)
    {
        bool handlerInvoked = false;
        AzPhysics::SystemEvents::OnDefaultSceneConfigurationChangedEvent::Handler defaultSceneConfigHandler(
            [&handlerInvoked]([[maybe_unused]] const AzPhysics::SceneConfiguration* config)
            {
                handlerInvoked = true;
            });

        // Initialize new configs with some non-default values.
        const AZ::Vector3 newGravity(2.f, 5.f, 7.f);
        const float newFixedTimeStep = 0.008f;
        const float newMaxTimeStep = 0.034f;

        AzPhysics::SceneConfiguration newConfiguration;

        newConfiguration.m_gravity = newGravity;

        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        physicsSystem->RegisterOnDefaultSceneConfigurationChangedEventHandler(defaultSceneConfigHandler);
        physicsSystem->UpdateDefaultSceneConfiguration(newConfiguration);

        EXPECT_TRUE(handlerInvoked);
    }
} // namespace PhysXEditorTests
