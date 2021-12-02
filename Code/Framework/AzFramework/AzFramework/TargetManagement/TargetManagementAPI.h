/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZFRAMEWORK_TARGETMANAGEMENTAPI_H
#define AZFRAMEWORK_TARGETMANAGEMENTAPI_H

#include <AzCore/base.h>
#include <AzCore/Debug/Budget.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/OSAllocator.h>

AZ_DECLARE_BUDGET(AzFramework);

namespace AZ
{
    class ReflectContext;
}

namespace AzFramework
{
    typedef AZ::u64 MsgSlotId;                          // meant to be a crc32, but using AZ::u64 so it can be other stuff (pointers)

    // id for the local application
    static const AZ::u32 k_selfNetworkId = static_cast<AZ::u32>(-1);

    enum TargetFlags
    {
        TF_NONE         = 0,
        TF_ONLINE       =   (1 << 0),
        TF_DEBUGGABLE   =   (1 << 1), // can we debug this guy (set breakpoints and step and so on)?
        TF_RUNNABLE     =   (1 << 2), // can we emit script to this guy? (execute remote?)
        TF_SELF         =   (1 << 3), // target represents local application (self)
    };

    class TargetManagementComponent;

    class TargetInfo final
    {
        friend class TargetManagementComponent;
        friend class TargetManagementNetworkImpl;

    public:
        AZ_CLASS_ALLOCATOR(TargetInfo, AZ::OSAllocator, 0);
        AZ_TYPE_INFO(TargetInfo, "{4D166C31-D7F4-4d49-B897-16365B8B0716}");

        TargetInfo(time_t lastSeen = 0, const AZStd::string& displayName = "", AZ::u32 networkId = 0, TargetFlags flags = (TargetFlags)0)
            : m_lastSeen(lastSeen)
            , m_displayName(displayName)
            , m_persistentId(0)
            , m_networkId(networkId)
            , m_flags(flags) {}

        bool IsSelf() const;
        bool  IsValid() const;
        const char* GetDisplayName() const;
        AZ::u32 GetPersistentId() const;
        int GetNetworkId() const;
        AZ::u32 GetStatusFlags() const;
        bool IsIdentityEqualTo(const TargetInfo& other) const;

    private:
        time_t          m_lastSeen;     // how long ago was this process seen?  Used internally for filtering and showing the user the GUI even when the target is temporarily disconnected.
        AZStd::string   m_displayName;
        AZ::u32         m_persistentId; // this string is set by the target and its CRC is currently used to determine desired targets.
        AZ::u32         m_networkId;    // this is the actual connection id, used for gridmate communications, and is NOT persistent.
        AZ::u32         m_flags;        // status flags
    };

    typedef AZStd::unordered_map<AZ::u32, TargetInfo> TargetContainer;

    class TmMsg
    {
        friend class TargetManagementComponent;

    public:
        AZ_CLASS_ALLOCATOR(TmMsg, AZ::OSAllocator, 0);
        AZ_RTTI(TmMsg, "{E16CA6C5-5C78-4AD0-8E9B-A8C1FB4D1DA8}");

        TmMsg()
            : m_msgId(0)
            , m_senderTargetId(0)
            , m_customBlob(NULL)
            , m_customBlobSize(0)
            , m_refCount(0)
            , m_isBlobOwner(false)
            , m_immediateSelfDispatch(false) {}
        TmMsg(MsgSlotId msgId)
            : m_msgId(msgId)
            , m_senderTargetId(0)
            , m_customBlob(NULL)
            , m_customBlobSize(0)
            , m_refCount(0)
            , m_isBlobOwner(false)
            , m_immediateSelfDispatch(false) {}

        virtual ~TmMsg()
        {
            if (m_isBlobOwner)
            {
                azfree(const_cast<void*>(m_customBlob), AZ::OSAllocator);
            }
        }

        void AddCustomBlob(const void* blob, size_t blobSize, bool  ownBlob = false)
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

        const void* GetCustomBlob() const       { return m_customBlob; }
        size_t      GetCustomBlobSize() const   { return m_customBlobSize; }

        MsgSlotId   GetId() const               { return m_msgId; }
        AZ::u32     GetSenderTargetId() const   { return m_senderTargetId; }

    protected:
        MsgSlotId   m_msgId;
        AZ::u32     m_senderTargetId;
        const void* m_customBlob;
        AZ::u32     m_customBlobSize;
        bool        m_isBlobOwner;
        bool        m_immediateSelfDispatch;

        //---------------------------------------------------------------------
        // refcount
        //---------------------------------------------------------------------
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;
        AZ_FORCE_INLINE void    add_ref()           { ++m_refCount; }
        AZ_FORCE_INLINE void    release()
        {
            if (--m_refCount == 0)
            {
                delete this;
            }
        }

        size_t  m_refCount;
        //---------------------------------------------------------------------
    };

    typedef AZStd::intrusive_ptr<TmMsg> TmMsgPtr;
    typedef AZStd::deque<TmMsgPtr, AZ::OSStdAllocator> TmMsgQueue;

    // Systems that intend to receive messages should connect to this bus based on the message Id.
    class TmMsgEvents
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        typedef AZ::OSStdAllocator      AllocatorType;
        typedef MsgSlotId               BusIdType;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        //////////////////////////////////////////////////////////////////////////

        virtual ~TmMsgEvents() {}

        virtual void    OnReceivedMsg(TmMsgPtr msg) = 0;
    };
    typedef AZ::EBus<TmMsgEvents> TmMsgBus;

    class TmMsgCallback
        : public TmMsgBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(TmMsgCallback, AZ::OSAllocator, 0);

        typedef AZStd::function<void (TmMsgPtr /*msg*/)> MsgCB;

        TmMsgCallback(const MsgCB& cb = NULL)
            : m_cb(cb) {}

        void OnReceivedMsg(TmMsgPtr msg) override
        {
            if (m_cb)
            {
                m_cb(msg);
            }
        }

    protected:
        MsgCB   m_cb;
    };

    // this is the bus you should implement if you're interested in receiving broadcasts from teh target manager
    // about network and target status.
    class TargetManagerClient
        : public AZ::EBusTraits
    {
    public:
        typedef AZ::EBus<TargetManagerClient>   Bus;
        typedef AZStd::recursive_mutex          MutexType;

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;  // broadcast to all
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // many handlers
        static const bool EnableEventQueue = true;

        virtual void DesiredTargetConnected(bool connected) { (void)connected; }
        virtual void TargetJoinedNetwork(TargetInfo info) { (void)info; }
        virtual void TargetLeftNetwork(TargetInfo info) { (void)info; }
        virtual void DesiredTargetChanged(AZ::u32 newTargetID, AZ::u32 oldTargetID) { (void)newTargetID; (void)oldTargetID; }

        virtual ~TargetManagerClient() {}
    };

    // this is the bus the targetmanager itself implements - send messages to this bus to talk to it!
    class TargetManager
        : public AZ::EBusTraits
    {
    public:
        typedef AZ::EBus<TargetManager> Bus;

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; // there's only one target manager right now
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;  // theres only one target manager right now
        virtual ~TargetManager() { }

        // call this function to retrieve the list of currently known targets - this is mainly used for GUIs
        // when they come online and attempt to enum (they won't have been listening for target coming and going)
        // you will only be shown targets that have been seen in a reasonable amount of time.
        virtual void EnumTargetInfos(TargetContainer& infos) = 0;

        // set the desired target, which we'll specifically keep track of.
        // the target controls who gets lua commands, tweak stuff, that kind of thing
        virtual void SetDesiredTarget(AZ::u32 desiredTargetID) = 0;

        virtual void SetDesiredTargetInfo(const TargetInfo& targetInfo) = 0;

        // retrieve what it was set to.
        virtual TargetInfo GetDesiredTarget() = 0;

        // given id, get info.
        virtual TargetInfo GetTargetInfo(AZ::u32 desiredTargetID) = 0;

        // check if target is online
        virtual bool IsTargetOnline(AZ::u32 desiredTargetID) = 0;

        virtual bool IsDesiredTargetOnline() = 0;

        // set/get the name that is going to identify the local node in the neighborhood
        virtual void SetMyPersistentName(const char* name) = 0;
        virtual const char* GetMyPersistentName() = 0;

        virtual TargetInfo GetMyTargetInfo() const = 0;

        // set/get the name of the neighborhood we want to connect to
        virtual void SetNeighborhood(const char* name) = 0;
        virtual const char* GetNeighborhood() = 0;

        // send a message to a remote target
        virtual void SendTmMessage(const TargetInfo& target, const TmMsg& msg) = 0;

        // manually force messages for a specific id to be delivered.
        virtual void DispatchMessages(MsgSlotId id) = 0;
    };
};

#endif // AZFRAMEWORK_TARGETMANAGEMENTAPI_H

#pragma once

