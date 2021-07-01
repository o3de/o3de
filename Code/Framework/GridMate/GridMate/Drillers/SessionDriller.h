/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_SESSION_DRILLER_H
#define GM_SESSION_DRILLER_H

#include <GridMate/Types.h>
#include <GridMate/Session/Session.h>
#include <AzCore/Driller/Driller.h>

namespace GridMate
{
    namespace Debug
    {
        /**
         * Session Driller
         *  \note Be careful which buses you attach. The drillers work in Multi threaded environment and expect that
         * a driller mutex (DrillerManager::DrillerManager) will be automatically locked on every write.
         * Otherwise in output stream corruption will happen (even is the stream is thread safe).
         */
        class SessionDriller
            : public AZ::Debug::Driller
            , public SessionDrillerBus::Handler
        {
            int m_drillerTag;
        public:
            AZ_CLASS_ALLOCATOR(SessionDriller, AZ::OSAllocator, 0);
            SessionDriller();

            //////////////////////////////////////////////////////////////////////////
            // Driller
            virtual const char*  GroupName() const          { return "GridMate"; }
            virtual const char*  GetName() const            { return "SessionDriller"; }
            virtual const char*  GetDescription() const     { return "Drills GridSession, Search, etc."; }
            virtual void         Start(const Param* params = NULL, int numParams = 0);
            virtual void         Stop();
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Session Event Bus
            /// Callback that is called when the Session service is ready to process sessions.
            virtual void OnSessionServiceReady();
            //virtual OnCommucationChanged() = 0  Callback that notifies the title when a member's communication settings change.
            /// Callback that notifies the title when a game search query have completed.
            virtual void OnGridSearchComplete(GridSearch* gridSearch);
            /// Callback that notifies the title when a new member joins the game session.
            virtual void OnMemberJoined(GridSession* session, GridMember* member);
            /// Callback that notifies the title that a member is leaving the game session. member pointer is NOT valid after the callback returns.
            virtual void OnMemberLeaving(GridSession* session, GridMember* member);
            // \todo a better way will be (after we solve migration) is to supply a reason to OnMemberLeaving... like the member was kicked.
            // this will require that we actually remove the replica at the same moment.
            /// Callback that host decided to kick a member. You will receive a OnMemberLeaving when the actual member leaves the session.
            virtual void OnMemberKicked(GridSession* session, GridMember* member);
            /// After this callback it is safe to access session features. If host session is fully operational if client wait for OnSessionJoined.
            virtual void OnSessionCreated(GridSession* session);
            /// Called on client machines to indicate that we join successfully.
            virtual void OnSessionJoined(GridSession* session);
            /// Callback that notifies the title when a session will be left. session pointer is NOT valid after the callback returns.
            virtual void OnSessionDelete(GridSession* session);
            /// Called when a session error occurs.
            virtual void OnSessionError(GridSession* session, const string& errorMsg);
            /// Called when the actual game(match) starts
            virtual void OnSessionStart(GridSession* session);
            /// Called when the actual game(match) ends
            virtual void OnSessionEnd(GridSession* session);
            /// Called when we have our last chance to write statistics data for member in the session.
            virtual void OnWriteStatistics(GridSession* session, GridMember* member, StatisticsData& data);
            //////////////////////////////////////////////////////////////////////////
        };
    }
}

#endif // GM_SESSION_DRILLER_H
