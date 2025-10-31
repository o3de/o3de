/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Graph/AssetStatusReporterState.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AtomToolsFramework
{
    //! AssetStatusReporter processes a queue of job status requests for a set of source files
    class AssetStatusReporter
    {
    public:
        AZ_RTTI(AssetStatusReporter, "{A646AC72-A5E5-4B92-8243-3A1F8BA083AB}");
        AZ_CLASS_ALLOCATOR(AssetStatusReporter, AZ::SystemAllocator);
        AZ_DISABLE_COPY_MOVE(AssetStatusReporter);

        AssetStatusReporter(const AZStd::vector<AZStd::string>& sourcePaths);
        virtual ~AssetStatusReporter() = default;

        AssetStatusReporterState Update();
        AssetStatusReporterState GetCurrentState() const;
        AZStd::string GetCurrentStateName() const;
        AZStd::string GetCurrentStatusMessage() const;
        AZStd::string GetCurrentPath() const;

    private:
        AZStd::vector<AZStd::string> m_sourcePaths;
        size_t m_index = {};
        bool m_failed = {};
    };
} // namespace AtomToolsFramework
