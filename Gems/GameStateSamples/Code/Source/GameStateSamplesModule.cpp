/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <GameStateSamples/GameOptionRequestBus.h>
#include <GameStateSamples/GameStateLevelRunning.h>
#include <GameStateSamples/GameStateMainMenu.h>
#include <GameStateSamples/GameStatePrimaryUserSelection.h>
#include <GameStateSamples/GameStateSamples_Traits_Platform.h>

#include <IConsole.h>
#include <IGem.h>

namespace GameStateSamples
{
    using namespace GameState;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! This Gem provides a set of sample game states that can be overridden (or replaced entirely)
    //! in order to customize the functionality as needed for your game. To circumvent this default
    //! set of game states, push a custom game state before GameStateModule::OnCrySystemInitialized
    //! is called, or just don't enable this Gem for your project (only the GameState Gem is needed
    //! if you plan on creating entirely custom game states). The flow of the sample game states in
    //! this Gem is roughly as follows:
    //!
    //! GameStatePrimaryUserSelection
    //!               |
    //!               V
    //!  GameStatePrimaryUserMonitor____
    //!               |                 |
    //!               V                 |
    //!       GameStateMainMenu         |
    //!               |                 |
    //!               V                 |
    //!     GameStateLevelLoading       |
    //!               |                 |
    //!               V                 |
    //!     GameStateLevelRunning       |
    //!               |                 |
    //!               V                 |
    //!     GameStateLevelPaused        |
    //!                                 |
    //! GameStatePrimaryUserSignedOut<--|
    //!                                 |
    //! PrimaryControllerDisconnected<--|
    //!
    class GameStateSamplesModule
        : public CryHooksModule
        , public AZ::TickBus::Handler
        , public GameOptionRequestBus::Handler
    {
    public:
        AZ_RTTI(GameStateSamplesModule, "{FC206260-D188-45A5-8B23-1D7A1DA6E82F}", AZ::Module);
        AZ_CLASS_ALLOCATOR(GameStateSamplesModule, AZ::SystemAllocator, 0);

        GameStateSamplesModule()
            : CryHooksModule()
        {
            m_gameOptions = AZStd::make_shared<GameOptions>();
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext,
                                                         &AZ::ComponentApplicationRequests::GetSerializeContext);
            if (serializeContext)
            {
                m_gameOptions->Reflect(*serializeContext);
            }
            GameOptionRequestBus::Handler::BusConnect();
        }

        ~GameStateSamplesModule()
        {
            GameOptionRequestBus::Handler::BusDisconnect();
            m_gameOptions.reset();
        }

    protected:
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override
        {
            CryHooksModule::OnCrySystemInitialized(system, systemInitParams);

            AZ::TickBus::Handler::BusConnect();
        }

        void OnTick([[maybe_unused]]float deltaTime, [[maybe_unused]]AZ::ScriptTimePoint scriptTimePoint) override
        {
            // Ideally this would be called at startup (either above in OnCrySystemInitialized, or better during AZ system component
            // initialisation), but because the initial game state depends on loading a UI canvas using LYShine we need to wait until
            // the first tick, because LyShine in turn is not properly initialized until UiRenderer::OnBootstrapSceneReady has been
            // called, which doesn't happen until a queued tick event that gets called right at the end of initialisation before we
            // enter the main game loop.
            CreateAndPushInitialGameState();
            AZ::TickBus::Handler::BusDisconnect();
        }

        void CreateAndPushInitialGameState()
        {
            REGISTER_INT("sys_primaryUserSelectionEnabled", 2, VF_NULL,
                         "Controls whether the game forces selection of a primary user at startup.\n"
                         "0 : Skip selection of a primary user at startup on all platform.\n"
                         "1 : Force selection of a primary user at startup on all platforms.\n"
                         "2 : Force selection of a primary user at startup on console platforms (default).\n");
            REGISTER_INT("sys_pauseOnApplicationConstrained", 2, VF_NULL,
                         "Controls whether the game should pause when the application is constrained.\n"
                         "0 : Don't pause the game when the application is constrained on any platform.\n"
                         "1 : Pause the game when the application is constrained on all platforms.\n"
                         "2 : Pause the game when the application is constrained on console platforms (default).\n");
            REGISTER_INT("sys_localUserLobbyEnabled", 2, VF_NULL,
                         "Controls whether the local user lobby should be enabled.\n"
                         "0 : Don't enable the local user lobby on any platform.\n"
                         "1 : Enable the local user lobby on all platforms.\n"
                         "2 : Enable the local user lobby on console platforms (default).\n");

            if (gEnv && gEnv->IsEditor())
            {
                // Don't push any game states when running in the editor
                return;
            }

            AZStd::shared_ptr<IGameState> activeGameState;
            GameStateRequestBus::BroadcastResult(activeGameState, &GameStateRequests::GetActiveGameState);
            if (activeGameState)
            {
                // The game has pushed a custom initial game state
                return;
            }

            bool primaryUserSelectionEnabled = false;
        #if AZ_TRAIT_GAMESTATESAMPLES_PRIMARY_USER_SELECTION_ENABLED
            primaryUserSelectionEnabled = true;
        #else
            primaryUserSelectionEnabled = false;
        #endif // AZ_TRAIT_GAMESTATESAMPLES_PRIMARY_USER_SELECTION_ENABLED
            if (gEnv && gEnv->pConsole && gEnv->pConsole->GetCVar("sys_primaryUserSelectionEnabled"))
            {
                switch (gEnv->pConsole->GetCVar("sys_primaryUserSelectionEnabled")->GetIVal())
                {
                    case 0: { primaryUserSelectionEnabled = false; } break;
                    case 1: { primaryUserSelectionEnabled = true; } break;
                    default: break; // Use the default value that was set above
                }
            }

            if (primaryUserSelectionEnabled)
            {
                GameStateRequests::CreateAndPushNewOverridableGameStateOfType<GameStatePrimaryUserSelection>();
            }
            else
            {
                GameStateRequests::CreateAndPushNewOverridableGameStateOfType<GameStateMainMenu>();
            }
        }

        void OnSystemEvent(ESystemEvent systemEvent, UINT_PTR wparam, UINT_PTR /*lparam*/) override
        {
            // This logic is a little confusing, but we need to check for both...

            // ...the END of a switch INTO game mode...
            if (systemEvent == ESYSTEM_EVENT_GAME_MODE_SWITCH_END && wparam)
            {
                OnEditorGameModeEntered();
            }
            // ...and the START of a switch OUT OF game mode.
            else if (systemEvent == ESYSTEM_EVENT_GAME_MODE_SWITCH_START && !wparam)
            {
                OnEditorGameModeExiting();
            }
        }

        AZStd::shared_ptr<GameOptions> GetGameOptions() override
        {
            return m_gameOptions;
        }

    private:
        void OnEditorGameModeEntered()
        {
            AZStd::shared_ptr<IGameState> activeGameState;
            GameStateRequestBus::BroadcastResult(activeGameState, &GameStateRequests::GetActiveGameState);
            AZ_Assert(!activeGameState, "OnEditorGameModeStart: The game state stack is not empty.");

            // After entering game mode from the editor, transition straight into the level running state
            GameStateRequests::CreateAndPushNewOverridableGameStateOfType<GameStateLevelRunning>();
        }

        void OnEditorGameModeExiting()
        {
            // Before exiting game mode from the editor, clear all active game states
            GameStateRequestBus::Broadcast(&GameStateRequests::PopAllGameStates);
        }

    private:
        AZStd::shared_ptr<GameOptions> m_gameOptions;
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_GameStateSamples, GameStateSamples::GameStateSamplesModule)
