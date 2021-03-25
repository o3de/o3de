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

#include <Framework/HttpRequestJob.h>
#include <Framework/ServiceJobConfig.h>
#include <Framework/RequestBuilder.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AWSCore
{
    /// A ServiceJob encapsulates an HttpJob with all of the functionality necessary for an auto-generated AWS feature service.
    /// Inherits protected from HttpJob to hide all of the functionality not normally require.
    class ServiceJob
        : protected HttpRequestJob
    {
    public:
        using IConfig = IServiceJobConfig;
        using Config = ServiceJobConfig;

        static Config* GetDefaultConfig()
        {
            static AwsApiJobConfigHolder<Config> s_configHolder{};
            return s_configHolder.GetConfig(HttpRequestJob::GetDefaultConfig());
        }

    public:
        // To use a different allocator, extend this class and use this macro.
        AZ_CLASS_ALLOCATOR(ServiceJob, AZ::SystemAllocator, 0);

        ServiceJob(bool isAutoDelete, IConfig* config)
            : HttpRequestJob(isAutoDelete, config)
        {
        }

        /// Access the underlying HttpRequestJob if lower-level access is needed
        HttpRequestJob& GetHttpRequestJob();
        const HttpRequestJob& GetHttpRequestJob() const;

        /// Start the asynchronous job
        void Start();

    private:
        /// Descendant classes must implement these in order to have their requests sent
        virtual bool BuildRequest(RequestBuilder& request) = 0;
        virtual std::shared_ptr<Aws::StringStream> GetBodyContent(RequestBuilder& requestBuilder);

        /// Configures the request using the custom properties of the service by calling BuildRequest()
        std::shared_ptr<Aws::Http::HttpRequest> InitializeRequest() override;
    };

} // namespace AWSCore
