/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzFramework/Physics/SystemBus.h>

namespace AzPhysics
{
    class Scene;
    struct TriggerEvent;
}

namespace PhysX
{
    //! PhysXDefaultWorldTest is a test fixture which creates a default world,
    //! and implements common default world behavior.
    class PhysXDefaultWorldTest
        : public ::testing::Test
        , protected Physics::DefaultWorldBus::Handler
    {
    protected:
        void SetUp() override;
        void TearDown() override;

        // DefaultWorldBus
        AzPhysics::SceneHandle GetDefaultSceneHandle() const override;

        AzPhysics::Scene* m_defaultScene = nullptr;
        AzPhysics::SceneHandle m_testSceneHandle = AzPhysics::InvalidSceneHandle;
    };

    //! Extension of PhysXDefaultWorldTest to support parameterized tests.
    template <typename T>
    class PhysXDefaultWorldTestWithParam
        : public PhysXDefaultWorldTest
        , public ::testing::WithParamInterface<T>
    {
    };
}
