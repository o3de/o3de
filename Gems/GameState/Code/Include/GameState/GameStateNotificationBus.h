/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
