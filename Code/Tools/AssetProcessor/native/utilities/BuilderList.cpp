/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/utilities/BuilderList.h>

namespace AssetProcessor
{
    void BuilderList::AddBuilder(AZStd::shared_ptr<Builder> builder, BuilderPurpose purpose)
    {
        if (purpose == BuilderPurpose::CreateJobs)
        {
            if (m_createJobsBuilder && m_createJobsBuilder->IsValid())
            {
                AZ_Error(
                    "BuilderList", false, "AddBuilder called with CreateJobs builder (%s) but a CreateJobs builder (%s) already exists and is valid",
                    builder->UuidString().c_str(), m_createJobsBuilder->UuidString().c_str());
                return;
            }

            m_createJobsBuilder = AZStd::move(builder);
        }
        else
        {
            m_builders.emplace(builder->GetUuid(), builder);
        }
    }

    AZStd::shared_ptr<Builder> BuilderList::Find(AZ::Uuid uuid)
    {
        if (m_createJobsBuilder && m_createJobsBuilder->GetUuid() == uuid)
        {
            return m_createJobsBuilder;
        }

        auto itr = m_builders.find(uuid);

        return itr != m_builders.end() ? itr->second : nullptr;
    }

    BuilderRef BuilderList::GetFirst(BuilderPurpose purpose)
    {
        if (purpose == BuilderPurpose::CreateJobs)
        {
            if (m_createJobsBuilder)
            {
                if (!m_createJobsBuilder->m_busy)
                {
                    m_createJobsBuilder->PumpCommunicator();

                    if (m_createJobsBuilder->IsValid())
                    {
                        return BuilderRef(m_createJobsBuilder);
                    }

                    m_createJobsBuilder = nullptr;
                }
                else
                {
                    AZ_Warning(
                        "BuilderList",
                        false,
                        "CreateJobs builder requested but existing builder is already busy.  There should not be multiple parallel "
                        "requests for CreateJobs builders");
                }
            }

            return {};
        }

        for (auto itr = m_builders.begin(); itr != m_builders.end();)
        {
            auto& builder = itr->second;

            if (!builder->m_busy)
            {
                builder->PumpCommunicator();

                if (builder->IsValid())
                {
                    return BuilderRef(builder);
                }

                itr = m_builders.erase(itr);
            }
            else
            {
                ++itr;
            }
        }

        return {};
    }

    AZStd::string BuilderList::RemoveByConnectionId(AZ::u32 connId)
    {
        AZStd::string uuidString;

        // Note that below the connectionId will be set to 0.
        // The builder might not be destroyed immediately if another thread is currently holding a reference.
        // If the builder is currently in use, this will signal to the waiting thread to not expect a reply
        // and fail the current job request.

        if (m_createJobsBuilder && m_createJobsBuilder->GetConnectionId() == connId)
        {
            uuidString = m_createJobsBuilder->UuidString();
            m_createJobsBuilder->m_connectionId = 0;
            m_createJobsBuilder = nullptr;

            return uuidString;
        }
        else
        {
            for (auto itr = m_builders.begin(); itr != m_builders.end(); ++itr)
            {
                auto& builder = itr->second;

                if (builder->GetConnectionId() == connId)
                {
                    uuidString = builder->UuidString();
                    builder->m_connectionId = 0;
                    m_builders.erase(itr);

                    return uuidString;
                }
            }
        }

        return {};
    }

    void BuilderList::RemoveByUuid(AZ::Uuid uuid)
    {
        if (m_createJobsBuilder && m_createJobsBuilder->GetUuid() == uuid)
        {
            m_createJobsBuilder = nullptr;
        }
        else
        {
            m_builders.erase(uuid);
        }
    }

    void BuilderList::PumpIdleBuilders()
    {
        if (m_createJobsBuilder && !m_createJobsBuilder->m_busy)
        {
            m_createJobsBuilder->PumpCommunicator();
        }

        for (auto pair : m_builders)
        {
            auto builder = pair.second;

            if (!builder->m_busy)
            {
                builder->PumpCommunicator();
            }
        }
    }
}
