/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/Math/Crc.h> // ensure that AZ_CRC is available to all users of this header

#include <AzFramework/Asset/AssetSystemTypes.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>

namespace AzFramework
{
    namespace AssetSystem
    {
        //! AssetSystemInfoBusTraits
        //! This bus is for events that occur in the asset system in general, and has no address
        class AssetSystemInfoNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // multi listener
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;   // single bus
            using MutexType = AZStd::recursive_mutex;

            virtual ~AssetSystemInfoNotifications() = default;

            //! Notifies listeners that the Asset Processor has claimed a file in the cache for updating.
            //! The absolute path is provided. This call will be followed by AssetFileReleased.
            virtual void AssetFileClaimed(const AZStd::string& /*assetPath*/) {}
            //! Notifies listeners that the Asset Processor has released a file in the cache it previously 
            // exclusively claimed with AssetFileClaim. The absolute path is provided.
            virtual void AssetFileReleased(const AZStd::string& /*assetPath*/) {}
            //! Notifies listeners the compilation of an asset has started.
            virtual void AssetCompilationStarted(const AZStd::string& /*assetPath*/) {}
            //! Notifies listeners the compilation of an asset has succeeded.
            virtual void AssetCompilationSuccess(const AZStd::string& /*assetPath*/) {}
            //! Notifies listeners the compilation of an asset has failed.
            virtual void AssetCompilationFailed(const AZStd::string& /*assetPath*/) {}
            //! Returns the number of assets in queue for processing.
            virtual void CountOfAssetsInQueue(const int& /*count*/) {}
            //! Notifies listeners an error has occurred in the asset system
            virtual void OnError(AssetSystemErrors /*error*/) {}
        };

        //! Stores the settings needed to make a connection either to or from an AssetProcessor instance
        struct ConnectionSettings
        {
            enum class ConnectionDirection : AZ::s64
            {
                ListenForConnectFromAssetProcessor,
                ConnectToAssetProcessor
            };

            //! Name of the game project that is used for negotiating a connection with the AssetProcessor
            //! The AssetProcessor needs to be processing Assets for the specified game project
            //! (Can be queried from Settings Registry - "project_name" under the ProjectSettingsRootKey)
            AZStd::fixed_string<64> m_projectName;
            //! The IP address to use either connect to or from the AssetProcessor
            //! (Can be queried from Settings Registry - "remote_ip")
            AZStd::fixed_string<32> m_assetProcessorIp{ "127.0.0.1" };
            //! The token used to indicate the application is attempting to connect to an AssetProcessor
            //! That was built from the same code branch
            //! (Can be queried from Settings Registry - "assetProcessor_branch_token")
            AZStd::fixed_string<32> m_branchToken;
            //! The identifier that will be used for negotiating a connection with the AssetProcessor
            AZStd::fixed_string<32> m_connectionIdentifier;
            //! The asset platform to use when negotiating a connection with the Asset Processor
            //! (Can be queried from Settings Registry - "assets")
            AZStd::fixed_string<32> m_assetPlatform{ "pc" };
            //! Determines if the connection should either be to the AssetProcessor
            //! from this application or if this application should listen for a connection from
            //! the AssetProcessor
            //! (Can be queried from Settings Registry - "connect_to_remote")
            ConnectionDirection m_connectionDirection{ ConnectionDirection::ConnectToAssetProcessor };
            //! The port number to use either connect to or from the AssetProcessor
            //! (Can be queried from Settings Registry - "remote_port")
            AZ::u16 m_assetProcessorPort{ 45643 };

            //! Timeout(units: seconds) to use when either connecting to an already launched AssetProcessor
            //! or listening for a connection from the AssetProcessor
            //! Defaults to 3 seconds
            //! (Can be queried from Settings Registry - "connect_ap_timeout")
            AZStd::chrono::duration<float> m_connectTimeout{ 3.0f };
            //! Timeout(units: seconds) to use when launching a new instance of the AssetProcessor and attempting
            //! to connect to that instance
            //! Defaults to 15 seconds
            //! (Can be queried from Settings Registry - "launch_ap_timeout")
            AZStd::chrono::duration<float> m_launchTimeout{ 15.0f };
            //! Timeout(units: seconds) that indicates how long to wait for the AssetProcessor to indicate
            //! it is ready after successfully connecting
            //! The AssetProcessor isn't ready until it processes all critical Assets so this timeout should be
            //! adjusted based on myriad of factors
            //! i.e How many critical assets a project contains?
            //! Are the critical assets being processed using debug AssetBuilders
            //! etc...
            //! Defaults to 20 minutes
            //! (Can be queried from Settings Registry - "wait_ap_ready_timeout")
            AZStd::chrono::duration<float> m_waitForReadyTimeout{ 1200.0f };

            // Callback which is invoked to output logging information during the connection attempt
            using LoggingCallback = AZStd::function<void(AZStd::string_view)>;
            LoggingCallback m_loggingCallback;

            //! Attempt to Launch the AssetProcessor if connection fails
            bool m_launchAssetProcessorOnFailedConnection{ true };
            //! Indicates whether to wait until the AssetProcessor sends a response that it is ready
            //! if a connection has been established
            bool m_waitUntilAssetProcessorIsReady{ true };
            //! If set the connection call will attempt to wait indefinitely until the
            //! AssetProcessor sends back a failed negotiation message
            //! (Can be queried from Settings Registry - "wait_for_connect")
            bool m_waitForConnect{};
        };

        //! Convenience function which can be used to read the AssetProcessor connection settings
        //! from the /Amazon/AzCore/Bootstrap section of the SettingsRegistry
        bool ReadConnectionSettingsFromSettingsRegistry(ConnectionSettings& outputConnectionSettings);

        //! Launch the asset processor
        //! \return Whether or not the asset processor launched
        bool LaunchAssetProcessor();

        //! AssetSystemRequestBusTraits
        //! This bus is for making requests to the asset system
        class AssetSystemRequests
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;   // single listener
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;   // single bus
            
            using MutexType = AZStd::recursive_mutex;
            static const bool LocklessDispatch = true; // no reason to block other threads when a thread is waiting for a response from AP.

            virtual ~AssetSystemRequests() = default;

            //! Function which can either start a connection to the AssetProcessor or...
            //! start a connection thread that will listen for the Asset Processor to connect to the current application
            //! If a connection with the AssetProcessor has been established, this method will check whether
            //! the ConnectionSettings::m_waitUntilAssetProcessorIsReady is set
            //! If it is not set, then it returns true
            //! otherwise the SystemEvent queue is pumped while until the AssetProcessor send a ready respond
            //! or the wait timeout is reached
            //!
            //! returns true if successfully connected and connection settings indicate that connection shouldn't wait
            //! for the AssetProcessor to be ready.
            //! Otherwise returns true if successfully connected and the AssetProcessor is ready
            virtual bool EstablishAssetProcessorConnection(const ConnectionSettings& connectionSettings) = 0;

            //! Wait until the asset processor is connected or timeout time is reached
            //! \return Whether or not we are connected
            virtual bool WaitUntilAssetProcessorConnected(AZStd::chrono::duration<float> timeout) = 0;

            //! Wait until the asset processor is ready or timeout time is reached
            //! \return Whether or not the asset processor is ready
            virtual bool WaitUntilAssetProcessorReady(AZStd::chrono::duration<float> timeout) = 0;

            //! Is the asset processor status ready
            //! \return Whether or not the asset processor is reporting ready status
            virtual bool AssetProcessorIsReady() = 0;

            //! Are we connected to the asset processor
            //! \return Whether or not we are connected
            virtual bool ConnectedWithAssetProcessor() = 0;

            //! Did the Negotiation with the asset processor fail
            //! \return Whether or not negotiation failed with asset processor
            virtual bool NegotiationWithAssetProcessorFailed() = 0;

            //! Starts the disconnecting thread
            virtual void StartDisconnectingAssetProcessor() = 0;

            //! Is the asset processor connection in the disconnected state
            //! \return true if in disconnected state false otherwise
            virtual bool DisconnectedWithAssetProcessor() = 0;

            //! Waits at most timeout time for the asset processor connection to go into the disconnected state
            //! \return true if in disconnected state false otherwise
            virtual bool WaitUntilAssetProcessorDisconnected(AZStd::chrono::duration<float> timeout) = 0;
            
            /** CompileAssetSync
             * Compile an asset synchronously.  This will only return after compilation, and also escalates it so that it builds immediately.
             * Note that the asset path will be heuristically matched like a search term, so things missing an extension or things that
             * are just a folder name will cause all assets which match that search term to be escalated and compiled.
             * PERFORMANCE WARNING: Only use the FlushIO version if you have just written an asset file you wish to immediately compile,
             *     potentially before the operating system's disk IO queue is finished writing it.
             *     It will force a flush of the OS file monitoring queue before considering the request.
            **/
            virtual AssetStatus CompileAssetSync(const AZStd::string& assetPath) = 0;
            virtual AssetStatus CompileAssetSync_FlushIO(const AZStd::string& assetPath) = 0;

            virtual AssetStatus CompileAssetSyncById(const AZ::Data::AssetId& assetId) = 0;
            virtual AssetStatus CompileAssetSyncById_FlushIO(const AZ::Data::AssetId& assetId) = 0;
            
            /** GetAssetStatusByUuid
            * Retrieve the status of an asset synchronously and  also escalate it so that it builds sooner than others that are not
            * escalated.  If possible, prefer this function over the string-based version below.
            * PERFORMANCE WARNING: Only use the FlushIO version if you have just written an asset file you wish to immediately compile,
            *     potentially before the operating system's disk IO queue is finished writing it.
            *     It will force a flush of the OS file monitoring queue before considering the request.
            **/
            virtual AssetStatus GetAssetStatusById(const AZ::Data::AssetId& assetId) = 0;
            virtual AssetStatus GetAssetStatusById_FlushIO(const AZ::Data::AssetId& assetId) = 0;

            /** GetAssetStatus
             * Retrieve the status of an asset synchronously and  also escalate it so that it builds sooner than others that are not escalated.
             * @param assetPath - a relpath to a product in the cache, or a relpath to a source file, or a full path to either
             * PERFORMANCE WARNING: Only use the FlushIO version if you have just written an asset file you wish to immediately query the status of,
             *     potentially before the operating system's disk IO queue is finished writing it.
             *     It will force a flush of the OS file monitoring queue before considering the request.
            **/
            virtual AssetStatus GetAssetStatus(const AZStd::string& assetPath) = 0;
            virtual AssetStatus GetAssetStatus_FlushIO(const AZStd::string& assetPath) = 0;
            
            /** GetAssetStatusSearchType
             * Retrieve the status of an asset synchronously and also escalate it so that it builds sooner than others that are not escalated.
             * @param searchTerm - provides a string parameter used for searching. The use of this parameter depends on searchType.
             * @param searchType - indicates which type of search to perform, and how to use searchTerm
             *                     RequestAssetStatus::SearchType::Default: Same as GetAssetStatus(). searchTerm is a relpath to a product in the cache,
             *                                                              or a relpath to a source file, or a full path to either.
             *                     RequestAssetStatus::SearchType::Exact:   searchTerm is an exact path to a source data file.
             *                     (see RequestAssetStatus::SearchType for more)
             * PERFORMANCE WARNING: Only use the FlushIO version if you have just written an asset file you wish to immediately query the status of,
             *     potentially before the operating system's disk IO queue is finished writing it.
             *     It will force a flush of the OS file monitoring queue before considering the request.
            **/
            virtual AssetStatus GetAssetStatusSearchType(const AZStd::string& searchTerm, int searchType) = 0;
            virtual AssetStatus GetAssetStatusSearchType_FlushIO(const AZStd::string& searchTerm, int searchType) = 0;

            /** Request that a particular asset be escalated to the top of the build queue, by uuid
             *  This is an async request - the return value only indicates whether it was sent, not whether it escalated or was found.
             *  Note that the Uuid of an asset is the Uuid of its source file.  If you have an AssetId field, this is the m_uuid part
             *  inside the AssetId, since that refers to the source file that produced the asset.
             *  @param assetUuid - the uuid to look up.
             * note that this request always flushes IO (on the AssetProcessor side), but you don't pay for it in the caller
             * process since it is a fire-and-forget message.  This means its the fastest possible way to reliably escalate an asset by UUID
            **/ 
            virtual bool EscalateAssetByUuid(const AZ::Uuid& assetUuid) = 0;
            /** EscalateAssetBySearchTerm
              * Request that a particular asset be escalated to the top of the build queue, by "search term" (ie, source file name)
              * This is an async request - the return value only indicates whether it was sent, not whether it escalated or was found.
              * Search terms can be:
              *     Source File Names
              *     fragments of source file names
              *     Folder Names or pieces of folder names
              *     Product file names
              *     fragments of product file names
              * The asset processor will find the closest match and escalate it.  So for example if you request escalation on
              * "mything.dds" and no such SOURCE FILE exists, it may match mything.fbx heuristically after giving up on the dds.
              * If possible, use the above EscalateAsset with the Uuid, which does not require a heuristic match, or use
              * the source file name (relative or absolute) as the input, instead of trying to work with product names.
              *  @param searchTerm - see above
              *
              * note that this request always flushes IO (on the AssetProcessor side), but you don't pay for it in the caller
              * process since it is a fire-and-forget message.  This means its the fastest possible way to reliably escalate an asset by name
             **/
            virtual bool EscalateAssetBySearchTerm(AZStd::string_view searchTerm) = 0;

            //! Show the AssetProcessor App
            virtual void ShowAssetProcessor() = 0;
            virtual void UpdateSourceControlStatus(bool newStatus) = 0;
            //! Show an asset in the AssetProcessor App
            virtual void ShowInAssetProcessor(const AZStd::string& assetPath) = 0;

            /** Returns the number of unresolved AssetId and path references for the given asset.
             * These are product assets that the given asset refers to which are not yet known by the Asset Processor.
             * This API can be used to determine if a given asset can safely be loaded and have its asset references resolve successfully.
             * @param assetId - Asset to lookup
             * @param unresolvedAssetIdReferences - number of AssetId-based references which are unresolved
             * @param unresolvedPathReferences - number of path-based references which are unresolved.  This count excludes wildcard references which are never resolved
            **/
            virtual void GetUnresolvedProductReferences(AZ::Data::AssetId assetId, AZ::u32& unresolvedAssetIdReferences, AZ::u32& unresolvedPathReferences) = 0;

            //! Compute the ping time between this client and the Asset Processor that's actually handling our requests (proxy relaying is included in the time)
            virtual float GetAssetProcessorPingTimeMilliseconds() = 0;
            //! Saves the catalog synchronously
            virtual bool SaveCatalog() = 0;
        };

        //! AssetSystemNegotiationBusTraits
        //! This bus is for events that occur during negotiation
        class AssetSystemConnectionNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // multi listener
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;   // single bus

            virtual ~AssetSystemConnectionNotifications() = default;

            //! Notifies listeners negotiation with Asset Processor failed
            virtual void NegotiationFailed() {};

            //! Notifies listeners that connection to the Asset Processor failed
            virtual void ConnectionFailed() {};
        };

        namespace ConnectionIdentifiers
        {
            static constexpr const char* Editor = "EDITOR";
            static constexpr const char* Game = "GAME";
        }

        //! AssetSystemStatusBusTraits
        //! This bus is for AssetSystem status change
        class AssetSystemStatus
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // multi listener
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;   // single bus

            virtual ~AssetSystemStatus() = default;

            //! Notifies listeners Asset System turns available
            virtual void AssetSystemAvailable() {}
            //! Notifies listeners Asset System turns not available
            virtual void AssetSystemUnavailable() {}
            //! Notifies listeners Asset System is still waiting.
            //! This will get called every N milliseconds during the Asset System startup
            //! until the Asset System becomes available.
            virtual void AssetSystemWaiting() {}
        };

    } // namespace AssetSystem

    /**
    * AssetSystemBus removed - if you have a system which was using it
    * use AssetCatalogEventBus for asset updates
    */
    using AssetSystemInfoBus = AZ::EBus<AssetSystem::AssetSystemInfoNotifications>;
    using AssetSystemRequestBus = AZ::EBus<AssetSystem::AssetSystemRequests>;
    using AssetSystemConnectionNotificationsBus = AZ::EBus<AssetSystem::AssetSystemConnectionNotifications>;
    using AssetSystemStatusBus = AZ::EBus<AssetSystem::AssetSystemStatus>;
} // namespace AzFramework

DECLARE_EBUS_EXTERN(AzFramework::AssetSystem::AssetSystemRequests);
