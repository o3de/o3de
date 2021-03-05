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

#include <GameState/GameStateRequestBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/deque.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameState
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! This system component manages game state instances and the transitions between them. A few
    //! default game states are implemented in the GameStateSamples Gem, and these can be extended
    //! as needed in order to provide a custom experience for each game, but it's also possible to
    //! create completely new states by inheriting from the abstract GameState::IGameState class.
    //! States are managed using a stack (pushdown automaton) in order to maintain their history.
    class GameStateSystemComponent : public AZ::Component
                                   , public AZ::TickBus::Handler
                                   , public GameStateRequestBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::Component Setup
        AZ_COMPONENT(GameStateSystemComponent, "{03A10E41-3339-42C1-A6C8-A81327CB034B}");

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::ComponentDescriptor::Reflect
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::ComponentDescriptor::GetProvidedServices
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::ComponentDescriptor::GetIncompatibleServices
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::Component::Activate
        void Activate() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::Component::Deactivate
        void Deactivate() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::TickEvents::GetTickOrder
        int GetTickOrder() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::TickEvents::OnTick
        void OnTick(float deltaTime, AZ::ScriptTimePoint scriptTimePoint) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameStateRequests::UpdateActiveGameState
        void UpdateActiveGameState() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameStateRequests::GetActiveGameState
        AZStd::shared_ptr<IGameState> GetActiveGameState() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameStateRequests::PushGameState
        bool PushGameState(AZStd::shared_ptr<IGameState> newGameState) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameStateRequests::PopActiveGameState
        bool PopActiveGameState() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameStateRequests::PopAllGameStates
        void PopAllGameStates() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameStateRequests::ReplaceActiveGameState
        bool ReplaceActiveGameState(AZStd::shared_ptr<IGameState> newGameState) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameStateRequests::DoesStackContainGameStateOfTypeId
        bool DoesStackContainGameStateOfTypeId(const AZ::TypeId& gameStateTypeId) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameStateRequests::AddGameStateFactoryOverrideForTypeId
        bool AddGameStateFactoryOverrideForTypeId(const AZ::TypeId& gameStateTypeId,
                                                  GameStateFactory factory) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameStateRequests::RemoveGameStateFactoryOverrideForTypeId
        bool RemoveGameStateFactoryOverrideForTypeId(const AZ::TypeId& gameStateTypeId) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameStateRequests::GetGameStateFactoryOverrideForTypeId
        GameStateFactory GetGameStateFactoryOverrideForTypeId(const AZ::TypeId& gameStateTypeId) override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The game state stack, where the top element is considered to be the active game state
        AZStd::deque<AZStd::shared_ptr<IGameState>> m_gameStateStack;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! A map of game state factory functions indexed by the game state type id to override
        AZStd::unordered_map<AZ::TypeId, GameStateFactory> m_gameStateFactoryOverrides;
    };
}
