/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <native/utilities/assetUtils.h>
#include <QDir>  // used in the inl file.
#include <utilities/Builder.h>
#include <utilities/BuilderList.h>

class ConnectionManager;

namespace AssetProcessor
{
    struct BuilderRef;

    //! Indicates if job request files should be created on success.  Can be useful for debugging
    static const bool s_createRequestFileForSuccessfulJob = false;

    //! This EBUS is used to request a free builder from the builder manager pool
    class BuilderManagerBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using MutexType = AZStd::recursive_mutex;

        virtual ~BuilderManagerBusTraits() = default;

        //! Returns a builder for doing work
        virtual BuilderRef GetBuilder(BuilderPurpose purpose) = 0;

        virtual void AddAssetToBuilderProcessedList(const AZ::Uuid& /*builderId*/, const AZStd::string& /*sourceAsset*/)
        {
        }
    };

    using BuilderManagerBus = AZ::EBus<BuilderManagerBusTraits>;

    class BuilderDebugOutput
    {
    public:
        AZStd::list<AZStd::string> m_assetsProcessed;
    };

    //! Manages the builder pool
    class BuilderManager
        : public BuilderManagerBus::Handler
    {
    public:
        explicit BuilderManager(ConnectionManager* connectionManager);
        ~BuilderManager();

        // Disable copy
        AZ_DISABLE_COPY_MOVE(BuilderManager);

        void ConnectionLost(AZ::u32 connId);

        //BuilderManagerBus
        BuilderRef GetBuilder(BuilderPurpose purpose) override;
        void AddAssetToBuilderProcessedList(const AZ::Uuid& builderId, const AZStd::string& sourceAsset) override;

    protected:

        //! Makes a new builder, adds it to the pool, and returns a shared pointer to it
        virtual AZStd::shared_ptr<Builder> AddNewBuilder(BuilderPurpose purpose);

        //! Handles incoming builder connections
        void IncomingBuilderPing(AZ::u32 connId, AZ::u32 type, AZ::u32 serial, QByteArray payload, QString platform);

        void PumpIdleBuilders();

        void PrintDebugOutput();

        AZStd::mutex m_buildersMutex;

        //! List of builders.  Must be locked before accessing
        BuilderList m_builderList;

        // Track debug output generated per builder.
        // This is done this way so that it can be output in order, to track down race conditions with asset builders.
        AZStd::unordered_map<AZ::Uuid, BuilderDebugOutput> m_builderDebugOutput;

        //! Indicates if we allow builders to connect that we haven't started up ourselves.  Useful for debugging
        bool m_allowUnmanagedBuilderConnections = false;

        //! Responsible for going through all the idle builders and pumping their communicators so they don't stall
        AZStd::thread m_pollingThread;

        AssetUtilities::QuitListener m_quitListener;
    };
} // namespace AssetProcessor

#include "native/utilities/BuilderManager.inl"
