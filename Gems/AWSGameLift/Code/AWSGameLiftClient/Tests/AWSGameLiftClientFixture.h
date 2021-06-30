/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
