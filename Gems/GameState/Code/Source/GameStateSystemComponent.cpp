/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GameStateSystemComponent.h>
#include <GameState/GameStateNotificationBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameState
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void GameStateSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<GameStateSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<GameStateSystemComponent>("GameState", "A generic framework for managing game states and the transitions between them.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void GameStateSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("GameStateService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void GameStateSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("GameStateService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void GameStateSystemComponent::Activate()
    {
        AZ::TickBus::Handler::BusConnect();
        GameStateRequestBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void GameStateSystemComponent::Deactivate()
    {
        GameStateRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    int GameStateSystemComponent::GetTickOrder()
    {
        return AZ::ComponentTickBus::TICK_GAME;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void GameStateSystemComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*scriptTimePoint*/)
    {
        UpdateActiveGameState();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void GameStateSystemComponent::UpdateActiveGameState()
    {
        AZStd::shared_ptr<IGameState> activeGameState = GetActiveGameState();
        if (activeGameState)
        {
            activeGameState->OnUpdate();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::shared_ptr<IGameState> GameStateSystemComponent::GetActiveGameState()
    {
        return m_gameStateStack.empty() ? nullptr : m_gameStateStack.back();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool GameStateSystemComponent::PushGameState(AZStd::shared_ptr<IGameState> newGameState)
    {
        // Error checking
        if (!newGameState)
        {
            AZ_Warning("GameStateSystemComponent", false,
                       "Trying to push a null game state.");
            return false;
        }

        // Error checking
        const bool isAlreadyInStack = AZStd::find(m_gameStateStack.begin(),
                                                  m_gameStateStack.end(),
                                                  newGameState) != m_gameStateStack.end();
        if (isAlreadyInStack)
        {
            AZ_Warning("GameStateSystemComponent", false,
                       "Trying to push a new game state that is already in the stack.");
            return false;
        }

        // Deactivate the currently active game state
        AZStd::shared_ptr<IGameState> oldGameState = GetActiveGameState();
        if (oldGameState)
        {
            oldGameState->OnExit();
        }

        // Push the new game state onto the stack to make it the active state
        m_gameStateStack.push_back(newGameState);
        newGameState->OnPushed();
        newGameState->OnEnter();

        // Inform any interested parties that the active game state has been changed
        GameStateNotificationBus::Broadcast(&GameStateNotifications::OnActiveGameStateChanged,
                                            oldGameState,
                                            newGameState);

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool GameStateSystemComponent::PopActiveGameState()
    {
        // Error checking
        if (m_gameStateStack.empty())
        {
            AZ_Warning("GameStateSystemComponent", false,
                       "Trying to pop the active game state but the stack is empty.");
            return false;
        }

        // Deactivate the currently active game state before popping it from the stack
        AZStd::shared_ptr<IGameState> oldGameState = m_gameStateStack.back();
        oldGameState->OnExit();
        m_gameStateStack.pop_back();
        oldGameState->OnPopped();

        // Reactivate the next game state in the stack
        AZStd::shared_ptr<IGameState> newGameState = GetActiveGameState();
        if (newGameState)
        {
            newGameState->OnEnter();
        }

        // Inform any interested parties that the active game state has been changed
        GameStateNotificationBus::Broadcast(&GameStateNotifications::OnActiveGameStateChanged,
                                            oldGameState,
                                            newGameState);

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void GameStateSystemComponent::PopAllGameStates()
    {
        AZStd::shared_ptr<IGameState> activeGameState = GetActiveGameState();
        while (activeGameState)
        {
            PopActiveGameState();
            activeGameState = GetActiveGameState();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool GameStateSystemComponent::ReplaceActiveGameState(AZStd::shared_ptr<IGameState> newGameState)
    {
        // Error checking
        if (!newGameState)
        {
            AZ_Warning("GameStateSystemComponent", false,
                       "Trying to replace the active game state with a null game state.");
            return false;
        }

        // If no game state is currently active just push the new game state
        AZStd::shared_ptr<IGameState> oldGameState = GetActiveGameState();
        if (!oldGameState)
        {
            return PushGameState(newGameState);
        }

        // Deactivate the currently active game state before popping it from the stack
        oldGameState->OnExit();
        m_gameStateStack.pop_back();
        oldGameState->OnPopped();

        // Push the new game state onto the stack to make it the active state
        m_gameStateStack.push_back(newGameState);
        newGameState->OnPushed();
        newGameState->OnEnter();

        // Inform any interested parties that the active game state has been changed
        GameStateNotificationBus::Broadcast(&GameStateNotifications::OnActiveGameStateChanged,
                                            oldGameState,
                                            newGameState);

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool GameStateSystemComponent::DoesStackContainGameStateOfTypeId(const AZ::TypeId& gameStateTypeId)
    {
        // Reverse iterate over the stack until we find a game state of the specified type
        for (auto it = m_gameStateStack.rbegin(); it != m_gameStateStack.rend(); ++it)
        {
            const IGameState* gameState = (*it).get();
            if (azrtti_istypeof(gameStateTypeId, gameState))
            {
                return true;
            }
        }
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool GameStateSystemComponent::AddGameStateFactoryOverrideForTypeId(const AZ::TypeId& gameStateTypeId,
                                                                        GameStateFactory factory)
    {
        if (!azrtti_istypeof(gameStateTypeId, factory().get()))
        {
            AZ_Warning("GameStateSystemComponent", false,
                       "Trying to override a game state type with one that doesn't derive from it.");
            return false;
        }

        if (m_gameStateFactoryOverrides.find(gameStateTypeId) != m_gameStateFactoryOverrides.end())
        {
            AZ_Warning("GameStateSystemComponent", false,
                       "Trying to override a game state type that has already been overriden.");
            return false;
        }

        m_gameStateFactoryOverrides[gameStateTypeId] = factory;
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool GameStateSystemComponent::RemoveGameStateFactoryOverrideForTypeId(const AZ::TypeId& gameStateTypeId)
    {
        return m_gameStateFactoryOverrides.erase(gameStateTypeId) > 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    GameStateFactory GameStateSystemComponent::GetGameStateFactoryOverrideForTypeId(const AZ::TypeId& gameStateTypeId)
    {
        const auto& it = m_gameStateFactoryOverrides.find(gameStateTypeId);
        return it != m_gameStateFactoryOverrides.end() ? it->second : GameStateFactory();
    }
}
