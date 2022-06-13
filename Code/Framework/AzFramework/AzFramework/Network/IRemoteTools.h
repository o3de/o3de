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
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    template<typename T>
    class Interface;
}

namespace AzFramework
{
    class RemoteToolsMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(RemoteToolsMessage, AZ::OSAllocator, 0);
        AZ_RTTI(RemoteToolsMessage, "{8512328C-949D-4F0C-B48D-77C26C207443}");

        RemoteToolsMessage()
            : m_msgId(0)
            , m_senderTargetId(0)
            , m_customBlob(nullptr)
            , m_customBlobSize(0)
            , m_refCount(0)
            , m_isBlobOwner(false)
            , m_immediateSelfDispatch(false)
        {
        }
        RemoteToolsMessage(AZ::u64 msgId)
            : m_msgId(msgId)
            , m_senderTargetId(0)
            , m_customBlob(nullptr)
            , m_customBlobSize(0)
            , m_refCount(0)
            , m_isBlobOwner(false)
            , m_immediateSelfDispatch(false)
        {
        }

        virtual ~RemoteToolsMessage();

        void AddCustomBlob(const void* blob, size_t blobSize, bool ownBlob = false);

        void SetImmediateSelfDispatchEnabled(bool immediateSelfDispatchEnabled);

        bool IsImmediateSelfDispatchEnabled() const;

        const void* GetCustomBlob() const;

        size_t GetCustomBlobSize() const;

        AZ::u64 GetId() const;

        AZ::u32 GetSenderTargetId() const;

        void SetSenderTargetId(AZ::u32 senderTargetId);

    protected:
        AZ::u64 m_msgId;
        AZ::u32 m_senderTargetId;
        const void* m_customBlob;
        AZ::u32 m_customBlobSize;
        bool m_isBlobOwner;
        bool m_immediateSelfDispatch;

        //---------------------------------------------------------------------
        // refcount
        //---------------------------------------------------------------------
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;
        AZ_FORCE_INLINE void add_ref()
        {
            ++m_refCount;
        }
        AZ_FORCE_INLINE void release()
        {
            if (--m_refCount == 0)
            {
                delete this;
            }
        }

        size_t m_refCount;
        //---------------------------------------------------------------------
    };

    typedef AZStd::intrusive_ptr<RemoteToolsMessage> RemoteToolsMessagePointer;
    typedef AZStd::deque<RemoteToolsMessagePointer, AZ::OSStdAllocator> RemoteToolsMessageQueue;
    
    // id for the local application
    static const AZ::u32 s_selfNetworkId = 0xFFFFFFFF;

    class RemoteToolsEndpointInfo final
    {

    public:
        AZ_CLASS_ALLOCATOR(RemoteToolsEndpointInfo, AZ::OSAllocator, 0);
        AZ_TYPE_INFO(RemoteToolsEndpointInfo, "{DD0E9B2A-3B25-43B1-951E-CACCEC5D6754}");

        RemoteToolsEndpointInfo(const AZStd::string& displayName = "", AZ::u32 networkId = 0)
            : m_displayName(displayName)
            , m_persistentId(0)
            , m_networkId(networkId)
        {
        }

        bool IsSelf() const;

        bool IsOnline() const;

        bool IsValid() const;

        const char* GetDisplayName() const;

        AZ::u32 GetPersistentId() const;

        AZ::u32 GetNetworkId() const;

        bool IsIdentityEqualTo(const RemoteToolsEndpointInfo& other) const;

        void SetInfo(AZStd::string displayName, AZ::u32 persistentId, AZ::u32 networkId);

    private:
        AZStd::string m_displayName;
        AZ::u32 m_persistentId; // this string is set by the target and its CRC is currently used to determine desired targets.
        AZ::u32 m_networkId; // this is the actual connection id, used for AzNetworking communications.
        bool m_isOnline;
    };

    typedef AZStd::unordered_map<AZ::u32, RemoteToolsEndpointInfo> RemoteToolsEndpointContainer;
    using RemoteToolsEndpointStatusEvent = AZ::Event<RemoteToolsEndpointInfo>;
    using RemoteToolsEndpointConnectedEvent = AZ::Event<bool>;
    using RemoteToolsEndpointChangedEvent = AZ::Event<AZ::u32, AZ::u32>;
    using ReceivedRemoteToolsMessages = AZStd::fixed_vector<RemoteToolsMessagePointer, 64>;

    class IRemoteTools
    {
    public:
        AZ_RTTI(IRemoteTools, "{1446BADE-E6F7-4E3C-8D37-669A544DB964}");

        virtual void RegisterToolingService(AZ::Crc32 key, AZ::Name name, uint16_t port) = 0;

        virtual const ReceivedRemoteToolsMessages* GetReceivedMessages(AZ::Crc32 key) const = 0;

        virtual void ClearReceivedMessages(AZ::Crc32 key) = 0;

        virtual void RegisterRemoteToolsEndpointJoinedHandler(AZ::Crc32 key, RemoteToolsEndpointStatusEvent::Handler handler) = 0;

        virtual void RegisterRemoteToolsEndpointLeftHandler(AZ::Crc32 key, RemoteToolsEndpointStatusEvent::Handler handler) = 0;

        virtual void RegisterRemoteToolsEndpointConnectedHandler(AZ::Crc32 key, RemoteToolsEndpointConnectedEvent::Handler handler) = 0;

        virtual void RegisterRemoteToolsEndpointChangedHandler(AZ::Crc32 key, RemoteToolsEndpointChangedEvent::Handler handler) = 0;

        // call this function to retrieve the list of currently known targets - this is mainly used for GUIs
        // when they come online and attempt to enum (they won't have been listening for target coming and going)
        // you will only be shown targets that have been seen in a reasonable amount of time.
        virtual void EnumTargetInfos(AZ::Crc32 key, RemoteToolsEndpointContainer& infos) = 0;

        // set the desired target, which we'll specifically keep track of.
        // the target controls who gets lua commands, tweak stuff, that kind of thing
        virtual void SetDesiredEndpoint(AZ::Crc32 key, AZ::u32 desiredTargetID) = 0;

        virtual void SetDesiredEndpointInfo(AZ::Crc32 key, const RemoteToolsEndpointInfo& targetInfo) = 0;

        // retrieve what it was set to.
        virtual RemoteToolsEndpointInfo GetDesiredEndpoint(AZ::Crc32 key) = 0;

        // given id, get info.
        virtual RemoteToolsEndpointInfo GetEndpointInfo(AZ::Crc32 key, AZ::u32 desiredTargetID) = 0;

        // check if target is online
        virtual bool IsEndpointOnline(AZ::Crc32 key, AZ::u32 desiredTargetID) = 0;

        // send a message to a remote target
        virtual void SendRemoteToolsMessage(const RemoteToolsEndpointInfo& target, const RemoteToolsMessage& msg) = 0;
    };

    using RemoteToolsInterface = AZ::Interface<IRemoteTools>;
} // namespace AzFramework

#include <AzFramework/Network/IRemoteTools.inl>
