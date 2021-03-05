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

#include <ISystem.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Game state that is active while a level is loading.
    class GameStateLevelLoading : public GameState::IGameState
                                , public ISystemEventListener
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(GameStateLevelLoading, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(GameStateLevelLoading, "{3ABD903B-4E9D-4BFB-A080-4795253F420C}", IGameState);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        GameStateLevelLoading() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~GameStateLevelLoading() override = default;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameState::OnEnter
        void OnEnter() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameState::OnExit
        void OnExit() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref ISystemEventListener::OnSystemEvent
        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
    };
} // namespace GameStateSamples

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <GameStateSamples/GameStateLevelLoading.inl>
