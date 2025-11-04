/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Configuration.h>

namespace AZ::RPI
{
    // This enum is a list of flags to track what pass changes have occured in the pipeline so that systems can update accordingly
    // This is an improvement on the previous system, where there was one global callback for when passes changed in a pipeline, and
    // systems that updated as a result could not discern what changes had occured, thereby often updating for a worst case scenario.
    enum PipelinePassChanges
    {
        NoPassChanges                       = 0,
        PassesAdded                         = 1 << 0,
        PassesRemoved                       = 1 << 1,
        MultisampleStateChanged             = 1 << 2,
        PassAssetHotReloaded                = 1 << 3,
        PipelineChangedByFeatureProcessor   = 1 << 4,
        PipelineViewTagChanged              = 1 << 5,

    };

    ATOM_RPI_PUBLIC_API bool PipelineNeedsRebuild(u32 flags);

    ATOM_RPI_PUBLIC_API bool PipelineViewsNeedRebuild(u32 flags);

    ATOM_RPI_PUBLIC_API bool PipelineStateLookupNeedsRebuild(u32 flags);
}
