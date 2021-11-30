/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ScriptEventsTestApplication.h"

#include <AzTest/AzTest.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzFramework/IO/LocalFileIO.h>

#include <ScriptEvents/ScriptEventsGem.h>

#include <ScriptEvents/Internal/VersionedProperty.h>
#include <ScriptEvents/ScriptEventParameter.h>
#include <ScriptEvents/ScriptEventsMethod.h>
#include <ScriptEvents/ScriptEventsAsset.h>

#include "ScriptEventTestUtilities.h"
#include <AzCore/Math/MathReflection.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace ScriptEventsTests
{
    class ScriptEventsTestFixture
        : public ::testing::Test
    {
        static const bool s_enableMemoryLeakChecking;

        static ScriptEventsTests::Application* GetApplication();

    protected:

        static ScriptEventsTests::Application* s_application;
        static UnitTest::AllocatorsBase s_allocatorSetup;

        static void SetUpTestCase()
        {
            s_allocatorSetup.SetupAllocator();

            if (s_application == nullptr)
            {
                AZ::ComponentApplication::StartupParameters appStartup;
                s_application = aznew ScriptEventsTests::Application();

                {
                    AZ::ComponentApplication::Descriptor descriptor;
                    descriptor.m_enableDrilling = false; // We'll manage our own driller in these tests
                    descriptor.m_useExistingAllocator = true; // Use the SystemAllocator we own in this test.

                    appStartup.m_createStaticModulesCallback =
                        [](AZStd::vector<AZ::Module*>& modules)
                        {
                            modules.emplace_back(new ScriptEvents::ScriptEventsModule);
                        };

                    s_application->Start(descriptor, appStartup);
                }

                AZ::SerializeContext* serializeContext = s_application->GetSerializeContext();
                serializeContext->RegisterGenericType<AZStd::string>();
                serializeContext->RegisterGenericType<AZStd::any>();

                AZ::BehaviorContext* behaviorContext = s_application->GetBehaviorContext();

                Utilities::Reflect(behaviorContext);

                AZ_Assert(s_application->FindEntity(AZ::SystemEntityId), "SystemEntity must exist");

            }

            AZ::TickBus::AllowFunctionQueuing(true);
        }

        static void TearDownTestCase()
        {
            AZ_Assert(s_application->FindEntity(AZ::SystemEntityId), "SystemEntity must exist");

            AZ::Data::AssetManager::Instance().DispatchEvents();

            if (s_application)
            {
                s_application->Stop();
                delete s_application;
                s_application = nullptr;
            }

            s_allocatorSetup.TeardownAllocator();
        }

        void SetUp() override
        {
            m_serializeContext = s_application->GetSerializeContext();
            m_behaviorContext = s_application->GetBehaviorContext();

            if (!AZ::IO::FileIOBase::GetInstance())
            {
                m_fileIO.reset(aznew AZ::IO::LocalFileIO());
                AZ::IO::FileIOBase::SetInstance(m_fileIO.get());
            }
            AZ_Assert(AZ::IO::FileIOBase::GetInstance(), "File IO was not properly installed");
        }

        void TearDown() override
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            m_fileIO = nullptr;
        }

        AZStd::unique_ptr<AZ::IO::FileIOBase> m_fileIO;
        AZ::SerializeContext* m_serializeContext;
        AZ::BehaviorContext* m_behaviorContext;
    };

}

