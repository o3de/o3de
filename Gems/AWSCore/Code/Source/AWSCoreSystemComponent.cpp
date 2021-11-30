/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Component/Component.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Jobs/JobCancelGroup.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AWSNativeSDKInit/AWSNativeSDKInit.h>

#include <AWSCoreSystemComponent.h>
#include <Configuration/AWSCoreConfiguration.h>
#include <Credential/AWSCredentialManager.h>
#include <Framework/AWSApiJob.h>
#include <ResourceMapping/AWSResourceMappingBus.h>
#include <ResourceMapping/AWSResourceMappingManager.h>

namespace AWSCore
{
    constexpr int DEFAULT_NUMBER_AWS_THREADS = 2;    /// Allow overlapping requests
    static constexpr const char COMPONENT_DISPLAY_NAME[] = "AWSCore";

    AWSCoreSystemComponent::AWSCoreSystemComponent()
        : m_firstThreadCPU(-1)
        , m_threadPriority(0)
        , m_threadStackSize(-1)
        , m_jobCancelGroup(nullptr)
    {
        m_awsCoreConfiguration = AZStd::make_unique<AWSCoreConfiguration>();
        m_awsCredentialManager = AZStd::make_unique<AWSCredentialManager>();
        m_awsResourceMappingManager = AZStd::make_unique<AWSResourceMappingManager>();
        m_threadCount = AZ::GetMin(static_cast<unsigned int>(DEFAULT_NUMBER_AWS_THREADS), AZStd::thread::hardware_concurrency());
    }

    void AWSCoreSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AWSCoreSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AWSCoreSystemComponent>("AWSCore", "Adds core support for working with AWS")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AWSResourceMappingRequestBus>("AWSResourceMappingRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, COMPONENT_DISPLAY_NAME)
                ->Event("GetDefaultAccountId", &AWSResourceMappingRequestBus::Events::GetDefaultAccountId)
                ->Event("GetDefaultRegion", &AWSResourceMappingRequestBus::Events::GetDefaultRegion)
                ->Event("GetResourceAccountId", &AWSResourceMappingRequestBus::Events::GetResourceAccountId,
                    {{{"Resource KeyName", "Resource mapping key name is used to identify individual resource attributes."}}})
                ->Event("GetResourceNameId", &AWSResourceMappingRequestBus::Events::GetResourceNameId,
                    {{{"Resource KeyName", "Resource mapping key name is used to identify individual resource attributes."}}})
                ->Event("GetResourceRegion", &AWSResourceMappingRequestBus::Events::GetResourceRegion,
                    {{{"Resource KeyName", "Resource mapping key name is used to identify individual resource attributes."}}})
                ->Event("GetResourceType", &AWSResourceMappingRequestBus::Events::GetResourceType,
                    {{{"Resource KeyName", "Resource mapping key name is used to identify individual resource attributes."}}})
                ->Event("ReloadConfigFile", &AWSResourceMappingRequestBus::Events::ReloadConfigFile,
                    {{{"Is Reloading Config FileName", "Whether reload resource mapping config file name from AWS core configuration settings registry file."}}})
                ;
        }
    }

    void AWSCoreSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AWSCoreService"));
    }

    void AWSCoreSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AWSCoreService"));
    }

    void AWSCoreSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void AWSCoreSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    bool AWSCoreSystemComponent::IsAWSApiInitialized() const
    {
        return AWSNativeSDKInit::InitializationManager::IsInitialized();
    }

    void AWSCoreSystemComponent::Init()
    {
        m_awsCoreConfiguration->InitConfig();
    }

    void AWSCoreSystemComponent::Activate()
    {
        m_awsCoreConfiguration->ActivateConfig();

        InitAWSApi();
        m_awsCredentialManager->ActivateManager();
        m_awsResourceMappingManager->ActivateManager();

        AWSCoreRequestBus::Handler::BusConnect();
        AWSCoreNotificationsBus::Broadcast(&AWSCoreNotifications::OnSDKInitialized);
    }

    void AWSCoreSystemComponent::Deactivate()
    {
        AWSCoreRequestBus::Handler::BusDisconnect();

        m_awsResourceMappingManager->DeactivateManager();
        m_awsCredentialManager->DeactivateManager();
        // Its required that anything that owns memory allocated by the AWS
        // API be destructed before this component is deactivated.
        AWSCoreNotificationsBus::Broadcast(&AWSCoreNotifications::OnSDKShutdownStarted);
        ShutdownAWSApi();

        m_awsCoreConfiguration->DeactivateConfig();
    }

    void AWSCoreSystemComponent::InitAWSApi()
    {
        AWSNativeSDKInit::InitializationManager::InitAwsApi();
    }

    void AWSCoreSystemComponent::ShutdownAWSApi()
    {
        AWSNativeSDKInit::InitializationManager::Shutdown();
    }

    AZ::JobContext* AWSCoreSystemComponent::GetDefaultJobContext()
    {
        if (m_threadCount < 1)
        {
            AZ::JobContext* jobContext{ nullptr };
            AZ::JobManagerBus::BroadcastResult(jobContext, &AZ::JobManagerEvents::GetGlobalContext);
            return jobContext;
        }
        else
        {
            if (!m_jobContext)
            {
                // If m_firstThreadCPU isn't -1, then each thread will be
                // assigned to a specific CPU starting with the specified CPU.
                AZ::JobManagerDesc jobManagerDesc{};
                AZ::JobManagerThreadDesc threadDesc(m_firstThreadCPU, m_threadPriority, m_threadStackSize);
                for (int i = 0; i < m_threadCount; ++i)
                {
                    jobManagerDesc.m_workerThreads.push_back(threadDesc);
                    if (threadDesc.m_cpuId > -1)
                    {
                        threadDesc.m_cpuId++;
                    }
                }

                m_jobCancelGroup.reset(aznew AZ::JobCancelGroup());
                m_jobManager.reset(aznew AZ::JobManager(jobManagerDesc));
                m_jobContext.reset(aznew AZ::JobContext(*m_jobManager, *m_jobCancelGroup));

            }
            return m_jobContext.get();
        }
    }

    AwsApiJobConfig* AWSCoreSystemComponent::GetDefaultConfig()
    {
        return AwsApiJob::GetDefaultConfig();
    }
} // namespace AWSCore
