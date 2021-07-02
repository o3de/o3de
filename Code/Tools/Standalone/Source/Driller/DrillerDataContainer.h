/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_DRILLER_DATA_CONTAINER
#define DRILLER_DRILLER_DATA_CONTAINER

#include <AzCore/Memory/SystemAllocator.h>

#include "DrillerNetworkMessages.h"

#include "AzFramework/Driller/RemoteDrillerInterface.h"

namespace DH
{
    namespace Debug
    {
        class DrillerInputMemoryStream;
    }
}

namespace Driller
{
    class DrillerDataHandler;

    /**
     * Driller data container. Contains all the data (aggregators)
     * for a driller session. It interfaces with with DrillerSession
     * for local file caching (TODO) and remote data transfer.
     */
    class DrillerDataContainer
        : public AzFramework::DrillerRemoteSession
        , public AzFramework::DrillerNetworkConsoleEventBus::Handler
    {
        friend class DrillerDataHandler;

    public:
        AZ_CLASS_ALLOCATOR(DrillerDataContainer, AZ::SystemAllocator, 0)

        DrillerDataContainer(int identity, const char* tmpCaptureFilename);
        ~DrillerDataContainer();

        //////////////////////////////////////////////////////////////////////////
        // DrillerRemoteSession
        virtual void ProcessIncomingDrillerData(const char* streamIdentifier, const void* data, size_t dataSize);
        virtual void OnDrillerConnectionLost();
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // DrillerNetworkConsoleEvents
        virtual void OnReceivedDrillerEnumeration(const AzFramework::DrillerInfoListType& availableDrillers);
        //////////////////////////////////////////////////////////////////////////

        void  StartDrilling();
        void  LoadCaptureData(const char* fileName);
        void  CloseCaptureData();
        void  CreateAggregators();

    protected:
        void  DestroyAggregators();
        Aggregator* CreateAggregator(AZ::u32 id, bool createUnsupported = false);

        DrillerNetworkMessages::AggregatorList      m_aggregators;
        DrillerDataHandler*                         m_dataHandler;
        AzFramework::DrillerInfoListType            m_availableDrillers;
        AZStd::string                               m_tmpCaptureFilename;
        int                                         m_identity;

    public:
        // data container is the one place that knows about all the aggregators
        // and indeed is responsible for creating them
        // and thus the best place to centralize their reflection
        static void Reflect(AZ::ReflectContext* context);
    };
}

#endif //DRILLER_DRILLER_DATA_CONTAINER
#pragma once
