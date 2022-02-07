/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            const char*  GroupName() const override          { return "GridMate"; }
            const char*  GetName() const override            { return "SessionDriller"; }
            const char*  GetDescription() const override     { return "Drills GridSession, Search, etc."; }
            void         Start(const Param* params = NULL, int numParams = 0) override;
            void         Stop() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Session Event Bus
            /// Callback that is called when the Session service is ready to process sessions.
            void OnSessionServiceReady() override;
            //virtual OnCommucationChanged() = 0  Callback that notifies the title when a member's communication settings change.
            /// Callback that notifies the title when a game search query have completed.
            void OnGridSearchComplete(GridSearch* gridSearch) override;
            /// Callback that notifies the title when a new member joins the game session.
            void OnMemberJoined(GridSession* session, GridMember* member) override;
            /// Callback that notifies the title that a member is leaving the game session. member pointer is NOT valid after the callback returns.
            void OnMemberLeaving(GridSession* session, GridMember* member) override;
            // \todo a better way will be (after we solve migration) is to supply a reason to OnMemberLeaving... like the member was kicked.
            // this will require that we actually remove the replica at the same moment.
            /// Callback that host decided to kick a member. You will receive a OnMemberLeaving when the actual member leaves the session.
            void OnMemberKicked(GridSession* session, GridMember* member) override;
            /// After this callback it is safe to access session features. If host session is fully operational if client wait for OnSessionJoined.
            void OnSessionCreated(GridSession* session) override;
            /// Called on client machines to indicate that we join successfully.
            void OnSessionJoined(GridSession* session) override;
            /// Callback that notifies the title when a session will be left. session pointer is NOT valid after the callback returns.
            void OnSessionDelete(GridSession* session) override;
            /// Called when a session error occurs.
            void OnSessionError(GridSession* session, const AZStd::string& errorMsg) override;
            /// Called when the actual game(match) starts
            void OnSessionStart(GridSession* session) override;
            /// Called when the actual game(match) ends
            void OnSessionEnd(GridSession* session) override;
            /// Called when we have our last chance to write statistics data for member in the session.
            void OnWriteStatistics(GridSession* session, GridMember* member, StatisticsData& data) override;
            //////////////////////////////////////////////////////////////////////////
        };
    }
}

#endif // GM_SESSION_DRILLER_H
