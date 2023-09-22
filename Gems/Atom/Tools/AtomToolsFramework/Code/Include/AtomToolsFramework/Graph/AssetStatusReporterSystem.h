/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Graph/AssetStatusReporter.h>
#include <AtomToolsFramework/Graph/AssetStatusReporterSystemRequestBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/string/string.h>

namespace AtomToolsFramework
{
    //! AssetStatusReporterSystem processes a queue of job status requests for sets of source files
    class AssetStatusReporterSystem : public AssetStatusReporterSystemRequestBus::Handler
    {
    public:
        AZ_RTTI(AssetStatusReporterSystem, "{83ECE3A0-BFE8-47C0-B057-E4C5BE30024E}");
        AZ_CLASS_ALLOCATOR(AssetStatusReporterSystem, AZ::SystemAllocator);
        AZ_DISABLE_COPY_MOVE(AssetStatusReporterSystem);

        AssetStatusReporterSystem(const AZ::Crc32& toolId);
        ~AssetStatusReporterSystem();

        // AssetStatusReporterSystemRequestBus::Handler overrides...
        void StartReporting(const AZ::Uuid& requestId, const AZStd::vector<AZStd::string>& sourcePaths) override;
        void StopReporting(const AZ::Uuid& requestId) override;
        void StopReportingAll() override;
        AssetStatusReporterState GetStatus(const AZ::Uuid& requestId) const override;

    private:
        void Update();

        const AZ::Crc32 m_toolId = {};
        AZStd::atomic_bool m_threadRunning = {};
        AZStd::thread m_thread;
        AZStd::thread_desc m_threadDesc;
        mutable AZStd::mutex m_requestMutex;

        using ReporterTable = AZStd::vector<AZStd::pair<AZ::Uuid, AZStd::shared_ptr<AssetStatusReporter>>>;
        ReporterTable m_activeReporterTable;
        ReporterTable m_inactiveReporterTable;

        AZStd::string m_lastStatusMessage;
    };
} // namespace AtomToolsFramework
