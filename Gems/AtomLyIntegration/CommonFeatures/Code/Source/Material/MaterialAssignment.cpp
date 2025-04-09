/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialAssignment.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        void MaterialAssignment::Reflect(ReflectContext* context)
        {
            MaterialAssignmentId::Reflect(context);

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
            : MaterialAssignment(Data::Asset<RPI::MaterialAsset>(materialAssetId, AZ::AzTypeInfo<AZ::RPI::MaterialAsset>::Uuid()))
        {
        }

        MaterialAssignment::MaterialAssignment(const Data::Asset<RPI::MaterialAsset>& asset)
            : MaterialAssignment(asset, Data::Instance<RPI::Material>())
        {
        }

        MaterialAssignment::MaterialAssignment(const Data::Asset<RPI::MaterialAsset>& asset, const Data::Instance<RPI::Material>& instance)
            : m_defaultMaterialAsset(asset)
            , m_materialAsset(asset)
            , m_materialInstance(instance)
        {
        }

        void MaterialAssignment::RebuildInstance()
        {
            if (m_materialInstancePreCreated)
            {
                return;
            }

            if (m_materialAsset.GetId().IsValid() && m_materialAsset.IsReady())
            {
                const bool createUniqueInstance = m_materialInstanceMustBeUnique || !m_propertyOverrides.empty();
                m_materialInstance = createUniqueInstance ? RPI::Material::Create(m_materialAsset) : RPI::Material::FindOrCreate(m_materialAsset);
                AZ_Error("MaterialAssignment", m_materialInstance, "Material instance not initialized");
                return;
            }

            if (m_defaultMaterialAsset.GetId().IsValid() && m_defaultMaterialAsset.IsReady())
            {
                const bool createUniqueInstance = m_materialInstanceMustBeUnique || !m_propertyOverrides.empty();
                m_materialInstance = createUniqueInstance ? RPI::Material::Create(m_defaultMaterialAsset) : RPI::Material::FindOrCreate(m_defaultMaterialAsset);
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

            // GCC is incorrectly flagging the const reference returned from GetMaterialAssignmentFromMap
            // as a dangling reference, despite the function returning a const reference.
        AZ_PUSH_DISABLE_WARNING_GCC("-Wdangling-reference")
            const MaterialAssignment& assetAssignment =
                GetMaterialAssignmentFromMap(materials, MaterialAssignmentId::CreateFromStableIdOnly(id.m_materialSlotStableId));
        AZ_POP_DISABLE_WARNING_GCC
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

        MaterialAssignmentMap GetDefaultMaterialMapFromModelAsset(const Data::Asset<AZ::RPI::ModelAsset> modelAsset)
        {
            MaterialAssignmentMap materials;
            materials[DefaultMaterialAssignmentId] = MaterialAssignment();

            if (modelAsset.IsReady())
            {
                MaterialAssignmentLodIndex lodIndex = 0;
                for (const auto& lod : modelAsset->GetLodAssets())
                {
                    for (const auto& mesh : lod->GetMeshes())
                    {
                        const auto slotId = mesh.GetMaterialSlotId();
                        const auto& slot = modelAsset->FindMaterialSlot(slotId);

                        const auto generalId = MaterialAssignmentId::CreateFromStableIdOnly(slotId);
                        materials[generalId] = MaterialAssignment(slot.m_defaultMaterialAsset);

                        const auto specificId = MaterialAssignmentId::CreateFromLodAndStableId(lodIndex, slotId);
                        materials[specificId] = MaterialAssignment(slot.m_defaultMaterialAsset);
                    }
                    ++lodIndex;
                }
            }

            return materials;
        }

        MaterialAssignmentLabelMap GetMaterialSlotLabelsFromModelAsset(const Data::Asset<AZ::RPI::ModelAsset> modelAsset)
        {
            MaterialAssignmentLabelMap labels;
            labels[DefaultMaterialAssignmentId] = "Default Material";

            if (modelAsset.IsReady())
            {
                MaterialAssignmentLodIndex lodIndex = 0;
                for (const Data::Asset<RPI::ModelLodAsset>& lod : modelAsset->GetLodAssets())
                {
                    for (const RPI::ModelLodAsset::Mesh& mesh : lod->GetMeshes())
                    {
                        const RPI::ModelMaterialSlot::StableId slotId = mesh.GetMaterialSlotId();
                        const RPI::ModelMaterialSlot& slot = modelAsset->FindMaterialSlot(slotId);

                        const MaterialAssignmentId generalId = MaterialAssignmentId::CreateFromStableIdOnly(slotId);
                        labels[generalId] = slot.m_displayName.GetStringView();

                        const MaterialAssignmentId specificId = MaterialAssignmentId::CreateFromLodAndStableId(lodIndex, slotId);
                        labels[specificId] = slot.m_displayName.GetStringView();
                    }
                    ++lodIndex;
                }
            }

            return labels;
        }

        MaterialAssignmentId GetMaterialSlotIdFromLodAsset(
            const Data::Asset<AZ::RPI::ModelAsset> modelAsset,
            const Data::Asset<AZ::RPI::ModelLodAsset>& lodAsset,
            const MaterialAssignmentLodIndex lodIndex,
            const AZStd::string& labelFilter)
        {
            for (const AZ::RPI::ModelLodAsset::Mesh& mesh : lodAsset->GetMeshes())
            {
                const auto slotId = mesh.GetMaterialSlotId();
                const auto& slot = modelAsset->FindMaterialSlot(slotId);
                if (AZ::StringFunc::Contains(slot.m_displayName.GetCStr(), labelFilter, true))
                {
                    return MaterialAssignmentId::CreateFromLodAndStableId(lodIndex, slotId);
                }
            }
            return MaterialAssignmentId();
        }

        MaterialAssignmentId GetMaterialSlotIdFromModelAsset(
            const Data::Asset<AZ::RPI::ModelAsset> modelAsset, const MaterialAssignmentLodIndex lodFilter, const AZStd::string& labelFilter)
        {
            if (modelAsset.IsReady() && !labelFilter.empty())
            {
                if (lodFilter < modelAsset->GetLodCount())
                {
                    return GetMaterialSlotIdFromLodAsset(modelAsset, modelAsset->GetLodAssets()[lodFilter], lodFilter, labelFilter);
                }

                for (size_t lodIndex = 0; lodIndex < modelAsset->GetLodCount(); ++lodIndex)
                {
                    const MaterialAssignmentId result = GetMaterialSlotIdFromLodAsset(
                        modelAsset, modelAsset->GetLodAssets()[lodIndex], MaterialAssignmentId::NonLodIndex, labelFilter);
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

        AZ::Render::CustomMaterialMap ConvertToCustomMaterialMap(const AZ::Render::MaterialAssignmentMap& materials)
        {
            AZ::Render::CustomMaterialMap customMaterials;
            customMaterials.reserve(materials.size());
            for (const auto& materialAssignment : materials)
            {
                customMaterials.emplace(
                    AZ::Render::CustomMaterialId{ materialAssignment.first.m_lodIndex, materialAssignment.first.m_materialSlotStableId },
                    AZ::Render::CustomMaterialInfo{ materialAssignment.second.m_materialInstance, materialAssignment.second.m_matModUvOverrides });
            }
            return customMaterials;
        }
    } // namespace Render
} // namespace AZ
