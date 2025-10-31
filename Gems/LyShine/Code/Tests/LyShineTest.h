/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/ComponentApplication.h>
#include <LyShineModule.h>

namespace UnitTest
{
    class LyShineTest
        : public LeakDetectionFixture
    {
    protected:
        LyShineTest()
            : m_application()
            , m_systemEntity(nullptr)
        {
        }

        void SetUp() override
        {
            SetupApplication();
            SetupEnvironment();
        }

        virtual void SetupApplication()
        {
            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
            appDesc.m_recordingMode = AZ::Debug::AllocationRecords::Mode::RECORD_FULL;

            m_systemEntity = m_application->Create(appDesc);
            m_systemEntity->Init();
            m_systemEntity->Activate();
        }

        virtual void SetupEnvironment()
        {
            m_env = AZStd::make_unique<StubEnv>();
            memset(&m_env->m_stubEnv, 0, sizeof(SSystemGlobalEnvironment));
            m_priorEnv = gEnv;
            gEnv = &m_env->m_stubEnv;
        }

        void TearDown() override
        {
            m_env.reset();
            gEnv = m_priorEnv;
            m_application->Destroy();
            delete m_application;
            m_application = nullptr;
        }

        struct StubEnv
        {
            SSystemGlobalEnvironment m_stubEnv;
        };

        AZ::ComponentApplication* m_application = nullptr;
        AZ::Entity* m_systemEntity = nullptr;
        AZStd::unique_ptr<StubEnv> m_env;

        SSystemGlobalEnvironment* m_priorEnv = nullptr;
    };
} // namespace UnitTest
