/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Material/MaterialAssignment.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
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
                    ->Version(2)
                    ->Field("MaterialAsset", &MaterialAssignment::m_materialAsset)
                    ->Field("PropertyOverrides", &MaterialAssignment::m_propertyOverrides)
                    ->Field("ModelUvOverrides", &MaterialAssignment::m_matModUvOverrides)
                    ;

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<MaterialAssignment>(
                        "Material Assignment", "Material Assignment")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialAssignment::m_materialAsset, "Material Asset", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialAssignment::m_propertyOverrides, "Property Overrides", "")
                        ;
                }
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
                    ->Property("propertyOverrides", BehaviorValueProperty(&MaterialAssignment::m_propertyOverrides));

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
            : m_materialAsset(materialAssetId, AZ::AzTypeInfo<AZ::RPI::MaterialAsset>::Uuid())
            , m_materialInstance()
        {
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
            if (m_materialInstancePreCreated)
            {
                return;
            }

            if (m_materialAsset.IsReady())
            {
                m_materialInstance = m_propertyOverrides.empty() ? RPI::Material::FindOrCreate(m_materialAsset) : RPI::Material::Create(m_materialAsset);
                AZ_Error("MaterialAssignment", m_materialInstance, "Material instance not initialized");
                return;
            }

            if (m_defaultMaterialAsset.IsReady())
            {
                m_materialInstance = m_propertyOverrides.empty() ? RPI::Material::FindOrCreate(m_defaultMaterialAsset) : RPI::Material::Create(m_defaultMaterialAsset);
                AZ_Error("MaterialAssignment", m_materialInstance, "Material instance not initialized");
                return;
            }

            // Clear the existing material instance if no asset was ready
            m_materialInstance = nullptr;
        }

        void MaterialAssignment::Release()
        {
            if (!m_materialInstancePreCreated)
            {
                m_materialInstance = nullptr;
            }
            m_materialAsset.Release();
            m_defaultMaterialAsset.Release();
        }

        bool MaterialAssignment::RequiresLoading() const
        {
            return
                !m_materialInstancePreCreated &&
                !m_materialAsset.IsReady() &&
                !m_materialAsset.IsLoading() &&
                !m_defaultMaterialAsset.IsReady() &&
                !m_defaultMaterialAsset.IsLoading();
        }

        bool MaterialAssignment::ApplyProperties()
        {
            // Immediately return true, skipping this material, if there is no instance or no properties to apply
            if (!m_materialInstance || m_propertyOverrides.empty())
            {
                return true;
            }

            for (const auto& propertyPair : m_propertyOverrides)
            {
                if (!propertyPair.second.empty())
                {
                    bool wasRenamed = false;
                    Name newName;
                    RPI::MaterialPropertyIndex materialPropertyIndex =
                        m_materialInstance->FindPropertyIndex(propertyPair.first, &wasRenamed, &newName);

                    // FindPropertyIndex will have already reported a message about what the old and new names are. Here we just add
                    // some extra info to help the user resolve it.
                    AZ_Warning(
                        "MaterialAssignment", !wasRenamed,
                        "Consider running \"Apply Automatic Property Updates\" to use the latest property names.",
                        propertyPair.first.GetCStr(), newName.GetCStr());

                    if (wasRenamed && m_propertyOverrides.find(newName) != m_propertyOverrides.end())
                    {
                        materialPropertyIndex.Reset();

                        AZ_Warning(
                            "MaterialAssignment", false,
                            "Material property '%s' has been renamed to '%s', and a property override exists for both. The one with "
                            "the old name will be ignored.",
                            propertyPair.first.GetCStr(), newName.GetCStr());
                    }

                    if (!materialPropertyIndex.IsNull())
                    {
                        const auto propertyDescriptor =
                            m_materialInstance->GetMaterialPropertiesLayout()->GetPropertyDescriptor(materialPropertyIndex);

                        m_materialInstance->SetPropertyValue(
                            materialPropertyIndex, ConvertMaterialPropertyValueFromScript(propertyDescriptor, propertyPair.second));
                    }
                }
            }

            // Return true if there is nothing to compile, meaning no properties changed, or the compile succeeded
            return !m_materialInstance->NeedsCompile() || m_materialInstance->Compile();
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
                GetMaterialAssignmentFromMap(materials, MaterialAssignmentId::CreateFromStableIdOnly(id.m_materialSlotStableId));
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
                            const MaterialAssignmentId generalId =
                                MaterialAssignmentId::CreateFromStableIdOnly(mesh.m_materialSlotStableId);
                            materials[generalId] = MaterialAssignment(mesh.m_material->GetAsset(), mesh.m_material);

                            const MaterialAssignmentId specificId =
                                MaterialAssignmentId::CreateFromLodAndStableId(lodIndex, mesh.m_materialSlotStableId);
                            materials[specificId] = MaterialAssignment(mesh.m_material->GetAsset(), mesh.m_material);
                        }
                    }
                    ++lodIndex;
                }
            }

            return materials;
        }

        MaterialAssignmentId FindMaterialAssignmentIdInLod(
            const Data::Instance<AZ::RPI::Model>& model,
            const Data::Instance<AZ::RPI::ModelLod>& lod,
            const MaterialAssignmentLodIndex lodIndex,
            const AZStd::string& labelFilter)
        {
            for (const AZ::RPI::ModelLod::Mesh& mesh : lod->GetMeshes())
            {
                const AZ::RPI::ModelMaterialSlot& slot = model->GetModelAsset()->FindMaterialSlot(mesh.m_materialSlotStableId);
                if (AZ::StringFunc::Contains(slot.m_displayName.GetCStr(), labelFilter, true))
                {
                    return MaterialAssignmentId::CreateFromLodAndStableId(lodIndex, mesh.m_materialSlotStableId);
                }
            }
            return MaterialAssignmentId();
        }

        MaterialAssignmentId FindMaterialAssignmentIdInModel(
            const Data::Instance<AZ::RPI::Model>& model, const MaterialAssignmentLodIndex lodFilter, const AZStd::string& labelFilter)
        {
            if (model && !labelFilter.empty())
            {
                if (lodFilter < model->GetLodCount())
                {
                    return FindMaterialAssignmentIdInLod(model, model->GetLods()[lodFilter], lodFilter, labelFilter);
                }

                for (size_t lodIndex = 0; lodIndex < model->GetLodCount(); ++lodIndex)
                {
                    const MaterialAssignmentId result =
                        FindMaterialAssignmentIdInLod(model, model->GetLods()[lodIndex], MaterialAssignmentId::NonLodIndex, labelFilter);
                    if (!result.IsDefault())
                    {
                        return result;
                    }
                }
            }

            return MaterialAssignmentId();
        }

        template<typename T>
        AZ::RPI::MaterialPropertyValue ConvertMaterialPropertyValueNumericType(const AZStd::any& value)
        {
            if (value.is<int32_t>())
            {
                return aznumeric_cast<T>(AZStd::any_cast<int32_t>(value));
            }
            if (value.is<uint32_t>())
            {
                return aznumeric_cast<T>(AZStd::any_cast<uint32_t>(value));
            }
            if (value.is<float>())
            {
                return aznumeric_cast<T>(AZStd::any_cast<float>(value));
            }
            if (value.is<double>())
            {
                return aznumeric_cast<T>(AZStd::any_cast<double>(value));
            }

            return AZ::RPI::MaterialPropertyValue::FromAny(value);
        }

        AZ::RPI::MaterialPropertyValue ConvertMaterialPropertyValueFromScript(
            const AZ::RPI::MaterialPropertyDescriptor* propertyDescriptor, const AZStd::any& value)
        {
            switch (propertyDescriptor->GetDataType())
            {
            case AZ::RPI::MaterialPropertyDataType::Enum:
                if (value.is<AZ::Name>())
                {
                    return propertyDescriptor->GetEnumValue(AZStd::any_cast<AZ::Name>(value));
                }
                if (value.is<AZStd::string>())
                {
                    return propertyDescriptor->GetEnumValue(AZ::Name(AZStd::any_cast<AZStd::string>(value)));
                }
                return ConvertMaterialPropertyValueNumericType<uint32_t>(value);
            case AZ::RPI::MaterialPropertyDataType::Int:
                return ConvertMaterialPropertyValueNumericType<int32_t>(value);
            case AZ::RPI::MaterialPropertyDataType::UInt:
                return ConvertMaterialPropertyValueNumericType<uint32_t>(value);
            case AZ::RPI::MaterialPropertyDataType::Float:
                return ConvertMaterialPropertyValueNumericType<float>(value);
            case AZ::RPI::MaterialPropertyDataType::Bool:
                return ConvertMaterialPropertyValueNumericType<bool>(value);
            default:
                break;
            }

            return AZ::RPI::MaterialPropertyValue::FromAny(value);
        }
    } // namespace Render
} // namespace AZ
