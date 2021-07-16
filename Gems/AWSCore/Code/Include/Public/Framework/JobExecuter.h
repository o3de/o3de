/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobFunction.h>

#include <aws/core/utils/threading/Executor.h>

namespace AWSCore
{

    /// AZ:Job type used by the JobExecuter. It is used call the callback 
    /// function provided by the AWS SDK.
    using ExecuterJob = AZ::JobFunction<std::function<void()>>;

    /// This class provides a simple alternative to using the the AwsRequestJob, 
    /// AwsApiClientJob, or AwsApiJob classes. Those classes provide configuration 
    /// management and more abstracted usage patterns. With JobExecutor you need
    /// to do all the configuration management and work directly with the AWS API.
    /// 
    /// An AWS API async executer that uses the AZ::Job system to make AWS service calls.
    /// To use, set the Aws::Client::ClientConfiguration executor field so it points to
    /// an instance of this class, then use that client configuration object when creating
    /// AWS service client objects. This will cause the Async APIs on the AWS service 
    /// client object to use the AZ::Job system to execute the request.
    class JobExecuter
        : public Aws::Utils::Threading::Executor
    {

    public:
        /// Initialize a JobExecuter object.
        ///
        /// \param context - The JobContext that will be used to execute the jobs created
        /// by the JobExecuter.
        ///
        /// By default the global JobContext is used. However, the AWS SDK currently 
        /// only supports blocking calls, so, to avoid impacting other jobs, it is 
        /// recommended that you create a JobContext with a JobManager dedicated to 
        /// dedicated to processing these jobs. This context can also be used with 
        /// AwsApiCore::HttpJob.
        JobExecuter(AZ::JobContext* context)
            : m_context{ context }
        {
        }

    protected:
        AZ::JobContext* m_context;

        /// Called by the AWS SDK to queue a callback for execution.
        bool SubmitToThread(std::function<void()>&& callback) override
        {
            ExecuterJob* job = aznew ExecuterJob(callback, true, m_context);
            job->Start();
        }

    };

} // namespace AWCore

