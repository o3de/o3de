/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>
#include <AzCore/Preprocessor/Enum.h>

#include <Atom/RPI.Reflect/Base.h>

// This file contains defines and settings for the pass system

// Enables debugging of the pass system
// Set this to 1 locally on your machine to facilitate pass debugging and get extra information
// about passes in the output window. DO NOT SUBMIT with value set to 1
#define AZ_RPI_ENABLE_PASS_DEBUGGING 0

namespace AZ
{
    namespace RPI
    {
        // This enum tracks the state of passes across build, initialization and rendering
        // 
        // Standard order of state progression:
        // 
        // Uninitialized -> Queued -> Resetting -> Reset -> Building -> Built -> Initializing -> Initialized -> Idle -> Rendering -> Idle ...
        //
        // Addition state transitions:
        //
        // Queued -> Resetting
        //        -> Building
        //        -> Initializing
        // 
        // Idle -> Queued
        //      -> Resetting
        //      -> Building
        //      -> Initializing
        //      -> Rendering
        //
        // Rendering -> Idle
        //           -> Queued (Rendering will transition to Queued if a pass was queued with the PassSystem during Rendering)
        //
        // Any State -> Orphaned  (transition to Orphaned state can be outside the jurisdiction of the pass and so can happen from any state)
        // Orphaned  -> Queued    (When coming out of Orphaned state, pass will queue itself for build. In practice this
        //                         (almost?) never happens as orphaned passes are re-created in most if not all cases.)
        //
        AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(PassState, uint8_t,
            // Default value, you should only ever see this in the Pass constructor
            // Once the constructor is done, the Pass will set it's state to Reset
            Uninitialized,
            //   |
            //   |
            //   V
            // Pass is queued with the Pass System for an update (see PassQueueState below)
            Queued,
            //   |
            //   |
            //   V
            // Pass is currently in the process of resetting
            Resetting,
            //   |
            //   |
            //   V
            // Pass has been reset and is awaiting build
            Reset,
            //   |
            //   |
            //   V
            // Pass is currently building
            Building,
            //   |
            //   |
            //   V
            // Pass has been built and is awaiting initialization
            Built,
            //   |
            //   |
            //   V
            // Pass is currently being initialized
            Initializing,
            //   |
            //   |
            //   V
            // Pass has been initialized
            Initialized,
            //   |
            //   |
            //   V
            // Idle state, pass is awaiting rendering
            Idle,
            //   |
            //   |
            //   V
            // Pass is currently rendering. Pass must be in Idle state before entering this state
            Rendering,

            // Special state: Orphaned State, pass was removed from it's parent and is awaiting deletion
            Orphaned
        );

        // This enum keeps track of what actions the pass is queued for with the pass system
        AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(PassQueueState, uint8_t,
        
            // The pass is currently not in any queued state and may therefore transition to any queued state
            NoQueue,

            // The pass is queued for Removal at the start of the next frame. Has the highest priority and cannot be overridden by any other queue state
            QueuedForRemoval,

            // The pass is queued for Build at the start of the frame. Note that any pass built at the start of the frame will also be Initialized.
            // This state can be overridden by QueuedForRemoval, as we don't want to build a pass that has been removed.
            QueuedForBuildAndInitialization,

            // The pass is queued for Initialization at the start of the frame.
            // This state has the lowest priority and can therefore be overridden by QueueForBuildAndInitialization or QueueForRemoval.
            QueuedForInitialization,
        );
    }
}
