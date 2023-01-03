/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Utils/Utils.h>
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
#include <LmbrCentral/Shape/ShapeComponentBus.h>

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

            AZ::IO::FixedMaxPath testDir = AZ::Utils::GetExecutableDirectory();
            testDir /= "Test.Assets/Gems/PhysX/Code/Tests";
            m_fileIo->SetAlias("@test@", testDir.c_str());

            //Test_PhysXSettingsRegistryManager will not do any file saving
            m_physXSystem = AZStd::make_unique<PhysX::PhysXSystem>(AZStd::make_unique<PhysX::TestUtils::Test_PhysXSettingsRegistryManager>(), PhysX::PxCooking::GetEditTimeCookingParams());
        }

        /// Allows derived environments to override to perform additional steps after creating the application.
        void PostCreateApplication() override
        {
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            // Ebus usage will allocate a global context on first usage. If that first usage occurs in a DLL, then the context will be
            // invalid on subsequent unit test runs if using gtest_repeat. However, if we force the ebus to create their global context in
            // the main test DLL (this one), the context will remain active throughout repeated runs. By creating them in
            // PostCreateApplication(), they will be created before the DLLs get loaded and any system components from those DLLs run, so we
            // can guarantee this will be the first usage.

            // These ebuses need their contexts created here before any of the dependent DLLs get loaded:
            LmbrCentral::ShapeComponentRequestsBus::GetOrCreateContext();
            LmbrCentral::ShapeComponentNotificationsBus::GetOrCreateContext();
            AZ::Data::AssetManagerNotificationBus::GetOrCreateContext();
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
            AZ::IO::FileIOBase::SetInstance(nullptr);
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
