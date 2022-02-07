/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Asset/BlastAsset.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/parallel/lock.h>
#include <Blast/BlastSystemBus.h>
#include <NvBlastExtAssetUtils.h>
#include <NvBlastExtPxSerialization.h>
#include <NvBlastExtSerialization.h>
#include <NvBlastExtTkSerialization.h>
#include <NvBlastTkAsset.h>
#include <NvBlastTypes.h>

namespace Blast
{
    AZStd::mutex BlastAsset::s_assetCreationMutex;

    BlastAsset::BlastAsset(Nv::Blast::ExtPxAsset* pxAsset, NvBlastExtDamageAccelerator* damageAccelerator)
        : m_pxAsset(pxAsset)
        , m_damageAccelerator(damageAccelerator)
        , m_bondHealthMax(0)
    {
    }

    void BlastAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BlastAsset, AZ::Data::AssetData>()
                ->Version(0)
                ;
        }
    }


    bool BlastAsset::LoadFromBuffer(void* buffer, size_t bytesSize)
    {
        Nv::Blast::ExtSerialization* serialization = nullptr;

        BlastSystemRequestBus::BroadcastResult(serialization, &BlastSystemRequestBus::Events::GetExtSerialization);

        if (serialization == nullptr)
        {
            AZ_Error("Blast", false, "Trying to load blast asset before blast system has initialized.");
            return false;
        }

        AZ::u32 objectTypeId;
        void* asset = nullptr;
        {
            AZStd::lock_guard<AZStd::mutex> lock(s_assetCreationMutex);
            asset = serialization->deserializeFromBuffer(buffer, bytesSize, &objectTypeId);
        }

        if (asset == nullptr)
        {
            AZ_Error("Blast", asset != nullptr, "can't load .blast file.");
            return false;
        }
        else if (objectTypeId == Nv::Blast::ExtPxObjectTypeID::Asset)
        {
            m_pxAsset.reset(reinterpret_cast<Nv::Blast::ExtPxAsset*>(asset));
            const Nv::Blast::TkAsset& tkAsset = m_pxAsset->getTkAsset();
            NvBlastAsset* llAsset = const_cast<NvBlastAsset*>(tkAsset.getAssetLL());
            m_damageAccelerator.reset(NvBlastExtDamageAcceleratorCreate(llAsset, 3));
            m_pxAsset->setAccelerator(m_damageAccelerator.get());
        }
        else
        {
            // in this case we'll want to extract the physics meshes from the scene file.
            // We don't necessarily have access to the mesh data though, so if we want to support this,
            // we'll need to come up with a way to associate with the mesh data
            // See BlastAssetModel in the SDK sample for how to create that data
            AZ_Error("Blast", false, "We currently don't support loading Non-ExtPx objects");
            return false;
        }

        // Initialize maximum bond health
        const auto& actorDesc = m_pxAsset->getDefaultActorDesc();
        if (actorDesc.initialBondHealths)
        {
            m_bondHealthMax = std::numeric_limits<float>::min();
            const uint32_t bondCount = m_pxAsset->getTkAsset().getBondCount();
            for (uint32_t i = 0; i < bondCount; ++i)
            {
                m_bondHealthMax = std::max<float>(m_bondHealthMax, actorDesc.initialBondHealths[i]);
            }
        }
        else
        {
            m_bondHealthMax = actorDesc.uniformInitialBondHealth;
        }

        return true;
    }
} // namespace Blast
