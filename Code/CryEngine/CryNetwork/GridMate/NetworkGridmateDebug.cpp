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
#include "CryNetwork_precompiled.h"

#include "NetworkGridMate.h"
#include "NetworkGridmateDebug.h"

#include "Replicas/EntityReplica.h"
#include <AzCore/PlatformIncl.h>

#include <CryPath.h>
#include <ILevelSystem.h>

#include <stdio.h>
#include <IRenderer.h>
#include <ITextModeConsole.h>

namespace
{
    /**
     * Helper for debug text printing, with colorization and formatting options.
     *
     * Example:
     *
     *  DebugTextHelper text( gEnv->pRenderer, 100, 100 ); // At screen pos (100,100)
     *  text.SetAutoNewlined( true );
     *  text.SetMonospaced( true );
     *  text.SetColor( Col_Yellow );
     *  text.SetSize( 2.f );
     *  text.AddText( "This is a yellow title" );
     *  text.SetSize( 1.5f );
     *  text.AddText( "This is some detail." );
     */
    class DebugTextHelper
    {
    public:
        static const int kTextModeRowSize = 10;
        static const int kTextModeColSize = 10;
        static const int kTextModeColCount = 128;   // WINDOWS_CONSOLE_WIDTH
        static const int kTextModeRowCount = 48;    // WINDOWS_CONSOLE_HEIGHT - 2

        DebugTextHelper(IRenderer* r,
            float x = 0.f, float y = 0.f,
            float fontSize = 1.5f,
            ColorF defaultColor = Col_White)
            : m_pos(x, y, 0.f)
            , m_defaultColor(defaultColor)
            , m_fontSize(fontSize)
            , m_flags(kFlag_AutoNewline | kFlag_Monospaced | kFlag_TextModeConsole)
            , m_renderer(r)
        {
        }

        ~DebugTextHelper() {};

        enum
        {
            kMaxLabelSize = 512
        };

        void AddText(const char* format, ...)
        {
            va_list argList;
            char buffer[kMaxLabelSize];
            va_start(argList, format);
            const int len = vsnprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1, format, argList);
            buffer[sizeof(buffer) - 1] = '\0';
            va_end(argList);

            AddText(m_defaultColor, buffer);
        }

        void AddText(ColorF color, const char* format, ...)
        {
            va_list argList;
            char buffer[kMaxLabelSize];
            va_start(argList, format);
            const int len = vsnprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1, format, argList);
            buffer[sizeof(buffer) - 1] = '\0';
            va_end(argList);

            const uint32 drawFlags = IsMonospaced() ?
                (eDrawText_2D | eDrawText_800x600 | eDrawText_FixedSize | eDrawText_Monospace) :
                (eDrawText_2D | eDrawText_800x600 | eDrawText_FixedSize);

            m_renderer->Draw2dLabelWithFlags(m_pos.x, m_pos.y, m_fontSize, color, drawFlags, buffer);

            if (m_flags & kFlag_TextModeConsole)
            {
                if (ITextModeConsole* textConsole = gEnv->pSystem->GetITextModeConsole())
                {
                    const int posX = static_cast<int>(m_pos.x) / kTextModeColSize;
                    const int posY = static_cast<int>(m_pos.y) / kTextModeRowSize;

                    if (posX < kTextModeColCount && posY < kTextModeRowCount)
                    {
                        textConsole->PutText(posX, posY, buffer);
                    }
                }
            }

            if (m_flags & kFlag_AutoNewline)
            {
                Newline();
            }
        }

        void ClearLines(float startY, float height)
        {
            const int startRow = std::min(static_cast<int>(startY) / kTextModeRowSize, int(kTextModeRowCount));
            const int numRows = std::min(static_cast<int>(height) / kTextModeRowSize, int(kTextModeRowCount));

            // Text mode console requires wiping the frame buffer to maintain a reasonable
            // quality display. The frame profiler doesn't do this, and as a result is
            // sometimes completely unreadable. It's very slow to set characters in the
            // console, so for now, we're wiping in scanline fashion.
            if (ITextModeConsole* textConsole = gEnv->pSystem->GetITextModeConsole())
            {
                static char s_emptyLine[kTextModeColCount + 1] = { '\0' };
                if (0 == s_emptyLine[0])
                {
                    for (int col = 0; col < kTextModeColCount; ++col)
                    {
                        s_emptyLine[col] = ' ';
                    }

                    s_emptyLine[kTextModeColCount] = 0;
                }

                for (int row = startRow; row < numRows; ++row)
                {
                    textConsole->PutText(0, row, s_emptyLine);
                }
            }
        }

        inline ColorF GetDefaultColor() const { return m_defaultColor; }
        inline void SetDefaultColor(ColorF color) { m_defaultColor = color; }

        inline Vec3 GetPosition() const { return m_pos; }
        inline void SetPosition(const Vec3& pos) { m_pos = pos; }

        inline float GetFontSize() const { return m_fontSize; }
        inline void SetFontSize(float fontSize) { m_fontSize = fontSize; }

        inline void Newline() { m_pos.y += m_fontSize * 10.f; }

        inline bool IsAutoNewlined() const { return !!(m_flags & kFlag_AutoNewline); }
        inline bool IsMonospaced() const { return !!(m_flags & kFlag_Monospaced); }

        inline void SetAutoNewlined(bool set)
        {
            if (set)
            {
                m_flags |= kFlag_AutoNewline;
            }
            else
            {
                m_flags &= ~kFlag_AutoNewline;
            }
        }

        inline void SetMonospaced(bool set)
        {
            if (set)
            {
                m_flags |= kFlag_Monospaced;
            }
            else
            {
                m_flags &= ~kFlag_Monospaced;
            }
        }

        inline void SetTextModeConsole(bool set)
        {
            if (set)
            {
                m_flags |= kFlag_TextModeConsole;
            }
            else
            {
                m_flags &= ~kFlag_TextModeConsole;
            }
        }

    private:

        Vec3            m_pos;
        ColorF          m_defaultColor;

        float           m_fontSize;

        enum Flags
        {
            kFlag_AutoNewline = (1 << 0),     // Auto advance to next line after AddText().
            kFlag_Monospaced = (1 << 1),     // Print out using monospaced font.
            kFlag_TextModeConsole = (1 << 2),     // Write to "text mode console" (dedicated server).
        };

        uint32          m_flags;

        IRenderer*      m_renderer;
    };
}

namespace GridMate
{
    namespace Debug
    {
        const char* const GetAspectNameByBitIndex(size_t aspectIndex)
        {
            static const char* AspectNames[] =
            {
                #define ADD_ASPECT(x, y)   #x,
                #include "Compatibility/GridMateNetSerializeAspects.inl"
                #undef ADD_ASPECT
            };

            STATIC_ASSERT(AZ_ARRAY_SIZE(AspectNames) <= NetSerialize::kNumAspectSlots,
                "Too many Engine aspects for the replica.");

            if (aspectIndex >= AZ_ARRAY_SIZE(AspectNames))
            {
                return "<invalid aspect index>";
            }

            return AspectNames[ aspectIndex ];
        }

        #if GRIDMATE_DEBUG_ENABLED

        int s_DebugDraw = 0;
        int s_TraceLevel = 0;
        int s_EnableAsserts = 0;

        struct TrackedDebugMsg
        {
            typedef CryFixedStringT<256> StringStorage;

            TrackedDebugMsg(DebugMessageType type, const char* msg)
                : m_type(type)
                , m_string(msg)
            {
                #ifdef WIN32
                time(&m_time);
                #endif // WIN32
            }

            #ifdef WIN32
            time_t m_time;
            #endif // WIN32

            DebugMessageType m_type;
            StringStorage m_string;
        };

        AZStd::vector<TrackedDebugMsg, AZ::StdLegacyAllocator> s_trackedMessages;
        CryCriticalSection s_debugLock;

        void TrackMessage(DebugMessageType type, const char* msg)
        {
            enum
            {
                kMaxTrackedMessages = 20
            };

            while (s_trackedMessages.size() >= kMaxTrackedMessages)
            {
                s_trackedMessages.erase(s_trackedMessages.begin());
            }

            s_trackedMessages.push_back(TrackedDebugMsg(type, msg));
        }

        void DebugTrace(bool isAssertFailure, const char* format, ...)
        {
            enum
            {
                kMaxTraceMessageSize = 512
            };

            CryAutoLock<CryCriticalSection> lock(s_debugLock);

            va_list    argList;
            char buffer[ kMaxTraceMessageSize ];
            va_start(argList, format);
            const int len = vsnprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1, format, argList);
            buffer[ sizeof(buffer) - 1 ] = '\0';
            va_end(argList);

            if (isAssertFailure)
            {
                CryWarning(VALIDATOR_MODULE_NETWORK, VALIDATOR_ERROR, "<GridMate Assert> %s", buffer);
                TrackMessage(DebugMessageType::kAssert, buffer);

                if (s_EnableAsserts)
                {
                    CryDebugBreak();
                }
            }
            else
            {
                CryLog("<GridMate Trace> %s", buffer);
                TrackMessage(DebugMessageType::kTrace, buffer);
            }
        }

        //-----------------------------------------------------------------------------
        static void CmdSetDebugDraw(IConsoleCmdArgs* args)
        {
            if (args->GetArgCount() > 1)
            {
                using namespace CryStringUtils;

                char* items = const_cast<char*>(args->GetArg(1));

                int value = 0;

                const char* delims = "+";
                char* nextToken = nullptr;
                char* token = azstrtok(items, 0, delims, &nextToken);
                while (token)
                {
                    if (nullptr != stristr(token, "basic"))
                    {
                        value |= Debug::Basic;
                    }
                    if (nullptr != stristr(token, "trace"))
                    {
                        value |= Debug::Trace;
                    }
                    if (nullptr != stristr(token, "stat"))
                    {
                        value |= Debug::Stats;
                    }
                    if (nullptr != stristr(token, "rep"))
                    {
                        value |= Debug::Replicas;
                    }
                    if (nullptr != stristr(token, "act"))
                    {
                        value |= Debug::Actors;
                    }
                    if (nullptr != stristr(token, "detail"))
                    {
                        value |= Debug::EntityDetail;
                    }
                    if (nullptr != stristr(token, "full"))
                    {
                        value = Debug::Full;
                        break;
                    }
                    token = azstrtok(nullptr, 0, delims, &nextToken);
                }

                Debug::s_DebugDraw = value;
            }
            else
            {
                Debug::s_DebugDraw = Debug::Full;
            }
        }

        //-----------------------------------------------------------------------------
        static void OnDumpStatsChanged(ICVar* /*cvar*/)
        {
            if (Network::s_DumpStatsFile)
            {
                fclose(Network::s_DumpStatsFile);
                Network::s_DumpStatsFile = nullptr;
            }

            ICVar* cvarFilename = gEnv->pConsole->GetCVar("gm_dumpstats_file");

            if (Network::s_DumpStatsEnabled > 0 &&
                cvarFilename && cvarFilename->GetString() && cvarFilename->GetString()[0])
            {
                const CryStringT<char> logFile = PathUtil::Make(
                    "@log@",
                    PathUtil::GetFile(cvarFilename->GetString()));

                char resolvedPath[MAX_PATH] = { 0 };
                gEnv->pFileIO->ResolvePath(logFile.c_str(), resolvedPath, MAX_PATH);

                Network::s_DumpStatsFile = nullptr;
                azfopen(&Network::s_DumpStatsFile, resolvedPath, "wt");
            }
        }

        void RegisterCVars()
        {
            REGISTER_CVAR2("gm_debugdraw", &s_DebugDraw, s_DebugDraw, VF_DEV_ONLY, "GridMate debugging visualization level.");
            REGISTER_CVAR2("gm_tracelevel", &s_TraceLevel, s_TraceLevel, VF_DEV_ONLY, "GridMate debugging trace verbosity level.");
            REGISTER_CVAR2("gm_asserts", &s_EnableAsserts, s_EnableAsserts, VF_DEV_ONLY, "GridMate asserts.");
            REGISTER_COMMAND("gm_setdebugdraw", CmdSetDebugDraw, VF_DEV_ONLY,
                "Helper for setting up debug draw level: e.g. gm_setdebugdraw Basic+Stats+Trace."
                "Options are Basic, Trace, Stats, Replicas, and Actors.");

            // Profiling commands.
            REGISTER_CVAR2_CB("gm_dumpstats", &Network::s_DumpStatsEnabled, Network::s_DumpStatsEnabled, VF_DEV_ONLY,
                "Enable dumping of net profiling stats to file.", OnDumpStatsChanged);
            REGISTER_STRING_CB("gm_dumpstats_file", "net_profile.log", VF_DEV_ONLY,
                "Target file for net profiling stats.", OnDumpStatsChanged);
            REGISTER_CVAR2("gm_stats_interval_msec", &Network::s_StatsIntervalMS, Network::s_StatsIntervalMS, VF_DEV_ONLY,
                "Net profiling statistics will be gathered on this interval (in milliseconds). "
                "If stats are being dumped to file, it will also occur on this interval.");

        }

        void UnregisterCVars()
        {
            UNREGISTER_CVAR("gm_stats_interval_msec");
            UNREGISTER_CVAR("gm_dumpstats_file");
            UNREGISTER_CVAR("gm_dumpstats");

            if (gEnv->pConsole)
            {
                gEnv->pConsole->RemoveCommand("gm_setdebugdraw");
            }

            UNREGISTER_CVAR("gm_setdebugdraw");
            UNREGISTER_CVAR("gm_asserts");
            UNREGISTER_CVAR("gm_tracelevel");
            UNREGISTER_CVAR("gm_debugdraw");
        }

        #endif // GRIDMATE_DEBUG_ENABLED
    } // namespace Debug

    //-----------------------------------------------------------------------------
    void Network::DebugDraw()
    {
        #if GRIDMATE_DEBUG_ENABLED

        using namespace Debug;

        if (0 == Debug::s_DebugDraw)
        {
            return;
        }

        auto* levelSystem = GetLevelSystem();

        static const float startX = 50.f;
        static const float startY = 50.f;
        static const float columnWidth = 500.f;

        DebugTextHelper text(gEnv->pRenderer, startX, startY, 1.2f);
        text.SetMonospaced(true);

        text.ClearLines(startY, gEnv->pRenderer->GetHeight() - startY);

        text.AddText(Col_Yellow, "=== GridMate ===");

        text.Newline();
        text.AddText(Col_Coral, "[Status]");
        text.AddText(Col_White, "%-20s %s", "Is Server?", gEnv->bServer ? "yes" : "no");
        text.AddText(Col_White, "%-20s %s", "Is Multiplayer?", gEnv->bMultiplayer ? "yes" : "no");

        text.AddText(Col_White, "%-20s %u", "Local Channel", m_localChannelId);

        string sessionStatusStr = "(none)";
        if (m_session)
        {
            sessionStatusStr = "Multiplayer";

            if (m_session->IsHost())
            {
                sessionStatusStr += " hosted";
            }
            else
            {
                sessionStatusStr += " joined";
            }
        }

        text.AddText(Col_White, "%-20s %s", "Session Status", sessionStatusStr.c_str());

        text.AddText(Col_White, "%-20s %s", "Current Level", (levelSystem && levelSystem->GetCurrentLevel()) ?
            levelSystem->GetCurrentLevel()->GetLevelInfo()->GetName() : "(none)");

        if (m_session)
        {
            text.Newline();

            const char* sessionType = (m_session->IsHost()) ? "Server" : "Client";
            text.AddText(Col_Coral, "[Session - %s]", sessionType);

            if (!m_session->IsHost())
            {
                text.AddText(Col_White, "%-20s %u", "Server Channel",
                    GetServerChannelId());
            }

            text.AddText(Col_White, "%-20s %u", "Members", m_session->GetNumberOfMembers());
        }

        if (!!(s_DebugDraw & Stats) && !(s_DebugDraw & EntityDetail))
        {
            text.Newline();
            text.AddText(Col_Coral, "[Overview (last %d msec)]", Network::s_StatsIntervalMS);
            text.AddText(Col_LightBlue, "%-20s %-10s %-12s %-14s %-14s %-10s %-10s", "To Channel", "RTT", "Packet Loss", "Data Sent(kb)", "Data Recv(kb)", "Pack Sent", "Pack Recv");

            for (const auto& stat : m_statisticsPerChannel)
            {
                text.AddText(Col_White, "%-20u %-10.2f %-12.2f %-14.2f %-14.2f %-10u %-10u",
                    stat.first, stat.second.m_rtt, stat.second.m_packetLossRate,
                    float( stat.second.m_totalSentBytes ) / 1024.f, float( stat.second.m_totalReceivedBytes ) / 1024.f,
                    stat.second.m_packetsSent, stat.second.m_packetsReceived);
            }

            const auto& stats = GetGameStatistics();
            auto& rmiActor = stats.m_rmiGlobalActor;
            auto& rmiLegacy = stats.m_rmiGlobalLegacy;
            auto& rmiScript = stats.m_rmiGlobalScript;

            text.Newline();
            text.AddText(Col_Coral, "[Lifetime RMI]");
            text.AddText(Col_LightBlue, "%-14s %-14s %-14s %-14s",
                "Num Sent", "Num Received", "Total Sent(kb)", "Total Received(kb)");
            text.AddText(Col_White, "%-14u %-14u %-14.2f %-14.2f",
                rmiActor.m_sendCount + rmiLegacy.m_sendCount + rmiScript.m_sendCount,
                rmiActor.m_receiveCount + rmiLegacy.m_receiveCount + rmiScript.m_receiveCount,
                float( rmiActor.m_totalSentBytes + rmiLegacy.m_totalSentBytes + rmiScript.m_totalSentBytes ) / 1024.f,
                float( rmiActor.m_totalReceivedBytes + rmiLegacy.m_totalReceivedBytes + rmiScript.m_totalReceivedBytes ) / 1024.f);

            text.Newline();
            text.AddText(Col_Coral, "[Lifetime Aspects]");
            text.AddText(Col_LightBlue, "%-14s %-14s %-14s %-14s",
                "Num Sent", "Num Received", "Total Sent(kb)", "Total Received(kb)");
            text.AddText(Col_White, "%-14u %-14u %-14.2f %-14.2f",
                stats.m_aspectsSent, stats.m_aspectsReceived,
                float( stats.m_aspectSentBytes ) / 1024.f,
                float( stats.m_aspectReceivedBytes ) / 1024.f);
        }

        if (!!(s_DebugDraw & Replicas) && !(s_DebugDraw & EntityDetail))
        {
            text.Newline();
            text.AddText(Col_Coral, "[Entity Replicas By Type]");
            text.AddText(Col_LightBlue, "%-20s %-10s", "Entity Class", "Count");

        }

        if (!!(s_DebugDraw & Actors) && !(s_DebugDraw & EntityDetail))
        {
            text.Newline();
            text.AddText(Col_Coral, "[Game Actors]");
            text.AddText(Col_LightBlue, "%-20s %-10s %-10s %-15s %-10s", "Name", "Channel", "Entity Id", "Client Actor?", "Player?");
        }

        if (!!(s_DebugDraw & Trace) && !(s_DebugDraw & EntityDetail))
        {
            text.Newline();
            text.AddText(Col_Coral, "[Trace]");

            for (auto iter = s_trackedMessages.rbegin(); iter != s_trackedMessages.rend(); ++iter)
            {
                const char* time = "";

                #ifdef WIN32
                char timeFriendly[ 128 ];
                {
                    tm timeStruct;
                    localtime_s(&timeStruct, &iter->m_time);
                    strftime(timeFriendly, 20, "%H:%M:%S", &timeStruct);
                    time = timeFriendly;
                }
                #endif // WIN32

                text.AddText(iter->m_type == DebugMessageType::kAssert ? Col_Red : Col_White,
                    "[%s] %s", time, iter->m_string.c_str());
            }
        }

        #endif // GRIDMATE_DEBUG_ENABLED
    }


    //-----------------------------------------------------------------------------
    void Network::DumpNetworkStatistics()
    {
        #if GRIDMATE_DEBUG_ENABLED

        if (s_DumpStatsFile)
        {
            static uint32 s_actorRMIOverhead = 0;
            static uint32 s_legacyRMIOverhead = 0;
            static uint32 s_scriptRMIOverhead = 0;
            static uint32 s_aspectOverhead = 0;
            static bool s_overheadsComputed = false;

            // Compute some overhead values.
            {
                char tempBuffer[2048];
                GridMate::WriteBufferStaticInPlace buffer(EndianType::BigEndian, tempBuffer, sizeof(tempBuffer));

                if (!s_overheadsComputed)
                {
                    {
                        buffer.Clear();
                        GridMate::RMI::ActorInvocationWrapper::Ptr invocation = new GridMate::RMI::ActorInvocationWrapper();
                        GridMate::RMI::ActorInvocationWrapper::Marshaler().Marshal(buffer, invocation);
                        s_actorRMIOverhead = buffer.Size();
                    }

                    {
                        buffer.Clear();
                        GridMate::RMI::LegacyInvocationWrapper::Ptr invocation = new GridMate::RMI::LegacyInvocationWrapper();
                        GridMate::RMI::LegacyInvocationWrapper::Marshaler().Marshal(buffer, invocation);
                        s_legacyRMIOverhead = buffer.Size();
                    }

                    {
                        buffer.Clear();
                        GridMate::RMI::ScriptInvocationWrapper::Ptr invocation = new GridMate::RMI::ScriptInvocationWrapper();
                        GridMate::RMI::ScriptInvocationWrapper::Marshaler().Marshal(buffer, invocation);
                        s_scriptRMIOverhead = buffer.Size();
                    }

                    {
                        buffer.Clear();
                        GridMate::NetSerialize::AspectSerializeState aspect;
                        GridMate::NetSerialize::AspectSerializeState::Marshaler().Marshal(buffer, aspect);
                        s_aspectOverhead = buffer.Size();
                    }

                    s_overheadsComputed = true;
                }
            }

            fprintf(s_DumpStatsFile, "Last %d msec\n", s_StatsIntervalMS);

            static const char* s_unknown = "<unknown>";

            //
            // Global stats.
            //

            auto& stats = GetGameStatistics();
            auto carrier = GetCarrierStatistics();

            fprintf(s_DumpStatsFile,
                "[Global]\n"
                "Ping\tTotalBytesSent\tTotalBytesRecv\t"
                "TotalPackSent\tTotalPackRecv\t"
                "TotalRMIsSent\tTotalRMIsRecv\t"
                "TotalRMIBytesSent\tTotalRMIBytesRecv\t"
                "TotalAspectsSent\tTotalAspectsRecv\t"
                "TotalAspectBytesSent\tTotalAspectBytesRecv\t"
                "PacketsLost\tPackLossRate\t"
                "ActorRMIOverheadBytes\tLegacyRMIOverheadBytes\tScriptRMIOverheadBytes\tAspectOverheadBytes\n");
            fprintf(s_DumpStatsFile, "%.2f\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%.2f\t%u\t%u\t%u\t%u\n",
                carrier.m_rtt,
                carrier.m_totalSentBytes, carrier.m_totalReceivedBytes,
                carrier.m_packetsSent, carrier.m_packetsReceived,
                stats.m_rmiGlobalActor.m_sendCount + stats.m_rmiGlobalLegacy.m_sendCount + stats.m_rmiGlobalScript.m_sendCount,
                stats.m_rmiGlobalActor.m_receiveCount + stats.m_rmiGlobalLegacy.m_receiveCount + stats.m_rmiGlobalScript.m_receiveCount,
                stats.m_rmiGlobalActor.m_totalSentBytes + stats.m_rmiGlobalLegacy.m_totalSentBytes + stats.m_rmiGlobalScript.m_totalSentBytes,
                stats.m_rmiGlobalActor.m_totalReceivedBytes + stats.m_rmiGlobalLegacy.m_totalReceivedBytes + stats.m_rmiGlobalScript.m_totalReceivedBytes,
                stats.m_aspectsSent, stats.m_aspectsReceived,
                stats.m_aspectSentBytes, stats.m_aspectReceivedBytes,
                carrier.m_packetsLost, carrier.m_packetLossRate,
                s_actorRMIOverhead, s_legacyRMIOverhead, s_scriptRMIOverhead, s_aspectOverhead);

            //
            // Per-entity detail.
            //

            fprintf(s_DumpStatsFile, "\n[Entity Detail]\n");
            fprintf(s_DumpStatsFile, "Entity\tClass\tEventType\tSendCount\tRecvCount\tSentBytes\tRecvBytes\tOverheadBytes\tTotalBytes\n");

            for (const auto& entityEntry : stats.m_entities)
            {
                const EntityId entityId = entityEntry.first;
                const auto& entityStats = entityEntry.second;

                bool hasTrafficData = false;

                if (!entityStats.m_rmiActor.empty() || !entityStats.m_rmiLegacy.empty())
                {
                    hasTrafficData = true;
                }

                if (!hasTrafficData)
                {
                    for (size_t aspectIndex = 0; aspectIndex < AZ_ARRAY_SIZE(entityStats.m_aspects); ++aspectIndex)
                    {
                        const auto& aspectStats = entityStats.m_aspects[ aspectIndex ];

                        if (aspectStats.m_receiveCount + aspectStats.m_sendCount > 0)
                        {
                            hasTrafficData = true;
                        }
                    }
                }

                if (!hasTrafficData)
                {
                    continue;
                }

                for (const auto& rmi : entityStats.m_rmiActor)
                {
                    const uint32 rmiRepId = rmi.first;
                    const GameStatistics::RMIStatistics& rmiStats = rmi.second;

                    const IActorRMIRep* rep = RMI::FindActorRMIRep(rmiRepId);
                    const char* rmiName = rep ? rep->GetDebugName() : s_unknown;

                    const uint32 overhead = s_actorRMIOverhead * (rmiStats.m_sendCount + rmiStats.m_receiveCount);

                    fprintf(s_DumpStatsFile,
                        "\t\tRMI: %s\t%u\t%u\t%u\t%u\t%u\t%u\n",
                        rmiName,
                        rmiStats.m_sendCount, rmiStats.m_receiveCount,
                        rmiStats.m_totalSentBytes, rmiStats.m_totalReceivedBytes,
                        overhead,
                        overhead + rmiStats.m_totalSentBytes + rmiStats.m_totalReceivedBytes);
                }

                for (const auto& rmi : entityStats.m_rmiLegacy)
                {
                    const uint32 rmiRepId = rmi.first;
                    const GameStatistics::RMIStatistics& rmiStats = rmi.second;

                    const uint32 overhead = s_actorRMIOverhead * (rmiStats.m_sendCount + rmiStats.m_receiveCount);

                    fprintf(s_DumpStatsFile,
                        "\t\tRMI: %s\t%u\t%u\t%u\t%u\t%u\t%u\n",
                        s_unknown,
                        rmiStats.m_sendCount, rmiStats.m_receiveCount,
                        rmiStats.m_totalSentBytes, rmiStats.m_totalReceivedBytes,
                        overhead,
                        overhead + rmiStats.m_totalSentBytes + rmiStats.m_totalReceivedBytes);
                }

                for (size_t aspectIndex = 0; aspectIndex < AZ_ARRAY_SIZE(entityStats.m_aspects); ++aspectIndex)
                {
                    const auto& aspectStats = entityStats.m_aspects[ aspectIndex ];

                    if (aspectStats.m_receiveCount + aspectStats.m_sendCount > 0)
                    {
                        const uint32 overhead = s_aspectOverhead * (aspectStats.m_receiveCount + aspectStats.m_sendCount);

                        fprintf(s_DumpStatsFile,
                            "\t\tAspect: %s\t%u\t%u\t%u\t%u\t%u\t%u\n",
                            GridMate::Debug::GetAspectNameByBitIndex(aspectIndex),
                            aspectStats.m_sendCount, aspectStats.m_receiveCount,
                            aspectStats.m_totalSentBytes, aspectStats.m_totalReceivedBytes,
                            overhead,
                            overhead + aspectStats.m_totalSentBytes + aspectStats.m_totalReceivedBytes);
                    }
                }
            }

            fprintf(s_DumpStatsFile, "\n");
        }

        #endif // GRIDMATE_DEBUG_ENABLED
    }
} // namespace GridMate
