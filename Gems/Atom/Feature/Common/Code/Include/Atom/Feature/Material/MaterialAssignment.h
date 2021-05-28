/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

            //! Returns a string composed of the asset path.
            AZStd::string ToString() const;

            Data::Asset<RPI::MaterialAsset> m_materialAsset;
            Data::Instance<RPI::Material> m_materialInstance;
            MaterialPropertyOverrideMap m_propertyOverrides;
            RPI::MaterialModelUvOverrideMap m_matModUvOverrides;
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

    } // namespace Render
} // namespace AZ
