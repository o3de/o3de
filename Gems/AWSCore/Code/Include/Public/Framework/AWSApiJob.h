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

#include <AzCore/Jobs/Job.h>

#include <Framework/AWSApiJobConfig.h>

namespace AWSCore
{

    /// Base class for all AWS jobs. Primarily exists so that 
    /// AwsApiJob::s_config can be used for settings that apply to
    /// all AWS jobs.
    class AwsApiJob
        : public AZ::Job
    {

    public:
        // To use a different allocator, extend this class and use this macro.
        AZ_CLASS_ALLOCATOR(AwsApiJob, AZ::SystemAllocator, 0);

        using IConfig = IAwsApiJobConfig;
        using Config = AwsApiJobConfig;

        static Config* GetDefaultConfig();

    protected:
        AwsApiJob(bool isAutoDelete, IConfig* config = GetDefaultConfig());
        virtual ~AwsApiJob();

        /// Used for error messages.
        static const char* COMPONENT_DISPLAY_NAME;

    };

} // namespace AWSCore

