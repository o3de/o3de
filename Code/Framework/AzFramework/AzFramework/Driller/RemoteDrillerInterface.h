/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZFRAMEWORK_REMOTE_DRILLER_INTERFACE_H
#define AZFRAMEWORK_REMOTE_DRILLER_INTERFACE_H

#include <AzCore/Driller/Driller.h>
#include <AzCore/Compression/Compression.h>
#include <AzCore/Component/Component.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Driller/DrillerConsoleAPI.h>
#include <AzFramework/TargetManagement/TargetManagementAPI.h>

//#define ENABLE_COMPRESSION_FOR_REMOTE_DRILLER

namespace AZ
{
    struct ClassDataReflection;
}

namespace AzFramework
{
    /**
     * Represents a remote driller session on the tool machine.
     * It is responsible for receiving and processing remote driller data.
     * Driller clients should derive from this class and implement the virtual interfaces.
     */
    class DrillerRemoteSession
        : public TmMsgBus::Handler
    {
    public:
        DrillerRemoteSession();
        ~DrillerRemoteSession();

        // Called when new driller data arrives
        virtual void ProcessIncomingDrillerData(const char* streamIdentifier, const void* data, size_t dataSize) = 0;
        // Called when the connection to the driller is lost. The session should be deleted in response to this message
        virtual void OnDrillerConnectionLost() = 0;

        // Start drilling the selected drillers as part of this session
        void    StartDrilling(const DrillerListType& drillers, const char* captureFile);
        // Stop this drill session
        void    StopDrilling();

        // Replay a previously captured driller session from file
        void    LoadCaptureData(const char* fileName);

    protected:
        //---------------------------------------------------------------------
        // TmMsgBus
        //---------------------------------------------------------------------
        virtual void OnReceivedMsg(TmMsgPtr msg);
        //---------------------------------------------------------------------

        void Decompress(const void* compressedBuffer, size_t compressedBufferSize);

        static const AZ::u32 c_decompressionBufferSize = 128 * 1024;

        AZStd::vector<char>         m_uncompressedMsgBuffer;
#ifdef ENABLE_COMPRESSION_FOR_REMOTE_DRILLER
        AZ::ZLib                    m_decompressor;
        char                        m_decompressionBuffer[c_decompressionBufferSize];
#endif
        AZ::IO::SystemFile          m_captureFile;
    };

    /**
     * Driller clients interested in receiving notification events from the
     * network console should implement this interface.
     */
    class DrillerNetworkConsoleEvents
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        typedef AZ::OSStdAllocator  AllocatorType;
        //////////////////////////////////////////////////////////////////////////

        virtual ~DrillerNetworkConsoleEvents() {}

        // A list of available drillers has been received from the target machine.
        virtual void    OnReceivedDrillerEnumeration(const DrillerInfoListType& availableDrillers) = 0;
    };
    typedef AZ::EBus<DrillerNetworkConsoleEvents> DrillerNetworkConsoleEventBus;

    /**
     * The network driller console implements this interface.
     * Commands can be sent to the network console through this interface.
     */
    class DrillerNetworkConsoleCommands
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        typedef AZ::OSStdAllocator  AllocatorType;

        // there's only one driller console instance allowed
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual ~DrillerNetworkConsoleCommands() {}

        // Request an enumeration of available drillers from the target machine
        virtual void    EnumerateAvailableDrillers() = 0;
        // Start a drilling session. This function is normally called internally by DrillerRemoteSession
        virtual void    StartRemoteDrillerSession(const DrillerListType& drillers, DrillerRemoteSession* handler) = 0;
        // Stop a drilling session. This function is normally called internally by DrillerRemoteSession
        virtual void    StopRemoteDrillerSession(AZ::u64 sessionId) = 0;
    };
    typedef AZ::EBus<DrillerNetworkConsoleCommands> DrillerNetworkConsoleCommandBus;

    class DrillerNetSessionStream;

    /**
     * Runs on the machine being drilled and is responsible for communications
     * with the DrillerNetworkConsole running on the tool side as well as
     * creating DrillerNetSessionStreams for each driller session being started.
     */
    class DrillerNetworkAgentComponent
        : public AZ::Component
        , public TargetManagerClient::Bus::Handler
    {
    public:
        AZ_COMPONENT(DrillerNetworkAgentComponent, "{B587A74D-6190-4149-91CB-0EA69936BD59}")

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        virtual void Init();
        virtual void Activate();
        virtual void Deactivate();
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void Reflect(AZ::ReflectContext* context);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TargetManagerClient
        virtual void TargetLeftNetwork(TargetInfo info);
        //////////////////////////////////////////////////////////////////////////

    protected:
        //////////////////////////////////////////////////////////////////////////
        // TmMsg handlers
        virtual void OnRequestDrillerEnum(TmMsgPtr msg);
        virtual void OnRequestDrillerStart(TmMsgPtr msg);
        virtual void OnRequestDrillerStop(TmMsgPtr msg);
        //////////////////////////////////////////////////////////////////////////

        TmMsgCallback   m_cbDrillerEnumRequest;
        TmMsgCallback   m_cbDrillerStartRequest;
        TmMsgCallback   m_cbDrillerStopRequest;

        AZStd::vector<DrillerNetSessionStream*> m_activeSessions;
    };

    /**
     * Runs on the tool machine and is responsible for communications with the
     * DrillerNetworkAgent.
     */
    class DrillerNetworkConsoleComponent
        : public AZ::Component
        , public DrillerNetworkConsoleCommandBus::Handler
        , public TargetManagerClient::Bus::Handler
    {
    public:
        AZ_COMPONENT(DrillerNetworkConsoleComponent, "{78ACADA4-F2C7-4320-8E97-59DD8B9BE33A}")

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        virtual void Init();
        virtual void Activate();
        virtual void Deactivate();
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void Reflect(AZ::ReflectContext* context);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // DrillerNetworkConsoleCommandBus
        virtual void    EnumerateAvailableDrillers();
        virtual void    StartRemoteDrillerSession(const DrillerListType& drillers, DrillerRemoteSession* handler);
        virtual void    StopRemoteDrillerSession(AZ::u64 sessionId);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TargetManagerClient
        virtual void DesiredTargetConnected(bool connected);
        virtual void DesiredTargetChanged(AZ::u32 newTargetID, AZ::u32 oldTargetID);
        //////////////////////////////////////////////////////////////////////////

    protected:
        //////////////////////////////////////////////////////////////////////////
        // TmMsg handlers
        virtual void OnReceivedDrillerEnum(TmMsgPtr msg);
        //////////////////////////////////////////////////////////////////////////

        typedef AZStd::vector<DrillerRemoteSession*> ActiveSessionListType;
        ActiveSessionListType           m_activeSessions;
        TargetInfo                      m_curTarget;
        TmMsgCallback                   m_cbDrillerEnum;
    };

    void ReflectNetDrillerClasses(AZ::ReflectContext* context);
}   // namespace AzFramework

#endif  // AZFRAMEWORK_REMOTE_DRILLER_INTERFACE_H
#pragma once
