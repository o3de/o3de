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
    // Helper class used by the PassSystem and RenderPipeline to contain and update passes
    // Passes owned by the container are stored as a tree under the container's root pass
    // The container has queues for pass building, initialization and removal. These queues
    // are so that logic modifying or removing passes isn't triggered while the passes are 
    // rendering, but instead at the start of the frame when it is safe to do so.
    class PassTree
    {
        // Everything is private, class should only be used by PassSystem and RenderPipeline
        friend class PassSystem;
        friend class RenderPipeline;

        // Used to remove passes from the update list
        void EraseFromLists(AZStd::function<bool(const RHI::Ptr<Pass>&)> predicate);

        // Clears all queues
        void ClearQueues();

        // Functions used for pass building, initialization and removal at the start of the frame before the pass system renders
        void RemovePasses();
        void BuildPasses();
        void InitializePasses();
        void Validate();
        bool ProcessQueuedChanges();

        // The root pass of the container holds all passes belonging to the container
        Ptr<ParentPass> m_rootPass = nullptr;

        // Lists for queuing passes for various function calls so they can be updating when the frame is not rendering.
        // The names of the lists reflect the pass functions they will call
        AZStd::vector< Ptr<Pass> > m_buildPassList;
        AZStd::vector< Ptr<Pass> > m_removePassList;
        AZStd::vector< Ptr<Pass> > m_initializePassList;

        // Tracks whether any changes to the passes in this container have occurred in the frame
        bool m_passesChangedThisFrame = false;
    };
}
