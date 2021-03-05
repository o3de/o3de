/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#ifndef AZFRAMEWORK_TARGETMANAGEMENTCOMPONENT_H
#define AZFRAMEWORK_TARGETMANAGEMENTCOMPONENT_H

#include <AzFramework/TargetManagement/TargetManagementAPI.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/atomic.h>

namespace AZ
{
    class SerializeContext;
}

namespace AzFramework
{
    class TargetManagementNetworkImpl;
    struct TargetManagementSettings;

    class TargetManagementComponent
        : public AZ::Component
        , private TargetManager::Bus::Handler
        , private AZ::SystemTickBus::Handler
    {
        friend TargetManagementNetworkImpl;

    public:
        AZ_COMPONENT(TargetManagementComponent, "{39899133-42B3-4e92-A579-CDDC85A23277}")

        TargetManagementComponent();
        ~TargetManagementComponent() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        //////////////////////////////////////////////////////////////////////////

    protected:
        //////////////////////////////////////////////////////////////////////////
        // target management api
        void EnumTargetInfos(TargetContainer& infos) override;
        void SetDesiredTarget(AZ::u32 desiredTargetID) override;
        void SetDesiredTargetInfo(const TargetInfo& targetInfo) override;
        TargetInfo GetDesiredTarget() override;
        TargetInfo GetTargetInfo(AZ::u32 desiredTargetID) override;
        bool IsTargetOnline(AZ::u32 desiredTargetID) override;
        bool IsDesiredTargetOnline() override;
        void SetMyPersistentName(const char* name) override;
        const char* GetMyPersistentName() override;
        TargetInfo GetMyTargetInfo() const override;
        void SetNeighborhood(const char* name) override;
        const char* GetNeighborhood() override;
        void SendTmMessage(const TargetInfo& target, const TmMsg& msg) override;
        void DispatchMessages(MsgSlotId id) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::SystemTickBus
        void OnSystemTick() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ObjectStream Load Callback
        void OnMsgParsed(TmMsg** ppMsg, void* classPtr, const AZ::Uuid& classId, const AZ::SerializeContext* sc);
        //////////////////////////////////////////////////////////////////////////

        // All communication updates run on a separate thread to avoid being blocked by breakpoints
        void TickThread();

        // Joins the hub's neighborhood
        void JoinNeighborhood();

        AZStd::intrusive_ptr<TargetManagementSettings>  m_settings;
        TargetContainer                                 m_availableTargets;
        AZStd::chrono::system_clock::time_point         m_reconnectionTime; // time of next connection attempt

        AZStd::vector<char, AZ::OSStdAllocator> m_tmpInboundBuffer;

        TmMsgQueue          m_inbox;
        AZStd::mutex        m_inboxMutex;

        typedef AZStd::pair<AZ::u32, AZStd::vector<char, AZ::OSStdAllocator> > OutboundBufferedTmMsg;
        typedef AZStd::deque<OutboundBufferedTmMsg, AZ::OSStdAllocator> TmMsgOutbox;
        TmMsgOutbox         m_outbox;
        AZStd::mutex        m_outboxMutex;

        AZ::SerializeContext*   m_serializeContext;

        // these are used for target communication
        AZStd::atomic_bool              m_stopRequested;
        AZStd::thread                   m_threadHandle;
        TargetManagementNetworkImpl*    m_networkImpl;
    };
}   // namespace AzFramework

#endif // AZFRAMEWORK_TARGETMANAGEMENTCOMPONENT_H
#pragma once


