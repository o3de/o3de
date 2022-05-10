/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Uuid.h>

#include <Material/BlastMaterialConfiguration.h>

namespace Blast
{
    // O3DE_DEPRECATION
    // Legacy blast material Id class used to identify the material in the collection of materials.
    // Used when converting old material asset to new one.
    class BlastMaterialId
    {
    public:
        AZ_CLASS_ALLOCATOR(BlastMaterialId, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(BlastMaterialId, "{BDB30505-C93E-4A83-BDD7-41027802DE0A}");

        static void Reflect(AZ::ReflectContext* context);

        AZ::Uuid m_id = AZ::Uuid::CreateNull();
    };

    //! MaterialAsset defines a single material, which includes the configuration to create a Material instance to use at runtime.
    class MaterialAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(Blast::MaterialAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(Blast::MaterialAsset, "{BA261DAC-2B87-4461-833B-914FD9020BD8}", AZ::Data::AssetData);

        MaterialAsset() = default;
        virtual ~MaterialAsset() = default;

        static void Reflect(AZ::ReflectContext* context);

        const MaterialConfiguration& GetMaterialConfiguration() const;

        BlastMaterialId GetLegacyBlastMaterialId() const;

    protected:
        MaterialConfiguration m_materialConfiguration;

        // Legacy Blast material Id is only used when converting from old blast material asset,
        // which holds a library of materials, to the new blast material asset.
        BlastMaterialId m_legacyBlastMaterialId;
        friend class BlastMaterialFromAssetConfiguration;
    };
} // namespace Blast
