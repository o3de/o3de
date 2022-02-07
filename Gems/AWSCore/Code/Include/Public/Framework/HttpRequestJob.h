/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Framework/AWSApiJob.h>
#include <Framework/HttpRequestJobConfig.h>
#include <Framework/Util.h>

namespace Aws
{
    namespace Client
    {
        class AWSAuthSigner;
    }

    namespace Http
    {
        class HttpClient;
        class HttpRequest;
        class HttpResponse;
    }
}

namespace AWSCore
{
    /////////////////////////////////////////////
    // HTTP Request run on a background thread
    /////////////////////////////////////////////
    class HttpRequestJob
        : public AwsApiJob
    {
    public:
        enum class HttpMethod
        {
            HTTP_GET,
            HTTP_POST,
            HTTP_DELETE,
            HTTP_PUT,
            HTTP_HEAD,
            HTTP_PATCH
        };

        enum class HeaderField
        {
            DATE,
            AWS_DATE,
            AWS_SECURITY_TOKEN,
            ACCEPT,
            ACCEPT_CHAR_SET,
            ACCEPT_ENCODING,
            AUTHORIZATION,
            AWS_AUTHORIZATION,
            COOKIE,
            CONTENT_LENGTH,
            CONTENT_TYPE,
            USER_AGENT,
            VIA,
            HOST,
            AMZ_TARGET,
            X_AMZ_EXPIRES,
            CONTENT_MD5,
        };

        class Response
        {
        public:
            const AZStd::string& GetResponseBody() const;
            int GetResponseCode() const;
            const std::shared_ptr<Aws::Http::HttpResponse>& GetUnderlyingResponse() const;

        private:
            std::shared_ptr<Aws::Http::HttpResponse> m_response;
            AZStd::string m_responseBody;
            int m_responseCode = 0;

            friend class HttpRequestJob;
        };

        using SuccessFn = AZStd::function<void(const AZStd::shared_ptr<Response>& response)>;
        using FailureFn = AZStd::function<void(const AZStd::shared_ptr<Response>& response)>;
        using StringMap = AZStd::unordered_map<AZStd::string, AZStd::string>;

    public:

        // To use a different allocator, extend this class and use this macro.
        AZ_CLASS_ALLOCATOR(HttpRequestJob, AZ::SystemAllocator, 0);

        using IConfig = IHttpRequestJobConfig;
        using Config = HttpRequestJobConfig;

        static Config* GetDefaultConfig()
        {
            static AwsApiJobConfigHolder<Config> s_configHolder{};
            return s_configHolder.GetConfig(AwsApiJob::GetDefaultConfig());
        }

        static void StaticInit();
        static void StaticShutdown();

    public:
        HttpRequestJob(bool isAutoDelete, IConfig* config)
            : AwsApiJob(isAutoDelete, config)
            , m_readRateLimiter(config->GetReadRateLimiter())
            , m_writeRateLimiter(config->GetWriteRateLimiter())
            , m_httpClient(config->GetHttpClient())
        {
            SetRequestHeader(HeaderField::USER_AGENT, Util::ToAZString(config->GetUserAgent()));
        }

        /// Get and set the URL for this request
        void SetUrl(AZStd::string url);
        const AZStd::string& GetUrl() const;

        /// Get and set the HTTP method for this request
        void SetMethod(HttpMethod method);
        bool SetMethod(const AZStd::string& method);
        HttpMethod GetMethod() const;

        /// Get and set headers for the HTTP request
        void SetRequestHeader(AZStd::string key, AZStd::string value);
        bool GetRequestHeader(const AZStd::string& key, AZStd::string* result = nullptr);

        /// Get and set pre-defined header fields. Equivalent to manually setting the corresponding header field, e.g. 
        /// HeaderField::CONTENT_LENGTH maps to "Content-Length".
        void SetRequestHeader(HeaderField field, AZStd::string value);
        bool GetRequestHeader(HeaderField field, AZStd::string* result = nullptr);

        /// Get the collection of all request headers.
        StringMap& GetRequestHeaders();
        const StringMap& GetRequestHeaders() const;

        /// Syntactic sugar to set popular headers. Equivalent to manually setting the corresponding header field.
        void SetAccept(AZStd::string accept);
        void SetAcceptCharSet(AZStd::string accept);
        void SetContentLength(AZStd::string contentLength);
        void SetContentType(AZStd::string contentType);

        /// Get and set AWS authorization signer for the request
        void SetAWSAuthSigner(const std::shared_ptr<Aws::Client::AWSAuthSigner>& authSigner);
        const std::shared_ptr<Aws::Client::AWSAuthSigner>& GetAWSAuthSigner() const;

        /// Get and set the body for the HTTP request. (You are responsible for setting the Content-Length header.)
        void SetBody(AZStd::string body);
        const AZStd::string& GetBody() const;
        AZStd::string& GetBody();

        /// Set callback functions for success and failure. These will be executed on the main thread.
        template<typename SuccessCallbackT, typename FailureCallbackT>
        void SetCallbacks(SuccessCallbackT&& successCB, FailureCallbackT&& failureCB);

        /// Converts an HttpMethod to a string. Used for debug output.
        static const char* HttpMethodToString(HttpMethod);
        static const char* HttpMethodToString(Aws::Http::HttpMethod);

        /// Converts a string to an HttpMethod.
        static AZStd::optional<HttpMethod> StringToHttpMethod(const AZStd::string& method);

    protected:
        /// Override to provide a custom instantiation of the HttpRequest.
        /// The default implementation uses the URL and method specified in this class.
        /// This is required mainly because Aws::Http::HttpRequest demands that these parameters be configured at the time of construction.
        /// You can also use this opportunity to set properties of the HttpRequestJob you want reflected in the request.
        /// WARNING: This gets called on the job's thread, so observe thread safety precautions.
        virtual std::shared_ptr<Aws::Http::HttpRequest> InitializeRequest();

        /// Override to customize HTTP request right before it is sent.
        /// Configuration specified in this class will already have been configured in the request.
        /// WARNING: This gets called on the job's thread, so observe thread safety precautions.
        virtual void CustomizeRequest(const std::shared_ptr<Aws::Http::HttpRequest>& request)
        {
            AZ_UNUSED(request);
        };

        /// Override to process the response to the HTTP request before callbacks are fired.
        /// WARNING: This gets called on the job's thread, so observe thread safety precautions.
        virtual void ProcessResponse(const std::shared_ptr<Aws::Http::HttpResponse>& response)
        {
            AZ_UNUSED(response);
        };

    private:
        /// Runs the HTTP request on the Job's thread.
        void Process() override;

    private:
        std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface> m_readRateLimiter;
        std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface> m_writeRateLimiter;
        std::shared_ptr<Aws::Http::HttpClient> m_httpClient;
        std::shared_ptr<Aws::Client::AWSAuthSigner> m_awsAuthSigner;
        SuccessFn m_successCallback;
        FailureFn m_failureCallback;
        StringMap m_requestHeaders;
        AZStd::string m_url;
        AZStd::string m_requestBody;
        HttpMethod m_method = HttpMethod::HTTP_GET;
    };

    template<typename SuccessCallbackT, typename FailureCallbackT>
    inline void HttpRequestJob::SetCallbacks(SuccessCallbackT&& successCB, FailureCallbackT&& failureCB)
    {
        m_successCallback = AZStd::forward<SuccessCallbackT>(successCB);
        m_failureCallback = AZStd::forward<FailureCallbackT>(failureCB);
    }

} // namespace AWSCore
