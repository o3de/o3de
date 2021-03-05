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
#ifndef INCLUDE_MULTIPLAYERCVARS_HEADER
#define INCLUDE_MULTIPLAYERCVARS_HEADER

#include <MultiplayerGameLiftClient.h>

#include <INetwork.h>
#include <CertificateManager/ICertificateManagerGem.h>
#include <GridMate/Session/Session.h>

struct IConsole;
struct IConsoleCmdArgs;

namespace GridMate
{
    class GridSearch;
}

namespace CertificateManager
{
    class FileDataSource;
}

namespace Multiplayer
{
    /*!
     * GridMate-specific network cvars.
     */
    class MultiplayerCVars
        : public GridMate::SessionEventBus::Handler
    {
    public:

        MultiplayerCVars();
        ~MultiplayerCVars();

        void RegisterCVars();
        void UnregisterCVars();

        void PostInitRegistration();

    private:

        //! Host a session (LAN).
        static void MPHostLANCmd(IConsoleCmdArgs* args);

        //! Attempt to join an existing session (LAN).
        static void MPJoinLANCmd(IConsoleCmdArgs* args);

#if defined(BUILD_GAMELIFT_CLIENT)
        //! Attempt to host a session on GameLift and join it.
        static void MPHostGameLiftCmd(IConsoleCmdArgs* args);

        //! Attempt to join an existing GameLift session.
        static void MPJoinGameLiftCmd(IConsoleCmdArgs* args);

        static void StopGameLiftClient(IConsoleCmdArgs* args);

        static void MPMatchmakingGameLiftCmd(IConsoleCmdArgs* args);
#endif

        //! Shut down current server or client session.
        static void MPDisconnectCmd(IConsoleCmdArgs* args);
        static void VerifyMaxPlayers(ICVar* pVar);

    private:

        void OnGridSearchComplete(GridMate::GridSearch* gridSearch) override;

#if defined(NET_SUPPORT_SECURE_SOCKET_DRIVER)
        static void OnPrivateKeyChanged(ICVar* cvar);
        static void OnCertificateChanged(ICVar* cvar);
        static void OnCAChanged(ICVar* cvar);

        static void CreateFileDataSource();
#endif

        bool m_autoJoin;
        GridMate::GridSearch* m_search;

#if defined(BUILD_GAMELIFT_CLIENT)
        MultiplayerGameLiftClient m_gameLift;
#endif

        static MultiplayerCVars* s_instance;
    };

} // namespace GridMate

#endif // INCLUDE_NETWORKGRIDMATECVARS_HEADER
