/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        ~AwsApiJob() override = default;

        /// Used for error messages.
        static const char* COMPONENT_DISPLAY_NAME;
    };

} // namespace AWSCore

