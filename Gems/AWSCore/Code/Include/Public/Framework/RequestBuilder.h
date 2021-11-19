/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Framework/JsonWriter.h>
// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.

#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/http/HttpTypes.h>
AZ_POP_DISABLE_WARNING

namespace Aws
{
    namespace Client
    {
        class AWSAuthSigner;
    }
}

namespace AWSCore
{
    /// Provides methods for replacing url path parameters, appending
    /// url query string parameters, and writing body content.
    class RequestBuilder
    {
    public:
        RequestBuilder();

        /// Converts the provided object to JSON and sends it as the
        /// body of the request. The object can implement the following
        /// function to enable serialization:
        ///
        ///    bool WriteJson(ServiceApi::JsonWriter& writer) const
        ///    {
        ///       bool ok = true;
        ///       ok = ok && writer.StartObject();
        ///       ...
        ///       ok = ok && writer.EndObject();
        ///       return ok;
        ///    }
        ///
        /// Alternatively you can provide a global function with the 
        /// following signature:
        ///
        /// namespace AWSCore {
        ///     template<>
        ///     bool GlobalWriteJson<MyObjectType>(ServiceApi::JsonWriter& writer, const MyObjectType& source)
        ///     {
        ///         bool ok = true;
        ///         ...
        ///         return ok;
        ///     }
        /// }
        /// 
        template<class ObjectType>
        bool WriteJsonBodyParameter(const ObjectType& bodyObject)
        {
            m_bodyContent = std::make_shared<Aws::StringStream>();
            JsonOutputStream stream{ *m_bodyContent.get() };
            return JsonWriter::WriteObject(stream, bodyObject);
        }

        bool WriteJsonBodyRaw(const AZStd::string& body)
        {
            m_bodyContent = std::make_shared<Aws::StringStream>();
            m_bodyContent->str(body.c_str());
            return true;
        }

        const Aws::String& GetRequestUrl()
        {
            return m_requestUrl;
        }

        void SetRequestUrl(const Aws::String& requestUrl)
        {
            m_requestUrl = requestUrl;
        }

        /// Replaces a key with an escaped value. Key should be 
        /// "{foo}" to replace the "{foo}" part of "/bar/{foo}".
        bool SetPathParameter(const char* key, const AZStd::string& value);
        bool SetPathParameter(const char* key, const char* value);
        bool SetPathParameter(const char* key, double value);
        bool SetPathParameter(const char* key, bool value);
        bool SetPathParameter(const char* key, int value);
        bool SetPathParameter(const char* key, int64_t value);
        bool SetPathParameter(const char* key, unsigned value);
        bool SetPathParameter(const char* key, uint64_t value);

        /// Appends a query parameter to the url. An "?" or "&" is added 
        /// as needed. The value is escaped.
        bool AddQueryParameter(const char* name, const AZStd::string& value);
        bool AddQueryParameter(const char* name, const char* value);
        bool AddQueryParameter(const char* name, double value);
        bool AddQueryParameter(const char* name, bool value);
        bool AddQueryParameter(const char* name, int value);
        bool AddQueryParameter(const char* name, int64_t value);
        bool AddQueryParameter(const char* name, unsigned value);
        bool AddQueryParameter(const char* name, uint64_t value);

        Aws::Http::HttpMethod GetHttpMethod()
        {
            return m_httpMethod;
        }

        void SetHttpMethod(Aws::Http::HttpMethod httpMethod)
        {
            m_httpMethod = httpMethod;
        }

        const AZStd::string& GetErrorMessage()
        {
            return m_errorMessage;
        }

        void SetErrorMessage(const AZStd::string& message)
        {
            m_errorMessage = message;
        }

        std::shared_ptr<Aws::StringStream> GetBodyContent()
        {
            return m_bodyContent;
        }

        void SetAWSAuthSigner(std::shared_ptr<Aws::Client::AWSAuthSigner> awsAuthSigner)
        {
            m_AWSAuthSigner = awsAuthSigner;
        }

        std::shared_ptr<Aws::Client::AWSAuthSigner> GetAWSAuthSigner()
        {
            return m_AWSAuthSigner;
        }

    private:
        bool SetPathParameterUnescaped(const char* key, const char* value);
        bool AddQueryParameterUnescaped(const char* name, const char* value);

        /// copy and assignment not supported.
        RequestBuilder(const RequestBuilder&) = delete;
        RequestBuilder& operator=(const RequestBuilder&) = delete;

        /// HTTP method for the request.
        Aws::Http::HttpMethod m_httpMethod;

        /// The url being modified.
        Aws::String m_requestUrl;

        /// Description of error should one occur.
        AZStd::string m_errorMessage;

        /// JSON format body content.
        std::shared_ptr<Aws::StringStream> m_bodyContent;

        /// Appends characters from value to target, escaping special characters.
        static Aws::String escape(const char* value);

        //? AWS signer to use for request signing
        std::shared_ptr<Aws::Client::AWSAuthSigner> m_AWSAuthSigner{ nullptr };

    };

} // namespace AWSCore
