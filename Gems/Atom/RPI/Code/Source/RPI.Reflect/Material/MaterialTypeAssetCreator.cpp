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

            return EndCommon(result);
        }

        bool MaterialTypeAssetCreator::UpdateShaderAssetForShaderResourceGroup(
            Data::Asset<ShaderAsset>& srgShaderAssetToUpdate,
            const Data::Asset<ShaderAsset>& newShaderAsset,
            const uint32_t bindingSlot,
            const char* srgDebugName)
        {
            const auto& newSrgLayout = newShaderAsset->FindShaderResourceGroupLayout(bindingSlot);

            if (!newSrgLayout)
            {
                // It's ok if newShaderAsset doesn't have the SRG. Only some of the shaders may have an SRG of a given type.
                return true;
            }

            if (srgShaderAssetToUpdate.GetId().IsValid())
            {
                AZ_Assert(srgShaderAssetToUpdate.Get(), "srgShaderAssetToUpdate has an AssetId but is not loaded");

                const auto& currentSrgLayout = srgShaderAssetToUpdate->FindShaderResourceGroupLayout(bindingSlot);
                if (currentSrgLayout->GetHash() != newSrgLayout->GetHash())
                {
                    ReportError("All shaders in a material must use the same %s ShaderResourceGroup.", srgDebugName);
                    return false;
                }
            }
            else
            {
                srgShaderAssetToUpdate = newShaderAsset;
            }

            return true;
        }

        void MaterialTypeAssetCreator::CacheMaterialSrgLayout()
        {
            if (!m_materialShaderResourceGroupLayout)
            {
                if (m_asset->m_shaderWithMaterialSrg)
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
            return m_asset->m_materialVersionUpdates.ValidateUpdates(
                m_asset->m_version, m_asset->GetMaterialPropertiesLayout(),
                [this](const char* message){ ReportError("%s", message); });
        }

        MaterialTypeAsset::MaterialPipelinePayload& MaterialTypeAssetCreator::GetMaterialPipelinePayload(const AZ::Name& materialPipelineName)
        {
            MaterialTypeAsset::MaterialPipelinePayload& pipeline = m_asset->m_materialPipelinePayloads[materialPipelineName];
            if (!pipeline.m_materialPropertiesLayout)
            {
                pipeline.m_materialPropertiesLayout = aznew MaterialPropertiesLayout;
            }

            return pipeline;
        }

        void MaterialTypeAssetCreator::AddShader(
            const AZ::Data::Asset<ShaderAsset>& shaderAsset,
            const ShaderVariantId& shaderVariantId,
            const AZ::Name& shaderTag,
            const AZ::Name& materialPipelineName)
        {
            if (ValidateIsReady() && ValidateNotNull(shaderAsset, "ShaderAsset"))
            {
                ShaderCollection* shaderCollection = nullptr;
                if (materialPipelineName.IsEmpty())
                {
                    shaderCollection = &m_asset->m_generalShaderCollection;
                }
                else
                {
                    shaderCollection = &GetMaterialPipelinePayload(materialPipelineName).m_shaderCollection;
                }

                AZ::Name finalShaderTag = !shaderTag.IsEmpty() ? shaderTag : AZ::Name{AZ::Uuid::CreateRandom().ToFixedString()};

                shaderCollection->m_shaderItems.push_back(ShaderCollection::Item{shaderAsset, finalShaderTag, shaderVariantId});
                if (!shaderCollection->m_shaderTagIndexMap.Insert(finalShaderTag, RHI::Handle<uint32_t>(shaderCollection->m_shaderItems.size() - 1)))
                {
                    ReportError("Failed to insert shader tag '%s' for pipeline '%s'. Shader tag must be unique.", finalShaderTag.GetCStr(), materialPipelineName.GetCStr());
                }

                UpdateShaderAssetForShaderResourceGroup(m_asset->m_shaderWithMaterialSrg, shaderAsset, SrgBindingSlot::Material, "material");
                UpdateShaderAssetForShaderResourceGroup(m_asset->m_shaderWithObjectSrg, shaderAsset, SrgBindingSlot::Object, "object");

                CacheMaterialSrgLayout();
            }
        }

        void MaterialTypeAssetCreator::SetVersion(uint32_t version)
        {
            m_asset->m_version = version;
        }

        void MaterialTypeAssetCreator::AddVersionUpdate(const MaterialVersionUpdate& materialVersionUpdate)
        {
            m_asset->m_materialVersionUpdates.AddVersionUpdate(materialVersionUpdate);
        }

        void MaterialTypeAssetCreator::ClaimShaderOptionOwnership(const Name& shaderOptionName)
        {
            bool optionFound = false;

            m_asset->ForAllShaderItems(
                [&](const Name&, ShaderCollection::Item& shaderItem, uint32_t)
                {
                    ShaderOptionIndex index = shaderItem.GetShaderOptions()->FindShaderOptionIndex(shaderOptionName);
                    if (index.IsValid())
                    {
                        shaderItem.m_ownedShaderOptionIndices.insert(index);
                        optionFound = true;
                    }
                    return true;
                });

            if (!optionFound)
            {
                ReportWarning("Option '%s' was not found in any of the MaterialType's shaders.", shaderOptionName.GetCStr());
            }
        }

        const MaterialPropertiesLayout* MaterialTypeAssetCreator::GetMaterialPropertiesLayout(const AZ::Name& materialPipelineName) const
        {
            if (materialPipelineName == MaterialPipelineNone)
            {
                return m_materialPropertiesLayout;
            }
            else
            {
                auto iter = m_asset->m_materialPipelinePayloads.find(materialPipelineName);
                if (iter == m_asset->m_materialPipelinePayloads.end())
                {
                    return nullptr;
                }

                return iter->second.m_materialPropertiesLayout.get();
            }
        }

        const RHI::ShaderResourceGroupLayout* MaterialTypeAssetCreator::GetMaterialShaderResourceGroupLayout() const
        {
            return m_materialShaderResourceGroupLayout;
        }

        void MaterialTypeAssetCreator::AddMaterialProperty(MaterialPropertyDescriptor&& materialProperty, const AZ::Name& materialPipelineName)
        {
            if (ValidateIsReady())
            {
                MaterialPropertiesLayout* layout = nullptr;
                AZStd::vector<MaterialPropertyValue>* propertyValues = nullptr;

                if (materialPipelineName.IsEmpty())
                {
                    layout = m_materialPropertiesLayout;
                    propertyValues = &m_asset->m_propertyValues;
                }
                else
                {
                    MaterialTypeAsset::MaterialPipelinePayload& pipeline = GetMaterialPipelinePayload(materialPipelineName);
                    layout = pipeline.m_materialPropertiesLayout.get();
                    propertyValues = &pipeline.m_defaultPropertyValues;
                }

                if (propertyValues->size() >= Limits::Material::PropertyCountMax)
                {
                    ReportError("Too many material propertyValues. Max is %d.", Limits::Material::PropertyCountMax);
                    return;
                }

                // Add the appropriate default property value for the property's data type.
                // Note, we use a separate switch statement from the one above just for clarity.
                switch (materialProperty.GetDataType())
                {
                case MaterialPropertyDataType::Bool:
                    propertyValues->emplace_back(false);
                    break;
                case MaterialPropertyDataType::Int:
                    propertyValues->emplace_back(0);
                    break;
                case MaterialPropertyDataType::UInt:
                    propertyValues->emplace_back(0u);
                    break;
                case MaterialPropertyDataType::Float:
                    propertyValues->emplace_back(0.0f);
                    break;
                case MaterialPropertyDataType::Vector2:
                    propertyValues->emplace_back(Vector2{0.0f, 0.0f});
                    break;
                case MaterialPropertyDataType::Vector3:
                    propertyValues->emplace_back(Vector3{0.0f, 0.0f, 0.0f});
                    break;
                case MaterialPropertyDataType::Vector4:
                    propertyValues->emplace_back(Vector4{0.0f, 0.0f, 0.0f, 0.0f});
                    break;
                case MaterialPropertyDataType::Color:
                    propertyValues->emplace_back(Color{1.0f, 1.0f, 1.0f, 1.0f});
                    break;
                case MaterialPropertyDataType::Image:
                    propertyValues->emplace_back(Data::Asset<ImageAsset>({}));
                    break;
                case MaterialPropertyDataType::Enum:
                    propertyValues->emplace_back(0u);
                    break;
                default:
                    ReportError("Material property '%s': Data type is invalid.", materialProperty.GetName().GetCStr());
                    return;
                }

                // Add the new descriptor
                MaterialPropertyIndex newIndex(static_cast<uint32_t>(layout->GetPropertyCount()));
                layout->m_materialPropertyIndexes.Insert(materialProperty.GetName(), newIndex);
                layout->m_materialPropertyDescriptors.emplace_back(AZStd::move(materialProperty));
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

        void MaterialTypeAssetCreator::BeginMaterialProperty(const Name& materialPropertyName, MaterialPropertyDataType dataType, const AZ::Name& materialPipelineName)
        {
            if (!ValidateIsReady())
            {
                return;
            }

            if (!ValidateEndMaterialProperty())
            {
                return;
            }

            const MaterialPropertiesLayout* layout = materialPipelineName.IsEmpty() ? m_materialPropertiesLayout : GetMaterialPipelinePayload(materialPipelineName).m_materialPropertiesLayout.get();

            if (layout->FindPropertyIndex(materialPropertyName).IsValid())
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
            m_wipMaterialPropertyPipeline = materialPipelineName;
        }

        void MaterialTypeAssetCreator::ConnectMaterialPropertyToShaderInput(const Name& shaderInputName)
        {
            if (!ValidateBeginMaterialProperty())
            {
                return;
            }

            if (m_wipMaterialPropertyPipeline != MaterialPipelineNone)
            {
                // Material pipelines do not have access to the Material ShaderResourceGroup.
                // The material type and material pipeline data are logically decoupled from each other, with careful separation of
                // concerns to ensure modularity. The definition of the material's ShaderResouceGroup (usually called "MaterialSrg") is strictly the
                // responsibility of the .materialtype file, and the .materialpipeline file cannot be aware of it. Even though the MaterialSrg *does*
                // appear in the final ShaderCollection inside each MaterialTypeAsset::MaterialPipelinePayload object, we do not allow the MaterialPipelinePayload's
                // properties to access it since the data originates from the .materialtype file.
                ReportError("Material property '%s': Connection type '%s' is not supported by internal material pipeline properties.",
                    m_wipMaterialProperty.GetName().GetCStr(), ToString(MaterialPropertyOutputType::ShaderInput));
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

        void MaterialTypeAssetCreator::ConnectMaterialPropertyToShaderOptions(const Name& shaderOptionName)
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

            bool foundShaderOptions = false;

            auto addConnection = [&](const Name& materialPipelineName, ShaderCollection::Item& shaderItem, uint32_t shaderIndex)
            {
                auto optionsLayout = shaderItem.GetShaderAsset()->GetShaderOptionGroupLayout();
                ShaderOptionIndex optionIndex = optionsLayout->FindShaderOptionIndex(shaderOptionName);
                if (optionIndex.IsValid())
                {
                    foundShaderOptions = true;

                    MaterialPropertyOutputId outputId;
                    outputId.m_type = MaterialPropertyOutputType::ShaderOption;
                    outputId.m_materialPipelineName = materialPipelineName;
                    outputId.m_containerIndex = RHI::Handle<uint32_t>{shaderIndex};
                    outputId.m_itemIndex = RHI::Handle<uint32_t>{optionIndex.GetIndex()};

                    m_wipMaterialProperty.m_outputConnections.push_back(outputId);

                    shaderItem.m_ownedShaderOptionIndices.insert(optionIndex);
                }

                return true;
            };

            if (m_wipMaterialPropertyPipeline == MaterialPipelineNone)
            {
                // For normal material properties, we must connect to every possible shader, including the ones inside material pipelines.
                // This is because the final compiled shaders will include a combination of code that comes from the material pipeline
                // and the code that comes from the .materialtype file's "materialShaderCode" field. The material type's shader code can
                // define shader options, and so these shader options must be accessible to material property connections.
                m_asset->ForAllShaderItems(addConnection);
            }
            else
            {
                // For internal material pipeline properties, we only allow connections to the local shader collection. This is because the
                // material pipeline should not be aware of any shader options that were defined by the material type's shader code, so
                // there is no reason for it to "reach across" to other material pipelines. It should only be concerned with the shader options
                // that are accessible to the material pipeline's template shader code. It is possible that other material pipelines will
                // include the exact same shader options, but in that case the other material pipelines will be responsible for setting those
                // shader options themselves.
                ShaderCollection& localShaderCollection = GetMaterialPipelinePayload(m_wipMaterialPropertyPipeline).m_shaderCollection;
                for (int shaderIndex = 0; shaderIndex < localShaderCollection.size(); ++shaderIndex)
                {
                    addConnection(m_wipMaterialPropertyPipeline, localShaderCollection[shaderIndex], shaderIndex);
                }
            }

            if (!foundShaderOptions)
            {
                ReportError("Material property '%s': Material contains no shaders with option '%s'.", m_wipMaterialProperty.GetName().GetCStr(), shaderOptionName.GetCStr());
                return;
            }
        }

        void MaterialTypeAssetCreator::ConnectMaterialPropertyToShaderEnabled(const Name& shaderTag)
        {
            if (!ValidateBeginMaterialProperty())
            {
                return;
            }

            if (m_wipMaterialProperty.GetDataType() != MaterialPropertyDataType::Bool)
            {
                ReportError("Material property '%s': Only a bool property can be mapped to a shader enable flag.", m_wipMaterialProperty.GetName().GetCStr());
                return;
            }

            // Material properties can only control shaders in their local ShaderCollection. This supports a decouple design where main
            // material properties from the .materialtype file don't know about the shaders that are built by the material pipeline,
            // and material pipelines cannot "reach over" and control the shaders that are in some other pipeline.
            ShaderCollection* shaderCollection = nullptr;
            if (m_wipMaterialPropertyPipeline == MaterialPipelineNone)
            {
                shaderCollection = &m_asset->m_generalShaderCollection;
            }
            else
            {
                shaderCollection = &GetMaterialPipelinePayload(m_wipMaterialPropertyPipeline).m_shaderCollection;
            }

            bool foundShader = false;
            for (size_t shaderIndex = 0; shaderIndex < shaderCollection->size(); ++shaderIndex)
            {
                if ((*shaderCollection)[shaderIndex].GetShaderTag() == shaderTag)
                {
                    foundShader = true;

                    MaterialPropertyOutputId outputId;
                    outputId.m_materialPipelineName = m_wipMaterialPropertyPipeline;
                    outputId.m_type = MaterialPropertyOutputType::ShaderEnabled;
                    outputId.m_containerIndex = RHI::Handle<uint32_t>{shaderIndex};

                    m_wipMaterialProperty.m_outputConnections.push_back(outputId);
                }
            }

            if (!foundShader)
            {
                ReportError("Material property '%s': Material contains no shaders with tag '%s'.", m_wipMaterialProperty.GetName().GetCStr(), shaderTag.GetCStr());
                return;
            }
        }

        void MaterialTypeAssetCreator::ConnectMaterialPropertyToInternalProperty(const Name& propertyName)
        {
            if (!ValidateBeginMaterialProperty())
            {
                return;
            }

            if (!m_wipMaterialPropertyPipeline.IsEmpty())
            {
                ReportError("Material property '%s': Internal properties cannot be connected to other internal properties.", m_wipMaterialProperty.GetName().GetCStr());
                return;
            }

            bool foundProperty = false;
            for (const auto& [materialPipelineName, materialPipeline] : m_asset->m_materialPipelinePayloads)
            {
                MaterialPropertyIndex propertyIndex = materialPipeline.m_materialPropertiesLayout->FindPropertyIndex(propertyName);
                if (propertyIndex.IsValid())
                {
                    foundProperty = true;

                    if (m_wipMaterialProperty.GetDataType() != materialPipeline.m_materialPropertiesLayout->GetPropertyDescriptor(propertyIndex)->GetDataType())
                    {
                        ReportError("Material property '%s': Cannot connect to internal property '%s' because the data types do not match.",
                            m_wipMaterialProperty.GetName().GetCStr(), propertyName.GetCStr());
                        continue;
                    }

                    MaterialPropertyOutputId outputId;
                    outputId.m_materialPipelineName = materialPipelineName;
                    outputId.m_type = MaterialPropertyOutputType::InternalProperty;
                    outputId.m_itemIndex = RHI::Handle<uint32_t>{propertyIndex.GetIndex()};

                    m_wipMaterialProperty.m_outputConnections.push_back(outputId);
                }
            }

            if (!foundProperty)
            {
                ReportError("Material property '%s': Material contains no internal property '%s'.",
                    m_wipMaterialProperty.GetName().GetCStr(), propertyName.GetCStr());
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

            AddMaterialProperty(AZStd::move(m_wipMaterialProperty), m_wipMaterialPropertyPipeline);

            m_wipMaterialProperty = MaterialPropertyDescriptor{};
        }
        
        bool MaterialTypeAssetCreator::PropertyCheck(TypeId typeId, const Name& propertyName, const AZ::Name& materialPipelineName)
        {
            const MaterialPropertiesLayout* layout = GetMaterialPropertiesLayout(materialPipelineName);
            if (!layout)
            {
                ReportError("There is no material pipeline named '%s'", materialPipelineName.GetCStr());
                return false;
            }

            MaterialPropertyIndex propertyIndex = layout->FindPropertyIndex(propertyName);
            if (!propertyIndex.IsValid())
            {
                ReportWarning("Material property '%s' not found", propertyName.GetCStr());
                return false;
            }

            const MaterialPropertyDescriptor* materialPropertyDescriptor = layout->GetPropertyDescriptor(propertyIndex);
            if (!materialPropertyDescriptor)
            {
                ReportError("A material property index was found but the property descriptor was null");
                return false;
            }

            if (!ValidateMaterialPropertyDataType(typeId, materialPropertyDescriptor, [this](const char* message){ReportError("%s", message);}))
            {
                return false;
            }

            return true;
        }

        void MaterialTypeAssetCreator::SetPropertyValue(const Name& name, const Data::Asset<ImageAsset>& imageAsset, const AZ::Name& materialPipelineName)
        {
            return SetPropertyValue(name, MaterialPropertyValue(imageAsset), materialPipelineName);
        }

        void MaterialTypeAssetCreator::SetPropertyValue(const Name& name, const MaterialPropertyValue& value, const AZ::Name& materialPipelineName)
        {
            if (PropertyCheck(value.GetTypeId(), name, materialPipelineName))
            {

                MaterialPropertiesLayout* layout = materialPipelineName.IsEmpty() ? m_materialPropertiesLayout : GetMaterialPipelinePayload(materialPipelineName).m_materialPropertiesLayout.get();
                MaterialPropertyIndex propertyIndex = layout->FindPropertyIndex(name);

                AZStd::vector<MaterialPropertyValue>& propertyValues = materialPipelineName.IsEmpty() ? m_asset->m_propertyValues : GetMaterialPipelinePayload(materialPipelineName).m_defaultPropertyValues;

                propertyValues[propertyIndex.GetIndex()] = value;
            }
        }

        void MaterialTypeAssetCreator::SetPropertyValue(const Name& name, const Data::Asset<StreamingImageAsset>& imageAsset, const AZ::Name& materialPipelineName)
        {
            SetPropertyValue(name, Data::Asset<ImageAsset>(imageAsset), materialPipelineName);
        }

        void MaterialTypeAssetCreator::SetPropertyValue(const Name& name, const Data::Asset<AttachmentImageAsset>& imageAsset, const AZ::Name& materialPipelineName)
        {
            SetPropertyValue(name, Data::Asset<ImageAsset>(imageAsset), materialPipelineName);
        }

        void MaterialTypeAssetCreator::AddMaterialFunctor(const Ptr<MaterialFunctor>& functor, const AZ::Name& materialPipelineName)
        {
            if (ValidateIsReady() && ValidateNotNull(functor, "MaterialFunctor"))
            {
                if (materialPipelineName.IsEmpty())
                {
                    m_asset->m_materialFunctors.emplace_back(functor);
                }
                else
                {
                    GetMaterialPipelinePayload(materialPipelineName).m_materialFunctors.emplace_back(functor);
                }
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
                else if (iter->m_uvName != uvName)
                {
                    ReportError("Multiple UV names are defined for shader input %s.", shaderInput.ToString().c_str());
                }
            }
        }
    } // namespace RPI
} // namespace AZ
