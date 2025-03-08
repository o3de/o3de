/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ExecutionLogAsset.h"
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>

namespace ScriptCanvas
{
    ExecutionLogData::ExecutionLogData(const ExecutionLogData& source)
    {
        *this = source;
    }

    ExecutionLogData::~ExecutionLogData()
    {
        Clear();
    }

    void ExecutionLogData::Clear()
    {
        m_events.clear();
    }

    ExecutionLogData& ExecutionLogData::operator=(const ExecutionLogData& source)
    {
        Clear();

        m_events.reserve(source.m_events.size());
    
        for (auto entry : source.m_events)
        {
            m_events.push_back(entry);
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

    AZ::IO::FixedMaxPath ExecutionLogAsset::GetDefaultDirectoryPath()
    {
        AZ::IO::FixedMaxPath logDirectoryPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(logDirectoryPath,
            AZ::IO::PathView("@log@/ScriptCanvas/Assets/Logs"));
        return logDirectoryPath;
    }

    ExecutionLogAsset::ExecutionLogAsset(const AZ::Data::AssetId& assetId, AZ::Data::AssetData::AssetStatus status)
        : AZ::Data::AssetData(assetId, status)
    {}
   
    void ExecutionLogAsset::SetData(const ExecutionLogData& runtimeData)
    {
        m_logData = runtimeData;
    }
}
