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

#include <EMotionFX/Source/Actor.h>
#include <Integration/Assets/ActorAsset.h>
#include <Integration/Rendering/Cry/CryRenderBackend.h>
#include <Integration/Rendering/Cry/CryRenderActor.h>
#include <Integration/Rendering/Cry/CryRenderActorInstance.h>
#include <Integration/System/SystemCommon.h>

namespace EMotionFX
{
    namespace Integration
    {
        AZ_CLASS_ALLOCATOR_IMPL(CryRenderBackend, EMotionFXAllocator, 0);

        RenderActor* CryRenderBackend::CreateActor(ActorAsset* asset)
        {
            CryRenderActor* renderActor = aznew CryRenderActor(asset);
            if (!renderActor->Init())
            {
                AZ_Warning("EMotionFX", false, "Cannot initialize Cry render actor for asset with id %s.", asset->GetId().ToString<AZStd::string>().c_str());
                delete renderActor;
                return nullptr;
            }

            return renderActor;
        }

        RenderActorInstance* CryRenderBackend::CreateActorInstance(AZ::EntityId entityId,
            const EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance,
            const AZ::Data::Asset<ActorAsset>& asset,
            const ActorAsset::MaterialList& materialPerLOD,
            SkinningMethod skinningMethod,
            const AZ::Transform& worldTransform)
        {
            CryRenderActorInstance* renderActorInstance = aznew CryRenderActorInstance(entityId, actorInstance, asset, worldTransform);

            renderActorInstance->SetMaterials(materialPerLOD);
            renderActorInstance->RegisterWithRenderer();
            renderActorInstance->SetSkinningMethod(skinningMethod);

            return renderActorInstance;
        }
    } // namespace Integration
} // namespace EMotionFX
