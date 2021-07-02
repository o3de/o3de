/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

#include <AutomatedLauncherTesting/AutomatedLauncherTestingBus.h>
#include <CrySystemBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class Entity;
}

namespace AutomatedLauncherTesting
{
    class AutomatedLauncherTestingSystemComponent
        : public AZ::Component
        , protected AutomatedLauncherTestingRequestBus::Handler
        , private CrySystemEventBus::Handler
        , private AZ::TickBus::Handler
    {

    private:
        enum class Phase
        {
            None,
            LoadingMap,
            RunningTest,
            Complete
        };

        enum class NextLevelLoad
        {
            None,
            WatchForNextLevelLoad,
            LevelLoadsComplete
        };

        enum class TestOperationType
        {
            None,
            LoadMap,
            SpawnDynamicSlice
        };

        struct TestOperation
        {
            TestOperation()
            {
            }

            TestOperation(TestOperationType type, const AZStd::string& value)
                : m_type(type)
                , m_value(value)

            {
            }

            TestOperationType m_type = TestOperationType::None;
            AZStd::string m_value;
            bool m_complete = false;
        };

    public:
        AZ_COMPONENT(AutomatedLauncherTestingSystemComponent, "{87A405E2-390B-43A9-9A96-94BDC0DF680B}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AutomatedLauncherTestingRequestBus interface implementation
        void CompleteTest(bool success, const AZStd::string& message) override;
        ////////////////////////////////////////////////////////////////////////

    protected:
        ////////////////////////////////////////////////////////////////////////////
        // CrySystemEvents
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override;
        void OnCrySystemShutdown(ISystem& system) override;

        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        void LogAlways(const char* format, ...);

    private:
        ISystem* m_system = nullptr;
        AZStd::vector<TestOperation> m_testOperations;
        AZStd::vector<AZStd::string> m_slices;
        MutexType m_testOperationsMutex;
        TestOperation m_currentTestOperation;
        AZStd::vector<AZStd::shared_ptr<AZ::Entity>> m_spawnedEntities;
        Phase m_phase = Phase::None;
        NextLevelLoad m_nextLevelLoad = NextLevelLoad::None;
    };
}
