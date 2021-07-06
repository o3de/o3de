/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AutomatedLauncherTestingSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <CryCommon/platform.h>
#include <CryCommon/ICmdLine.h>
#include <CryCommon/IConsole.h>
#include <CryCommon/ISystem.h>
#include "SpawnDynamicSlice.h"
#include <AzCore/Component/Entity.h>


namespace AutomatedLauncherTesting
{
    void AutomatedLauncherTestingSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AutomatedLauncherTestingSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AutomatedLauncherTestingSystemComponent>("AutomatedLauncherTesting", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AutomatedLauncherTestingRequestBus>("AutomatedLauncherTestingRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Testing")
                ->Event("CompleteTest", &AutomatedLauncherTestingRequestBus::Events::CompleteTest)
                ;
        }
    }

    void AutomatedLauncherTestingSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("AutomatedLauncherTestingService"));
    }

    void AutomatedLauncherTestingSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("AutomatedLauncherTestingService"));
    }

    void AutomatedLauncherTestingSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void AutomatedLauncherTestingSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void AutomatedLauncherTestingSystemComponent::Init()
    {
    }

    void AutomatedLauncherTestingSystemComponent::Activate()
    {
        AutomatedLauncherTestingRequestBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void AutomatedLauncherTestingSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
        AutomatedLauncherTestingRequestBus::Handler::BusDisconnect();
    }

    void AutomatedLauncherTestingSystemComponent::CompleteTest(bool success, const AZStd::string& message)
    {
        AZ_Assert(
            m_phase == Phase::RunningTest,
            "Expected current phase to be RunningTest (%d), got %d, will skip printing CompleteTest message.",
            Phase::RunningTest, m_phase);

        if (m_phase == Phase::RunningTest)
        {
            if (!message.empty())
            {
                LogAlways("AutomatedLauncher: %s", message.c_str());
            }

            // Make sure this is always printed, in case log severity is turned down.
            LogAlways("AutomatedLauncher: %s", success ? "AUTO_LAUNCHER_TEST_COMPLETE" : "AUTO_LAUNCHER_TEST_FAIL");

            m_phase = Phase::Complete;
        }
    }

    void AutomatedLauncherTestingSystemComponent::OnCrySystemInitialized(ISystem& system, [[maybe_unused]] const SSystemInitParams& systemInitParams)
    {
        m_system = &system;
        // Only allow any testing to actually happen in non-release builds.
#if !defined(_RELEASE)
        ICmdLine* cmdLine = m_system->GetICmdLine();
        if (cmdLine)
        {
            AZ_Printf("AutomatedLauncher", "Checking for automated launcher testing command line arguments.");
            const ICmdLineArg* mapArg = cmdLine->FindArg(eCLAT_Pre, "ltest_map");
            if (mapArg)
            {
                AZStd::string map = mapArg->GetValue();
                AZ_Printf("AutomatedLauncher", "Found ltest_map arg %s.", map.c_str());
                if(map.compare("default") != 0)
                {
                    AZStd::lock_guard<MutexType> lock(m_testOperationsMutex);
                    m_testOperations.push_back(TestOperation(TestOperationType::LoadMap, map.c_str()));
                }
                else
                {
                    // Allow the default menu to load, watch for the next level to load
                    m_phase = Phase::LoadingMap;
                    m_nextLevelLoad = NextLevelLoad::WatchForNextLevelLoad;
                }
            }

            const ICmdLineArg* sliceArg = cmdLine->FindArg(eCLAT_Pre, "ltest_slice");
            if (sliceArg)
            {
                AZStd::string slice = sliceArg->GetValue();
                AZ_Printf("AutomatedLauncher", "Found ltest_slice arg %s.", slice.c_str());

                AzFramework::StringFunc::Tokenize(slice.c_str(), m_slices, ",");

                if (!m_slices.empty())
                {
                    AZStd::lock_guard<MutexType> lock(m_testOperationsMutex);
                    m_testOperations.push_back(TestOperation(TestOperationType::SpawnDynamicSlice, m_slices[0].c_str()));
                    m_slices.erase(m_slices.begin());
                }
            }
        }
#endif
    }

    void AutomatedLauncherTestingSystemComponent::OnCrySystemShutdown([[maybe_unused]] ISystem& system)
    {
        m_system = nullptr;
    }

    void AutomatedLauncherTestingSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        // Check to see if there is a load map operation in flight
        if (m_currentTestOperation.m_type == TestOperationType::LoadMap && !m_currentTestOperation.m_complete)
        {
            if (m_system->GetSystemGlobalState() == ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_COMPLETE)
            {
                m_currentTestOperation.m_complete = true;
            }
        }
        // Only start a new operation if there isn't already one in flight right now.
        else if (!m_testOperations.empty())
        {
            // Grab the first operation from the list
            {
                AZStd::lock_guard<MutexType> lock(m_testOperationsMutex);
                m_currentTestOperation = m_testOperations.at(0);
                m_testOperations.erase(m_testOperations.begin());
            }

            // If it was a map command, go ahead and launch it.
            if (m_currentTestOperation.m_type == TestOperationType::LoadMap)
            {
                AZ_Assert(m_phase == Phase::None, "Expected current phase to be None (%d), got %d", Phase::None, m_phase);
                AZStd::string command = AZStd::string::format("map %s", m_currentTestOperation.m_value.c_str());
                m_system->GetIConsole()->ExecuteString(command.c_str());
                m_phase = Phase::LoadingMap;
            }
            // If it was a spawn dynamic slice command, go ahead and spawn it.
            else if (m_currentTestOperation.m_type == TestOperationType::SpawnDynamicSlice)
            {
                AZ_Assert((m_phase == Phase::LoadingMap) || (m_phase == Phase::RunningTest), "Expected current phase to be LoadMap or RunningTest (%d), got %d", Phase::LoadingMap, m_phase);
                AZ::Entity* spawnedEntity = SpawnDynamicSlice::CreateSpawner(m_currentTestOperation.m_value, "Automated Testing Dynamic Slice Spawner");
                if (spawnedEntity)
                {
                    m_spawnedEntities.emplace_back(std::move(spawnedEntity));
                }
                m_phase = Phase::RunningTest;
            }
        }
        else if ((m_nextLevelLoad == NextLevelLoad::None) && (m_phase == Phase::RunningTest) && (m_system->GetSystemGlobalState() == ESYSTEM_GLOBAL_STATE_RUNNING))
        {
            AZ_Printf("AutomatedLauncher", "Running Test - Watching for a next level load");
            m_nextLevelLoad = NextLevelLoad::WatchForNextLevelLoad;
        }
        else if ((m_nextLevelLoad == NextLevelLoad::WatchForNextLevelLoad) && (m_system->GetSystemGlobalState() == ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_COMPLETE))
        {
            AZ_Printf("AutomatedLauncher", "Next level loaded, adding operations");
            if (!m_slices.empty())
            {
                m_testOperations.push_back(TestOperation(TestOperationType::SpawnDynamicSlice, m_slices[0].c_str()));
                m_slices.erase(m_slices.begin());

                m_nextLevelLoad = NextLevelLoad::None;
            }
            else
            {
                m_nextLevelLoad = NextLevelLoad::LevelLoadsComplete;
            }
        }
    }

    void AutomatedLauncherTestingSystemComponent::LogAlways(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        m_system->GetILog()->LogV(ILog::eAlways, format, args);
        va_end(args);
    }
}
