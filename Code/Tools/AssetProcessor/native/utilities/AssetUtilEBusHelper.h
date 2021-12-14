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
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <native/assetprocessor.h>
#include <QByteArray>

namespace AssetProcessor
{
    struct BuilderParams;
}

namespace AssetUtilities
{
    class QuitListener;
    class JobLogTraceListener;
}


//This EBUS broadcasts the platform of the connection the AssetProcessor connected or disconnected with
class AssetProcessorPlaformBusTraits
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // multi listener
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus

    virtual ~AssetProcessorPlaformBusTraits() {}
    //Informs that the AP got a connection for this platform.
    virtual void AssetProcessorPlatformConnected(const AZStd::string platform) {}
    //Informs that a connection got disconnected for this platform.
    virtual void AssetProcessorPlatformDisconnected(const AZStd::string platform) {}
};

using AssetProcessorPlatformBus = AZ::EBus<AssetProcessorPlaformBusTraits>;

class ApplicationServerBusTraits
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    typedef AZStd::recursive_mutex MutexType;

    virtual ~ApplicationServerBusTraits() {};

    //! Returns the port the server is set to listen on
    virtual int GetServerListeningPort() const = 0;
};

using ApplicationServerBus = AZ::EBus<ApplicationServerBusTraits>;

namespace AzFramework
{
    namespace AssetSystem
    {
        class BaseAssetProcessorMessage;
    }
}

namespace AssetProcessor
{
    // This bus sends messages to connected clients/proxies identified by their connection ID. The bus
    // is addressed by the connection ID as assigned by the ConnectionManager.
    class ConnectionBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        typedef unsigned int BusIdType;
        typedef AZStd::recursive_mutex MutexType;

        virtual ~ConnectionBusTraits() {}

        // Sends an unsolicited message to the connection
        virtual size_t Send(unsigned int serial, const AzFramework::AssetSystem::BaseAssetProcessorMessage& message) = 0;
        // Sends a raw buffer to the connection
        virtual size_t SendRaw(unsigned int type, unsigned int serial, const QByteArray& data) = 0;

        // Sends a message to the connection if the platform match
        virtual size_t SendPerPlatform(unsigned int serial, const AzFramework::AssetSystem::BaseAssetProcessorMessage& message, const QString& platform) = 0;
        // Sends a raw buffer to the connection if the platform match
        virtual size_t SendRawPerPlatform(unsigned int type, unsigned int serial, const QByteArray& data, const QString& platform) = 0;

        using ResponseCallback = AZStd::function<void(AZ::u32, QByteArray)>;

        // Sends a message to the connection which expects a response.
        virtual unsigned int SendRequest(const AzFramework::AssetSystem::BaseAssetProcessorMessage& message, const ResponseCallback& callback) = 0;

        // Sends a response to the connection
        virtual size_t SendResponse(unsigned int serial, const AzFramework::AssetSystem::BaseAssetProcessorMessage& message) = 0;

        // Removes a response handler that is no longer needed
        virtual void RemoveResponseHandler(unsigned int serial) = 0;
    };

    using ConnectionBus = AZ::EBus<ConnectionBusTraits>;

    class MessageInfoBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // multi listener
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus
        typedef AZStd::recursive_mutex MutexType;

        virtual ~MessageInfoBusTraits() {}
        //Show a message window to the user
        virtual void NegotiationFailed() {}
        // Notifies listeners of a given Asset failing to process
        virtual void OnAssetFailed(const AZStd::string& /*sourceFileName*/) {}
        // Notifies listener about a general error
        virtual void OnErrorMessage([[maybe_unused]] const char* error) {}
    };

    using MessageInfoBus = AZ::EBus<MessageInfoBusTraits>;


    typedef AZStd::vector <AssetBuilderSDK::AssetBuilderDesc> BuilderInfoList;

    // This EBUS is used to retrieve asset builder Information
    class AssetBuilderInfoBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; // single listener
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus
        using MutexType = AZStd::recursive_mutex;

        virtual ~AssetBuilderInfoBusTraits() {}

        // For a given asset returns a list of all asset builder that are interested in it.

        virtual void GetMatchingBuildersInfo(const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& /*builderInfoList*/) = 0;
        virtual void GetAllBuildersInfo(AssetProcessor::BuilderInfoList& /*builderInfoList*/) = 0;
    };

    using  AssetBuilderInfoBus = AZ::EBus<AssetBuilderInfoBusTraits>;

    // This EBUS is used to broadcast information about the currently processing job
    class ProcessingJobInfoBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; // single listener
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus
        typedef AZStd::recursive_mutex MutexType;

        virtual ~ProcessingJobInfoBusTraits() {}

        // Will notify other systems a product is about to be updated in the cache. This can mean that
        // it will be created, overwritten with new data or deleted. BeginCacheFileUpdate is pared with
        // EndCacheFileUpdate.
        virtual void BeginCacheFileUpdate(const char* /*productPath*/) {};
        // Will notify other systems that a file in the cache has been updated along with status of whether it
        // succeeded or failed. EndCacheFileUpdate is paired with BeginCacheFileUpdate.
        virtual void EndCacheFileUpdate(const char* /*productPath*/, bool /*queueAgainForDeletion*/) {};
        virtual AZ::u32 GetJobFingerprint(const AssetProcessor::JobIndentifier& /*jobIndentifier*/) { return 0; };
    };

    using ProcessingJobInfoBus = AZ::EBus<ProcessingJobInfoBusTraits>;

    // This EBUS is used to issue requests to the AssetCatalog
    class AssetRegistryRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; // single listener
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus
        typedef AZStd::recursive_mutex MutexType;

        // This function will either return the registry version of the next registry save or of the current one, if it is in progress
        // It will not put another save registry event in the event pump if we are currently in the process of saving the registry
        virtual int SaveRegistry() = 0;

        // This method checks for cyclic preload dependency for all the currently processed assets.
        virtual void ValidatePreLoadDependency() = 0;
    };

    typedef AZ::EBus<AssetRegistryRequests> AssetRegistryRequestBus;

    // This EBUS issues notifications when the catalog begins and finishes saving the asset registry
    class AssetRegistryNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // single listener
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus
        typedef AZStd::recursive_mutex MutexType;

        // The asset catalog has finished saving the registry
        virtual void OnRegistrySaveComplete(int /*assetCatalogVersion*/, bool /*allCatalogsSaved*/) {}
    };

    using AssetRegistryNotificationBus = AZ::EBus<AssetRegistryNotifications>;

    // This EBUS is used to check if there is sufficient disk space
    class DiskSpaceInfoBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        typedef AZStd::recursive_mutex MutexType;

        // Returns true if there is at least `requiredSpace` bytes plus 256kb free disk space at the specified path
        // savePath must be a folder path, not a file path
        // If shutdownIfInsufficient is true, an error will be displayed and the application will be shutdown
        virtual bool CheckSufficientDiskSpace(const QString& /*savePath*/, qint64 /*requiredSpace*/, bool /*shutdownIfInsufficient*/) { return true; }
    };

    using DiskSpaceInfoBus = AZ::EBus<DiskSpaceInfoBusTraits>;

    // This EBUS is used to perform Asset Server related tasks.
    class AssetServerBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        typedef AZStd::recursive_mutex MutexType;
        static const bool LocklessDispatch = true;
        //! This will return true if we were able to verify the server address as being valid, otherwise return false.
        virtual bool IsServerAddressValid() = 0;
        //! StoreJobResult should store all the files in the temp folder provided by the builderParams to the server
        //! As well as any outputProducts which are outside the temp folder intended to be copied directly to the
        //! Cache without going through the temp folder
        //! It should associate those files with the server key provided by the builderParams because
        //! it will be send the same server key to retrieve these files by the client.
        //! This will return true if it was able to save all the relevant job data to the server, otherwise return false.
        virtual bool StoreJobResult(const AssetProcessor::BuilderParams& builderParams, AZStd::vector<AZStd::string>& sourceFileList) = 0;
        //! RetrieveJobResult should retrieve all the files associated with the server key provided in the builderParams
        //! and put them in the temporary directory provided by the builderParam.
        //! This will return true if it was able to retrieve all the relevant job data from the server, otherwise return false.
        virtual bool RetrieveJobResult(const AssetProcessor::BuilderParams& builderParams) = 0;
    };

    using AssetServerBus = AZ::EBus<AssetServerBusTraits>;
} // namespace AssetProcessor
