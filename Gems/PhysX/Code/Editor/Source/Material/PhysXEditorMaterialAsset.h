/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Physics/Material/Legacy/LegacyPhysicsMaterialSelection.h>

#include <PhysX/Material/PhysXMaterialConfiguration.h>

namespace PhysicsLegacy
{
    class MaterialFromAssetConfiguration;
} // namespace PhysicsLegacy

namespace PhysX
{
    //! EditorMaterialAsset defines a single PhysX material asset.
    //! This is an editor asset and it's authored by Asset Editor.
    //! When this asset is processed by Asset Processor it creates
    //! a generic Physics material asset in the cache (agnostic to PhysX backend).
    class EditorMaterialAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysX::EditorMaterialAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(PhysX::EditorMaterialAsset, "{BC7B88B9-EE31-4FBF-A01E-2A93624C49D3}", AZ::Data::AssetData);

        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* FileExtension = "physxmaterial";

        EditorMaterialAsset() = default;
        virtual ~EditorMaterialAsset() = default;

        const MaterialConfiguration& GetMaterialConfiguration() const;

        PhysicsLegacy::MaterialId GetLegacyPhysicsMaterialId() const;

    protected:
        MaterialConfiguration m_materialConfiguration;

        // Legacy Physics material Id is only used when converting from old physics material asset,
        // which holds a library of materials, to the new physx material asset.
        PhysicsLegacy::MaterialId m_legacyPhysicsMaterialId;
        friend PhysicsLegacy::MaterialFromAssetConfiguration;
    };
} // namespace PhysX
