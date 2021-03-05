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


#include <AzCore/Asset/AssetCommon.h>
#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>

namespace ScriptCanvas
{
    struct LoggableEvent;

    struct ExecutionLogData
    {
        AZ_TYPE_INFO(ExecutionLogData, "{8813C6D6-7FC6-4A41-B77B-5B8BFC9C4C01}");
        AZ_CLASS_ALLOCATOR(ExecutionLogData, AZ::SystemAllocator, 0);
                
        static void Reflect(AZ::ReflectContext* reflectContext);
        
        AZStd::vector<LoggableEvent*> m_events;
        
        
        ExecutionLogData() = default;
        ExecutionLogData(const ExecutionLogData& source);
        ExecutionLogData(ExecutionLogData&&);
        ~ExecutionLogData();
                
        void Clear();
        ExecutionLogData& operator=(const ExecutionLogData& source);
        ExecutionLogData& operator=(ExecutionLogData&&);
    };

    class ExecutionLogAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(ExecutionLogAsset, "{2D49C2E2-2CAF-4F3F-8BED-D613DC16D3F5}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(ExecutionLogAsset, AZ::SystemAllocator, 0);
                
        static void Reflect(AZ::ReflectContext* reflectContext);

        static const char* GetDisplayName() { return "ScriptCanvas Log"; }
        static const char* GetGroup() { return "ScriptCanvasLogs"; }
        static const char* GetFileExtension() { return "scriptcanvas_log"; }
        static const char* GetFileFilter() { return "*.scriptcanvas_log"; }
        static const char* GetDefaultDirectoryRoot();
        static const char* GetDefaultDirectoryPath() { return "/Gems/ScriptCanvas/Assets/Logs/"; }

        ExecutionLogAsset(const AZ::Data::AssetId& assetId = AZ::Data::AssetId(), AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded);
        
        const ExecutionLogData& GetData() const { return m_logData; }
        ExecutionLogData& GetData() { return m_logData; }
        void SetData(const ExecutionLogData& runtimeData);

    protected:
        friend class ExecutionLogAssetHandler;
        ExecutionLogAsset(const ExecutionLogAsset&) = delete;

        ExecutionLogData m_logData;
    };
}
