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

#include <Integration/Rendering/RenderActor.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/Asset/AssetCommon.h>

namespace EMotionFX::Integration
{
    class ActorAsset;
}

namespace AZ
{
    namespace Render
    {
        class SkinnedMeshInputBuffers;


        struct SkinInfluences
        {
            AZStd::vector<AZStd::array<AZ::u32, 4>> boneIndices;
            AZStd::vector<AZStd::array<float, 4>> boneWeights;
        };


        class AtomActor
            : public EMotionFX::Integration::RenderActor
        {
        public:
            AZ_RTTI(EMotionFX::Integration::AtomActor, "{A24ED299-27D3-4227-9D97-D273E5D7BACC}", EMotionFX::Integration::RenderActor);
            AZ_CLASS_ALLOCATOR_DECL;

            AtomActor(EMotionFX::Integration::ActorAsset* actorAsset);
            ~AtomActor();

            AZStd::intrusive_ptr<AZ::Render::SkinnedMeshInputBuffers> FindOrCreateSkinnedMeshInputBuffers();

        private:
            AZStd::intrusive_ptr<AZ::Render::SkinnedMeshInputBuffers> m_skinnedMeshInputBuffers;
            EMotionFX::Integration::ActorAsset* m_actorAsset;
        };
    } // namespace Render
} // namespace AZ
