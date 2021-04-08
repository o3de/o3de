/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <AzCore/IO/FileIO.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/NameConstants.h>
#include <AzFramework/Physics/ClassConverters.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/PhysicsSystem.h>

namespace
{
    const char* const s_entireObjectSlotName = "Entire object";
    const AZ::Data::Asset<Physics::MaterialLibraryAsset> s_invalidMaterialLibrary = { AZ::Data::AssetLoadBehavior::NoLoad };
}

namespace Physics
{
    class MaterialLibraryAssetEventHandler
        : public AZ::SerializeContext::IEventHandler
    {
        void OnReadBegin(void* classPtr)
        {
            auto matAsset = static_cast<MaterialLibraryAsset*>(classPtr);
            matAsset->GenerateMissingIds();
        }
    };

    class MaterialSelectionEventHandler
        : public AZ::SerializeContext::IEventHandler
    {
        void OnReadEnd(void* classPtr)
        {
            auto materialSelection = static_cast<MaterialSelection*>(classPtr);
            if (materialSelection->GetMaterialIdsAssignedToSlots().empty())
            {
                materialSelection->SetMaterialSlots(Physics::MaterialSelection::SlotsArray());
            }
            if (materialSelection->IsDefaultMaterialLibraryAsset())
            {
                materialSelection->SyncSelectionToMaterialLibrary();
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////

    const AZ::Crc32 MaterialConfiguration::s_stringGroup = AZ_CRC("StringGroup", 0x878e4bbd);
    const AZ::Crc32 MaterialConfiguration::s_forbiddenStringSet = AZ_CRC("ForbiddenStringSet", 0x8c132196);
    const AZ::Crc32 MaterialConfiguration::s_configLineEdit = AZ_CRC("ConfigLineEdit", 0x3e41d737);

    void MaterialConfiguration::Reflect(AZ::ReflectContext* context)
    {
        MaterialId::Reflect(context);
        MaterialFromAssetConfiguration::Reflect(context);
        MaterialSelection::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Physics::MaterialConfiguration>()
                ->Version(3, &VersionConverter)
                ->Field("SurfaceType", &MaterialConfiguration::m_surfaceType)
                ->Field("DynamicFriction", &MaterialConfiguration::m_dynamicFriction)
                ->Field("StaticFriction", &MaterialConfiguration::m_staticFriction)
                ->Field("Restitution", &MaterialConfiguration::m_restitution)
                ->Field("FrictionCombine", &MaterialConfiguration::m_frictionCombine)
                ->Field("RestitutionCombine", &MaterialConfiguration::m_restitutionCombine)
                ->Field("Density", &MaterialConfiguration::m_density)
                ->Field("DebugColor", &MaterialConfiguration::m_debugColor)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                AZStd::unordered_set<AZStd::string> forbiddenSurfaceTypeNames;
                forbiddenSurfaceTypeNames.insert("Default");
                editContext->Class<Physics::MaterialConfiguration>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Physics Material")
                    ->DataElement(MaterialConfiguration::s_configLineEdit, &MaterialConfiguration::m_surfaceType, "Surface type", "Game surface type") // Uses ConfigStringLineEditCtrl in PhysX gem.
                        ->Attribute(AZ::Edit::Attributes::MaxLength, 64)
                        ->Attribute(MaterialConfiguration::s_stringGroup, AZ_CRC("LineEditGroupSurfaceType", 0x6670659e))
                        ->Attribute(MaterialConfiguration::s_forbiddenStringSet, forbiddenSurfaceTypeNames)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_staticFriction, "Static friction", "Friction coefficient when object is still")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_dynamicFriction, "Dynamic friction", "Friction coefficient when object is moving")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_restitution, "Restitution", "Restitution coefficient")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MaterialConfiguration::m_frictionCombine, "Friction combine", "How the friction is combined between colliding objects")
                        ->EnumAttribute(Material::CombineMode::Average, "Average")
                        ->EnumAttribute(Material::CombineMode::Minimum, "Minimum")
                        ->EnumAttribute(Material::CombineMode::Maximum, "Maximum")
                        ->EnumAttribute(Material::CombineMode::Multiply, "Multiply")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MaterialConfiguration::m_restitutionCombine, "Restitution combine", "How the restitution is combined between colliding objects")
                        ->EnumAttribute(Material::CombineMode::Average, "Average")
                        ->EnumAttribute(Material::CombineMode::Minimum, "Minimum")
                        ->EnumAttribute(Material::CombineMode::Maximum, "Maximum")
                        ->EnumAttribute(Material::CombineMode::Multiply, "Multiply")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_density, "Density", "Material density")
                        ->Attribute(AZ::Edit::Attributes::Min, MaterialConfiguration::MinDensityLimit)
                        ->Attribute(AZ::Edit::Attributes::Max, MaterialConfiguration::MaxDensityLimit)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetDensityUnit())

                    ->DataElement(AZ::Edit::UIHandlers::Color, &MaterialConfiguration::m_debugColor, "Debug Color", "Debug color to use for this material")
                    ;
            }
        }
    }

    AZ::Color MaterialConfiguration::GenerateDebugColor(const char* materialName)
    {
        static const AZ::Color colors[] =
        {
            AZ::Colors::Aqua,   AZ::Colors::Silver,
            AZ::Colors::Gray,   AZ::Colors::Maroon,
            AZ::Colors::Green,  AZ::Colors::Blue,
            AZ::Colors::Navy,   AZ::Colors::Yellow,
            AZ::Colors::Orange, AZ::Colors::Olive,
            AZ::Colors::Purple, AZ::Colors::Fuchsia,
            AZ::Colors::Teal,   AZ::Colors::Lime,
            AZ::Colors::White
        };

        unsigned int selection = static_cast<unsigned int>(AZ_CRC(materialName)) % AZ_ARRAY_SIZE(colors);
        return colors[selection];
    }

    bool MaterialConfiguration::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        bool success = true;

        if (classElement.GetVersion() <= 1)
        {
            const int surfaceTypeElemIndex = classElement.FindElement(AZ_CRC("SurfaceType", 0x8b1fc300));
            AZ::Color debugColor = AZ::Colors::White;
            if (surfaceTypeElemIndex >= 0)
            {
                AZStd::string surfaceType;
                AZ::SerializeContext::DataElementNode& surfaceTypeElem = classElement.GetSubElement(surfaceTypeElemIndex);
                surfaceTypeElem.GetData(surfaceType);
                debugColor = GenerateDebugColor(surfaceType.c_str());
            }
            classElement.AddElementWithData(context, "DebugColor", debugColor);
        }

        return success;
    }

    //////////////////////////////////////////////////////////////////////////

    void MaterialLibraryAsset::Reflect(AZ::ReflectContext * context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Physics::MaterialLibraryAsset, AZ::Data::AssetData>()
                ->Version(2, &ClassConverters::MaterialLibraryAssetConverter)
                ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                ->EventHandler<MaterialLibraryAssetEventHandler>()
                ->Field("Properties", &MaterialLibraryAsset::m_materialLibrary)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<Physics::MaterialLibraryAsset>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialLibraryAsset::m_materialLibrary, "Physics Materials", "List of physics materials")
                        ->Attribute("EditButton", "")
                        ->Attribute(AZ::Edit::Attributes::ForceAutoExpand, true)
                    ;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void MaterialLibraryAssetReflectionWrapper::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Physics::MaterialLibraryAssetReflectionWrapper>()
                ->Version(1)
                ->Field("Asset", &MaterialLibraryAssetReflectionWrapper::m_asset)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<Physics::MaterialLibraryAssetReflectionWrapper>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialLibraryAssetReflectionWrapper::m_asset, "Physics Material Library", "Physics Material Library")
                        ->Attribute("EditButton", "")
                    ;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////


    void DefaultMaterialLibraryAssetReflectionWrapper::Reflect(AZ::ReflectContext* context)
    {
         AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Physics::DefaultMaterialLibraryAssetReflectionWrapper>()
                ->Version(1)
                ->Field("Asset", &DefaultMaterialLibraryAssetReflectionWrapper::m_asset)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<Physics::DefaultMaterialLibraryAssetReflectionWrapper>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DefaultMaterialLibraryAssetReflectionWrapper::m_asset, "Default Physics Material Library", "Library to use by default")
                        ->Attribute(AZ::Edit::Attributes::AllowClearAsset, false)
                        ->Attribute("EditButton", "")
                    ;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void MaterialFromAssetConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Physics::MaterialFromAssetConfiguration>()
                ->Version(1)
                ->Field("Configuration", &MaterialFromAssetConfiguration::m_configuration)
                ->Field("UID", &MaterialFromAssetConfiguration::m_id)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<Physics::MaterialFromAssetConfiguration>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialFromAssetConfiguration::m_configuration, "Physics Material", "Physics Material properties")
                        ->Attribute(AZ::Edit::Attributes::ForceAutoExpand, true)
                    ;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////

    bool MaterialLibraryAsset::GetDataForMaterialId(const MaterialId& materialId, MaterialFromAssetConfiguration& configuration) const
    {
        auto foundMaterialConfiguration = AZStd::find_if(m_materialLibrary.begin(), m_materialLibrary.end(), [materialId](const auto& data)
        {
            return data.m_id == materialId;
        });

        if (foundMaterialConfiguration != m_materialLibrary.end())
        {
            configuration = *foundMaterialConfiguration;
            return true;
        }

        return false;
    }

    bool MaterialLibraryAsset::HasDataForMaterialId(const MaterialId& materialId) const
    {
        auto foundMaterialConfiguration = AZStd::find_if(m_materialLibrary.begin(), m_materialLibrary.end(), [materialId](const auto& data)
        {
            return data.m_id == materialId;
        });
        return foundMaterialConfiguration != m_materialLibrary.end();
    }

    bool MaterialLibraryAsset::GetDataForMaterialName(const AZStd::string& materialName, MaterialFromAssetConfiguration& configuration) const
    {
        auto foundMaterialConfiguration = AZStd::find_if(m_materialLibrary.begin(), m_materialLibrary.end(), [&materialName](const auto& data)
        {
            return data.m_configuration.m_surfaceType == materialName;
        });

        if (foundMaterialConfiguration != m_materialLibrary.end())
        {
            configuration = *foundMaterialConfiguration;
            return true;
        }

        return false;
    }

    void MaterialLibraryAsset::AddMaterialData(const MaterialFromAssetConfiguration& data)
    {
        MaterialFromAssetConfiguration existingConfiguration;
        if (!data.m_id.IsNull() && GetDataForMaterialId(data.m_id, existingConfiguration))
        {
            AZ_Warning("MaterialLibraryAsset", false, "Trying to add material that already exists");
            return;
        }

        m_materialLibrary.push_back(data);
        GenerateMissingIds();
    }

    void MaterialLibraryAsset::GenerateMissingIds()
    {
        for (auto& materialData : m_materialLibrary)
        {
            if (materialData.m_id.IsNull())
            {
                materialData.m_id = MaterialId::Create();
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void MaterialId::Reflect(AZ::ReflectContext * context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Physics::MaterialId>()
                ->Version(1)
                ->Field("MaterialId", &Physics::MaterialId::m_id)
                ;
        }
    }

    MaterialId MaterialId::Create()
    {
        MaterialId id; 
        id.m_id = AZ::Uuid::Create(); 
        return id;
    }

    MaterialId MaterialId::FromUUID(const AZ::Uuid& uuid)
    {
        MaterialId id;
        id.m_id = uuid;
        return id;
    }

    //////////////////////////////////////////////////////////////////////////

    void MaterialSelection::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Physics::MaterialSelection>()
                ->Version(2, &ClassConverters::MaterialSelectionConverter)
                ->EventHandler<MaterialSelectionEventHandler>()
                ->Field("Material", &MaterialSelection::m_materialLibrary)
                ->Field("MaterialIds", &MaterialSelection::m_materialIdsAssignedToSlots)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Physics::MaterialSelection>("Physics Material", "Select physics material library and which materials to use for the object")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialSelection::m_materialLibrary, "Library", "Physics material library to use for this object")
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                        ->Attribute("EditButton", "")
                        ->Attribute("EditDescription", "Open in Asset Editor")
                        ->Attribute(AZ::Edit::Attributes::DefaultAsset, &MaterialSelection::GetDefaultMaterialLibraryId)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MaterialSelection::OnMaterialLibraryChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialSelection::m_materialIdsAssignedToSlots, "Mesh Surfaces", "Specify which Physics Material to use for each element of this object")
                        ->ElementAttribute(Attributes::MaterialLibraryAssetId, &MaterialSelection::GetMaterialLibraryAssetId)
                        ->Attribute(AZ::Edit::Attributes::IndexedChildNameLabelOverride, &MaterialSelection::GetMaterialSlotLabel)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->ElementAttribute(AZ::Edit::Attributes::ReadOnly, &MaterialSelection::AreMaterialSlotsReadOnly)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                    ;
            }
        }
    }

    AZ::u32 MaterialSelection::OnMaterialLibraryChanged()
    {
        SyncSelectionToMaterialLibrary();
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZStd::string MaterialSelection::GetMaterialSlotLabel(int index)
    {
        if (index < m_materialSlots.size())
        {
            return m_materialSlots[index];
        }
        else if (m_materialIdsAssignedToSlots.size() == 1)
        {
            // this is valid scenario to allow MaterialSelection to
            // be used by just reflecting it to editorContext, in simple cases
            // when we only need to assign one default material, such as terrain or ragdoll
            return s_entireObjectSlotName;
        }
        else
        {
            // If there is more than one material slot 
            // the caller must use SetMaterialSlots function
            return "<error>";
        }
    }

    AZ::Data::AssetId MaterialSelection::GetMaterialLibraryAssetId() const
    {
        return GetMaterialLibraryAsset().GetId();
    }

    const Physics::MaterialLibraryAsset* MaterialSelection::GetMaterialLibraryAssetData() const
    {
        return GetMaterialLibraryAsset().Get();
    }

    const AZStd::string& MaterialSelection::GetMaterialLibraryAssetHint() const
    {
        return m_materialLibrary.GetHint();
    }

    void MaterialSelection::OnDefaultMaterialLibraryChanged(const AZ::Data::AssetId& defaultMaterialLibraryId)
    {
        AZ_UNUSED(defaultMaterialLibraryId);
        if (IsDefaultMaterialLibraryAsset())
        {
            OnMaterialLibraryChanged();
        }
    }

    void MaterialSelection::SetSlotsReadOnly(bool readOnly)
    {
        m_slotsReadOnly = readOnly;
    }

    bool MaterialSelection::IsMaterialLibraryValid() const
    {
        if (GetMaterialLibraryAssetId().IsValid())
        {
            auto materialAsset = LoadAsset();
            const auto& materialsData = materialAsset.Get()->GetMaterialsData();

            if (materialsData.size() != 0)
            {
                return true;
            }
        }
        return false;
    }

    bool MaterialSelection::GetMaterialConfiguration(Physics::MaterialFromAssetConfiguration& configuration, const Physics::MaterialId& materialId) const
    {
        if (IsMaterialLibraryValid())
        {
            auto materialAsset = LoadAsset();
            if (materialAsset.Get())
            {
                return materialAsset.Get()->GetDataForMaterialId(materialId, configuration);
            }
        }
        return false;
    }

    void MaterialSelection::SetMaterialLibrary(const AZ::Data::AssetId& assetId)
    {
        m_materialLibrary = AZ::Data::AssetManager::Instance().GetAsset<Physics::MaterialLibraryAsset>(assetId, m_materialLibrary.GetAutoLoadBehavior());
        m_materialLibrary.BlockUntilLoadComplete();
    }

    void MaterialSelection::ResetToDefaultMaterialLibrary()
    {
        m_materialLibrary = {};
    }

    void MaterialSelection::SetMaterialSlots(const SlotsArray& slots)
    {
        if (slots.empty())
        {
            m_materialSlots = { s_entireObjectSlotName };
        }
        else
        {
            m_materialSlots = slots;
        }

        m_materialIdsAssignedToSlots.resize(m_materialSlots.size());
    }

    const AZStd::vector<Physics::MaterialId>& MaterialSelection::GetMaterialIdsAssignedToSlots() const
    {
        return m_materialIdsAssignedToSlots;
    }

    Physics::MaterialId MaterialSelection::GetMaterialId(int slotIndex) const
    {
        if (slotIndex >= m_materialIdsAssignedToSlots.size() || slotIndex < 0)
        {
            return Physics::MaterialId();
        }

        return m_materialIdsAssignedToSlots[slotIndex];
    }

    void MaterialSelection::SetMaterialId(const Physics::MaterialId& materialId, int slotIndex)
    {
        if (m_materialIdsAssignedToSlots.empty())
        {
            m_materialIdsAssignedToSlots.resize(1);
        }

        slotIndex = AZ::GetClamp(slotIndex, 0, static_cast<int>(m_materialIdsAssignedToSlots.size()) - 1);
        m_materialIdsAssignedToSlots[slotIndex] = materialId;
    }

    AZ::Data::Asset<Physics::MaterialLibraryAsset> MaterialSelection::LoadAsset() const
    {
        AZ::Data::Asset<MaterialLibraryAsset> asset = AZ::Data::AssetManager::Instance()
            .GetAsset<Physics::MaterialLibraryAsset>(GetMaterialLibraryAssetId(), AZ::Data::AssetLoadBehavior::Default);

        asset.BlockUntilLoadComplete();

        return asset;
    }

    void MaterialSelection::SyncSelectionToMaterialLibrary()
    {
        if (GetMaterialLibraryAssetId().IsValid())
        {
            auto materialLibraryAsset = AZ::Data::AssetManager::Instance().GetAsset<Physics::MaterialLibraryAsset>(GetMaterialLibraryAssetId(), AZ::Data::AssetLoadBehavior::Default);

            materialLibraryAsset.BlockUntilLoadComplete();

            // We try to check whether existing selection matches any materials in the newly assigned library and do one of the following:
            // 1. If previous MaterialId is invalid for this material library, and it is not the Default material, we set it to the Default material from the library.
            // 2. If it's valid, or it is the Default material, we don't change it (useful when user accidentally re-assigns the same library: previous selection won't go away).

            if (materialLibraryAsset.Get())
            {
                for (Physics::MaterialId& materialId : m_materialIdsAssignedToSlots)
                {
                    if (!materialLibraryAsset.Get()->HasDataForMaterialId(materialId)
                        && !materialId.IsNull()) // Null materialId is the Default material.
                    {
                        materialId = MaterialId();
                    }
                }
            }
            else
            {
                AZ_Warning("PhysX", false, "MaterialSelection: invalid material library");
            }
        }
    }

    const AZ::Data::Asset<Physics::MaterialLibraryAsset>& MaterialSelection::GetMaterialLibraryAsset() const
    {
        if (IsDefaultMaterialLibraryAsset())
        {
            const AZ::Data::Asset<Physics::MaterialLibraryAsset>& defaultMaterialLibrary = GetDefaultMaterialLibrary();
            return defaultMaterialLibrary;
        }

        return m_materialLibrary;
    }

    bool MaterialSelection::IsDefaultMaterialLibraryAsset() const
    {
        return !m_materialLibrary.GetId().IsValid();
    }

    const AZ::Data::Asset<Physics::MaterialLibraryAsset>& MaterialSelection::GetDefaultMaterialLibrary()
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            return physicsSystem->GetDefaultMaterialLibrary();
        }
        return s_invalidMaterialLibrary;
    }

    const AZ::Data::AssetId& MaterialSelection::GetDefaultMaterialLibraryId()
    {
        return GetDefaultMaterialLibrary().GetId();
    }

    bool MaterialSelection::AreMaterialSlotsReadOnly() const
    {
        return m_slotsReadOnly;
    }

} // namespace Physics
