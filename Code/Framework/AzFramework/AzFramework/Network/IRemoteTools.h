/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/smart_ptr/intrusive_refcount.h>
#include <AzCore/std/string/string.h>

#if !defined(_RELEASE)
    #define ENABLE_REMOTE_TOOLS 1
#endif

namespace AZ
{
    template<typename T>
    class Interface;

    class Name;
}

namespace AzFramework
{
    class RemoteToolsMessage : public AZStd::intrusive_refcount<size_t>
    {
    public:
        AZ_CLASS_ALLOCATOR(RemoteToolsMessage, AZ::OSAllocator);
        AZ_RTTI(RemoteToolsMessage, "{8512328C-949D-4F0C-B48D-77C26C207443}");

        RemoteToolsMessage() = default;
        explicit RemoteToolsMessage(AZ::u64 msgId) : m_msgId(msgId) {}

        virtual ~RemoteToolsMessage();

        //! Add a custom data blob to a Remote Tools message
        //! @param blob Pointer to the start of the data blob
        //! @param blobSize Size of the data blob
        //! @param ownBlob Whether this message owns the life cycle of the blob memory
        void AddCustomBlob(const void* blob, size_t blobSize, bool ownBlob = false);

        //! Add a custom data blob to a Remote Tools message
        //! @param blob Span of byte representing the blob
        //! @param ownBlob Whether this message owns the life cycle of the blob memory
        void AddCustomBlob(AZStd::span<AZStd::byte const> blob, bool ownBlob = false);

        //! Gets the custom data blob associated with this message
        //! @return Memory address of the start of the blob or nullptr if there is none
        const AZStd::span<AZStd::byte const> GetCustomBlob() const;

        //! Gets the size of the custom data blob associated with this message
        //! @return Size of the data blob or 0 if there is none
        size_t GetCustomBlobSize() const;

        //! Gets if this message owns blob memory
        //! @return True if the message owns the blob
        bool GetIsBlobOwner() const;

        //! Gets the numeric ID of this message
        //! @return The ID of this message
        AZ::u64 GetId() const;

        //! Gets this message's sender's ID
        //! @return ID of the message's sender
        AZ::u32 GetSenderTargetId() const;

        //! Sets the ID of this message's sender
        //! @param senderTargetId The ID to set for this message's sender
        void SetSenderTargetId(AZ::u32 senderTargetId);

        //! Reflect RemoteToolsMessage
        //! @param reflection Context to reflect to
        static void Reflect(AZ::ReflectContext* reflection);

    protected:
        AZ::u64 m_msgId = 0;
        AZ::u32 m_senderTargetId = 0;
        AZStd::span<AZStd::byte const> m_customBlob;
        bool m_isBlobOwner = false;
    };

    using RemoteToolsMessagePointer = AZStd::intrusive_ptr<RemoteToolsMessage>;
    using RemoteToolsMessageQueue = AZStd::deque<RemoteToolsMessagePointer, AZ::OSStdAllocator>;
    
    // id for the local application
    static constexpr AZ::u32 SelfNetworkId = 0xFFFFFFFF;
    // const value for an invalid connection based AzNetworking::ConnectionId
    static constexpr AZ::u32 InvalidRemoteToolsConnectionId = 0xFFFFFFFF;

    class RemoteToolsEndpointInfo final
    {

    public:
        AZ_CLASS_ALLOCATOR(RemoteToolsEndpointInfo, AZ::OSAllocator);
        AZ_TYPE_INFO(RemoteToolsEndpointInfo, "{DD0E9B2A-3B25-43B1-951E-CACCEC5D6754}");

        explicit RemoteToolsEndpointInfo(AZStd::string displayName = AZStd::string{}, AZ::u32 networkId = 0)
            : m_displayName(AZStd::move(displayName))
            , m_networkId(networkId)
        {
        }

        //! Gets if this endpoint is ourself
        //! @return true if this endpoint is ourself
        bool IsSelf() const;

        //! Gets if this endpoint is online
        //! @return true if this endpoint is online
        bool IsOnline() const;

        //! Gets if this endpoint is valid
        //! @return true if this endpoint is named, has a network ID and persistent ID
        bool IsValid() const;

        //! Gets the display name of this endpoint
        //! @return A string display name of this endpoint
        const char* GetDisplayName() const;

        //! Gets the Persistent ID of this endpoint
        //! @return A Crc32 representing the Persistent ID of this endpoint
        AZ::u32 GetPersistentId() const;

        //! Gets the AzNetworking Network ID of this endpoint
        //! @return The AzNetworking Network ID for the connection of this endpoint
        AZ::u32 GetNetworkId() const;

        //! Sets information used to determine validity of the endpoint
        //! @param displayName The string display name of this endpoint
        //! @param persistentId The Crc32 based persistent ID of this endpoint
        //! @param networkId The AzNetworking Network ID for this endpoint's connection
        void SetInfo(AZStd::string displayName, AZ::u32 persistentId, AZ::u32 networkId);

        bool IsIdentityEqualTo(const RemoteToolsEndpointInfo& other) const;

        //! Reflect RemoteToolsEndpointInfo
        //! @param reflection Context to reflect to
        static void Reflect(AZ::ReflectContext* reflection);

    private:
        AZStd::string m_displayName;
        AZ::u32 m_persistentId = 0; // this is a CRC key used to identify a RemoteTools target
        AZ::u32 m_networkId = 0; // this is the connection id, used for AzNetworking communications.
    };

    using RemoteToolsEndpointContainer = AZStd::unordered_map<AZ::u32, RemoteToolsEndpointInfo>;
    using RemoteToolsEndpointStatusEvent = AZ::Event<RemoteToolsEndpointInfo>;
    using RemoteToolsEndpointConnectedEvent = AZ::Event<bool>;
    using RemoteToolsEndpointChangedEvent = AZ::Event<AZ::u32, AZ::u32>;
    using ReceivedRemoteToolsMessages = AZStd::fixed_vector<RemoteToolsMessagePointer, 128>;

    class IRemoteTools
    {
    public:
        AZ_RTTI(IRemoteTools, "{1446BADE-E6F7-4E3C-8D37-669A544DB964}");

        //! Registers the application as a client of a Remote Tools service with a pre-defined key, name and target port
        //! @param key A Crc32 key used to identify this service
        //! @param name The string name of this service
        //! @param port The port on which this service connects
        virtual void RegisterToolingServiceClient(AZ::Crc32 key, AZ::Name name, uint16_t port) = 0;

        //! Registers the application as a host of a Remote Tools service with a pre-defined key, name and target port
        //! @param key A Crc32 key used to identify this service
        //! @param name The string name of this service
        //! @param port The port on which this service starts listening on registration
        virtual void RegisterToolingServiceHost(AZ::Crc32 key, AZ::Name name, uint16_t port) = 0;

        //! Gets pending received messages for a given Remote Tools Service
        //! @param key The key of the service to retrieve messages for
        //! @return A vector of received messages pending processing for the given service
        virtual const ReceivedRemoteToolsMessages* GetReceivedMessages(AZ::Crc32 key) const = 0;

        //! Clears pending received messages for a given Remote Tools Service.
        //! Useful for situations in which messages must be processed out of band.
        //! @param key The key of the service to clear messages for
        virtual void ClearReceivedMessages(AZ::Crc32 key) = 0;

        //! Will clear pending received messages at the start of the next tick
        //! Useful if multiple areas of the same service wants to read the messages
        //! @param key The key of the service to clear messages for
        virtual void ClearReceivedMessagesForNextTick(AZ::Crc32 key) = 0;

        virtual void RegisterRemoteToolsEndpointJoinedHandler(AZ::Crc32 key, RemoteToolsEndpointStatusEvent::Handler& handler) = 0;

        virtual void RegisterRemoteToolsEndpointLeftHandler(AZ::Crc32 key, RemoteToolsEndpointStatusEvent::Handler& handler) = 0;

        virtual void RegisterRemoteToolsEndpointConnectedHandler(AZ::Crc32 key, RemoteToolsEndpointConnectedEvent::Handler& handler) = 0;

        virtual void RegisterRemoteToolsEndpointChangedHandler(AZ::Crc32 key, RemoteToolsEndpointChangedEvent::Handler& handler) = 0;

        //! Retrieves a list of currently known endpoints for a given service, useful for GUI
        //! @param key The key fo the service to fetch endpoints for
        //! @param infos Out param of endpoint infos
        virtual void EnumTargetInfos(AZ::Crc32 key, RemoteToolsEndpointContainer& infos) = 0;

        //! Set the desired endpoint for a given service
        //! @param key The key of the service to set desired endpoint on
        //! @param desiredTargetID The ID of the endpoint to set as targeted
        virtual void SetDesiredEndpoint(AZ::Crc32 key, AZ::u32 desiredTargetID) = 0;

        //! Set the desired endpoint info for a given service
        //! @param key The key of the service to set desired endpoint on
        //! @param targetInfo The info to set for the desired endpoint
        virtual void SetDesiredEndpointInfo(AZ::Crc32 key, const RemoteToolsEndpointInfo& targetInfo) = 0;

        //! Get the desired endpoint info for a given service
        //! @param key The key of the service to get the desired endpoint of
        //! @return The info of the desired endpoint
        virtual RemoteToolsEndpointInfo GetDesiredEndpoint(AZ::Crc32 key) = 0;

        //! Get the endpoint info for a given service of a given id
        //! @param key The key of the service to get the desired endpoint of
        //! @param desiredTargetID The ID of the endpoint to fetch info for
        //! @return The info of the desired endpoint
        virtual RemoteToolsEndpointInfo GetEndpointInfo(AZ::Crc32 key, AZ::u32 desiredTargetID) = 0;

        //! Check if target is online
        //! @param key The key of the service to check the endpoint of
        //! @param desiredTargetID The ID of the endpoint to check
        //! @return true if the endpoint is online
        virtual bool IsEndpointOnline(AZ::Crc32 key, AZ::u32 desiredTargetID) = 0;

        //! Send a message to a remote endpoint
        //! @param target The endpoint to send a message to
        //! @param msg The message to send
        virtual void SendRemoteToolsMessage(const RemoteToolsEndpointInfo& target, const RemoteToolsMessage& msg) = 0;
    };

    using RemoteToolsInterface = AZ::Interface<IRemoteTools>;
} // namespace AzFramework

#include <AzFramework/Network/IRemoteTools.inl>
