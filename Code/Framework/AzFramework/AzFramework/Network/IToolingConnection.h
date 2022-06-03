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
    class ToolingMessage
    {
    public:
        AZ_CLASS_ALLOCATOR(ToolingMessage, AZ::OSAllocator, 0);
        AZ_RTTI(ToolingMessage, "{8512328C-949D-4F0C-B48D-77C26C207443}");

        ToolingMessage()
            : m_msgId(0)
            , m_senderTargetId(0)
            , m_customBlob(nullptr)
            , m_customBlobSize(0)
            , m_refCount(0)
            , m_isBlobOwner(false)
            , m_immediateSelfDispatch(false)
        {
        }
        ToolingMessage(AZ::u64 msgId)
            : m_msgId(msgId)
            , m_senderTargetId(0)
            , m_customBlob(nullptr)
            , m_customBlobSize(0)
            , m_refCount(0)
            , m_isBlobOwner(false)
            , m_immediateSelfDispatch(false)
        {
        }

        virtual ~ToolingMessage();

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

    typedef AZStd::intrusive_ptr<ToolingMessage> ToolingMessagePointer;
    typedef AZStd::deque<ToolingMessagePointer, AZ::OSStdAllocator> ToolingMessageQueue;
    
    // id for the local application
    static const AZ::u32 s_selfNetworkId = 0xFFFFFFFF;

    class ToolingEndpointInfo final
    {

    public:
        AZ_CLASS_ALLOCATOR(ToolingEndpointInfo, AZ::OSAllocator, 0);
        AZ_TYPE_INFO(ToolingEndpointInfo, "{DD0E9B2A-3B25-43B1-951E-CACCEC5D6754}");

        ToolingEndpointInfo(const AZStd::string& displayName = "", AZ::u32 networkId = 0)
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

        bool IsIdentityEqualTo(const ToolingEndpointInfo& other) const;

        void SetInfo(AZStd::string displayName, AZ::u32 persistentId, AZ::u32 networkId);

    private:
        AZStd::string m_displayName;
        AZ::u32 m_persistentId; // this string is set by the target and its CRC is currently used to determine desired targets.
        AZ::u32 m_networkId; // this is the actual connection id, used for AzNetworking communications.
        bool m_isOnline;
    };

    typedef AZStd::unordered_map<AZ::u32, ToolingEndpointInfo> ToolingEndpointContainer;
    using ToolingEndpointStatusEvent = AZ::Event<ToolingEndpointInfo>;
    using ToolingEndpointConnectedEvent = AZ::Event<bool>;
    using ToolingEndpointChangedEvent = AZ::Event<AZ::u32, AZ::u32>;
    using ReceivedToolingMessages = AZStd::fixed_vector<ToolingMessagePointer, 64>;
    using ToolingServiceKey = AZ::u32;

    class IToolingConnection
    {
    public:
        AZ_RTTI(IToolingConnection, "{1446BADE-E6F7-4E3C-8D37-669A544DB964}");

        virtual ToolingServiceKey RegisterToolingService(AZStd::string name, uint16_t port) = 0;

        virtual const ReceivedToolingMessages* GetReceivedMessages(ToolingServiceKey key) const = 0;

        virtual void ClearReceivedMessages(ToolingServiceKey key) = 0;

        virtual void RegisterToolingEndpointJoinedHandler(ToolingServiceKey key, ToolingEndpointStatusEvent::Handler handler) = 0;

        virtual void RegisterToolingEndpointLeftHandler(ToolingServiceKey key, ToolingEndpointStatusEvent::Handler handler) = 0;

        virtual void RegisterToolingEndpointConnectedHandler(ToolingServiceKey key, ToolingEndpointConnectedEvent::Handler handler) = 0;

        virtual void RegisterToolingEndpointChangedHandler(ToolingServiceKey key, ToolingEndpointChangedEvent::Handler handler) = 0;

        // call this function to retrieve the list of currently known targets - this is mainly used for GUIs
        // when they come online and attempt to enum (they won't have been listening for target coming and going)
        // you will only be shown targets that have been seen in a reasonable amount of time.
        virtual void EnumTargetInfos(ToolingServiceKey key, ToolingEndpointContainer& infos) = 0;

        // set the desired target, which we'll specifically keep track of.
        // the target controls who gets lua commands, tweak stuff, that kind of thing
        virtual void SetDesiredEndpoint(ToolingServiceKey key, AZ::u32 desiredTargetID) = 0;

        virtual void SetDesiredEndpointInfo(ToolingServiceKey key, const ToolingEndpointInfo& targetInfo) = 0;

        // retrieve what it was set to.
        virtual ToolingEndpointInfo GetDesiredEndpoint(ToolingServiceKey key) = 0;

        // given id, get info.
        virtual ToolingEndpointInfo GetEndpointInfo(ToolingServiceKey key, AZ::u32 desiredTargetID) = 0;

        // check if target is online
        virtual bool IsEndpointOnline(ToolingServiceKey key, AZ::u32 desiredTargetID) = 0;

        // send a message to a remote target
        virtual void SendToolingMessage(const ToolingEndpointInfo& target, const ToolingMessage& msg) = 0;
    };

    using ToolsConnectionInterface = AZ::Interface<IToolingConnection>;
} // namespace AzFramework

#include <AzFramework/Network/IToolingConnection.inl>
