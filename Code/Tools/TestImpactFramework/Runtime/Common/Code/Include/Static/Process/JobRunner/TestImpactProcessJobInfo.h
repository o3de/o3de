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
            using IdType = IdType;
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

        //! Copy and move constructors/assignment operators.
        JobInfo(const JobInfo& other);
        JobInfo(JobInfo&& other);
        JobInfo& operator=(const JobInfo& other);
        JobInfo& operator=(JobInfo&& other);

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
    JobInfo<AdditionalInfo>::JobInfo(const JobInfo& other)
        : AdditionalInfo(other)
        , m_id(other.m_id)
        , m_command(other.m_command)
    {
    }

    template<typename AdditionalInfo>
    JobInfo<AdditionalInfo>::JobInfo(JobInfo&& other)
        : AdditionalInfo(AZStd::move(other))
        , m_id(AZStd::move(other.m_id))
        , m_command(AZStd::move(other.m_command))
    {
    }

    template<typename AdditionalInfo>
    JobInfo<AdditionalInfo>& JobInfo<AdditionalInfo> ::operator=(const JobInfo& other)
    {
        m_id = other.m_id;
        m_command = other.m_command;
        return *this;
    }

    template<typename AdditionalInfo>
    JobInfo<AdditionalInfo>& JobInfo<AdditionalInfo>::operator=(JobInfo&& other)
    {
        m_id = other.m_id;
        m_command = AZStd::move(other.m_command);
        return *this;
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

    //! Comparison between ids of the same job type.
    template<typename AdditionalInfo>
    bool operator==(const typename JobInfo<AdditionalInfo>::Id& lhs, const typename JobInfo<AdditionalInfo>::Id& rhs)
    {
        return lhs.m_value == rhs.m_value;
    }

    //! Comparison between jobs of the same job type.
    template<typename AdditionalInfo>
    bool operator==(const JobInfo<AdditionalInfo>& lhs, const JobInfo<AdditionalInfo>& rhs)
    {
        return lhs.GetId().m_value == rhs.GetId().m_value;
    }
} // namespace TestImpact

namespace AZStd
{
    //! Less function for JobInfo types for use in maps and sets.
    template<typename AdditionalInfo>
    struct less<typename TestImpact::JobInfo<AdditionalInfo>>
    {
        bool operator()(const typename TestImpact::JobInfo<AdditionalInfo>& lhs, const typename TestImpact::JobInfo<AdditionalInfo>& rhs) const
        {
            return lhs.GetId().m_value < rhs.GetId().m_value;
        }
    };

    //! Hash function for JobInfo types for use in unordered maps and sets.
    template<typename AdditionalInfo>
    struct hash<typename TestImpact::JobInfo<AdditionalInfo>>
    {
        size_t operator()(const typename TestImpact::JobInfo<AdditionalInfo>& jobInfo) const noexcept
        {
            return hash<typename TestImpact::JobInfo<AdditionalInfo>::IdType>{}(jobInfo.GetId().m_value);
        }
    };
} // namespace std
