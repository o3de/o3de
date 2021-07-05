/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Framework/AWSApiJob.h>

namespace AWSCore
{

    const char* AwsApiJob::COMPONENT_DISPLAY_NAME = "AWSCoreFramework";

    AwsApiJob::AwsApiJob(bool isAutoDelete, IConfig* config)
        : AZ::Job(isAutoDelete, config->GetJobContext())
    {
    }

    AwsApiJob::~AwsApiJob()
    {
    }

    AwsApiJob::Config* AwsApiJob::GetDefaultConfig()
    {
        static AwsApiJobConfigHolder<AwsApiJob::Config> s_configHolder{};
        return s_configHolder.GetConfig(nullptr,
            [](AwsApiJobConfig& config) {
                config.userAgent = "/O3DE_AwsApiJob";
                config.requestTimeoutMs = 30000;
                config.connectTimeoutMs = 30000;
            }
        );
    };

}
