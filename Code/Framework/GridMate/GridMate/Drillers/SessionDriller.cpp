/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GridMate/Drillers/SessionDriller.h>

using namespace AZ::Debug;

namespace GridMate
{
    namespace Debug
    {
        //=========================================================================
        // SessionDriller
        // [4/14/2011]
        //=========================================================================
        SessionDriller::SessionDriller()
        {
            m_drillerTag = AZ_CRC("SessionDriller", 0x30b916a9);
        }

        //=========================================================================
        // Start
        // [4/14/2011]
        //=========================================================================
        void SessionDriller::Start(const Param* params, int numParams)
        {
            (void)params;
            (void)numParams;
            // Collect current session information ?
            SessionDrillerBus::Handler::BusConnect();

            //
            //m_output->BeginTag(m_drillerTag);
            //m_output->BeginTag(AZ_CRC("StartDrill"));
            //if(sessionMgr->m_activeSession)
            //{
            //  GridSession* gs = sessionMgr->m_activeSession;
            //  // store the current session state
            //  m_output->BeginTag(AZ_CRC("Session"));
            //  m_output->Write(AZ_CRC("SessionId"),gs->GetId());
            //  m_output->Write(AZ_CRC("Carrier"),gs->GetCarrier());
            //  m_output->Write(AZ_CRC("ReplicaMgr"),gs->GetReplicaMgr());
            //  m_output->Write(AZ_CRC("Topology"),(char)gs->GetTopology());
            //  m_output->Write(AZ_CRC("Time"),gs->GetTime());
            //  m_output->Write(AZ_CRC("State"),(char)gs->m_sm.GetCurrentState());
            //  m_output->Write(AZ_CRC("IsHost"),gs->IsHost());
            //  // There are endless params add as needed

            //  for(unsigned int i = 0; i < gs->GetNumberOfMembers(); ++i )
            //  {
            //      GridMember* gm = gs->GetMember(i);
            //      m_output->BeginTag(AZ_CRC("Member"));
            //      m_output->Write(AZ_CRC("Id"),gm->GetId().ToString());
            //      m_output->Write(AZ_CRC("Name"),gm->GetName());
            //      m_output->Write(AZ_CRC("ConnectionId"),gm->GetConnectionId());
            //      m_output->Write(AZ_CRC("NAT"),(char)gm->GetNatType());
            //      m_output->Write(AZ_CRC("CommFilter"),gm->GetCommFilter());
            //      m_output->Write(AZ_CRC("IsHost"),gm->IsHost());
            //      m_output->Write(AZ_CRC("IsLocal"),gm->IsLocal());
            //      m_output->Write(AZ_CRC("IsInvited"),gm->IsInvited());
            //      m_output->EndTag(AZ_CRC("Member"));
            //  }

            //  m_output->EndTag(AZ_CRC("Session"));
            //}
            //else if(sessionMgr->m_activeSearch)
            //{
            //  GridSearch* gs = sessionMgr->m_activeSearch;
            //  m_output->BeginTag(AZ_CRC("GridSearch"));
            //  m_output->Write(AZ_CRC("SearchId"),gs);
            //  m_output->Write(AZ_CRC("IsDone"),gs->IsDone());
            //  m_output->Write(AZ_CRC("NumResults"),gs->GetNumResults());
            //  // add platform specific drill or just generic reporting
            //  m_output->EndTag(AZ_CRC("GridSearch"));
            //}
            //m_output->EndTag(AZ_CRC("StartDrill"));
            //m_output->EndTag(m_drillerTag);
        }

        //=========================================================================
        // Stop
        // [4/14/2011]
        //=========================================================================
        void SessionDriller::Stop()
        {
            SessionDrillerBus::Handler::BusDisconnect();
        }

        //=========================================================================
        // OnSessionServiceReady
        // [4/15/2011]
        //=========================================================================
        void
        SessionDriller::OnSessionServiceReady()
        {
            //  m_output->BeginTag(m_drillerTag);
            //  m_output->EndTag(m_drillerTag);
        }

        //=========================================================================
        // OnGridSearchComplete
        // [4/15/2011]
        //=========================================================================
        void
        SessionDriller::OnGridSearchComplete(GridSearch* gridSearch)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->BeginTag(AZ_CRC("GridSearchComplete", 0x974b5717));
            m_output->Write(AZ_CRC("SearchId", 0x4f7ef2d2), gridSearch);
            m_output->Write(AZ_CRC("NumResults", 0xdfb1542f), gridSearch->GetNumResults());
            // add platform specific drill or just generic reporting
            m_output->EndTag(AZ_CRC("GridSearchComplete", 0x974b5717));
            m_output->EndTag(m_drillerTag);
        }

        //=========================================================================
        // OnMemberJoined
        // [4/15/2011]
        //=========================================================================
        void
        SessionDriller::OnMemberJoined(GridSession* session, GridMember* member)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->BeginTag(AZ_CRC("MemberJoined", 0xbde4706c));
            m_output->Write(AZ_CRC("SessionId", 0xacd49154), session->GetId());
            m_output->Write(AZ_CRC("Id", 0xbf396750), member->GetId().ToString());
            m_output->Write(AZ_CRC("Name", 0x5e237e06), member->GetName());
            m_output->Write(AZ_CRC("ConnectionId", 0x4592a200), member->GetConnectionId());
            m_output->Write(AZ_CRC("NAT", 0x9686d0fb), (char)member->GetNatType());
            //m_output->Write(AZ_CRC("MuteList"),member->GetMuteList());
            m_output->Write(AZ_CRC("IsHost", 0xce28a9cf), member->IsHost());
            m_output->Write(AZ_CRC("IsLocal", 0x4300d6d2), member->IsLocal());
            m_output->Write(AZ_CRC("IsInvited", 0x29d785f7), member->IsInvited());
            m_output->EndTag(AZ_CRC("MemberJoined", 0xbde4706c));
            m_output->EndTag(m_drillerTag);
        }

        //=========================================================================
        // OnMemberLeaving
        // [4/15/2011]
        //=========================================================================
        void
        SessionDriller::OnMemberLeaving(GridSession* session, GridMember* member)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->BeginTag(AZ_CRC("MemberLeaving", 0xd10ee176));
            m_output->Write(AZ_CRC("SessionId", 0xacd49154), session->GetId());
            m_output->Write(AZ_CRC("Id", 0xbf396750), member->GetId().ToString());
            m_output->EndTag(AZ_CRC("MemberLeaving", 0xd10ee176));
            m_output->EndTag(m_drillerTag);
        }

        //=========================================================================
        // OnMemberKicked
        // [4/15/2011]
        //=========================================================================
        void
        SessionDriller::OnMemberKicked(GridSession* session, GridMember* member)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->BeginTag(AZ_CRC("MemberKicked", 0x908e74e6));
            m_output->Write(AZ_CRC("SessionId", 0xacd49154), session->GetId());
            m_output->Write(AZ_CRC("Id", 0xbf396750), member->GetId().ToString());
            m_output->EndTag(AZ_CRC("MemberKicked", 0x908e74e6));
            m_output->EndTag(m_drillerTag);
        }

        //=========================================================================
        // OnSessionCreated
        // [4/15/2011]
        //=========================================================================
        void
        SessionDriller::OnSessionCreated(GridSession* session)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->BeginTag(AZ_CRC("SessionCreated", 0x24655a62));
            m_output->Write(AZ_CRC("SessionId", 0xacd49154), session->GetId());
            m_output->Write(AZ_CRC("Carrier", 0x4739f11c), session->GetCarrier());
            m_output->Write(AZ_CRC("ReplicaMgr", 0x41cf3853), session->GetReplicaMgr());
            m_output->Write(AZ_CRC("Topology", 0x1198610c), (char)session->GetTopology());
            m_output->Write(AZ_CRC("Time", 0x6f949845), session->GetTime());
            m_output->Write(AZ_CRC("State", 0xa393d2fb), (char)session->m_sm.GetCurrentState());
            m_output->Write(AZ_CRC("IsHost", 0xce28a9cf), session->IsHost());
            m_output->EndTag(AZ_CRC("SessionCreated", 0x24655a62));
            m_output->EndTag(m_drillerTag);
        }
        //=========================================================================
        // OnSessionJoined
        // [4/15/2011]
        //=========================================================================
        void
        SessionDriller::OnSessionJoined(GridSession* session)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->BeginTag(AZ_CRC("SessionJoined", 0x04b85d49));
            m_output->Write(AZ_CRC("SessionId", 0xacd49154), session->GetId());
            m_output->Write(AZ_CRC("Carrier", 0x4739f11c), session->GetCarrier());
            m_output->Write(AZ_CRC("ReplicaMgr", 0x41cf3853), session->GetReplicaMgr());
            m_output->Write(AZ_CRC("Topology", 0x1198610c), (char)session->GetTopology());
            m_output->Write(AZ_CRC("Time", 0x6f949845), session->GetTime());
            m_output->Write(AZ_CRC("State", 0xa393d2fb), (char)session->m_sm.GetCurrentState());
            m_output->Write(AZ_CRC("IsHost", 0xce28a9cf), session->IsHost());
            m_output->EndTag(AZ_CRC("SessionJoined", 0x04b85d49));
            m_output->EndTag(m_drillerTag);
        }
        //=========================================================================
        // OnSessionDelete
        // [4/15/2011]
        //=========================================================================
        void
        SessionDriller::OnSessionDelete(GridSession* session)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->Write(AZ_CRC("SessionDelete", 0x6b5728cd), session->GetId());
            m_output->EndTag(m_drillerTag);
        }
        //=========================================================================
        // OnSessionError
        // [4/15/2011]
        //=========================================================================
        void
        SessionDriller::OnSessionError(GridSession* session, const string& errorMsg)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->BeginTag(AZ_CRC("SessionError", 0xc689cc40));
            m_output->Write(AZ_CRC("SessionId", 0xacd49154), session ? session->GetId() : "NoId");
            m_output->Write(AZ_CRC("Error", 0x5dddbc71), errorMsg);
            m_output->EndTag(AZ_CRC("SessionError", 0xc689cc40));
            m_output->EndTag(m_drillerTag);
        }
        //=========================================================================
        // OnSessionStart
        // [4/15/2011]
        //=========================================================================
        void
        SessionDriller::OnSessionStart(GridSession* session)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->Write(AZ_CRC("SessionStart", 0x042d25be), session->GetId());
            m_output->EndTag(m_drillerTag);
        }
        //=========================================================================
        // OnSessionEnd
        // [4/15/2011]
        //=========================================================================
        void
        SessionDriller::OnSessionEnd(GridSession* session)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->Write(AZ_CRC("SessionEnd", 0x07821a5e), session->GetId());
            m_output->EndTag(m_drillerTag);
        }
        //=========================================================================
        // OnWriteStatistics
        // [6/8/2011]
        //=========================================================================
        void
        SessionDriller::OnWriteStatistics(GridSession* session, GridMember* member, StatisticsData& data)
        {
            m_output->BeginTag(m_drillerTag);
            m_output->BeginTag(AZ_CRC("WriteStatistics", 0xcf7f12aa));
            m_output->Write(AZ_CRC("SessionId", 0xacd49154), session->GetId());
            m_output->Write(AZ_CRC("Id", 0xbf396750), member->GetId().ToString());
            // data...
            (void)data;
            m_output->EndTag(AZ_CRC("WriteStatistics", 0xcf7f12aa));
            m_output->EndTag(m_drillerTag);
        }
    } // namespace Debug
} // namespace GridMate
