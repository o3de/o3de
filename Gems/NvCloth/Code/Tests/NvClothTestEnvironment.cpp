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
#include <AzFramework/Components/TransformComponent.h>

#include <System/FabricCooker.h>
#include <System/TangentSpaceHelper.h>
#include <System/SystemComponent.h>
#include <Components/ClothComponent.h>

namespace UnitTest
{
    //! Sets up gem test environment, required components, and shared objects used by cloth (e.g. FabricCooker) for all test cases.
    class NvClothTestEnvironment
        : public AZ::Test::GemTestEnvironment
    {
    public:
        // AZ::Test::GemTestEnvironment overrides ...
        void AddGemsAndComponents() override;
        void PreCreateApplication() override;
        void PostSystemEntityActivate() override;
        void PostDestroyApplication() override;

    private:
        AZStd::unique_ptr<NvCloth::FabricCooker> m_fabricCooker;
        AZStd::unique_ptr<NvCloth::TangentSpaceHelper> m_tangentSpaceHelper;

        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIO;
    };

    void NvClothTestEnvironment::AddGemsAndComponents()
    {
        AddDynamicModulePaths({
            "LmbrCentral",
            "EMotionFX"
        });

        AddComponentDescriptors({
            AzFramework::TransformComponent::CreateDescriptor(),
            NvCloth::SystemComponent::CreateDescriptor(),
            NvCloth::ClothComponent::CreateDescriptor()
        });

        AddRequiredComponents({
            NvCloth::SystemComponent::TYPEINFO_Uuid()
        });
    }

    void NvClothTestEnvironment::PreCreateApplication()
    {
        NvCloth::SystemComponent::InitializeNvClothLibrary(); // SystemAllocator creation must come before this call.
        m_fabricCooker = AZStd::make_unique<NvCloth::FabricCooker>();
        m_tangentSpaceHelper = AZStd::make_unique<NvCloth::TangentSpaceHelper>();

        // EMotionFX SystemComponent activation requires a valid AZ::IO::LocalFileIO
        m_fileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
        AZ::IO::FileIOBase::SetInstance(m_fileIO.get());
    }

    void NvClothTestEnvironment::PostSystemEntityActivate()
    {
        // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
        // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
        // in the unit tests.
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
    }

    void NvClothTestEnvironment::PostDestroyApplication()
    {
        AZ::IO::FileIOBase::SetInstance(nullptr);
        m_fileIO.reset();

        m_tangentSpaceHelper.reset();
        m_fabricCooker.reset();
        NvCloth::SystemComponent::TearDownNvClothLibrary(); // SystemAllocator destruction must come after this call.
    }
} // namespace UnitTest

AZ_UNIT_TEST_HOOK(new UnitTest::NvClothTestEnvironment);
