/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AWSNativeSDKInit/AWSNativeSDKInit.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/UnitTest/TestTypes.h>

class AWSGameLiftClientFixture
    : public UnitTest::ScopedAllocatorSetupFixture
{
public:
    AWSGameLiftClientFixture() {}
    virtual ~AWSGameLiftClientFixture() = default;

    void SetUp() override
    {
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();
        AZ::AllocatorInstance<AZ::PoolAllocator>::Create();

        AZ::JobManagerDesc jobManagerDesc;
        AZ::JobManagerThreadDesc threadDesc;

        m_jobManager.reset(aznew AZ::JobManager(jobManagerDesc));
        m_jobCancelGroup.reset(aznew AZ::JobCancelGroup());
        jobManagerDesc.m_workerThreads.push_back(threadDesc);
        jobManagerDesc.m_workerThreads.push_back(threadDesc);
        jobManagerDesc.m_workerThreads.push_back(threadDesc);
        m_jobContext.reset(aznew AZ::JobContext(*m_jobManager, *m_jobCancelGroup));
        AZ::JobContext::SetGlobalContext(m_jobContext.get());

        AWSNativeSDKInit::InitializationManager::InitAwsApi();
    }

    void TearDown() override
    {
        AWSNativeSDKInit::InitializationManager::Shutdown();

        AZ::JobContext::SetGlobalContext(nullptr);
        m_jobContext.reset();
        m_jobCancelGroup.reset();
        m_jobManager.reset();
        AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
    }

    AZStd::unique_ptr<AZ::JobContext> m_jobContext;
    AZStd::unique_ptr<AZ::JobCancelGroup> m_jobCancelGroup;
    AZStd::unique_ptr<AZ::JobManager> m_jobManager;
};
