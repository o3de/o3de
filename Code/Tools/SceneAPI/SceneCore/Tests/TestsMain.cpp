/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <SceneAPI/SceneCore/SceneCoreStandaloneAllocator.h>

class SceneCoreTestEnvironment
    : public AZ::Test::ITestEnvironment
{
public:
    virtual ~SceneCoreTestEnvironment() {}

protected:
    void SetupEnvironment() override
    {
        AZ::SceneAPI::SceneCoreStandaloneAllocator::Initialize();
    }

    void TeardownEnvironment() override
    {
        AZ::SceneAPI::SceneCoreStandaloneAllocator::TearDown();
    }
};

AZ_UNIT_TEST_HOOK(new SceneCoreTestEnvironment);
