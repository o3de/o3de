/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <BlastMaterial/MaterialConfiguration.h>

namespace Blast
{
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

    protected:
        MaterialConfiguration m_materialConfiguration;
    };
} // namespace Blast
