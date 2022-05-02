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
    /**
     * Indicator status for DataSystemSourceNotifications and Requests
     */
    enum class BuilderSourceStatus
    {
        Failed,
        Good,
        Removed,
        Unloadable,
    };

    /**
     * Status and property data for DataSystemSourceNotifications and Requests 
     */
    struct BuilderSourceResult
    {
        BuilderSourceStatus status = BuilderSourceStatus::Failed;
        const BuildVariableOverrides* data = nullptr;
    };

    /**
     * Provides notifications of changes, failures, and removals of ScriptCanvas source files in the project folders only.
     * This alleviates clients which are only interested in ScriptCanvas source file status from having to listen to the AssetSystemBus
     * themselves, and checking for or re-parsing for ScriptCanvas builder data.
     */
    class DataSystemSourceNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::Uuid;

        /// sent when a source file has been modified or newly added
        virtual void SourceFileChanged(const BuilderSourceResult& result, AZStd::string_view relativePath, AZStd::string_view scanFolder) = 0;
        /// sent when a source file failed to load or process
        virtual void SourceFileFailed(AZStd::string_view relativePath, AZStd::string_view scanFolder) = 0;
        /// sent when file was removed from the tracked system
        virtual void SourceFileRemoved(AZStd::string_view relativePath, AZStd::string_view scanFolder) = 0;
    };
    using DataSystemSourceNotificationsBus = AZ::EBus<DataSystemSourceNotifications>;

    /**
     * Builder requests allow clients to query the source Builder status and data on demand.
     */
    class DataSystemSourceRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        /// Makes an on-demand request for the compiled builder data of the source
        virtual BuilderSourceResult CompileBuilderData(ScriptCanvasEditor::SourceHandle sourceHandle) = 0;
    };
    using DataSystemSourceRequestsBus = AZ::EBus<DataSystemSourceRequests>;

    /**
    * Notifications distilled from asset processor notifications relating to ScriptCanvas assets.
    */
    class DataSystemAssetNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::Uuid;
        /// the asset, possibly due to change, is immediately available for execution
        virtual void OnReady(ScriptCanvas::RuntimeAssetPtr asset) = 0;
        /// the asset, possibly due to change or removal, is no longer available for execution at all
        virtual void OnAssetNotReady() = 0;
    };
    using DataSystemAssetNotificationsBus = AZ::EBus<DataSystemAssetNotifications>;

    /**
     * Indicator status DataSystemAssetRequests
     */
    enum class BuilderAssetStatus
    {
        Ready,
        Pending,
        Error
    };

    /**
     * Status and asset data for DataSystemAssetRequests
     */
    struct BuilderAssetResult
    {
        BuilderAssetStatus status = BuilderAssetStatus::Error;
        ScriptCanvas::RuntimeAssetPtr data;
    };

    /**
     * Requests for asset status and data
     */
    class DataSystemAssetRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        /// Returns status and the Asset (if there is one) for the supplied SourceHandle. If
        /// the status is 'Ready', the asset can be executed immediately.
        /// If it is 'Pending', the system is waiting on results of processing since the source
        /// has recently changed.
        virtual BuilderAssetResult LoadAsset(ScriptCanvasEditor::SourceHandle sourceHandle) = 0;
    };
    using DataSystemAssetRequestsBus = AZ::EBus<DataSystemAssetRequests>;
}
