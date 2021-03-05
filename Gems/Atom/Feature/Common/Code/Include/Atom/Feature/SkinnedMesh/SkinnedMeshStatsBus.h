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
            size_t readOnlyBufferViewCount = 0;
            size_t writableBufferViewCount = 0;
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
