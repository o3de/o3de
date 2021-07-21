/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/ComponentApplication.h>

#include <EditorPythonBindings/EditorPythonBindingsSymbols.h>

#include "Source/PythonAssetBuilderSystemComponent.h"
#include <PythonAssetBuilder/PythonAssetBuilderBus.h>
#include <PythonAssetBuilder/PythonBuilderRequestBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace UnitTest
{
    class PythonAssetBuilderTest
        : public ScopedAllocatorSetupFixture
    {
    protected:
        AZStd::unique_ptr<AZ::ComponentApplication> m_app;
        AZ::Entity* m_systemEntity = nullptr;

        void SetUp() override
        {
            AZ::ComponentApplication::Descriptor appDesc;
            m_app = AZStd::make_unique<AZ::ComponentApplication>();
            m_systemEntity = m_app->Create(appDesc);
        }

        void TearDown() override
        {
            m_app.reset();
        }
    };

    TEST_F(PythonAssetBuilderTest, SystemComponent_InitActivate)
    {
        m_app->RegisterComponentDescriptor(PythonAssetBuilder::PythonAssetBuilderSystemComponent::CreateDescriptor());

        m_systemEntity->CreateComponent<PythonAssetBuilder::PythonAssetBuilderSystemComponent>();
        m_systemEntity->Init();
        EXPECT_EQ(AZ::Entity::State::Init, m_systemEntity->GetState());
        m_systemEntity->Activate();
        EXPECT_EQ(AZ::Entity::State::Active, m_systemEntity->GetState());
    }

    TEST_F(PythonAssetBuilderTest, SystemComponent_RegisterAssetBuilder)
    {
        using namespace PythonAssetBuilder;

        m_app->RegisterComponentDescriptor(PythonAssetBuilderSystemComponent::CreateDescriptor());
        m_systemEntity->CreateComponent<PythonAssetBuilderSystemComponent>();
        m_systemEntity->Init();
        m_systemEntity->Activate();

        AssetBuilderSDK::AssetBuilderDesc mockAssetBuilderDesc;
        mockAssetBuilderDesc.m_busId = AZ::Uuid::CreateString("{C68C8E96-223A-46BD-8D4A-E159A85AC02A}");

        AZ::Outcome<bool, AZStd::string> result;
        PythonAssetBuilderRequestBus::BroadcastResult(result, &PythonAssetBuilderRequestBus::Events::RegisterAssetBuilder, mockAssetBuilderDesc);
        EXPECT_TRUE(result.IsSuccess());
    }

    TEST_F(PythonAssetBuilderTest, PythonAssetBuilderRequestBus_GetExecutableFolder_Works)
    {
        using namespace PythonAssetBuilder;

        EXPECT_FALSE(PythonAssetBuilderRequestBus::HasHandlers());

        m_app->RegisterComponentDescriptor(PythonAssetBuilderSystemComponent::CreateDescriptor());
        m_systemEntity->CreateComponent<PythonAssetBuilderSystemComponent>();
        m_systemEntity->Init();
        m_systemEntity->Activate();

        EXPECT_TRUE(PythonAssetBuilderRequestBus::HasHandlers());

        AZ::Outcome<AZStd::string, AZStd::string> result;
        PythonAssetBuilderRequestBus::BroadcastResult(
            result,
            &PythonAssetBuilderRequestBus::Events::GetExecutableFolder);
        EXPECT_TRUE(result.IsSuccess());
    }

    // test bus API exists

    TEST_F(PythonAssetBuilderTest, PythonBuilderRequestBus_CreateEditorEntity_Exists)
    {
        using namespace PythonAssetBuilder;

        EXPECT_FALSE(PythonBuilderRequestBus::HasHandlers());

        // Some static tests to make sure the public API has not changed since that
        // would break Python asset builders using this EBus
        {
            AZ::Outcome<AZ::EntityId, AZStd::string> result;
            AZStd::string name;
            PythonBuilderRequestBus::BroadcastResult(
                result,
                &PythonBuilderRequestBus::Events::CreateEditorEntity,
                name);
            EXPECT_FALSE(result.IsSuccess());
        }

        m_app->RegisterComponentDescriptor(PythonAssetBuilderSystemComponent::CreateDescriptor());
        m_systemEntity->CreateComponent<PythonAssetBuilderSystemComponent>();
        m_systemEntity->Init();
        m_systemEntity->Activate();

        EXPECT_TRUE(PythonBuilderRequestBus::HasHandlers());
    }

    TEST_F(PythonAssetBuilderTest, PythonBuilderRequestBus_WriteSliceFile_Exists)
    {
        using namespace PythonAssetBuilder;

        EXPECT_FALSE(PythonBuilderRequestBus::HasHandlers());

        // Some static tests to make sure the public API has not changed since that
        // would break Python asset builders using this EBus
        {
            AZ::Outcome<AZ::Data::AssetType, AZStd::string> result;
            AZStd::string_view filename;
            AZStd::vector<AZ::EntityId> entities;
            bool makeDynamic = {};
            PythonBuilderRequestBus::BroadcastResult(
                result,
                &PythonBuilderRequestBus::Events::WriteSliceFile,
                filename,
                entities,
                makeDynamic);
            EXPECT_FALSE(result.IsSuccess());
        }

        m_app->RegisterComponentDescriptor(PythonAssetBuilderSystemComponent::CreateDescriptor());
        m_systemEntity->CreateComponent<PythonAssetBuilderSystemComponent>();
        m_systemEntity->Init();
        m_systemEntity->Activate();

        EXPECT_TRUE(PythonBuilderRequestBus::HasHandlers());
    }

}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
