/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <AzFramework/Physics/Material/PhysicsMaterialConfiguration.h>

namespace Physics
{
    //! MaterialAsset defines a single material, which includes the configuration to create a Material instance to use at runtime.
    class MaterialAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(Physics::MaterialAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(Physics::MaterialAsset, "{E4EF58EE-B1D1-46C8-BE48-BB62B8247386}", AZ::Data::AssetData);

        MaterialAsset() = default;
        virtual ~MaterialAsset() = default;

        static void Reflect(AZ::ReflectContext* context);

        //! Sets the data for this material asset and marks it as ready.
        //! This is necessary to be called when creating an in-memory material asset.
        void SetData(const MaterialConfiguration2& materialConfiguraiton);

        const MaterialConfiguration2& GetMaterialConfiguration() const;

    protected:
        MaterialConfiguration2 m_materialConfiguration;
    };
} // namespace Physics
