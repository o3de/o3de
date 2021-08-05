/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Framework/AWSApiClientJobConfig.h>
#include <Framework/AWSApiJob.h>

namespace AWSCore
{
    //! A job that uses an AWS API client. To use, extend this class and 
    //! implement the AZ::Job defined Process function. That function can
    //! use the protected m_client object to make AWS requests.
    template<class ClientType>
    class AwsApiClientJob
        : public AwsApiJob
    {

    public:

        using AwsApiClientJobType = AwsApiClientJob<ClientType>;

        // To use a different allocator, extend this class and use this macro.
        AZ_CLASS_ALLOCATOR(AwsApiClientJob, AZ::SystemAllocator, 0);

        using IConfig = IAwsApiClientJobConfig<ClientType>;
        using Config = AwsApiClientJobConfig<ClientType>;

        static Config* GetDefaultConfig()
        {
            static AwsApiJobConfigHolder<Config> s_configHolder{};
            return s_configHolder.GetConfig(AwsApiJob::GetDefaultConfig());
        }

    protected:
        AwsApiClientJob(bool isAutoDelete, IConfig* config = GetDefaultConfig())
            : AwsApiJob(isAutoDelete, config)
            , m_client{config->GetClient()}
        {
        }

        std::shared_ptr<ClientType> m_client;

    private:
        AwsApiClientJob(const AwsApiClientJob&) = delete;
        AwsApiClientJob& operator=(const AwsApiClientJob&) = delete;

    };

} // namespace AWSCore

