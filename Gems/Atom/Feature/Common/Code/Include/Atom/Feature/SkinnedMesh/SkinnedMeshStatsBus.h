/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Base.h>
#include <AzCore/EBus/EBus.h>

namespace AZ
{
    namespace Render
    {
        struct SkinnedMeshSceneStats
        {
            size_t skinnedMeshRenderProxyCount = 0;
            size_t dispatchItemCount = 0;
            size_t boneCount = 0;
            size_t vertexCount = 0;
        };

        //! Ebus for getting stats about the usage of skinned meshes in the current scene
        class SkinnedMeshStatsRequests
            : public AZ::EBusTraits
        {
        public :
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = RPI::SceneId;

            virtual SkinnedMeshSceneStats GetSceneStats() = 0;
        };
        using SkinnedMeshStatsRequestBus = AZ::EBus<SkinnedMeshStatsRequests>;
    } // namespace Render
} // namespace AZ
