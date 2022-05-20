/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>

namespace Physics
{
    //! MaterialAsset defines a single material, which includes all the properties to create a Material instance to use at runtime.
    //! This physics material asset is generic and independent from the physics backend used.
    class MaterialAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(Physics::MaterialAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(Physics::MaterialAsset, "{E4EF58EE-B1D1-46C8-BE48-BB62B8247386}", AZ::Data::AssetData);

        static constexpr const char* FileExtension = "physicsmaterial";
        static constexpr const char* AssetGroup = "Physics Material";
        static constexpr AZ::u32 AssetSubId = 1;

        static void Reflect(AZ::ReflectContext* context);

        MaterialAsset() = default;
        virtual ~MaterialAsset() = default;

        //! Sets the data for this material asset and marks it as ready.
        //! This is necessary to be called when creating an in-memory material asset.
        void SetData(const AZStd::unordered_map<AZStd::string, float>& materialProperties);

        const AZStd::unordered_map<AZStd::string, float>& GetMaterialProperties() const;

    protected:
        // TODO: Make this for generic types
        AZStd::unordered_map<AZStd::string, float> m_materialProperties;
    };
} // namespace Physics
