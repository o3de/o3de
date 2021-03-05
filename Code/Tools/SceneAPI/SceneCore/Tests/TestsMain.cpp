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
        AZ::Environment::Create(nullptr);
        AZ::SceneAPI::SceneCoreStandaloneAllocator::Initialize(AZ::Environment::GetInstance());
    }

    void TeardownEnvironment() override
    {
        AZ::SceneAPI::SceneCoreStandaloneAllocator::TearDown();
        AZ::Environment::Destroy();
    }
};

AZ_UNIT_TEST_HOOK(new SceneCoreTestEnvironment);
