/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/ComponentApplication.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <EditorPythonBindings/EditorPythonBindingsSymbols.h>

#include "Source/PythonAssetBuilderSystemComponent.h"
#include <PythonAssetBuilder/PythonAssetBuilderBus.h>

namespace UnitTest
{
    class PythonBuilderRegisterJobsTest
        : public LeakDetectionFixture
    {
    protected:
        AZStd::unique_ptr<AZ::ComponentApplication> m_app;
        AZ::Entity* m_systemEntity = nullptr;

        void SetUp() override
        {
            AZ::ComponentApplication::Descriptor appDesc;
            m_app = AZStd::make_unique<AZ::ComponentApplication>();
            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_systemEntity = m_app->Create(appDesc, startupParameters);
        }

        void TearDown() override
        {
            m_app.reset();
        }
    };

    TEST_F(PythonBuilderRegisterJobsTest, PythonBuilder_RegisterBuilder_Regex)
    {
        using namespace PythonAssetBuilder;

        m_app->RegisterComponentDescriptor(PythonAssetBuilderSystemComponent::CreateDescriptor());
        m_systemEntity->CreateComponent<PythonAssetBuilderSystemComponent>();
        m_systemEntity->Init();
        m_systemEntity->Activate();

        AssetBuilderSDK::AssetBuilderPattern buildPattern;
        buildPattern.m_pattern = R"_(^.*\.foo$)_";
        buildPattern.m_type = AssetBuilderSDK::AssetBuilderPattern::Regex;

        AssetBuilderSDK::AssetBuilderDesc builderDesc;
        builderDesc.m_busId = AZ::Uuid::CreateRandom();
        builderDesc.m_name = "Mock Builder Regex";
        builderDesc.m_patterns.push_back(buildPattern);
        builderDesc.m_version = 0;

        AZ::Outcome<bool, AZStd::string> result;
        PythonAssetBuilderRequestBus::BroadcastResult(result, &PythonAssetBuilderRequestBus::Events::RegisterAssetBuilder, builderDesc);
        EXPECT_TRUE(result.IsSuccess());
    }

    TEST_F(PythonBuilderRegisterJobsTest, PythonBuilder_RegisterBuilder_Wildcard)
    {
        using namespace PythonAssetBuilder;

        m_app->RegisterComponentDescriptor(PythonAssetBuilderSystemComponent::CreateDescriptor());
        m_systemEntity->CreateComponent<PythonAssetBuilderSystemComponent>();
        m_systemEntity->Init();
        m_systemEntity->Activate();

        AssetBuilderSDK::AssetBuilderPattern buildPattern;
        buildPattern.m_pattern = "a/path/to/*.foo";
        buildPattern.m_type = AssetBuilderSDK::AssetBuilderPattern::Wildcard;

        AssetBuilderSDK::AssetBuilderDesc builderDesc;
        builderDesc.m_busId = AZ::Uuid::CreateRandom();
        builderDesc.m_name = "Mock Builder Wildcard";
        builderDesc.m_patterns.push_back(buildPattern);
        builderDesc.m_version = 0;

        AZ::Outcome<bool, AZStd::string> result;
        PythonAssetBuilderRequestBus::BroadcastResult(result, &PythonAssetBuilderRequestBus::Events::RegisterAssetBuilder, builderDesc);
        EXPECT_TRUE(result.IsSuccess());
    }
}
