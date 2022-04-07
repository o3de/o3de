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
#include <ScriptCanvas/Asset/RuntimeAsset.h>

namespace ScriptCanvasBuilder
{
    // sources...
    enum class BuilderSourceStatus
    {
        Failed,
        Good,
        Removed,
        Unloadable,
    };

    struct BuilderSourceResult
    {
        BuilderSourceStatus status = BuilderSourceStatus::Failed;
        const BuildVariableOverrides* data = nullptr;
    };

    class DataSystemSourceNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::Uuid;

        // the file has been modified
        virtual void SourceFileChanged(const BuilderSourceResult& result, AZStd::string_view relativePath, AZStd::string_view scanFolder) = 0;
        // the file failed to load or process
        virtual void SourceFileFailed(AZStd::string_view relativePath, AZStd::string_view scanFolder) = 0;
        // the file was removed from the tracked system
        virtual void SourceFileRemoved(AZStd::string_view relativePath, AZStd::string_view scanFolder) = 0;
    };
    using DataSystemSourceNotificationsBus = AZ::EBus<DataSystemSourceNotifications>;

    class DataSystemSourceRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual BuilderSourceResult CompileBuilderData(ScriptCanvasEditor::SourceHandle sourceHandle) = 0;
    };
    using DataSystemSourceRequestsBus = AZ::EBus<DataSystemSourceRequests>;


    // assets...
    class DataSystemAssetNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::Uuid;
        // the asset, possibly due to change, is immediately available
        virtual void OnReady(ScriptCanvas::RuntimeAssetPtr asset) = 0;
        // the asset, possibly due to change, is no longer available
        virtual void OnAssetNotReady() = 0;
    };
    using DataSystemAssetNotificationsBus = AZ::EBus<DataSystemAssetNotifications>;

    enum class BuilderAssetStatus
    {
        Ready,
        Pending,
        Error
    };

    struct BuilderAssetResult
    {
        BuilderAssetStatus status = BuilderAssetStatus::Error;
        ScriptCanvas::RuntimeAssetPtr data;
    };

    class DataSystemAssetRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual BuilderAssetResult LoadAsset(ScriptCanvasEditor::SourceHandle sourceHandle) = 0;
    };
    using DataSystemAssetRequestsBus = AZ::EBus<DataSystemAssetRequests>;
}
