/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/GemTestEnvironment.h>

#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>

#include <System/FabricCooker.h>
#include <System/TangentSpaceHelper.h>
#include <System/SystemComponent.h>
#include <Components/ClothComponent.h>

#include <Editor/EditorSystemComponent.h>
#include <Components/EditorClothComponent.h>
#include <Pipeline/SceneAPIExt/ClothRuleBehavior.h>

namespace UnitTest
{
    class NvClothToolsTestApplication
        : public ToolsTestApplication
    {
    public:
        explicit NvClothToolsTestApplication(AZStd::string applicationName)
            : ToolsTestApplication(applicationName)
        {
        }
    };

    //! Sets up gem test environment, required components, and shared objects used by cloth (e.g. FabricCooker) for all test cases.
    class NvClothEditorTestEnvironment
        : public AZ::Test::GemTestEnvironment
    {
    public:
        // AZ::Test::GemTestEnvironment overrides ...
        void AddGemsAndComponents() override;
        void PreCreateApplication() override;
        void PostSystemEntityActivate() override;
        void PostDestroyApplication() override;
        AZ::ComponentApplication* CreateApplicationInstance() override;

    private:
        AZStd::unique_ptr<NvCloth::FabricCooker> m_fabricCooker;
        AZStd::unique_ptr<NvCloth::TangentSpaceHelper> m_tangentSpaceHelper;

        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIO;
    };

    void NvClothEditorTestEnvironment::AddGemsAndComponents()
    {
        AddDynamicModulePaths({
            "LmbrCentral.Editor",
            "EMotionFX.Editor"
        });

        AddComponentDescriptors({
            NvCloth::SystemComponent::CreateDescriptor(),
            NvCloth::ClothComponent::CreateDescriptor(),

            NvCloth::EditorSystemComponent::CreateDescriptor(),
            NvCloth::EditorClothComponent::CreateDescriptor(),
            NvCloth::Pipeline::ClothRuleBehavior::CreateDescriptor(),
        });

        AddRequiredComponents({
            NvCloth::SystemComponent::TYPEINFO_Uuid(),
            NvCloth::EditorSystemComponent::TYPEINFO_Uuid(),
        });
    }

    void NvClothEditorTestEnvironment::PreCreateApplication()
    {
        NvCloth::SystemComponent::InitializeNvClothLibrary(); // SystemAllocator creation must come before this call.
        m_fabricCooker = AZStd::make_unique<NvCloth::FabricCooker>();
        m_tangentSpaceHelper = AZStd::make_unique<NvCloth::TangentSpaceHelper>();

        // EMotionFX SystemComponent activation requires a valid AZ::IO::LocalFileIO
        m_fileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
        AZ::IO::FileIOBase::SetInstance(m_fileIO.get());
    }

    void NvClothEditorTestEnvironment::PostSystemEntityActivate()
    {
        // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
        // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
        // in the unit tests.
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
    }

    void NvClothEditorTestEnvironment::PostDestroyApplication()
    {
        AZ::IO::FileIOBase::SetInstance(nullptr);
        m_fileIO.reset();

        m_tangentSpaceHelper.reset();
        m_fabricCooker.reset();
        NvCloth::SystemComponent::TearDownNvClothLibrary(); // SystemAllocator destruction must come after this call.
    }

    AZ::ComponentApplication* NvClothEditorTestEnvironment::CreateApplicationInstance()
    {
        return aznew NvClothToolsTestApplication("NvClothEditorTests");
    }
} // namespace UnitTest

AZ_TOOLS_UNIT_TEST_HOOK(new UnitTest::NvClothEditorTestEnvironment);
