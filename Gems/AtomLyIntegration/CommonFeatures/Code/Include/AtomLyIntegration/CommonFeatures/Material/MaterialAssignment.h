/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialAssignmentId.h>
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
            AZ_CLASS_ALLOCATOR(AZ::Render::MaterialAssignment, SystemAllocator);
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
            bool m_materialInstanceMustBeUnique = false;
        };

        using MaterialAssignmentMap = AZStd::unordered_map<MaterialAssignmentId, MaterialAssignment>;
        using MaterialAssignmentLabelMap = AZStd::unordered_map<MaterialAssignmentId, AZStd::string>;
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
        MaterialAssignmentMap GetDefaultMaterialMapFromModelAsset(const Data::Asset<AZ::RPI::ModelAsset> modelAsset);

        //! Get material slot labels from a model
        MaterialAssignmentLabelMap GetMaterialSlotLabelsFromModelAsset(const Data::Asset<AZ::RPI::ModelAsset> modelAsset);

        //! Find an assignment id corresponding to the lod and label substring filters
        MaterialAssignmentId GetMaterialSlotIdFromModelAsset(
            const Data::Asset<AZ::RPI::ModelAsset> modelAsset,
            const MaterialAssignmentLodIndex lodFilter,
            const AZStd::string& labelFilter);

        //! Special case handling to convert script values to supported types
        AZ::RPI::MaterialPropertyValue ConvertMaterialPropertyValueFromScript(
            const AZ::RPI::MaterialPropertyDescriptor* propertyDescriptor, const AZStd::any& value);

        //! Converting material assignment map to a mesh feature processor custom material map
        AZ::Render::CustomMaterialMap ConvertToCustomMaterialMap(const AZ::Render::MaterialAssignmentMap& materials);
    } // namespace Render
} // namespace AZ
