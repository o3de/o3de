/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Material/MaterialAssignmentId.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    namespace Render
    {
        using MaterialPropertyOverrideMap = AZStd::unordered_map<Name, AZStd::any>;

        struct MaterialAssignment final
        {
            AZ_RTTI(AZ::Render::MaterialAssignment, "{C66E5214-A24B-4722-B7F0-5991E6F8F163}");
            AZ_CLASS_ALLOCATOR(AZ::Render::MaterialAssignment, SystemAllocator, 0);
            static void Reflect(ReflectContext* context);

            MaterialAssignment() = default;

            MaterialAssignment(const AZ::Data::AssetId& materialAssetId);

            MaterialAssignment(const Data::Asset<RPI::MaterialAsset>& asset);

            MaterialAssignment(const Data::Asset<RPI::MaterialAsset>& asset, const Data::Instance<RPI::Material>& instance);

            //! Recreates the material instance from the asset if it has been loaded.
            //! If amy property overrides have been specified then a unique instance will be created.
            //! Otherwise an attempt will be made to find or create a shared instance.
            void RebuildInstance();

            //! Release asset and instance references
            void Release();

            //! Return true if contained assets have not been loaded
            bool RequiresLoading() const;

            //! Applies property overrides to material instance
            bool ApplyProperties();

            //! Returns a string composed of the asset path.
            AZStd::string ToString() const;

            Data::Asset<RPI::MaterialAsset> m_materialAsset;
            Data::Asset<RPI::MaterialAsset> m_defaultMaterialAsset;
            Data::Instance<RPI::Material> m_materialInstance;
            MaterialPropertyOverrideMap m_propertyOverrides;
            RPI::MaterialModelUvOverrideMap m_matModUvOverrides;
            bool m_materialInstancePreCreated = false;
        };

        using MaterialAssignmentMap = AZStd::unordered_map<MaterialAssignmentId, MaterialAssignment>;
        static const MaterialAssignment DefaultMaterialAssignment;
        static const MaterialAssignmentId DefaultMaterialAssignmentId;
        static const MaterialAssignmentMap DefaultMaterialAssignmentMap;

        //! Utility function for retrieving a material entry from a MaterialAssignmentMap
        const MaterialAssignment& GetMaterialAssignmentFromMap(const MaterialAssignmentMap& materials, const MaterialAssignmentId& id);

        //! Utility function for retrieving a material entry from a MaterialAssignmentMap, falling back to defaults for a particular asset
        //! or the entire model
        const MaterialAssignment& GetMaterialAssignmentFromMapWithFallback(
            const MaterialAssignmentMap& materials, const MaterialAssignmentId& id);

        //! Utility function for generating a set of available material assignments in a model
        MaterialAssignmentMap GetMaterialAssignmentsFromModel(Data::Instance<AZ::RPI::Model> model);

        //! Find an assignment id corresponding to the lod and label substring filters
        MaterialAssignmentId FindMaterialAssignmentIdInModel(
            const Data::Instance<AZ::RPI::Model>& model, const MaterialAssignmentLodIndex lodFilter, const AZStd::string& labelFilter);

        //! Special case handling to convert script values to supported types
        AZ::RPI::MaterialPropertyValue ConvertMaterialPropertyValueFromScript(
            const AZ::RPI::MaterialPropertyDescriptor* propertyDescriptor, const AZStd::any& value);

    } // namespace Render
} // namespace AZ
