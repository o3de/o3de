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

#include <GameState/GameState.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/functional.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameState
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Alias for verbose function pointer type
    using GameStateFactory = AZStd::function<AZStd::shared_ptr<IGameState>()>;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to submit requests related to game state management
    class GameStateRequests : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can only be sent to and addressed by a single instance (singleton)
        ///@{
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Create a new game state
        //! \tparam GameStateType The game state type to create
        //! \param[in] checkForOverrides True if we should check for an override, false otherwise
        //! \return A shared pointer to the new game state that was created
        template<class GameStateType>
        static AZStd::shared_ptr<IGameState> CreateNewOverridableGameStateOfType(bool checkForOverride = true);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Create a new game state and push it onto the stack to make it the active game state.
        //! New game states are created and stored in the stack using a shared_ptr, so they will
        //! be destroyed automatically once they are popped off the stack (assuming that nothing
        //! else retains a reference, say via GameStateNotifications::OnActiveGameStateChanged).
        //! \tparam GameStateType The game state type to create and activate
        //! \param[in] checkForOverrides True if we should check for an override, false otherwise
        template<class GameStateType>
        static void CreateAndPushNewOverridableGameStateOfType(bool checkForOverride = true);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Pop  game states from the stack until the active game state is of the specified type. If
        //! no game state of the specified type exists in the game state stack it will be left empty.
        //! \tparam GameStateType The game state type in the stack that we want to be active
        //! \return True if the active game state is now of the specified type, false otherwise
        template<class GameStateType>
        static bool PopActiveGameStateUntilOfType();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Query whether the active game state is of the specified type
        //! \tparam GameStateType The game state type to check whether is active
        //! \return True if the active game state is of the specified type, false otherwise
        template<class GameStateType>
        static bool IsActiveGameStateOfType();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Query whether the game state stack contains a game state of the specified type
        //! \tparam GameStateType The game state type to check whether is in the stack
        //! \return True if the stack contains a game state of the specified type, false otherwise
        template<class GameStateType>
        static bool DoesStackContainGameStateOfType();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add a game state override so that a request to push a new game state of a certain type
        //! will result in pushing a new game state of a different type instead. This is useful for
        //! situations where we want to use a set of default game states but override some (or all)
        //! of them with custom versions which satisfy the requirements of a specific game project.
        //! \tparam GameStateType The original game state type to be overridden
        //! \param[in] factory The factory function that will create the game state override
        //! \return True if the game state override was successfully added, false otherwise
        template<class GameStateType>
        static bool AddGameStateFactoryOverrideForType(GameStateFactory factory);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Remove a game state override that was added using AddGameStateFactoryOverrideForType.
        //! \tparam GameStateType The original game state type that was overridden
        //! \return True if the game state override was successfully removed, false otherwise
        template<class GameStateType>
        static bool RemoveGameStateFactoryOverrideForType();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Retrieve a game state override that was added using AddGameStateFactoryOverrideForType.
        //! \tparam GameStateType The original game state type that was overridden
        //! \return The factory function used to create the game state override, nullptr otherwise
        template<class GameStateType>
        static GameStateFactory GetGameStateFactoryOverrideForType();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Update the active game state. This is called during the AZ::ComponentTickBus::TICK_GAME
        //! priority update of the AZ::TickBus, but can be called independently any time if needed.
        virtual void UpdateActiveGameState() = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Request the active game state (if any)
        //! \return A shared pointer to the active game state (will be empty if there is none)
        virtual AZStd::shared_ptr<IGameState> GetActiveGameState() = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Push a game state onto the stack, which will result in it becoming the active game state.
        //! If newGameState is already found in the stack this will fail and return false, however
        //! it is possible for multiple instances of the same game state type to occupy the stack.
        //! \param[in] newGameState The new game state to push onto the stack
        //! \return True if the game state was successfully pushed onto the stack, false otherwise
        virtual bool PushGameState(AZStd::shared_ptr<IGameState> newGameState) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Pop the active game state from the stack, which will result in it being deactivated and
        //! the game state below it in the stack (if any) becoming the active game state again.
        //! \return True if the active game state was successfully popped, false otherwise
        virtual bool PopActiveGameState() = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Pop all game states from the stack, leaving it empty.
        virtual void PopAllGameStates() = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Replace the active game state with another game state that will become the active state.
        //! If the stack is currently empty, newGameState will be pushed to become the active state.
        //! If newGameState is already found in the stack this will fail and return false, however
        //! it is possible for multiple instances of the same game state type to occupy the stack.
        //! This differs from calling PopActiveGameState followed by PushGameState(newGameState),
        //! which would result in the state below the currently active state being activated then
        //! immediately deactivated when newGameState is pushed onto the stack; calling this will
        //! the state below the currently active state unchanged.
        //! \param[in] newGameState The new game state with which to replace the active game state
        //! \return True if the active game state was successfully replaced, false otherwise
        virtual bool ReplaceActiveGameState(AZStd::shared_ptr<IGameState> newGameState) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Query whether the game state stack contains a game state of the specified type
        //! \param[in] gameStateTypeId The game state type to check whether is in the stack
        //! \return True if the stack contains a game state of the specified type, false otherwise
        virtual bool DoesStackContainGameStateOfTypeId(const AZ::TypeId& gameStateTypeId) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add a game state override so that a request to push a new game state of a certain type
        //! will result in pushing a new game state of a derived type instead. This is useful for
        //! situations where we want to use a set of default game states but override some (or all)
        //! of them with custom versions which satisfy the requirements of a specific game project.
        //! \param[in] gameStateTypeId The original game state type id to be overridden
        //! \param[in] factory The factory function that will create the game state override
        //! \return True if the game state override was successfully added, false otherwise
        virtual bool AddGameStateFactoryOverrideForTypeId(const AZ::TypeId& gameStateTypeId,
                                                          GameStateFactory factory) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Remove a game state override that was added using AddGameStateFactoryOverrideForTypeId.
        //! \param[in] gameStateTypeId The original game state type id that was overridden
        //! \return True if the game state override was successfully removed, false otherwise
        virtual bool RemoveGameStateFactoryOverrideForTypeId(const AZ::TypeId& gameStateTypeId) = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Retrieve a game state override that was added using AddGameStateFactoryOverrideForTypeId.
        //! \param[in] gameStateTypeId The original game state type id that was overridden
        //! \return The factory function used to create the game state override, nullptr otherwise
        virtual GameStateFactory GetGameStateFactoryOverrideForTypeId(const AZ::TypeId& gameStateTypeId) = 0;
    };
    using GameStateRequestBus = AZ::EBus<GameStateRequests>;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<class GameStateType>
    inline AZStd::shared_ptr<IGameState> GameStateRequests::CreateNewOverridableGameStateOfType(bool checkForOverride)
    {
        AZStd::shared_ptr<IGameState> newGameState;
        if (checkForOverride)
        {
            auto factoryFunction = GetGameStateFactoryOverrideForType<GameStateType>();
            if (factoryFunction)
            {
                newGameState = factoryFunction();
                if (!azrtti_istypeof<GameStateType>(newGameState.get()))
                {
                    AZ_Warning("GameStateSystemComponent", false,
                               "Trying to override a game state type with one that doesn't derive from it.");
                    newGameState.reset();
                }
            }
        }

        return newGameState ? newGameState : AZStd::make_shared<GameStateType>();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<class GameStateType>
    inline void GameStateRequests::CreateAndPushNewOverridableGameStateOfType(bool checkForOverride)
    {
        AZStd::shared_ptr<IGameState> newGameState = CreateNewOverridableGameStateOfType<GameStateType>(checkForOverride);
        AZ_Assert(newGameState, "Failed to create new game state");
        bool result = false;
        GameStateRequestBus::BroadcastResult(result, &GameStateRequests::PushGameState, newGameState);
        AZ_Assert(result, "Failed to push new game state");
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<class GameStateType>
    inline bool GameStateRequests::PopActiveGameStateUntilOfType()
    {
        AZStd::shared_ptr<IGameState> activeGameState;
        GameStateRequestBus::BroadcastResult(activeGameState, &GameStateRequests::GetActiveGameState);
        while (activeGameState && !azrtti_istypeof<GameStateType>(activeGameState.get()))
        {
            GameStateRequestBus::Broadcast(&GameStateRequests::PopActiveGameState);
            GameStateRequestBus::BroadcastResult(activeGameState, &GameStateRequests::GetActiveGameState);
        }
        return activeGameState != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<class GameStateType>
    inline bool GameStateRequests::IsActiveGameStateOfType()
    {
        AZStd::shared_ptr<IGameState> activeGameState;
        GameStateRequestBus::BroadcastResult(activeGameState, &GameStateRequests::GetActiveGameState);
        return activeGameState ? azrtti_istypeof<GameStateType>(activeGameState.get()) : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<class GameStateType>
    inline bool GameStateRequests::DoesStackContainGameStateOfType()
    {
        bool doesStackContainGameStateOfType = false;
        GameStateRequestBus::BroadcastResult(doesStackContainGameStateOfType,
                                             &GameStateRequests::DoesStackContainGameStateOfTypeId,
                                             azrtti_typeid<GameStateType>());
        return doesStackContainGameStateOfType;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<class GameStateType>
    inline bool GameStateRequests::AddGameStateFactoryOverrideForType(GameStateFactory factory)
    {
        bool overrideAdded = false;
        GameStateRequestBus::BroadcastResult(overrideAdded,
                                             &GameStateRequests::AddGameStateFactoryOverrideForTypeId,
                                             azrtti_typeid<GameStateType>(),
                                             factory);
        return overrideAdded;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<class GameStateType>
    inline bool GameStateRequests::RemoveGameStateFactoryOverrideForType()
    {
        bool overrideRemoved = false;
        GameStateRequestBus::BroadcastResult(overrideRemoved,
                                             &GameStateRequests::RemoveGameStateFactoryOverrideForTypeId,
                                             azrtti_typeid<GameStateType>());
        return overrideRemoved;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<class GameStateType>
    inline GameStateFactory GameStateRequests::GetGameStateFactoryOverrideForType()
    {
        GameStateFactory overrideFactoryFunction;
        GameStateRequestBus::BroadcastResult(overrideFactoryFunction,
                                             &GameStateRequests::GetGameStateFactoryOverrideForTypeId,
                                             azrtti_typeid<GameStateType>());
        return overrideFactoryFunction;
    }
} // namespace GameState
