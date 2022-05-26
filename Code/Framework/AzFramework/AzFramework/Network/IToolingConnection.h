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

        virtual ~ToolingMessage()
        {
            if (m_isBlobOwner)
            {
                azfree(const_cast<void*>(m_customBlob), AZ::OSAllocator);
            }
        }

        void AddCustomBlob(const void* blob, size_t blobSize, bool ownBlob = false)
        {
            m_customBlob = blob;
            m_customBlobSize = static_cast<AZ::u32>(blobSize);
            m_isBlobOwner = ownBlob;
        }

        void SetImmediateSelfDispatchEnabled(bool immediateSelfDispatchEnabled)
        {
            m_immediateSelfDispatch = immediateSelfDispatchEnabled;
        }

        bool IsImmediateSelfDispatchEnabled() const
        {
            return m_immediateSelfDispatch;
        }

        const void* GetCustomBlob() const
        {
            return m_customBlob;
        }
        size_t GetCustomBlobSize() const
        {
            return m_customBlobSize;
        }

        AZ::u64 GetId() const
        {
            return m_msgId;
        }

        AZ::u32 GetSenderTargetId() const
        {
            return m_senderTargetId;
        }

        void SetSenderTargetId(AZ::u32 senderTargetId)
        {
            m_senderTargetId = senderTargetId;
        }

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
    using ToolingMessageEvent = AZ::Event<ToolingMessagePointer>;

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

        bool IsSelf() const
        {
            return m_networkId == s_selfNetworkId;
        }

        bool IsValid() const
        {
            return true;
        }

        const char* GetDisplayName() const
        {
            return m_displayName.c_str();
        }

        AZ::u32 GetPersistentId() const
        {
            return m_persistentId;
        }

        AZ::u32 GetNetworkId() const
        {
            return m_networkId;
        }

        bool IsIdentityEqualTo(const ToolingEndpointInfo& other) const
        {
            return m_persistentId == other.m_persistentId&& m_networkId == other.m_networkId;
        }

        void SetInfo(AZStd::string displayName, AZ::u32 persistentId, AZ::u32 networkId)
        {
            m_displayName = displayName;
            m_persistentId = persistentId;
            m_networkId = networkId;
        }

    private:
        AZStd::string m_displayName;
        AZ::u32 m_persistentId; // this string is set by the target and its CRC is currently used to determine desired targets.
        AZ::u32 m_networkId; // this is the actual connection id, used for AzNetworking communications.
    };

    typedef AZStd::unordered_map<AZ::u32, ToolingEndpointInfo> ToolingEndpointContinaer;

    class IToolingConnection
    {
    public:
        AZ_RTTI(IToolingConnection, "{1446BADE-E6F7-4E3C-8D37-669A544DB964}");

        //! Registers a ToolingMessageEvent handler for a client attempting to connect to a server at the designated hostname and port
        //! @param hostname The hostname the client wants to connect to
        //! @param port The port the client wants to connect on
        //! @param handler The ToolingMessageEvent handler to be invoked when the client receives a message from the host
        virtual void RegisterToolingMessageClientHandler(const char* hostname, uint16_t port, ToolingMessageEvent::Handler handler) = 0;

        //! Registers a ToolingMessageEvent handler for a host listening for clients on the designated port
        //! @param port The port the host wants to listen on
        //! @param handler The ToolingMessageEvent handler to be invoked when the host receives a message from a client
        virtual void RegisterToolingMessageHostHandler(uint16_t port, ToolingMessageEvent::Handler handler) = 0;

        // call this function to retrieve the list of currently known targets - this is mainly used for GUIs
        // when they come online and attempt to enum (they won't have been listening for target coming and going)
        // you will only be shown targets that have been seen in a reasonable amount of time.
        virtual void EnumTargetInfos(ToolingEndpointContinaer& infos) = 0;

        // set the desired target, which we'll specifically keep track of.
        // the target controls who gets lua commands, tweak stuff, that kind of thing
        virtual void SetDesiredEndpoint(AZ::u32 desiredTargetID) = 0;

        virtual void SetDesiredEndpointInfo(const ToolingEndpointInfo& targetInfo) = 0;

        // retrieve what it was set to.
        virtual ToolingEndpointInfo GetDesiredEndpoint() = 0;

        // given id, get info.
        virtual ToolingEndpointInfo GetEndpointInfo(AZ::u32 desiredTargetID) = 0;

        // check if target is online
        virtual bool IsEndpointOnline(AZ::u32 desiredTargetID) = 0;

        // send a message to a remote target
        virtual void SendToolingMessage(const ToolingEndpointInfo& target, const ToolingMessage& msg) = 0;

        // manually force messages for a specific id to be delivered.
        virtual void DispatchMessages(AZ::u64 id) = 0;
    };
} // namespace AZ
