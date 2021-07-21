/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PhysX_precompiled.h"

#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzTest/GemTestEnvironment.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <ComponentDescriptors.h>
#include <EditorComponentDescriptors.h>
#include <QApplication>
#include <SystemComponent.h>
#include <TestColliderComponent.h>
#include <Editor/Source/Components/EditorSystemComponent.h>
#include <Configuration/PhysXSettingsRegistryManager.h>
#include <System/PhysXSystem.h>
#include <System/PhysXCookingParams.h>
#include <Tests/PhysXTestCommon.h>
#include <UnitTest/ToolsTestApplication.h>

namespace Physics
{
    class PhysXEditorTestToolsApplication
        : public UnitTest::ToolsTestApplication
    {
    public:
        PhysXEditorTestToolsApplication(AZStd::string appName)
            : UnitTest::ToolsTestApplication(AZStd::move(appName))
        {
        }

        bool IsPrefabSystemEnabled() const override
        {
            // Some physx tests fail if prefabs are enabled for the application,
            // for now, make them use slices
            return false;
        }
    };

    class PhysXEditorTestEnvironment
        : public AZ::Test::GemTestEnvironment
    {
        /// Allows derived environments to override to set up which gems, components etc the environment should load.
        void AddGemsAndComponents() override
        {
            AddDynamicModulePaths({ "LmbrCentral.Editor" });

            const auto& physxDescriptors = PhysX::GetDescriptors();
            const auto& physxEditorDescriptors = PhysX::GetEditorDescriptors();

            AZStd::vector<AZ::ComponentDescriptor*> descriptors(physxDescriptors.begin(), physxDescriptors.end());
            descriptors.insert(descriptors.end(), physxEditorDescriptors.begin(), physxEditorDescriptors.end());

            descriptors.emplace_back( UnitTest::TestColliderComponentMode::CreateDescriptor());

            AddComponentDescriptors(descriptors);

            AddRequiredComponents({ PhysX::SystemComponent::TYPEINFO_Uuid(), PhysX::EditorSystemComponent::TYPEINFO_Uuid() });
        }

        /// Allows derived environments to override to perform additional steps prior to creating the application.
        void PreCreateApplication() override
        {
            m_fileIo = AZStd::make_unique<AZ::IO::LocalFileIO>();

            AZ::IO::FileIOBase::SetInstance(m_fileIo.get());

            char testDir[AZ_MAX_PATH_LEN];
            m_fileIo->ConvertToAbsolutePath("../Gems/PhysX/Code/Tests", testDir, AZ_MAX_PATH_LEN);
            m_fileIo->SetAlias("@test@", testDir);

            //Test_PhysXSettingsRegistryManager will not do any file saving
            m_physXSystem = AZStd::make_unique<PhysX::PhysXSystem>(new PhysX::TestUtils::Test_PhysXSettingsRegistryManager(), PhysX::PxCooking::GetEditTimeCookingParams());
        }

        /// Allows derived environments to override to perform additional steps after creating the application.
        void PostCreateApplication() override
        {
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
        }

        /// Allows derived environments to override to perform additional steps prior to destroying the application.
        void PreDestroyApplication() override
        {
            // Clear out any queued OnAssetError/OnAssetReady messages
            AZ::Data::AssetManager::Instance().DispatchEvents();
            m_physXSystem->Shutdown();
            m_physXSystem.reset();
        }

        /// Allows derived environments to override to perform additional steps after destroying the application.
        void PostDestroyApplication() override
        {
            m_fileIo.reset();
        }

        AZ::ComponentApplication* CreateApplicationInstance() override
        {
            return aznew PhysXEditorTestToolsApplication("PhysXEditor");
        }

        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIo;
        AZStd::unique_ptr<PhysX::PhysXSystem> m_physXSystem = nullptr;
    };

    class PhysXEditorTest
        : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
        }

        void TearDown() override
        {
        }
    };

    TEST_F(PhysXEditorTest, EditorDummyTest_NoState_TrivialPass)
    {
        EXPECT_TRUE(true);
    }

} // namespace Physics

AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    QApplication app(argc, argv);
    AZ::Test::ApplyGlobalParameters(&argc, argv);
    AZ::Test::printUnusedParametersWarning(argc, argv);
    AZ::Test::addTestEnvironments({ new Physics::PhysXEditorTestEnvironment });
    int result = RUN_ALL_TESTS();
    return result;
}

IMPLEMENT_TEST_EXECUTABLE_MAIN();
