/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/base.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Transform.h>
#include <Integration/System/SystemCommon.h>
#include <Integration/Assets/ActorAsset.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/SubMesh.h>
#include <EMotionFX/Source/SkinningInfoVertexAttributeLayer.h>
#include <AtomActor.h>
#include <ActorAsset.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshInputBuffers.h>

namespace AZ
{
    namespace Render
    {
        AZ_CLASS_ALLOCATOR_IMPL(AtomActor, EMotionFX::Integration::EMotionFXAllocator);

        AtomActor::AtomActor(EMotionFX::Integration::ActorAsset* actorAsset)
            : RenderActor()
            , m_actorAsset(actorAsset)
        {
            AZ_Assert(m_actorAsset, "AtomActor created with a null EmotionFX ActorAsset.");
            const AZ::Data::AssetId& actorAssetId = m_actorAsset->GetId();
            if (actorAssetId.IsValid())
            {
                AZ_Assert(m_actorAsset->GetActor(), "AtomActor created with a null EMotionFX Actor.");
            }
        }

        AtomActor::~AtomActor()
        {
            m_skinnedMeshInputBuffers.reset();
        }

        AZStd::intrusive_ptr<AZ::Render::SkinnedMeshInputBuffers> AtomActor::FindOrCreateSkinnedMeshInputBuffers()
        {
            if (!m_skinnedMeshInputBuffers)
            {
                m_skinnedMeshInputBuffers = AZ::Render::CreateSkinnedMeshInputFromActor(m_actorAsset->GetId(), m_actorAsset->GetActor());
            }
            return m_skinnedMeshInputBuffers;
        }
    } // namespace Render
} // namespace AZ
