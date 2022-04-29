/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// This header file is for declaring types used for RPI System classes to avoid recursive includes
 
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>

namespace AZ::RPI
{
    // Enum to track the different states of PassContainer (used for validation and debugging)
    enum class PassContainerState : u32
    {
        Unitialized,        // Default state, 
        RemovingPasses,     // Processing passes queued for Removal. Transitions to Idle
        BuildingPasses,     // Processing passes queued for Build (and their child passes). Transitions to Idle
        InitializingPasses, // Processing passes queued for Initialization (and their child passes). Transitions to Idle
        ValidatingPasses,   // Validating the hierarchy under root pass is in a valid state after Build and Initialization. Transitions to Idle
        Idle,               // Container is idle and can transition to any other state (except FrameEnd)
        Rendering,          // Rendering a frame. Transitions to FrameEnd
        FrameEnd,           // Finishing a frame. Transitions to Idle
    };

    class PassContainer
    {
        friend class PassSystem;
        friend class RenderPipeline;

        void RemovePasses();
        void BuildPasses();
        void InitializePasses();
        void Validate();
        void ProcessQueuedChanges(bool& outChanged);

        Ptr<ParentPass> m_rootPass = nullptr;

        // Lists for queuing passes for various function calls
        // Name of the list reflects the pass function it will call
        AZStd::vector< Ptr<Pass> > m_buildPassList;
        AZStd::vector< Ptr<Pass> > m_removePassList;
        AZStd::vector< Ptr<Pass> > m_initializePassList;

        bool m_passesChangedThisFrame = false;
        PassContainerState m_state = PassContainerState::Unitialized;
    };


}
