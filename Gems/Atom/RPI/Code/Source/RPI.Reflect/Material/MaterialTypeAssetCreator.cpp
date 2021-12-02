/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Reflect/Material/MaterialTypeAssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Limits.h>

namespace AZ
{
    namespace RPI
    {
        void MaterialTypeAssetCreator::Begin(const Data::AssetId& assetId)
        {
            BeginCommon(assetId);

            if (ValidateIsReady())
            {
                m_materialPropertiesLayout = aznew MaterialPropertiesLayout;
                m_asset->m_materialPropertiesLayout = m_materialPropertiesLayout;

                auto warningFunc = [this](const char* message)
                {
                    ReportWarning("%s", message);
                };
                auto errorFunc = [this](const char* message)
                {
                    ReportError("%s", message);
                };
                // Set empty for UV names as material type asset doesn't have overrides.
                MaterialAssetCreatorCommon::OnBegin(m_materialPropertiesLayout, &(m_asset->m_propertyValues), warningFunc, errorFunc);
            }
        }

        bool MaterialTypeAssetCreator::End(Data::Asset<MaterialTypeAsset>& result)
        {
            if (!ValidateIsReady() || !ValidateEndMaterialProperty() || !ValidateMaterialVersion())
            {
                return false; 
            }

            m_asset->SetReady();

            m_materialShaderResourceGroupLayout = nullptr;
            m_materialPropertiesLayout = nullptr;

            MaterialAssetCreatorCommon::OnEnd();

            return EndCommon(result);
        }

        bool MaterialTypeAssetCreator::UpdateShaderIndexForShaderResourceGroup(
            uint32_t& srgShaderIndexToUpdate, const Data::Asset<ShaderAsset>& newShaderAsset, const uint32_t newShaderAssetIndex, const uint32_t bindingSlot, const char* srgDebugName)
        {
            const auto& newSrgLayout = newShaderAsset->FindShaderResourceGroupLayout(bindingSlot);

            if (!newSrgLayout)
            {
                // It's ok if newShaderAsset doesn't have the SRG. Only some of the shaders may have an SRG of a given type.
                return true;
            }

            if (srgShaderIndexToUpdate != MaterialTypeAsset::InvalidShaderIndex)
            {
                const auto& currentShaderAsset = m_asset->m_shaderCollection[srgShaderIndexToUpdate].GetShaderAsset();
                const auto& currentSrgLayout = currentShaderAsset->FindShaderResourceGroupLayout(bindingSlot);
                if (currentSrgLayout->GetHash() != newSrgLayout->GetHash())
                {
                    ReportError("All shaders in a material must use the same %s ShaderResourceGroup.", srgDebugName);
                    return false;
                }
            }
            else
            {
                srgShaderIndexToUpdate = newShaderAssetIndex;
            }

            return true;
        }

        void MaterialTypeAssetCreator::CacheMaterialSrgLayout()
        {
            if (!m_materialShaderResourceGroupLayout)
            {
                if (m_asset->m_materialSrgShaderIndex != MaterialTypeAsset::InvalidShaderIndex)
                {
                    // [GFX TODO] At the moment we are using the default supervariant.
                    //            In the future it may be necessary to get the layout
                    //            from a particular supervariant.
                    m_materialShaderResourceGroupLayout = m_asset->GetMaterialSrgLayout().get();
                    if (!m_materialShaderResourceGroupLayout)
                    {
                        ReportError("Shader resource group has a null layout.");
                    }
                }
            }
        }

        bool MaterialTypeAssetCreator::ValidateMaterialVersion()
        {
            if (m_asset->m_materialVersionUpdates.empty())
            {
                return true;
            }

            uint32_t prevVersion = 0;
            for(const MaterialVersionUpdate& versionUpdate : m_asset->m_materialVersionUpdates)
            {
                if (versionUpdate.GetVersion() <= prevVersion)
                {
                    ReportError("Version updates are not sequential. See version update '%u'.", versionUpdate.GetVersion());
                    return false;
                }
                
                if (versionUpdate.GetVersion() > m_asset->m_version)
                {
                    ReportError("Version updates go beyond the current material type version. See version update '%u'.", versionUpdate.GetVersion());
                    return false;
                }

                prevVersion = versionUpdate.GetVersion();
            }

            const auto& lastMaterialVersionUpdate = m_asset->m_materialVersionUpdates.back();
            for (const auto& action : lastMaterialVersionUpdate.GetActions())
            {                
                const auto propertyIndex = m_asset->m_materialPropertiesLayout->FindPropertyIndex(AZ::Name{ action.m_toPropertyId });
                if (!propertyIndex.IsValid())
                {
                    ReportError("Renamed property '%s' not found in material property layout. Check that the property name has been "
                            "upgraded to the correct version",
                            action.m_toPropertyId.GetCStr());
                    return false;
                }

            }

            return true;
        }

        void MaterialTypeAssetCreator::AddShader(const AZ::Data::Asset<ShaderAsset>& shaderAsset, const ShaderVariantId& shaderVaraintId, const AZ::Name& shaderTag)
        {
            if (ValidateIsReady() && ValidateNotNull(shaderAsset, "ShaderAsset"))
            {
                m_asset->m_shaderCollection.m_shaderItems.push_back(ShaderCollection::Item{shaderAsset, shaderTag, shaderVaraintId});
                if (!m_asset->m_shaderCollection.m_shaderTagIndexMap.Insert(shaderTag, RHI::Handle<uint32_t>(m_asset->m_shaderCollection.m_shaderItems.size() - 1)))
                {
                    ReportError(AZStd::string::format("Failed to insert shader tag '%s'. Shader tag must be unique.", shaderTag.GetCStr()).c_str());
                }

                uint32_t newShaderIndex = aznumeric_caster(m_asset->m_shaderCollection.m_shaderItems.size() - 1);
                UpdateShaderIndexForShaderResourceGroup(m_asset->m_materialSrgShaderIndex, shaderAsset, newShaderIndex, SrgBindingSlot::Material, "material");
                UpdateShaderIndexForShaderResourceGroup(m_asset->m_objectSrgShaderIndex, shaderAsset, newShaderIndex, SrgBindingSlot::Object, "object");

                CacheMaterialSrgLayout();
            }
        }

        void MaterialTypeAssetCreator::AddShader(const AZ::Data::Asset<ShaderAsset>& shaderAsset, const AZ::Name& shaderTag)
        {
            AddShader(shaderAsset, ShaderVariantId{}, shaderTag);
        }

        void MaterialTypeAssetCreator::SetVersion(uint32_t version)
        {
            m_asset->m_version = version;
        }

        void MaterialTypeAssetCreator::AddVersionUpdate(const MaterialVersionUpdate& materialVersionUpdate)
        {
            m_asset->m_materialVersionUpdates.push_back(materialVersionUpdate);
        }

        void MaterialTypeAssetCreator::ClaimShaderOptionOwnership(const Name& shaderOptionName)
        {
            bool optionFound = false;

            for (ShaderCollection::Item& shaderItem : m_asset->m_shaderCollection)
            {
                ShaderOptionIndex index = shaderItem.GetShaderOptions()->FindShaderOptionIndex(shaderOptionName);
                if (index.IsValid())
                {
                    shaderItem.m_ownedShaderOptionIndices.insert(index);
                    optionFound = true;
                }
            }

            if (!optionFound)
            {
                ReportWarning("Option '%s' was not found in any of the MaterialType's shaders.", shaderOptionName.GetCStr());
            }
        }

        const MaterialPropertiesLayout* MaterialTypeAssetCreator::GetMaterialPropertiesLayout() const
        {
            return m_materialPropertiesLayout;
        }

        const RHI::ShaderResourceGroupLayout* MaterialTypeAssetCreator::GetMaterialShaderResourceGroupLayout() const
        {
            return m_materialShaderResourceGroupLayout;
        }

        const ShaderCollection* MaterialTypeAssetCreator::GetShaderCollection() const
        {
            return &m_asset->m_shaderCollection;
        }

        void MaterialTypeAssetCreator::AddMaterialProperty(MaterialPropertyDescriptor&& materialProperty)
        {
            if (ValidateIsReady())
            {
                if (m_asset->m_propertyValues.size() >= Limits::Material::PropertyCountMax)
                {
                    ReportError("Too man material properties. Max is %d.", Limits::Material::PropertyCountMax);
                    return;
                }

                // Add the appropriate default property value for the property's data type.
                // Note, we use a separate switch statement from the one above just for clarity.
                switch (materialProperty.GetDataType())
                {
                case MaterialPropertyDataType::Bool:
                    m_asset->m_propertyValues.emplace_back(false);
                    break;
                case MaterialPropertyDataType::Int:
                    m_asset->m_propertyValues.emplace_back(0);
                    break;
                case MaterialPropertyDataType::UInt:
                    m_asset->m_propertyValues.emplace_back(0u);
                    break;
                case MaterialPropertyDataType::Float:
                    m_asset->m_propertyValues.emplace_back(0.0f);
                    break;
                case MaterialPropertyDataType::Vector2:
                    m_asset->m_propertyValues.emplace_back(Vector2{ 0.0f, 0.0f });
                    break;
                case MaterialPropertyDataType::Vector3:
                    m_asset->m_propertyValues.emplace_back(Vector3{ 0.0f, 0.0f, 0.0f });
                    break;
                case MaterialPropertyDataType::Vector4:
                    m_asset->m_propertyValues.emplace_back(Vector4{ 0.0f, 0.0f, 0.0f, 0.0f });
                    break;
                case MaterialPropertyDataType::Color:
                    m_asset->m_propertyValues.emplace_back(Color{ 1.0f, 1.0f, 1.0f, 1.0f });
                    break;
                case MaterialPropertyDataType::Image:
                    m_asset->m_propertyValues.emplace_back(Data::Asset<ImageAsset>({}));
                    break;
                case MaterialPropertyDataType::Enum:
                    m_asset->m_propertyValues.emplace_back(0u);
                    break;
                default:
                    ReportError("Material property '%s': Data type is invalid.", materialProperty.GetName().GetCStr());
                    return;
                }

                // Add the new descriptor
                MaterialPropertyIndex newIndex(static_cast<uint32_t>(m_materialPropertiesLayout->GetPropertyCount()));
                m_materialPropertiesLayout->m_materialPropertyIndexes.Insert(materialProperty.GetName(), newIndex);
                m_materialPropertiesLayout->m_materialPropertyDescriptors.emplace_back(AZStd::move(materialProperty));
            }
        }

        bool MaterialTypeAssetCreator::ValidateBeginMaterialProperty()
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            if (m_wipMaterialProperty.GetDataType() == MaterialPropertyDataType::Invalid)
            {
                ReportError("BeginMaterialProperty() must be called first.");
                return false;
            }

            return true;
        }

        bool MaterialTypeAssetCreator::ValidateEndMaterialProperty()
        {
            if (m_wipMaterialProperty.GetDataType() != MaterialPropertyDataType::Invalid)
            {
                ReportError("EndMaterialProperty() must be called first.");
                return false;
            }

            return true;
        }

        void MaterialTypeAssetCreator::BeginMaterialProperty(const Name& materialPropertyName, MaterialPropertyDataType dataType)
        {
            if (!ValidateIsReady())
            {
                return;
            }

            if (!ValidateEndMaterialProperty())
            {
                return;
            }

            if (m_materialPropertiesLayout->FindPropertyIndex(materialPropertyName).IsValid())
            {
                ReportError("Material property '%s': A property with this ID already exists.", materialPropertyName.GetCStr());
                return;
            }

            if (dataType == MaterialPropertyDataType::Invalid)
            {
                ReportError("Material property '%s': Data type is invalid.", materialPropertyName.GetCStr());
                return;
            }

            m_wipMaterialProperty.m_nameId = materialPropertyName;
            m_wipMaterialProperty.m_dataType = dataType;
        }

        void MaterialTypeAssetCreator::ConnectMaterialPropertyToShaderInput(const Name& shaderInputName)
        {
            if (!ValidateBeginMaterialProperty())
            {
                return;
            }

            MaterialPropertyOutputId outputId;
            outputId.m_type = MaterialPropertyOutputType::ShaderInput;

            if (!m_materialShaderResourceGroupLayout)
            {
                ReportError("Material property '%s': Could not map this property to shader input '%s' because there is no material ShaderResourceGroup.",
                    m_wipMaterialProperty.GetName().GetCStr(), shaderInputName.GetCStr());
                return;
            }

            switch (m_wipMaterialProperty.GetDataType())
            {
            case MaterialPropertyDataType::Bool:
            case MaterialPropertyDataType::Int:
            case MaterialPropertyDataType::UInt:
            case MaterialPropertyDataType::Float:
            case MaterialPropertyDataType::Vector2:
            case MaterialPropertyDataType::Vector3:
            case MaterialPropertyDataType::Vector4:
            case MaterialPropertyDataType::Color:
            case MaterialPropertyDataType::Enum:
                outputId.m_itemIndex = RHI::Handle<uint32_t>{m_materialShaderResourceGroupLayout->FindShaderInputConstantIndex(shaderInputName).GetIndex()};
                if (outputId.m_itemIndex.IsNull())
                {
                    ReportError("Material property '%s': Could not find shader constant input '%s'.", m_wipMaterialProperty.GetName().GetCStr(), shaderInputName.GetCStr());
                }
                break;
            case MaterialPropertyDataType::Image:
                outputId.m_itemIndex = RHI::Handle<uint32_t>{m_materialShaderResourceGroupLayout->FindShaderInputImageIndex(shaderInputName).GetIndex()};
                if (outputId.m_itemIndex.IsNull())
                {
                    ReportError("Material property '%s': Could not find shader image input '%s'.", m_wipMaterialProperty.GetName().GetCStr(), shaderInputName.GetCStr());
                }
                break;
            default:
                ReportError("Material property '%s': Properties of this type cannot be mapped to a ShaderResourceGroup input.", m_wipMaterialProperty.GetName().GetCStr());
                return;
            }


            m_wipMaterialProperty.m_outputConnections.push_back(outputId);
        }

        void MaterialTypeAssetCreator::ConnectMaterialPropertyToShaderOption(const Name& shaderOptionName, uint32_t shaderIndex)
        {
            if (!ValidateBeginMaterialProperty())
            {
                return;
            }

            switch (m_wipMaterialProperty.GetDataType())
            {
            case MaterialPropertyDataType::Bool:
            case MaterialPropertyDataType::Int:
            case MaterialPropertyDataType::UInt:
            case MaterialPropertyDataType::Enum:
                break;
            case MaterialPropertyDataType::Float:
            case MaterialPropertyDataType::Vector2:
            case MaterialPropertyDataType::Vector3:
            case MaterialPropertyDataType::Vector4:
            case MaterialPropertyDataType::Color:
            case MaterialPropertyDataType::Image:
                ReportError("Material property '%s': This property cannot be mapped to a shader option.", m_wipMaterialProperty.GetName().GetCStr());
                return;
            default:
                ReportError("Material property '%s': Unhandled MaterialPropertyDataType.", m_wipMaterialProperty.GetName().GetCStr());
                return;
            }

            if (shaderIndex >= m_asset->m_shaderCollection.size())
            {
                ReportError("Material property '%s': This material does not have a shader at index %d.", m_wipMaterialProperty.GetName().GetCStr(), shaderIndex);
                return;
            }

            ShaderCollection::Item& shaderItem = m_asset->m_shaderCollection[shaderIndex];

            auto optionsLayout = shaderItem.GetShaderAsset()->GetShaderOptionGroupLayout();
            ShaderOptionIndex optionIndex = optionsLayout->FindShaderOptionIndex(shaderOptionName);
            if (!optionIndex.IsValid())
            {
                ReportError("Material property '%s': Shader [%d] has no option '%s'.", m_wipMaterialProperty.GetName().GetCStr(), shaderIndex, shaderOptionName.GetCStr());
                return;
            }

            MaterialPropertyOutputId outputId;
            outputId.m_type = MaterialPropertyOutputType::ShaderOption;
            outputId.m_containerIndex = RHI::Handle<uint32_t>{ shaderIndex };
            outputId.m_itemIndex = RHI::Handle<uint32_t>{optionIndex.GetIndex()};

            m_wipMaterialProperty.m_outputConnections.push_back(outputId);

            shaderItem.m_ownedShaderOptionIndices.insert(optionIndex);
        }

        void MaterialTypeAssetCreator::ConnectMaterialPropertyToShaderOptions(const Name& shaderOptionName)
        {
            if (!ValidateBeginMaterialProperty())
            {
                return;
            }

            bool foundShaderOptions = false;
            for (int i = 0; i < m_asset->m_shaderCollection.size() && GetErrorCount() == 0; ++i)
            {
                ShaderCollection::Item& shaderItem = m_asset->m_shaderCollection[i];
                auto optionsLayout = shaderItem.GetShaderAsset()->GetShaderOptionGroupLayout();
                ShaderOptionIndex optionIndex = optionsLayout->FindShaderOptionIndex(shaderOptionName);
                if (optionIndex.IsValid())
                {
                    foundShaderOptions = true;
                    ConnectMaterialPropertyToShaderOption(shaderOptionName, i);
                    shaderItem.m_ownedShaderOptionIndices.insert(optionIndex);
                }
            }

            if (!foundShaderOptions)
            {
                ReportError("Material property '%s': Material contains no shaders with option '%s'.", m_wipMaterialProperty.GetName().GetCStr(), shaderOptionName.GetCStr());
                return;
            }
        }

        void MaterialTypeAssetCreator::SetMaterialPropertyEnumNames(const AZStd::vector<AZStd::string>& enumNames)
        {
            if (!ValidateBeginMaterialProperty())
            {
                return;
            }

            if (m_wipMaterialProperty.GetDataType() != MaterialPropertyDataType::Enum)
            {
                ReportError("Material property '%s' is not an enum but tries to store enum names.", m_wipMaterialProperty.GetName().GetCStr());
                return;
            }

            AZ_Assert(m_wipMaterialProperty.m_enumNames.empty(), "enumNames should be empty before storing!");
            m_wipMaterialProperty.m_enumNames.reserve(enumNames.size());
            for (const AZStd::string& enumName : enumNames)
            {
                m_wipMaterialProperty.m_enumNames.push_back(AZ::Name(enumName));
            }
        }

        void MaterialTypeAssetCreator::EndMaterialProperty()
        {
            if (!ValidateBeginMaterialProperty())
            {
                return;
            }

            AddMaterialProperty(AZStd::move(m_wipMaterialProperty));

            m_wipMaterialProperty = MaterialPropertyDescriptor{};
        }

        void MaterialTypeAssetCreator::AddMaterialFunctor(const Ptr<MaterialFunctor>& functor)
        {
            if (ValidateIsReady() && ValidateNotNull(functor, "MaterialFunctor"))
            {
                m_asset->m_materialFunctors.emplace_back(functor);
            }
        }

        void MaterialTypeAssetCreator::AddUvName(const RHI::ShaderSemantic& shaderInput, const Name& uvName)
        {
            if (ValidateIsReady())
            {
                auto iter = AZStd::find_if(m_asset->m_uvNameMap.begin(), m_asset->m_uvNameMap.end(),
                    [&shaderInput](const UvNamePair& uvNamePair)
                    {
                        // Cost of linear search UV names is low because the size is extremely limited.
                        return uvNamePair.m_shaderInput == shaderInput;
                    });
                if (iter == m_asset->m_uvNameMap.end())
                {
                    m_asset->m_uvNameMap.push_back(UvNamePair(shaderInput, uvName));
                }
                else
                {
                    ReportError("Multiple UV names are defined for shader input %s.", shaderInput.ToString().c_str());
                }
            }
        }
    } // namespace RPI
} // namespace AZ
