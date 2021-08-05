/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.

#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpClientFactory.h>
AZ_POP_DISABLE_WARNING

#include <Framework/RequestBuilder.h>
#include <Framework/ServiceJob.h>
#include <Framework/ServiceJobUtil.h>

namespace AWSCore
{
    HttpRequestJob& ServiceJob::GetHttpRequestJob()
    {
        return *this;
    }

    const HttpRequestJob& ServiceJob::GetHttpRequestJob() const
    {
        return *this;
    }

    void ServiceJob::Start()
    {
        HttpRequestJob::Start();
    }

    std::shared_ptr<Aws::Http::HttpRequest> ServiceJob::InitializeRequest()
    {
        std::shared_ptr<Aws::Http::HttpRequest> request;
        RequestBuilder requestBuilder{};

        if (BuildRequest(requestBuilder))
        {
            request = Aws::Http::CreateHttpRequest(
                requestBuilder.GetRequestUrl(),
                requestBuilder.GetHttpMethod(),
                &Aws::Utils::Stream::DefaultResponseStreamFactoryMethod
            );

            SetAWSAuthSigner(requestBuilder.GetAWSAuthSigner());

            AZStd::string bodyString;

            if (std::shared_ptr<Aws::StringStream> bodyContent = GetBodyContent(requestBuilder))
            {
                std::istreambuf_iterator<AZStd::string::value_type> eos;
                bodyString = AZStd::string{ std::istreambuf_iterator<AZStd::string::value_type>(*bodyContent), eos };
            }

            ConfigureJsonServiceRequest(*this, bodyString);
        }

        return request;
    }


    std::shared_ptr<Aws::StringStream> ServiceJob::GetBodyContent(RequestBuilder& requestBuilder)
    {
        return requestBuilder.GetBodyContent();
    }

} // namespace AWSCore
