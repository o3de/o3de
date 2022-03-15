/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <Builder/ScriptCanvasBuilder.h>

namespace ScriptCanvasBuilder
{
    enum class BuilderDataStatus
    {
        Failed,
        Good,
        Removed,
        Unloadable,
    };

    struct BuildResult
    {
        BuilderDataStatus status = BuilderDataStatus::Failed;
        BuildVariableOverrides data;
    };

    class DataSystemRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual BuildResult CompileBuilderData(ScriptCanvasEditor::SourceHandle sourceHandle) = 0;
    };

    using DataSystemRequestsBus = AZ::EBus<DataSystemRequests>;

    class DataSystemNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::Uuid;

        // the file has been modified
        virtual void SourceFileChanged(const BuildResult& result, AZStd::string_view relativePath, AZStd::string_view scanFolder) = 0;
        // the file failed to load or process
        virtual void SourceFileFailed(AZStd::string_view relativePath, AZStd::string_view scanFolder) = 0;
        // the file was removed from the tracked system
        virtual void SourceFileRemoved(AZStd::string_view relativePath, AZStd::string_view scanFolder) = 0;
    };

    using DataSystemNotificationsBus = AZ::EBus<DataSystemNotifications>;
}
