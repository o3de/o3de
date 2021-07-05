/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "DrillerDataContainer.h"

#include <AzCore/Driller/Stream.h>
#include <AzCore/Driller/DrillerRootHandler.h>
#include <AzCore/Driller/DefaultStringPool.h>

#include <AzCore/IO/SystemFile.h>  // temp for DebugHackProcessFile

#include <AzCore/XML/rapidxml_print.h> // metadata is stored this way

#include "Unsupported/UnsupportedDataAggregator.hxx"
#include "Memory/MemoryDataAggregator.hxx"
#include "Trace/TraceMessageDataAggregator.hxx"
#include "Profiler/ProfilerDataAggregator.hxx"
#include "Carrier/CarrierDataAggregator.hxx"
#include "Replica/ReplicaDataAggregator.hxx"
#include "Rendering/VRAM/VRAMDataAggregator.hxx"
#include "EventTrace/EventTraceDataAggregator.h"
// IMPORTANT: include new aggregators above


namespace Driller
{
    class DrillerDataHandler : public AZ::Debug::DrillerHandlerParser
    {
    public:
        AZ_CLASS_ALLOCATOR(DrillerDataHandler,AZ::SystemAllocator,0)

        DrillerDataHandler(int identity, DrillerDataContainer* container)
        : AZ::Debug::DrillerHandlerParser(/*true*/false)
        , m_identity(identity)
        , m_currentFrame(-1)
        , m_dataContainer(container)
        , m_dataParser(nullptr)
        {
            m_drillerSessionInfo.m_platform = static_cast<int>(AZ::g_currentPlatform); /// Init with current platform so no endian swapping till we read all the initial settings.
            m_inputStream.SetStringPool(&m_stringPool);
            m_dataParser = aznew AZ::Debug::DrillerSAXParserHandler(this);
        }

        ~DrillerDataHandler()
        {
            delete m_dataParser;
        }

        virtual  AZ::Debug::DrillerHandlerParser*   OnEnterTag(AZ::u32 tagName)
        {
            if( tagName == AZ_CRC("StartData", 0xecf3f53f) )
                return &m_drillerSessionInfo;
            if( tagName == AZ_CRC("Frame", 0xb5f83ccd) )
                return this;
            for(DrillerNetworkMessages::AggregatorList::iterator it = m_dataContainer->m_aggregators.begin(); it != m_dataContainer->m_aggregators.end(); ++it)
            {
                if( (*it)->GetDrillerId() == tagName )
                    return (*it)->GetDrillerDataParser();
            }

            AZ_TracePrintf("Driller", "We should never get here as we should have added 'Unsupported driller(s)' in OnExitTag('StartData')");
            return nullptr;
        }

        virtual void                    OnExitTag(DrillerHandlerParser* handler, AZ::u32 tagName)
        {
            (void)handler;
            if( tagName == AZ_CRC("StartData", 0xecf3f53f) )
            {
                // create all drillers that were in the session data
                for(AZ::Debug::DrillerManager::DrillerListType::iterator itDriller = m_drillerSessionInfo.m_drillers.begin(); itDriller != m_drillerSessionInfo.m_drillers.end(); ++itDriller )
                {
                    AZ::u32 drillerId = itDriller->id;
                    bool isCreated = false;
                    for(DrillerNetworkMessages::AggregatorList::iterator it = m_dataContainer->m_aggregators.begin(); it != m_dataContainer->m_aggregators.end(); ++it)
                    {
                        if( (*it)->GetDrillerId() == drillerId )
                        {
                            isCreated = true;
                            break;
                        }
                    }

                    if( !isCreated )
                    {
                        // Create the aggregator, if the driller is missing add UnsupportedAggregator
                        Aggregator* aggr = m_dataContainer->CreateAggregator(drillerId, true);
                        if( aggr )
                            m_dataContainer->m_aggregators.push_back(aggr);

                        // two ways to add aggregators:
                        // 1) "NewAggregatorList" send an entire list which replaces the current setup, which is most efficient
                        // 2) "AddAggregator" send a single aggregator which gets appended
                        EBUS_EVENT_ID(m_identity, DrillerNetworkMessages::Bus,AddAggregator, *m_dataContainer->m_aggregators.back());
                    }
                }
            }
        }

        virtual void                    OnData(const AZ::Debug::DrillerSAXParser::Data& dataNode)
        {
            if( dataNode.m_name == AZ_CRC("FrameNum", 0x85a1a919) )
            {
                // Send event that previous frame has finished
                if( m_currentFrame != -1 )
                {
                    EBUS_EVENT_ID(m_identity, DrillerNetworkMessages::Bus,EndFrame, m_currentFrame);
                }
                dataNode.Read(m_currentFrame);
                for(DrillerNetworkMessages::AggregatorList::iterator it = m_dataContainer->m_aggregators.begin(); it != m_dataContainer->m_aggregators.end(); ++it)
                {
                    (*it)->AddNewFrame();
                }
            }
        }

        void ProcessStream(const char* streamIdentifier, const void* data, unsigned int dataSize)
        {
            m_inputStream.SetData(streamIdentifier, data, dataSize);
            m_dataParser->ProcessStream(m_inputStream);
        }

        AZ::Debug::DrillerStartdataHandler      m_drillerSessionInfo;
        int                                     m_currentFrame;
        DrillerDataContainer*                   m_dataContainer;
        AZ::Debug::DrillerSAXParserHandler*     m_dataParser;
        AZ::Debug::DrillerInputMemoryStream     m_inputStream;
        AZ::Debug::DrillerDefaultStringPool     m_stringPool;
        int                                     m_identity;
    };

    DrillerDataContainer::DrillerDataContainer(int identity, const char* tmpCaptureFilename)
        : m_dataHandler(nullptr)
        , m_identity(identity)
        , m_tmpCaptureFilename(tmpCaptureFilename)
    {
        AzFramework::DrillerNetworkConsoleEventBus::Handler::BusConnect();
        EBUS_EVENT(AzFramework::DrillerNetworkConsoleCommandBus, EnumerateAvailableDrillers);
    }

    DrillerDataContainer::~DrillerDataContainer()
    {
        AzFramework::DrillerNetworkConsoleEventBus::Handler::BusDisconnect();
        DestroyAggregators();

        delete m_dataHandler;
    }

    void DrillerDataContainer::OnReceivedDrillerEnumeration(const AzFramework::DrillerInfoListType& availableDrillers)
    {
        // TODO: Decide how the available driller list should influence the behavior of the driller
        // display. For now we will display whatever is available.
        m_availableDrillers = availableDrillers;
        EBUS_EVENT_ID(m_identity, DrillerNetworkMessages::Bus, NewAggregatorsAvailable);
    }

    void DrillerDataContainer::CreateAggregators()
    {
        DestroyAggregators();

        if (m_availableDrillers.size())
        {
            for (size_t i = 0; i < m_availableDrillers.size(); ++i)
            {
                Aggregator* aggr =  CreateAggregator(m_availableDrillers[i].m_id, true);
                if( aggr )
                    m_aggregators.push_back(aggr);
            }
        }
        EBUS_EVENT_ID(m_identity, DrillerNetworkMessages::Bus, NewAggregatorList,m_aggregators);
    }

    void  DrillerDataContainer::DestroyAggregators()
    {
        EBUS_EVENT_ID(m_identity, DrillerNetworkMessages::Bus, DiscardAggregators);

        for(DrillerNetworkMessages::AggregatorList::iterator it = m_aggregators.begin(); it != m_aggregators.end(); ++it )
            delete *it;
        m_aggregators.clear();
    }

    Aggregator* DrillerDataContainer::CreateAggregator(AZ::u32 id, bool createUnsupported)
    {
        if( id == MemoryDataAggregator::DrillerId() )
        {
            return aznew MemoryDataAggregator(m_identity);
        }
        else if( id == TraceMessageDataAggregator::DrillerId() )
        {
                return aznew TraceMessageDataAggregator(m_identity);
        }
        else if (id == ProfilerDataAggregator::DrillerId() )
        {
            return aznew ProfilerDataAggregator(m_identity);
        }
        else if(id == CarrierDataAggregator::DrillerId())
        {
            return aznew CarrierDataAggregator(m_identity);
        }
        else if(id == ReplicaDataAggregator::DrillerId())
        {
            return aznew ReplicaDataAggregator(m_identity);
        }
        else if(id == VRAM::VRAMDataAggregator::DrillerId())
        {
            return aznew VRAM::VRAMDataAggregator(m_identity);
        }
        else if (id == EventTraceDataAggregator::DrillerId())
        {
            return aznew EventTraceDataAggregator(m_identity);
        }
        // IMPORTANT: Add new aggregators here

        return createUnsupported ? aznew UnsupportedDataAggregator(id) : nullptr;
    }

    void DrillerDataContainer::ProcessIncomingDrillerData(const char* streamIdentifier, const void* data, size_t dataSize)
    {
        AZ_Assert(m_dataHandler,"You must have a valid data handler parser to parse the data!");
        m_dataHandler->ProcessStream(streamIdentifier, data, static_cast<unsigned int>(dataSize));
    }

    void DrillerDataContainer::OnDrillerConnectionLost()
    {
        StopDrilling();
    }

    void DrillerDataContainer::StartDrilling()
    {
        DrillerEvent::ResetGlobalEventId();

        AzFramework::DrillerListType drillersToStart;
        for(DrillerNetworkMessages::AggregatorList::iterator it = m_aggregators.begin(); it != m_aggregators.end(); ++it)
        {
            (*it)->Reset();

            if ((*it)->IsCaptureEnabled())
            {
                drillersToStart.push_back((*it)->GetDrillerId());
            }
        }
        if (drillersToStart.size())
        {
            delete m_dataHandler;
            m_dataHandler = aznew DrillerDataHandler(m_identity, this);

            AzFramework::DrillerRemoteSession::StartDrilling(drillersToStart, m_tmpCaptureFilename.c_str());
        }
    }

    void DrillerDataContainer::LoadCaptureData(const char* fileName)
    {
        DrillerEvent::ResetGlobalEventId();
        // Reset data
        DestroyAggregators();

        AZStd::string baseFilename( fileName );

        delete m_dataHandler;
        m_dataHandler = aznew DrillerDataHandler(m_identity, this);
        AzFramework::DrillerRemoteSession::LoadCaptureData(fileName);
    }

    void DrillerDataContainer::CloseCaptureData()
    {
        StopDrilling();
        DestroyAggregators();
    }

    void DrillerDataContainer::Reflect(AZ::ReflectContext* context)
    {
        MemoryDataAggregator::Reflect(context);
        TraceMessageDataAggregator::Reflect(context);
        ProfilerDataAggregator::Reflect(context);
        ReplicaDataAggregator::Reflect(context);
    }

}//namespace Driller
