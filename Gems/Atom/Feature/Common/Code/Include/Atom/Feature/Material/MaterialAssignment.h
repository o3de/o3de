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

#include <Atom/RPI.Public/Material/Material.h>
#include <AzCore/std/containers/unordered_map.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/Feature/Material/MaterialAssignmentId.h>
#include <AzCore/Asset/AssetManagerBus.h>

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

            MaterialAssignment(const AZ::Data::AssetId& materialAssetId)
                : m_materialInstance()
            {
                m_materialAsset.Create(materialAssetId);
            }

            MaterialAssignment(const Data::Asset<RPI::MaterialAsset>& asset)
                : m_materialAsset(asset)
                , m_materialInstance()
            {
            }

            MaterialAssignment(const Data::Asset<RPI::MaterialAsset>& asset, const Data::Instance<RPI::Material>& instance)
                : m_materialAsset(asset)
                , m_materialInstance(instance)
            {
            }

            void RebuildInstance()
            {
                if (m_materialAsset.IsReady())
                {
                    m_materialInstance = m_propertyOverrides.empty() ? RPI::Material::FindOrCreate(m_materialAsset) : RPI::Material::Create(m_materialAsset);
                    AZ_Error("MaterialAssignment", m_materialInstance, "Material instance not initialized");
                }
            }

            AZStd::string ToString() const
            {
                AZStd::string assetPathString;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPathString, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_materialAsset.GetId());
                return assetPathString;
            }

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
        AZ_INLINE const MaterialAssignment& GetMaterialAssignmentFromMap(const MaterialAssignmentMap& materials, const MaterialAssignmentId& id)
        {
            const auto& materialItr = materials.find(id);
            return materialItr != materials.end() ? materialItr->second : DefaultMaterialAssignment;
        }

        //! Utility function for retrieving a material entry from a MaterialAssignmentMap, falling back to defaults for a particular asset or the entire model
        AZ_INLINE const MaterialAssignment& GetMaterialAssignmentFromMapWithFallback(const MaterialAssignmentMap& materials, const MaterialAssignmentId& id)
        {
            const MaterialAssignment& lodAssignment = GetMaterialAssignmentFromMap(materials, id);
            if (lodAssignment.m_materialInstance.get())
            {
                return lodAssignment;
            }

            const MaterialAssignment& assetAssignment = GetMaterialAssignmentFromMap(materials, MaterialAssignmentId::CreateFromAssetOnly(id.m_materialAssetId));
            if (assetAssignment.m_materialInstance.get())
            {
                return assetAssignment;
            }

            const MaterialAssignment& defaultAssignment = GetMaterialAssignmentFromMap(materials, DefaultMaterialAssignmentId);
            if (defaultAssignment.m_materialInstance.get())
            {
                return defaultAssignment;
            }

            return DefaultMaterialAssignment;
        }

        //! Utility function for generating a set of available material assignments in a model
        AZ_INLINE MaterialAssignmentMap GetMaterialAssignmentsFromModel(Data::Instance<AZ::RPI::Model> model)
        {
            MaterialAssignmentMap materials;
            materials[DefaultMaterialAssignmentId] = MaterialAssignment();

            if (model)
            {
                size_t lodIndex = 0;
                for (const Data::Instance<AZ::RPI::ModelLod>& lod : model->GetLods())
                {
                    for (const AZ::RPI::ModelLod::Mesh& mesh : lod->GetMeshes())
                    {
                        if (mesh.m_material)
                        {
                            const MaterialAssignmentId generalId = MaterialAssignmentId::CreateFromAssetOnly(mesh.m_material->GetAssetId());
                            materials[generalId] = MaterialAssignment(mesh.m_material->GetAsset(), mesh.m_material);

                            const MaterialAssignmentId specificId = MaterialAssignmentId::CreateFromLodAndAsset(lodIndex, mesh.m_material->GetAssetId());
                            materials[specificId] = MaterialAssignment(mesh.m_material->GetAsset(), mesh.m_material);
                        }
                    }
                    ++lodIndex;
                }
            }

            return materials;
        }

    } // namespace Render
} // namespace AZ
