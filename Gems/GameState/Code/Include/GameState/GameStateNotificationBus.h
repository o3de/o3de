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

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameState
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to listen for notifications related to game state management/transitions
    class GameStateNotifications : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: game state notifications are addressed to a single address
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: game state notifications can be handled by multiple listeners
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Called when a game state transition occurs
        //! \param[in] oldGameState The old game state we are transitioning from (can be null)
        //! \param[in] newGameState The new game state we are transitioning into (can be null)
        virtual void OnActiveGameStateChanged(AZStd::shared_ptr<IGameState> oldGameState,
                                              AZStd::shared_ptr<IGameState> newGameState) {}
    };
    using GameStateNotificationBus = AZ::EBus<GameStateNotifications>;
} // namespace GameState
