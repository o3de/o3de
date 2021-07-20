/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
