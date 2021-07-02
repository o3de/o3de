/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Material/MaterialAssignment.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Material/MaterialAssignmentSerializer.h>

namespace AZ
{
    namespace Render
    {
        void MaterialAssignment::Reflect(ReflectContext* context)
        {
            MaterialAssignmentId::Reflect(context);

            if (auto jsonContext = azrtti_cast<JsonRegistrationContext*>(context))
            {
                jsonContext->Serializer<JsonMaterialAssignmentSerializer>()->HandlesType<MaterialAssignment>();
            }

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<MaterialAssignmentMap>();
                serializeContext->RegisterGenericType<MaterialPropertyOverrideMap>();

                serializeContext->Class<MaterialAssignment>()
                    ->Version(1)
                    ->Field("MaterialAsset", &MaterialAssignment::m_materialAsset)
                    ->Field("PropertyOverrides", &MaterialAssignment::m_propertyOverrides)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<MaterialAssignment>("MaterialAssignment")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Constructor()
                    ->Constructor<const MaterialAssignment&>()
                    ->Constructor<const AZ::Data::AssetId&>()
                    ->Constructor<const Data::Asset<RPI::MaterialAsset>&>()
                    ->Constructor<const Data::Asset<RPI::MaterialAsset>&, const Data::Instance<RPI::Material>&>()
                    ->Method("ToString", &MaterialAssignment::ToString)
                    ->Property("materialAsset", BehaviorValueProperty(&MaterialAssignment::m_materialAsset))
                    ->Property("propertyOverrides", BehaviorValueProperty(&MaterialAssignment::m_propertyOverrides))
                    ;

                behaviorContext->ConstantProperty("DefaultMaterialAssignment", BehaviorConstant(DefaultMaterialAssignment))
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render");

                behaviorContext->ConstantProperty("DefaultMaterialAssignmentId", BehaviorConstant(DefaultMaterialAssignmentId))
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render");

                behaviorContext->ConstantProperty("DefaultMaterialAssignmentMap", BehaviorConstant(DefaultMaterialAssignmentMap))
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render");

            }
        }

        MaterialAssignment::MaterialAssignment(const AZ::Data::AssetId& materialAssetId)
            : m_materialInstance()
        {
            m_materialAsset.Create(materialAssetId);
        }

        MaterialAssignment::MaterialAssignment(const Data::Asset<RPI::MaterialAsset>& asset)
            : m_materialAsset(asset)
            , m_materialInstance()
        {
        }

        MaterialAssignment::MaterialAssignment(const Data::Asset<RPI::MaterialAsset>& asset, const Data::Instance<RPI::Material>& instance)
            : m_materialAsset(asset)
            , m_materialInstance(instance)
        {
        }

        void MaterialAssignment::RebuildInstance()
        {
            if (m_materialAsset.IsReady())
            {
                m_materialInstance =
                    m_propertyOverrides.empty() ? RPI::Material::FindOrCreate(m_materialAsset) : RPI::Material::Create(m_materialAsset);
                AZ_Error("MaterialAssignment", m_materialInstance, "Material instance not initialized");
            }
        }

        AZStd::string MaterialAssignment::ToString() const
        {
            AZStd::string assetPathString;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetPathString, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_materialAsset.GetId());
            return assetPathString;
        }

        const MaterialAssignment& GetMaterialAssignmentFromMap(const MaterialAssignmentMap& materials, const MaterialAssignmentId& id)
        {
            const auto& materialItr = materials.find(id);
            return materialItr != materials.end() ? materialItr->second : DefaultMaterialAssignment;
        }

        const MaterialAssignment& GetMaterialAssignmentFromMapWithFallback(
            const MaterialAssignmentMap& materials, const MaterialAssignmentId& id)
        {
            const MaterialAssignment& lodAssignment = GetMaterialAssignmentFromMap(materials, id);
            if (lodAssignment.m_materialInstance.get())
            {
                return lodAssignment;
            }

            const MaterialAssignment& assetAssignment =
                GetMaterialAssignmentFromMap(materials, MaterialAssignmentId::CreateFromAssetOnly(id.m_materialAssetId));
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

        MaterialAssignmentMap GetMaterialAssignmentsFromModel(Data::Instance<AZ::RPI::Model> model)
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

                            const MaterialAssignmentId specificId =
                                MaterialAssignmentId::CreateFromLodAndAsset(lodIndex, mesh.m_material->GetAssetId());
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
