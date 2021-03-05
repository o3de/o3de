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
#include "Multiplayer_precompiled.h"
#include "Multiplayer/BehaviorContext/GridSearchContext.h"

#include <GridMate/NetworkGridMate.h>

#include "Multiplayer/MultiplayerEventsComponent.h"
#include "Multiplayer/BehaviorContext/GridSystemContext.h"
#include "Multiplayer/GridMateServiceWrapper/GridMateServiceWrapper.h"
#include <Multiplayer/MultiplayerUtils.h>

namespace Multiplayer
{
    /**
     * Wrapper around a GridMate::GridSearch pointer
     */
    struct GridSearchTicket
    {
        AZ_TYPE_INFO(GridSearchTicket, "{ADFA9839-4D38-4B3E-8909-9D55261E69D5}");
        AZ_CLASS_ALLOCATOR(GridSearchTicket, AZ::SystemAllocator, 0);

        GridSearchTicket(GridMate::GridSearch* ptr) : m_ptr(ptr)
        {
        }

        ~GridSearchTicket()
        {
            Reset();
        }

        int GetNumResults() const
        {
            if (m_ptr)
            {
                return m_ptr->GetNumResults();
            }
            return 0;
        }

        void Reset()
        {
            m_ptr = nullptr;
        }

        GridMate::GridSearch* GetGridSearch() const
        {
            return m_ptr;
        }

    private:        
        GridMate::GridSearch* m_ptr;
    };

    /** 
     * Helper class to manage SessionEventBus events on behalf of GridSearchBusHandler
     */
    struct GridSessionCallbacksHandler
        : public GridMate::SessionEventBus::Handler
    {
        GridSessionCallbacksHandler()
        {
        }

        virtual ~GridSessionCallbacksHandler()
        {
            Disconnect();
        }

        void Connect(const SessionDesc& m_sessionDesc, GridMate::IGridMate* gridMate)
        {
            m_desc = m_sessionDesc;
            GridMate::SessionEventBus::Handler::BusConnect(gridMate);
        }

        void Disconnect()
        {
            GridMate::SessionEventBus::Handler::BusDisconnect();
            for (auto it : m_ticketMap)
            {
                delete it.second;
            }
            m_ticketMap.clear();
        }

        GridSearchTicket* CreateTicket(GridMate::GridSearch* gridSearch)
        {
            GridSearchTicket* gridSearchTicket = aznew GridSearchTicket(gridSearch);
            m_ticketMap.insert(AZStd::make_pair(gridSearch, gridSearchTicket));
            return gridSearchTicket;
        }

        bool ReleaseTicket(GridSearchTicket* ticket)
        {
            return ReleaseGridSearch(ticket->GetGridSearch());
        }

        bool ReleaseGridSearch(GridMate::GridSearch* gridSearch)
        {
            if (gridSearch == nullptr)
            {
                return false;
            }
            auto it = m_ticketMap.find(gridSearch);
            if (it != m_ticketMap.end())
            {
                delete it->second;
                m_ticketMap.erase(it);
                return true;
            }
            return false;
        }

        GridSearchTicket* FindOrCreateGridSearchTicket(GridMate::GridSearch* gridSearch)
        {
            auto it = m_ticketMap.find(gridSearch);
            if (it != m_ticketMap.end())
            {
                return it->second;
            }
            GridSearchTicket* newTicket = aznew GridSearchTicket(gridSearch);
            m_ticketMap.insert(AZStd::make_pair(gridSearch, newTicket));
            return newTicket;
        }

        //
        //  GridMate::SessionEventBus::Handler
        //
        void OnGridSearchComplete(GridMate::GridSearch* gridSearch) override
        {
            for (size_t i = 0; i < gridSearch->GetNumResults(); ++i)
            {
                EBUS_EVENT(GridSearchBus, OnSearchInfo, gridSearch->GetResult(i));
            }
            EBUS_EVENT(GridSearchBus, OnSearchComplete, FindOrCreateGridSearchTicket(gridSearch));
        }
        void OnGridSearchRelease(GridMate::GridSearch* gridSearch) override
        {
            ReleaseGridSearch(gridSearch);
        }
        void OnGridSearchStart(GridMate::GridSearch* gridSearch) override
        { 
            (void)gridSearch;
        }
        void OnSessionDelete(GridMate::GridSession* session) override
        {
            (void)session;
            Disconnect();
        }
        void OnSessionJoined(GridMate::GridSession* session) override
        { 
            EBUS_EVENT(GridSearchBus, OnJoinComplete, session);
        }
        
    private:
        SessionDesc m_desc;
        AZStd::unordered_map<GridMate::GridSearch*, GridSearchTicket*> m_ticketMap;
    };

    /**
      * Extends the Grid parameters with an optional security string
      */      
    struct SearchGridParameters
    {
        GridMate::string m_securityString;
        const SessionDesc& m_sessionDesc;

        SearchGridParameters(const SessionDesc& sessionDesc)
            : m_securityString()
            , m_sessionDesc(sessionDesc)
        {
        }

        GridMate::GridSessionParam FetchGridSessionParam(const char* key)
        {
            GridMate::GridSessionParam param;
            if (GridMateSystemContext::FetchParam(key, m_sessionDesc, param))
            {
                return param;
            }
            else if (!strcmp(key, "gm_securityData")) // has to come from a CFG
            {
                if (gEnv && gEnv->pConsole && gEnv->pConsole->GetCVar("gm_securityData"))
                {
                    m_securityString = gEnv->pConsole->GetCVar("gm_securityData")->GetString();
                    param.SetValue(m_securityString);
                }
            }
            return param;
        }
    };

    /**
      * Handles grid searches for a behavior context
      */
    class GridSearchBusHandler
        : public GridSearchBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(GridSearchBusHandler, "{83FF3AEB-2513-43A0-9BEE-ED8980449AEB}", AZ::SystemAllocator
            , StartSearch
            , StopSearch
            , OnSearchComplete
            , OnSearchError
            , OnSearchInfo
            , OnSearchClosed
            , OnJoinComplete
        );

    protected:
        //
        // GridSearchInterface::Handler
        //
        const GridSearchTicket* StartSearch(const SessionDesc& sessionDesc) override
        {
            m_sessionDesc = sessionDesc;

            GridMate::IGridMate* gridMate = FindGridMate();
            if (!gridMate)
            {
                OnSearchError("Global GridMate not ready");
                return nullptr;
            }
            m_gridSessionCallbacksHandler.Connect(m_sessionDesc, gridMate);

            m_gridMateServiceWrapper.reset(GridMateSystemContext::RegisterServiceWrapper(m_sessionDesc.m_serviceType));
            if (!m_gridMateServiceWrapper)
            {
                // error
                return nullptr;
            }

            SearchGridParameters searchGridParameters(m_sessionDesc);
            GridMateServiceParams gridMateServiceParams({}, AZStd::bind(&SearchGridParameters::FetchGridSessionParam, &searchGridParameters, AZStd::placeholders::_1));

            GridMate::GridSearch* search = m_gridMateServiceWrapper->ListServers(gridMate, gridMateServiceParams);
            if (search == nullptr)
            {
                EBUS_EVENT(GridSearchBus, OnSearchError, "ListServers failed to start a GridSearch.");
            }

            return m_gridSessionCallbacksHandler.CreateTicket(search);
        }
        bool JoinSession(const GridMate::SearchInfo* searchInfo)
        {
            GridMate::GridSession* session = nullptr;
            GridMate::CarrierDesc carrierDesc;

            SearchGridParameters searchGridParameters(m_sessionDesc); 
            GridMateServiceParams gridMateServiceParams({}, AZStd::bind(&SearchGridParameters::FetchGridSessionParam, &searchGridParameters, AZStd::placeholders::_1));
            GridMateSystemContext::InitCarrierDesc(gridMateServiceParams, carrierDesc);
            Multiplayer::NetSec::ConfigureCarrierDescForJoin(carrierDesc);

            session = m_gridMateServiceWrapper->JoinSession(FindGridMate(), carrierDesc, searchInfo);
            if (session == nullptr)
            {
                OnSearchClosed(false);
                Multiplayer::NetSec::OnSessionFailedToCreate(carrierDesc);
                EBUS_EVENT(GridSearchBus, OnSearchError, "ListServers failed to start a GridSearch.");
            }
            else
            {
                OnSearchClosed(true);
            }
            return session != nullptr;
        }
        bool StopSearch(GridSearchTicket* ticket) override
        {
            if (ticket)
            {
                m_gridSessionCallbacksHandler.ReleaseTicket(ticket);
                return true;
            }
            return false;
        }
        void OnSearchComplete(const GridSearchTicket* gridSearchTicket) override
        {
            Call(FN_OnSearchComplete, gridSearchTicket);
        }
        void OnSearchError(const GridMate::string& errorMsg)
        {
            Call(FN_OnSearchError, errorMsg);
        }
        void OnSearchInfo(const GridMate::SearchInfo* searchInfo) override
        {
            Call(FN_OnSearchInfo, searchInfo);
        }
        void OnSearchClosed(bool isJoiningSession) override
        {
            Call(FN_OnSearchClosed, isJoiningSession);
        }
        void OnJoinComplete(const GridMate::GridSession* gridSession) override
        {
            Call(FN_OnJoinComplete, gridSession);
        }

        //
        // helper method(s)
        //
        GridMate::IGridMate* FindGridMate()
        {
            if (gEnv && gEnv->pNetwork)
            {
                return gEnv->pNetwork->GetGridMate();
            }
            return nullptr;
        }

        GridSessionCallbacksHandler m_gridSessionCallbacksHandler;
        SessionDesc m_sessionDesc;
        AZStd::unique_ptr<GridMateServiceWrapper> m_gridMateServiceWrapper;
    };

    /**
    * Exposes Grid searching events and callbacks to a behavior context such as Lua
    */
    namespace GridSearchBehavior
    {
        void Reflect(AZ::ReflectContext* reflectContext)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

            if (serializeContext)
            {
                serializeContext->Class<GridMate::SearchInfo>()
                    ->Version(1)
                    ->Field("SessionId", &GridMate::SearchInfo::m_sessionId)
                    ->Field("FreePublicSlots", &GridMate::SearchInfo::m_numFreePublicSlots)
                    ->Field("FreePrivateSlots", &GridMate::SearchInfo::m_numFreePrivateSlots)
                    ->Field("UsedPublicSlots", &GridMate::SearchInfo::m_numUsedPublicSlots)
                    ->Field("UsedPrivateSlots", &GridMate::SearchInfo::m_numUsedPrivateSlots)
                    ->Field("NumPlayers", &GridMate::SearchInfo::m_numPlayers)
                ;
            }

            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext);
            if (behaviorContext)
            {
                behaviorContext->Class<GridSearchTicket>()
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                    ->Method("GetNumResults", &GridSearchTicket::GetNumResults)
                    ;

                behaviorContext->Class<GridMate::SearchInfo>()
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                    ->Property("numPlayers", BehaviorValueProperty(&GridMate::SearchInfo::m_numPlayers))
                    ->Property("numFreePrivateSlots", BehaviorValueProperty(&GridMate::SearchInfo::m_numFreePrivateSlots))
                    ->Property("numUsedPrivateSlots", BehaviorValueProperty(&GridMate::SearchInfo::m_numUsedPrivateSlots))
                    ->Property("numFreePublicSlots", BehaviorValueProperty(&GridMate::SearchInfo::m_numFreePublicSlots))
                    ->Property("numUsedPublicSlots", BehaviorValueProperty(&GridMate::SearchInfo::m_numUsedPublicSlots))
                    ;

                behaviorContext->EBus<GridSearchBus>("GridSearchBusHandler")
                    ->Handler<GridSearchBusHandler>()
                    ->Event("StartSearch", &GridSearchBus::Events::StartSearch)
                    ->Event("StopSearch", &GridSearchBus::Events::StopSearch)
                    ->Event("JoinSession", &GridSearchBus::Events::JoinSession)
                    ;
            }
        }
    }
};

