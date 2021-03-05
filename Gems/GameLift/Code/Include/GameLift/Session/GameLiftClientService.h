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
#pragma once

#if defined(BUILD_GAMELIFT_CLIENT)

#include <AzCore/PlatformDef.h>
// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.
AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/Aws.h>
AZ_POP_DISABLE_WARNING
#include <GameLift/Session/GameLiftClientServiceBus.h>
#include <GameLift/Session/GameLiftClientServiceEventsBus.h>
#include <GameLift/Session/GameLiftSessionDefs.h>
#include <GameLift/Session/GameLiftRequestInterface.h>

#include <GridMate/Session/Session.h>
#include <AzCore/std/smart_ptr/scoped_ptr.h>

#include <aws/gamelift/GameLiftClient.h>

namespace GridMate
{
    class GameLiftSession;
    struct GameLiftSessionRequestParams;
    class GameLiftSearch;
    class GameLiftSessionRequest;

    /*!
    * GameLift session service settings.
    */
    struct GameLiftClientServiceDesc
        : public SessionServiceDesc
    {
        GameLiftClientServiceDesc()
            : m_useGameLiftLocalServer(false)
        {}

        string m_region; //< AWS Region
        string m_endpoint; //< GameLift endpoint host

        string m_accessKey; //< AWS Access key
        string m_secretKey; //< AWS Secret key

        string m_playerId;

        // Allow client to use a local GameLift server for development.
        bool m_useGameLiftLocalServer; // Default (false) is use SSL validation and HTTPS. A value of true enables the use of the GameLift local server, turns off SSL validation and uses HTTP.

    };

    /*!
    * GameLift session service.
    */
    class GameLiftClientService
        : public SessionService
        , public GameLiftClientServiceBus::Handler
    {
    public:

        GM_CLASS_ALLOCATOR(GameLiftClientService);
        GRIDMATE_SERVICE_ID(GameLiftClientService);

        GameLiftClientService(const GameLiftClientServiceDesc& desc);
        ~GameLiftClientService() override;

        bool IsReady() const;
        AZStd::shared_ptr<Aws::GameLift::GameLiftClient> GetClient() const;
        Aws::String GetPlayerId() const;
        bool UseGameLiftLocal() const { return m_serviceDesc.m_useGameLiftLocalServer; }
        string GetEndpoint() const { return m_serviceDesc.m_endpoint; }

        // SessionService
        void OnServiceRegistered(IGridMate* gridMate) override;
        void OnServiceUnregistered(IGridMate* gridMate) override;
        void Update() override;

        // GameLiftClientServiceBus implementation
        GridSession* JoinSessionBySearchInfo(const GameLiftSearchInfo& params, const CarrierDesc& carrierDesc) override;
        GridSearch* RequestSession(const GameLiftSessionRequestParams& params) override;
        GridSearch* StartMatchmaking(const AZStd::string& matchmakingConfig) override;
        GameLiftSearch* StartSearch(const GameLiftSearchParams& params) override;
        GameLiftClientSession* QueryGameLiftSession(const GridSession* session) override;
        GameLiftSearch* QueryGameLiftSearch(const GridSearch* search) override;

    protected:
        bool StartGameLiftClient();
        virtual void CreateSharedAWSGameLiftClient();
        virtual bool ValidateAWSCredentials();

        enum GameLiftStatus
        {
            GameLift_NotInited, ///< GameLift SDK is not initialized
            GameLift_Initing, ///< Pending GameLift SDK initialization
            GameLift_Ready, ///< GameLift SDK is ready to use
            GameLift_Failed, ///< GameLift SDK failed to initialize
            GameLift_Terminated ///< Current instance was force terminated by the user (only applies to the server)
        };

        GameLiftClientServiceDesc m_serviceDesc;
        GameLiftStatus m_clientStatus;
        AZStd::shared_ptr<Aws::GameLift::GameLiftClient> m_clientSharedPtr;
        Aws::GameLift::Model::ListBuildsOutcomeCallable m_listBuildsOutcomeCallable;
        Aws::SDKOptions m_optionsSdk;

    };
} // namespace GridMate

#endif // BUILD_GAMELIFT_CLIENT
