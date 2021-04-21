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
