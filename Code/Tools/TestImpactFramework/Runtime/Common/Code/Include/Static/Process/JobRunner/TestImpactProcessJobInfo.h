/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        using CommandType = AZStd::string;

        //! Client-provided identifier to distinguish between different jobs.
        //! @note Ids of different job types are not interchangeable.
        struct Id
        {
            IdType m_value;
        };

        //! Command used my ProcessScheduler to execute this job.
        //! @note Commands of different job types are not interchangeable.
        struct Command
        {
            CommandType m_args;
        };

        //! Constructs the job information with any additional information required by the job.
        //! @param jobId The client-provided unique identifier for the job.
        //! @param command The command used to launch the process running the job.
        //! @param additionalInfo The arguments to be provided to the additional information data structure.
        template<typename... AdditionalInfoArgs>
        JobInfo(Id jobId, const Command& command, AdditionalInfoArgs&&... additionalInfo);

        //! Returns the id of this job.
        Id GetId() const;

        //! Returns the command arguments used to execute this job.
        const Command& GetCommand() const;

    private:
        Id m_id;
        Command m_command;
    };

    template<typename AdditionalInfo>
    template<typename... AdditionalInfoArgs>
    JobInfo<AdditionalInfo>::JobInfo(Id jobId, const Command& command, AdditionalInfoArgs&&... additionalInfo)
        : AdditionalInfo{std::forward<AdditionalInfoArgs>(additionalInfo)...}
        , m_id(jobId)
        , m_command(command)
    {
    }

    template<typename AdditionalInfo>
    typename JobInfo<AdditionalInfo>::Id JobInfo<AdditionalInfo>::GetId() const
    {
        return m_id;
    }

    template<typename AdditionalInfo>
    const typename JobInfo<AdditionalInfo>::Command& JobInfo<AdditionalInfo>::GetCommand() const
    {
        return m_command;
    }
} // namespace TestImpact
