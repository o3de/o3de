/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ExecutionLogAsset.h"
#include <AzCore/IO/FileIO.h>

namespace ScriptCanvas
{
    ExecutionLogData::ExecutionLogData(const ExecutionLogData& source)
    {
        *this = source;
    }
    
    ExecutionLogData::ExecutionLogData(ExecutionLogData&& other)
        : m_events(AZStd::move(other.m_events))
    {
    }
        
    ExecutionLogData::~ExecutionLogData()
    {
        Clear();
    }
    
    void ExecutionLogData::Clear()
    {
        for (auto entry : m_events)
        {
            delete entry;
        }

        m_events.resize(0);
    }
    
    ExecutionLogData& ExecutionLogData::operator=(const ExecutionLogData& source)
    {
        Clear();

        m_events.reserve(source.m_events.size());
    
        for (auto entry : source.m_events)
        {
            AZ_Assert(entry, "there should never bee a nullptr entry in an event log");
            m_events.push_back(entry->Duplicate());
        }

        return *this;
    }
    
    ExecutionLogData& ExecutionLogData::operator=(ExecutionLogData&& other)
    {
        if (this != &other)
        {
            m_events = AZStd::move(other.m_events);
        }

        return *this;
    }

    void ExecutionLogData::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<ExecutionLogData>()
                ->Version(0)
                ->Field("events", &ExecutionLogData::m_events)
                ;
        }
    }
    
    void ExecutionLogAsset::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<ExecutionLogAsset>()
                ->Version(0)
                ->Field("logData", &ExecutionLogAsset::m_logData)
                ;
        }
    }

    const char* ExecutionLogAsset::GetDefaultDirectoryRoot()
    {
        return AZ::IO::FileIOBase::GetInstance()->GetAlias("@engroot@");
    }

    ExecutionLogAsset::ExecutionLogAsset(const AZ::Data::AssetId& assetId, AZ::Data::AssetData::AssetStatus status)
        : AZ::Data::AssetData(assetId, status)
    {}
   
    void ExecutionLogAsset::SetData(const ExecutionLogData& runtimeData)
    {
        m_logData = runtimeData;
    }
}
