/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <AWSCoreBus.h>

namespace AZ
{
    class JobManager;
    class JobCancelGroup;
    class JobContext;
}

namespace AWSCore
{
    class AWSCoreConfiguration;
    class AWSCredentialManager;
    class AWSResourceMappingManager;

    class AWSCoreSystemComponent
        : public AZ::Component
        , protected AWSCoreRequestBus::Handler
    {
    public:
        static const char* AWS_API_ALLOC_TAG;
        static const char* AWS_API_LOG_PREFIX;

        AZ_COMPONENT(AWSCoreSystemComponent, "{940EEC1D-BABE-4F28-8E70-8AC12E22BD58}");

        AWSCoreSystemComponent();

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ////////////////////////////////////////////////////////////////////////
        // AWSCoreRequestBus interface implementation
        AZ::JobContext* GetDefaultJobContext() override;
        AwsApiJobConfig* GetDefaultConfig() override;

        //! Returns true if the AWS C++ SDK has been initialized and is ready for use
        bool IsAWSApiInitialized() const;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        void InitAWSApi();
        void ShutdownAWSApi();

    private:
        int m_threadCount;          ///! Number of threads used for AWS requests
        int m_firstThreadCPU;       ///! Internal tracker of cpu id for first thread
        int m_threadPriority;       ///! Priority size for AWS threads, defaults to platform value
        int m_threadStackSize;      ///! Stack size for threads, defaults to platform value

        // Order here is of importance. To be correct, JobContext needs to 
        // destruct before the JobManager and the JobCancelGroup.
        AZStd::unique_ptr<AZ::JobCancelGroup> m_jobCancelGroup;
        AZStd::unique_ptr<AZ::JobManager> m_jobManager;
        AZStd::unique_ptr<AZ::JobContext> m_jobContext;

        AZStd::unique_ptr<AWSCoreConfiguration> m_awsCoreConfiguration;
        AZStd::unique_ptr<AWSCredentialManager> m_awsCredentialManager;
        AZStd::unique_ptr<AWSResourceMappingManager> m_awsResourceMappingManager;
    };

} // namespace AWSCore
