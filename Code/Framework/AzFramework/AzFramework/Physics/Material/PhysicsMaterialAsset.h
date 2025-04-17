/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <AzFramework/Physics/Material/PhysicsMaterialPropertyValue.h>

namespace Physics
{
    //! MaterialAsset defines a single material, which includes all the properties to create a Material instance to use at runtime.
    //! This physics material asset is generic and independent from the physics backend used.
    class MaterialAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(Physics::MaterialAsset, AZ::SystemAllocator);
        AZ_RTTI(Physics::MaterialAsset, "{E4EF58EE-B1D1-46C8-BE48-BB62B8247386}", AZ::Data::AssetData);

        static constexpr const char* FileExtension = "physicsmaterial";
        static constexpr const char* AssetGroup = "Physics Material";
        static constexpr AZ::u32 AssetSubId = 1;

        static void Reflect(AZ::ReflectContext* context);

        MaterialAsset() = default;
        virtual ~MaterialAsset() = default;

        using MaterialProperties = AZStd::unordered_map<AZStd::string, MaterialPropertyValue>;

        //! Sets the data for this material asset and marks it as ready.
        //! This is necessary to be called when creating an in-memory material asset.
        void SetData(
            const AZStd::string& materialType,
            AZ::u32 version,
            const MaterialProperties& materialProperties);

        const AZStd::string& GetMaterialType() const;
        AZ::u32 GetVersion() const;
        const MaterialProperties& GetMaterialProperties() const;

    protected:
        //! String that identifies the type of the material.
        //! Each physics backend must provide a different one.
        AZStd::string m_materialType;

        //! Version number of the material.
        //! Used to identify changes in the properties.
        AZ::u32 m_version = 0;

        //! List of material properties.
        MaterialProperties m_materialProperties;
    };
} // namespace Physics
