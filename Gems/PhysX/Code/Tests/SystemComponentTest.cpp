/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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

        AzPhysics::SceneConfiguration newConfiguration;

        newConfiguration.m_gravity = newGravity;

        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        physicsSystem->RegisterOnDefaultSceneConfigurationChangedEventHandler(defaultSceneConfigHandler);
        physicsSystem->UpdateDefaultSceneConfiguration(newConfiguration);

        EXPECT_TRUE(handlerInvoked);
    }
} // namespace PhysXEditorTests
