/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/Feature/SkinnedMesh/SkinnedMeshInputBuffers.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshInstance.h>
#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    namespace Render
    {
        /**
         * SkinnedMeshOverrideBus provides an interface for components to disable skinning
         */
        class SkinnedMeshOverrideRequests
            : public ComponentBus
        {
        public:
            //! Enable the GPU skinning for a mesh if it was previously disabled
            virtual void EnableSkinning(uint32_t lodIndex, uint32_t meshIndex) = 0;
            //! Disable the GPU skinning for a mesh
            virtual void DisableSkinning(uint32_t lodIndex, uint32_t meshIndex) = 0;
        };
        using SkinnedMeshOverrideRequestBus = EBus<SkinnedMeshOverrideRequests>;
    } // namespace Render
} // namespace AZ
