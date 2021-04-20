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

#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Per-job information to configure and run jobs and process the resulting artifacts.
    //! @tparam AdditionalInfo Additional information to be provided to each job to be consumed by client.
    template<typename AdditionalInfo>
    class JobInfo
        : public AdditionalInfo
    {
    public:
        using IdType = size_t;

        //! Client-provided identifier to distinguish between different jobs.
        //! @note Ids of different job types are not interchangeable.
        struct Id
        {
            IdType m_value;
        };

        //! Constructs the job information with any additional information required by the job.
        //! @param jobId The client-provided unique identifier for the job.
        //! @param args The arguments used to launch the process running the job.
        //! @param additionalInfo The arguments to be provided to the additional information data structure.
        template<typename... AdditionalInfoArgs>
        JobInfo(Id jobId, const AZStd::string& args, AdditionalInfoArgs&&... additionalInfo);

        //! Returns the id of this job.
        Id GetId() const;

        //! Returns the command arguments used to execute this job.
        const AZStd::string& GetArgs() const;

    private:
        Id m_id;
        AZStd::string m_args;
    };

    template<typename AdditionalInfo>
    template<typename... AdditionalInfoArgs>
    JobInfo<AdditionalInfo>::JobInfo(Id jobId, const AZStd::string& args, AdditionalInfoArgs&&... additionalInfo)
        : AdditionalInfo{std::forward<AdditionalInfoArgs>(additionalInfo)...}
        , m_id(jobId)
        , m_args(args)
    {
    }

    template<typename AdditionalInfo>
    typename JobInfo<AdditionalInfo>::Id JobInfo<AdditionalInfo>::GetId() const
    {
        return m_id;
    }

    template<typename AdditionalInfo>
    const AZStd::string& JobInfo<AdditionalInfo>::GetArgs() const
    {
        return m_args;
    }
} // namespace TestImpact
