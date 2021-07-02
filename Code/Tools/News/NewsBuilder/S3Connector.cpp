/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "S3Connector.h"

#include <fstream>
#include <iostream>
#include <AzFramework/AzFramework_Traits_Platform.h>


AZ_PUSH_DISABLE_WARNING(4251 4819, "-Wunknown-warning-option") // Invalid character not in default code page
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/core/utils/Outcome.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/core/utils/memory/stl/AwsStringStream.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/Aws.h>
AZ_POP_DISABLE_WARNING

namespace News
{
    const char* S3Connector::ALLOCATION_TAG = "NewsBuilder";

    S3Connector::S3Connector()
        : m_s3Client(nullptr)
        , m_limiter(nullptr)
        , m_valid(false)
    {
    }

    S3Connector::~S3Connector() {}

    bool S3Connector::Init(const char* awsProfileName, const char* bucket, Aws::String& error)
    {
        Aws::SDKOptions options;
        options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Trace;
        Aws::InitAPI(options);

        Aws::Client::ClientConfiguration config;
        config.enableTcpKeepAlive = AZ_TRAIT_AZFRAMEWORK_AWS_ENABLE_TCP_KEEP_ALIVE_SUPPORTED;
        config.scheme = Aws::Http::Scheme::HTTPS;
        config.connectTimeoutMs = 30000;
        config.requestTimeoutMs = 30000;
        config.readRateLimiter = m_limiter;
        config.writeRateLimiter = m_limiter;

        auto provider =
            Aws::MakeShared<Aws::Auth::ProfileConfigFileAWSCredentialsProvider>(
                ALLOCATION_TAG,
                awsProfileName);

        auto accessKeyId = provider->GetAWSCredentials().GetAWSAccessKeyId();

        //! verify that credential file exists
        if (accessKeyId.empty())
        {
            error = "LY_NEWS_DEVELOPER AWS credentials not found. Add credentials in LY Editor AWS->ClientManager";
            m_valid = false;
            return false;
        }

        m_bucket = Aws::String(bucket);
        m_s3Client = Aws::MakeShared<Aws::S3::S3Client>(
                ALLOCATION_TAG,
                provider,
                config);

        m_valid = true;
        return true;
    }

    bool S3Connector::GetObject(const char* key,
        Aws::String& data,
        Aws::String& url,
        Aws::String& error) const
    {
        if (!m_valid)
        {
            error = "Client not initialized";
            return false;
        }

        Aws::S3::Model::GetObjectRequest getObjectRequest;
        getObjectRequest.SetBucket(m_bucket);
        getObjectRequest.SetKey(key);
        auto outcome = m_s3Client->GetObject(getObjectRequest);
        if (!outcome.IsSuccess())
        {
            error = outcome.GetError().GetMessage();
            return false;
        }
        url = m_s3Client->GeneratePresignedUrl(m_bucket, key, Aws::Http::HttpMethod::HTTP_GET);
        Aws::StringStream ss;
        ss << outcome.GetResult().GetBody().rdbuf();
        data = ss.str();
        return true;
    }

    bool S3Connector::PutObject(const char* key,
        STREAM_PTR stream,
        Aws::String& url,
        Aws::String& error) const
    {
        return PutObject(key, stream, GetStreamLength(stream), url, error);
    }

    bool S3Connector::PutObject(const char* key,
        STREAM_PTR stream,
        int length,
        Aws::String& url,
        Aws::String& error) const
    {
        if (!m_valid)
        {
            error = "Client not initialized";
            return false;
        }

        Aws::S3::Model::PutObjectRequest putObjectRequest;
        putObjectRequest.SetBucket(m_bucket);
        putObjectRequest.SetBody(stream);
        putObjectRequest.SetContentLength(length);
        putObjectRequest.SetContentMD5(
            Aws::Utils::HashingUtils::Base64Encode(
                Aws::Utils::HashingUtils::CalculateMD5(*putObjectRequest.GetBody())));
        putObjectRequest.SetContentType(Aws::String("binary/octet-stream"));
        putObjectRequest.SetKey(key);
        putObjectRequest.SetACL(Aws::S3::Model::ObjectCannedACL::public_read);
        auto outcome = m_s3Client->PutObject(putObjectRequest);
        if (!outcome.IsSuccess())
        {
            error = outcome.GetError().GetMessage();
            return false;
        }
        url = m_s3Client->GeneratePresignedUrl(m_bucket, key, Aws::Http::HttpMethod::HTTP_GET);
        return true;
    }

    bool S3Connector::DeleteObject(const char* key, Aws::String& error) const
    {
        if (!m_valid)
        {
            error = "Client not initialized";
            return false;
        }

        Aws::S3::Model::DeleteObjectRequest deleteObjectRequest;
        deleteObjectRequest.SetBucket(m_bucket);
        deleteObjectRequest.SetKey(key);
        auto outcome = m_s3Client->DeleteObject(deleteObjectRequest);
        if (!outcome.IsSuccess())
        {
            error = outcome.GetError().GetMessage();
            return false;
        }
        return true;
    }

    int S3Connector::GetStreamLength(STREAM_PTR stream)
    {
        stream->seekg(0, std::ios::end);
        auto length = stream->tellg();
        stream->seekg(0, std::ios::beg);
        return static_cast<int>(length);
    }

}
