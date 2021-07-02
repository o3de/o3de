/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformDef.h>
#include <memory>
AZ_PUSH_DISABLE_WARNING(4251 4355 4996, "-Wunknown-warning-option") // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/s3/S3Client.h>
AZ_POP_DISABLE_WARNING

typedef std::shared_ptr<Aws::IOStream> STREAM_PTR;

namespace News
{
    //! A light wrapper around AWS SDK
    class S3Connector
    {
    public:
        static const char* ALLOCATION_TAG;

        S3Connector();
        ~S3Connector();

        //! Make s3 client using credentials stored in [user]/.aws/credentials file
        /*!
            \param awsProfileName - name of AWS credentials
            \param bucket - name of s3 bucket to use for AWS operations
            \param error - error string
            \retval bool - true if success else false
        */
        bool Init(const char* awsProfileName, const char* bucket, Aws::String& error);

        bool GetObject(const char* key,
            Aws::String& data,
            Aws::String& url,
            Aws::String& error) const;

        bool PutObject(const char* key,
            STREAM_PTR stream,
            Aws::String& url,
            Aws::String& error) const;

        bool PutObject(const char* key,
            STREAM_PTR stream,
            int length,
            Aws::String& url,
            Aws::String& error) const;

        bool DeleteObject(const char* key,
            Aws::String& error) const;

    private:
        Aws::String m_bucket;
        std::shared_ptr<Aws::S3::S3Client> m_s3Client;
        std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface> m_limiter;
        bool m_valid;

        static int GetStreamLength(STREAM_PTR stream);
    };
} // namespace News
