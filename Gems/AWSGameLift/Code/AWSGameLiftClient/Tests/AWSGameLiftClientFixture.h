/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AWSNativeSDKTestManager.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/Utils.h>

class AWSGameLiftClientFixture
    : public UnitTest::LeakDetectionFixture
{
public:
    AWSGameLiftClientFixture() {}
    virtual ~AWSGameLiftClientFixture() = default;

    void SetUp() override
    {
        AZ::JobManagerDesc jobManagerDesc;
        AZ::JobManagerThreadDesc threadDesc;

        m_jobManager.reset(aznew AZ::JobManager(jobManagerDesc));
        m_jobCancelGroup.reset(aznew AZ::JobCancelGroup());
        jobManagerDesc.m_workerThreads.push_back(threadDesc);
        jobManagerDesc.m_workerThreads.push_back(threadDesc);
        jobManagerDesc.m_workerThreads.push_back(threadDesc);
        m_jobContext.reset(aznew AZ::JobContext(*m_jobManager, *m_jobCancelGroup));
        AZ::JobContext::SetGlobalContext(m_jobContext.get());

        AWSNativeSDKTestLibs::AWSNativeSDKTestManager::Init();
    }

    void TearDown() override
    {
        AWSNativeSDKTestLibs::AWSNativeSDKTestManager::Shutdown();

        AZ::JobContext::SetGlobalContext(nullptr);
        m_jobContext.reset();
        m_jobCancelGroup.reset();
        m_jobManager.reset();
    }

    AZStd::unique_ptr<AZ::JobContext> m_jobContext;
    AZStd::unique_ptr<AZ::JobCancelGroup> m_jobCancelGroup;
    AZStd::unique_ptr<AZ::JobManager> m_jobManager;
};
