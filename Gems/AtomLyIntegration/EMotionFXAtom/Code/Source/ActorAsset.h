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

#include <Integration/Rendering/RenderActorInstance.h>
#include <AtomCore/Instance/Instance.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>

namespace EMotionFX
{
    class Actor;
    class ActorInstance;
}

namespace AZ
{
    namespace RPI
    {
        class Model;
        class Buffer;
    }

    namespace Render
    {
        class SkinnedMeshInputBuffers;

        //! Create a buffers and buffer views that are shared between all actor instances that use the same actor asset.
        AZStd::intrusive_ptr<SkinnedMeshInputBuffers> CreateSkinnedMeshInputFromActor(const Data::AssetId& actorAssetId, const EMotionFX::Actor* actor);

        //! Get the bone transforms from the actor instance and adjust them to be in the format needed by the renderer
        void GetBoneTransformsFromActorInstance(const EMotionFX::ActorInstance* actorInstance, AZStd::vector<float>& boneTransforms, EMotionFX::Integration::SkinningMethod skinningMethod);

        //! Create a buffer for bone transforms that can be used as input to the skinning shader
        Data::Instance<RPI::Buffer> CreateBoneTransformBufferFromActorInstance(const EMotionFX::ActorInstance* actorInstance, EMotionFX::Integration::SkinningMethod skinningMethod);

    } // namespace Render
} // namespace AZ
