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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/make_shared.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameState
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Base class for all game states
    class IGameState
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(IGameState, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(IGameState, "{AF3F218C-37E0-4351-86EC-03B9BA49C5C7}");

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        IGameState() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~IGameState() = default;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // GameStateSystemComponent is a friend so it can call the state transition functions below,
        // which flow as follows:
        //
        //  state1->OnPushed
        //  | state1->OnEnter
        //  | | (state1 active, user pushes state2)
        //  | state1->OnExit
        //  | state2->OnPushed
        //  | | state2->OnEnter
        //  | | | (state2 active, state1 stil in stack, user pops state2)
        //  | | state2->OnExit
        //  | state2->OnPopped
        //  | state1->OnEnter
        //  | | (state1 active, user pops state1)
        //  | state1->OnExit
        //  state1->OnPopped
        friend class GameStateSystemComponent;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Called when this game state is pushed onto the stack
        virtual void OnPushed() {};

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Called when this game state is popped from the stack
        virtual void OnPopped() {};

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Called when this game state is set as the active game state
        virtual void OnEnter() {};

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Called when this game state is replaced as the active game state
        virtual void OnExit() {};

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Called each frame while this game state is the active game state
        virtual void OnUpdate() {};
    };
} // namespace GameState
