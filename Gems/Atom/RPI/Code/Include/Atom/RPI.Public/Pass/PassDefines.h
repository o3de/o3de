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

#include <AzCore/PlatformDef.h>

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
        enum class PassState : u8
        {
            // Default value, you should only ever see this in the Pass constructor
            // Once the constructor is done, the Pass will set it's state to Reset
            Uninitialized,

            // Pass is queued with the Pass System for an update (see PassQueueState below)
            // From Queued, the pass can transition into Resetting, Building or Initializing depending on the PassQueueState
            Queued,

            // Pass is currently in the process of resetting
            // From Resetting, the pass can transition into 
            Resetting,

            // Pass has been reset and is await build
            // From Reset, the pass can transition to Building
            Reset,

            // Pass is currently building
            // From Building, the pass can transition to Built
            Building,

            // Pass has been built and is awaiting initialization
            // From Built, the pass can transition to Initializing
            Built,

            // Pass is currently being initialized
            // From Initializing, the pass can transition to Initialized
            Initializing,

            // Pass has been initialized
            // From Initialized, the pass can transition to Idle
            Initialized,

            // Idle state, pass is awaiting rendering
            // From Idle, the pass can transition to Queued, Resetting, Building, Initializing or Rendering
            Idle,

            // Pass is currently rendering. Pass must be in Idle state before entering this state
            // From Rendering, the pass can transition to Idle or Queue if the pass was queued with the Pass System during Rendering
            Rendering
        };

        // This enum keeps track of what actions the pass is queued for with the pass system
        enum class PassQueueState : u8
        {
            // The pass is currently not in any queued state and may therefore transition to any queued state
            NoQueue,

            // The pass is queued for Removal at the start of the next frame. Cannot be overridden by any other queue state
            QueuedForRemoval,

            // The pass is queued for Build at the start of the frame. Note that any pass built at the start of the frame will also be initialized.
            // This state can be overridden by QueuedForRemoval
            QueuedForBuild,

            // The pass is queued for Initialization at the start of the frame.
            // This state has the lowest priority and can therefore be overridden by QueueForBuild or QueueForRemoval
            QueuedForInitialization,
        };
    }
}
