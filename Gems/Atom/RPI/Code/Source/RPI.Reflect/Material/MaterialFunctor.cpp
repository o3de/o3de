/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyCollection.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Reflect/Material/ShaderCollection.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>

namespace AZ
{
    namespace RPI
    {
        void MaterialFunctor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialFunctor>()
                    ->Version(2)
                    ->Field("materialPropertyDependencies", &MaterialFunctor::m_materialPropertyDependencies)
                    ;
            }
        }

        MaterialFunctorAPI::ConfigureShaders::ConfigureShaders(ShaderCollection* localShaderCollection)
            : m_localShaderCollection(localShaderCollection)
        {
        }

        void MaterialFunctorAPI::ConfigureShaders::ForAllShaderItems(AZStd::function<bool(ShaderCollection::Item& shaderItem)> callback)
        {
            for (ShaderCollection::Item& shaderItem : *m_localShaderCollection)
            {
                if (!callback(shaderItem))
                {
                    return;
                }
            }
        }

        template<typename ValueType>
        bool MaterialFunctorAPI::ConfigureShaders::SetShaderOptionValueHelper(const Name& name, const ValueType& value)
        {
            bool didSetOne = false;
            bool materialDoesntOwnOption = false;

            ForAllShaderItems([&](ShaderCollection::Item& shaderItem)
                {
                    ShaderOptionGroup* shaderOptionGroup = shaderItem.GetShaderOptions();
                    const ShaderOptionGroupLayout* layout = shaderOptionGroup->GetShaderOptionLayout();
                    ShaderOptionIndex optionIndex = layout->FindShaderOptionIndex(name);

                    if (!optionIndex.IsValid())
                    {
                        return true; // skip this and continue
                    }

                    if (!shaderItem.MaterialOwnsShaderOption(optionIndex))
                    {
                        materialDoesntOwnOption = true;
                        return false; // break;
                    }

                    if (shaderOptionGroup->SetValue(optionIndex, value))
                    {
                        didSetOne = true;
                    }

                    return true; // continue
                });

            if (materialDoesntOwnOption)
            {
                AZ_Error("MaterialFunctor", false, "Shader option '%s' is not owned by this material.", name.GetCStr());
                AZ_Assert(!didSetOne, "The material build pipeline should have ensured that MaterialOwnsShaderOption is consistent across all shaders.");
            }
            else if (!didSetOne)
            {
                AZ_Error("MaterialFunctor", false, "Shader option '%s' not found.", name.GetCStr());
            }

            return didSetOne;
        }

        bool MaterialFunctorAPI::ConfigureShaders::SetShaderOptionValue(const Name& optionName, ShaderOptionValue value)
        {
            return SetShaderOptionValueHelper(optionName, value);
        }

        bool MaterialFunctorAPI::ConfigureShaders::SetShaderOptionValue(const Name& optionName, const Name& value)
        {
            return SetShaderOptionValueHelper(optionName, value);
        }

        AZStd::size_t MaterialFunctorAPI::ConfigureShaders::GetShaderCount() const
        {
            return m_localShaderCollection->size();
        }

        void MaterialFunctorAPI::ConfigureShaders::SetShaderEnabled(AZStd::size_t shaderIndex, bool enabled)
        {
            (*m_localShaderCollection)[shaderIndex].SetEnabled(enabled);
        }

        void MaterialFunctorAPI::ConfigureShaders::SetShaderEnabled(const AZ::Name& shaderTag, bool enabled)
        {
            (*m_localShaderCollection)[shaderTag].SetEnabled(enabled);
        }

        void MaterialFunctorAPI::ConfigureShaders::SetShaderDrawListTagOverride(AZStd::size_t shaderIndex, const Name& drawListTagName)
        {
            (*m_localShaderCollection)[shaderIndex].SetDrawListTagOverride(drawListTagName);
        }

        void MaterialFunctorAPI::ConfigureShaders::SetShaderDrawListTagOverride(const AZ::Name& shaderTag, const Name& drawListTagName)
        {
            (*m_localShaderCollection)[shaderTag].SetDrawListTagOverride(drawListTagName);
        }

        void MaterialFunctorAPI::ConfigureShaders::ApplyShaderRenderStateOverlay(AZStd::size_t shaderIndex, const RHI::RenderStates& renderStatesOverlay)
        {
            RHI::MergeStateInto(renderStatesOverlay, *((*m_localShaderCollection)[shaderIndex].GetRenderStatesOverlay()));
        }

        void MaterialFunctorAPI::ConfigureShaders::ApplyShaderRenderStateOverlay(const AZ::Name& shaderTag, const RHI::RenderStates& renderStatesOverlay)
        {
            RHI::MergeStateInto(renderStatesOverlay, *((*m_localShaderCollection)[shaderTag].GetRenderStatesOverlay()));
        }

        MaterialFunctorAPI::RuntimeContext::RuntimeContext(
            const MaterialPropertyCollection& materialProperties,
            const MaterialPropertyFlags* materialPropertyDependencies,
            MaterialPropertyPsoHandling psoHandling,
            ShaderResourceGroup* shaderResourceGroup,
            ShaderCollection* generalShaderCollection,
            MaterialPipelineDataMap* materialPipelineData
        )
            : MaterialFunctorAPI::CommonRuntimeConfiguration(psoHandling)
            , MaterialFunctorAPI::ReadMaterialPropertyValues(materialProperties, materialPropertyDependencies)
            , MaterialFunctorAPI::ConfigureShaders(generalShaderCollection)
            , m_shaderResourceGroup(shaderResourceGroup)
            , m_materialPipelineData(materialPipelineData)
        {
        }

        void MaterialFunctorAPI::RuntimeContext::ForAllShaderItems(AZStd::function<bool(ShaderCollection::Item& shaderItem)> callback)
        {
            MaterialFunctorAPI::ConfigureShaders::ForAllShaderItems(callback);

            for (auto& materialPipelinePair : *m_materialPipelineData)
            {
                for (ShaderCollection::Item& shaderItem : materialPipelinePair.second.m_shaderCollection)
                {
                    if (!callback(shaderItem))
                    {
                        return;
                    }
                }
            }
        }

        ShaderResourceGroup* MaterialFunctorAPI::RuntimeContext::GetShaderResourceGroup()
        {
            return m_shaderResourceGroup;
        }

        bool MaterialFunctorAPI::RuntimeContext::SetInternalMaterialPropertyValue(const Name& propertyId, const MaterialPropertyValue& value)
        {
            bool somethingWasSet = false;

            for (auto& materialPipelinePair : *m_materialPipelineData)
            {
                MaterialPropertyCollection& properties = materialPipelinePair.second.m_materialProperties;

                MaterialPropertyIndex index = properties.GetMaterialPropertiesLayout()->FindPropertyIndex(propertyId);
                if (!index.IsValid())
                {
                    continue;
                }

                if (properties.SetPropertyValue(index, value))
                {
                    somethingWasSet = true;
                }
            }

            return somethingWasSet;
        }

        MaterialFunctorAPI::PipelineRuntimeContext::PipelineRuntimeContext(
            const MaterialPropertyCollection& internalProperties,
            const MaterialPropertyFlags* internalMaterialPropertyDependencies,
            MaterialPropertyPsoHandling psoHandling,
            ShaderCollection* pipelineShaderCollections
        )
            : MaterialFunctorAPI::CommonRuntimeConfiguration(psoHandling)
            , MaterialFunctorAPI::ReadMaterialPropertyValues(internalProperties, internalMaterialPropertyDependencies)
            , MaterialFunctorAPI::ConfigureShaders(pipelineShaderCollections)
        {
        }

        MaterialFunctorAPI::EditorContext::EditorContext(
            const MaterialPropertyCollection& materialProperties,
            AZStd::unordered_map<Name, MaterialPropertyDynamicMetadata>& propertyMetadata,
            AZStd::unordered_map<Name, MaterialPropertyGroupDynamicMetadata>& propertyGroupMetadata,
            AZStd::unordered_set<Name>& updatedPropertiesOut,
            AZStd::unordered_set<Name>& updatedPropertyGroupsOut,
            const MaterialPropertyFlags* materialPropertyDependencies
        )
            : MaterialFunctorAPI::ReadMaterialPropertyValues(materialProperties, materialPropertyDependencies)
            , m_propertyMetadata(propertyMetadata)
            , m_propertyGroupMetadata(propertyGroupMetadata)
            , m_updatedPropertiesOut(updatedPropertiesOut)
            , m_updatedPropertyGroupsOut(updatedPropertyGroupsOut)
        {}

        const MaterialPropertyDynamicMetadata* MaterialFunctorAPI::EditorContext::GetMaterialPropertyMetadata(const Name& propertyId) const
        {
            return QueryMaterialPropertyMetadata(propertyId);
        }

        const MaterialPropertyDynamicMetadata* MaterialFunctorAPI::EditorContext::GetMaterialPropertyMetadata(const MaterialPropertyIndex& index) const
        {
            const Name& name = m_materialProperties.GetMaterialPropertiesLayout()->GetPropertyDescriptor(index)->GetName();
            return GetMaterialPropertyMetadata(name);
        }
        
        const MaterialPropertyGroupDynamicMetadata* MaterialFunctorAPI::EditorContext::GetMaterialPropertyGroupMetadata(const Name& propertyId) const
        {
            return QueryMaterialPropertyGroupMetadata(propertyId);
        }
        
        bool MaterialFunctorAPI::EditorContext::SetMaterialPropertyGroupVisibility(const Name& propertyGroupName, MaterialPropertyGroupVisibility visibility)
        {
            MaterialPropertyGroupDynamicMetadata* metadata = QueryMaterialPropertyGroupMetadata(propertyGroupName);
            if (!metadata)
            {
                return false;
            }

            if (metadata->m_visibility != visibility)
            {
                metadata->m_visibility = visibility;
                m_updatedPropertyGroupsOut.insert(propertyGroupName);
            }

            return true;
        }

        bool MaterialFunctorAPI::EditorContext::SetMaterialPropertyVisibility(const Name& propertyId, MaterialPropertyVisibility visibility)
        {
            MaterialPropertyDynamicMetadata* metadata = QueryMaterialPropertyMetadata(propertyId);
            if (!metadata)
            {
                return false;
            }

            if (metadata->m_visibility != visibility)
            {
                metadata->m_visibility = visibility;
                m_updatedPropertiesOut.insert(propertyId);
            }

            return true;
        }

        bool MaterialFunctorAPI::EditorContext::SetMaterialPropertyVisibility(const MaterialPropertyIndex& index, MaterialPropertyVisibility visibility)
        {
            const Name& name = m_materialProperties.GetMaterialPropertiesLayout()->GetPropertyDescriptor(index)->GetName();
            return SetMaterialPropertyVisibility(name, visibility);
        }

        bool MaterialFunctorAPI::EditorContext::SetMaterialPropertyDescription(const Name& propertyId, AZStd::string description)
        {
            MaterialPropertyDynamicMetadata* metadata = QueryMaterialPropertyMetadata(propertyId);
            if (!metadata)
            {
                return false;
            }

            if (metadata->m_description != description)
            {
                metadata->m_description = description;
                m_updatedPropertiesOut.insert(propertyId);
            }

            return true;
        }

        bool MaterialFunctorAPI::EditorContext::SetMaterialPropertyDescription(const MaterialPropertyIndex& index, AZStd::string description)
        {
            const Name& name = m_materialProperties.GetMaterialPropertiesLayout()->GetPropertyDescriptor(index)->GetName();
            return SetMaterialPropertyDescription(name, description);
        }

        bool MaterialFunctorAPI::EditorContext::SetMaterialPropertyMinValue(const Name& propertyId, const MaterialPropertyValue& min)
        {
            MaterialPropertyDynamicMetadata* metadata = QueryMaterialPropertyMetadata(propertyId);
            if (!metadata)
            {
                return false;
            }

            if(metadata->m_propertyRange.m_min != min)
            {
                metadata->m_propertyRange.m_min = min;
                m_updatedPropertiesOut.insert(propertyId);
            }

            return true;
        }

        bool MaterialFunctorAPI::EditorContext::SetMaterialPropertyMinValue(const MaterialPropertyIndex& index, const MaterialPropertyValue& min)
        {
            const Name& name = m_materialProperties.GetMaterialPropertiesLayout()->GetPropertyDescriptor(index)->GetName();
            return SetMaterialPropertyMinValue(name, min);
        }

        bool MaterialFunctorAPI::EditorContext::SetMaterialPropertyMaxValue(const Name& propertyId, const MaterialPropertyValue& max)
        {
            MaterialPropertyDynamicMetadata* metadata = QueryMaterialPropertyMetadata(propertyId);
            if (!metadata)
            {
                return false;
            }

            if (metadata->m_propertyRange.m_max != max)
            {
                metadata->m_propertyRange.m_max = max;
                m_updatedPropertiesOut.insert(propertyId);
            }

            return true;
        }

        bool MaterialFunctorAPI::EditorContext::SetMaterialPropertyMaxValue(const MaterialPropertyIndex& index, const MaterialPropertyValue& max)
        {
            const Name& name = m_materialProperties.GetMaterialPropertiesLayout()->GetPropertyDescriptor(index)->GetName();
            return SetMaterialPropertyMaxValue(name, max);
        }

        bool MaterialFunctorAPI::EditorContext::SetMaterialPropertySoftMinValue(const Name& propertyId, const MaterialPropertyValue& min)
        {
            MaterialPropertyDynamicMetadata* metadata = QueryMaterialPropertyMetadata(propertyId);
            if (!metadata)
            {
                return false;
            }

            if (metadata->m_propertyRange.m_softMin != min)
            {
                metadata->m_propertyRange.m_softMin = min;
                m_updatedPropertiesOut.insert(propertyId);
            }

            return true;
        }

        bool MaterialFunctorAPI::EditorContext::SetMaterialPropertySoftMinValue(const MaterialPropertyIndex& index, const MaterialPropertyValue& min)
        {
            const Name& name = m_materialProperties.GetMaterialPropertiesLayout()->GetPropertyDescriptor(index)->GetName();
            return SetMaterialPropertySoftMinValue(name, min);
        }

        bool MaterialFunctorAPI::EditorContext::SetMaterialPropertySoftMaxValue(const Name& propertyId, const MaterialPropertyValue& max)
        {
            MaterialPropertyDynamicMetadata* metadata = QueryMaterialPropertyMetadata(propertyId);
            if (!metadata)
            {
                return false;
            }

            if (metadata->m_propertyRange.m_softMax != max)
            {
                metadata->m_propertyRange.m_softMax = max;
                m_updatedPropertiesOut.insert(propertyId);
            }

            return true;
        }

        bool MaterialFunctorAPI::EditorContext::SetMaterialPropertySoftMaxValue(const MaterialPropertyIndex& index, const MaterialPropertyValue& max)
        {
            const Name& name = m_materialProperties.GetMaterialPropertiesLayout()->GetPropertyDescriptor(index)->GetName();
            return SetMaterialPropertySoftMaxValue(name, max);
        }

        MaterialPropertyDynamicMetadata* MaterialFunctorAPI::EditorContext::QueryMaterialPropertyMetadata(const Name& propertyId) const
        {
            auto it = m_propertyMetadata.find(propertyId);
            if (it == m_propertyMetadata.end())
            {
                AZ_Error("MaterialFunctor", false, "Couldn't find metadata for material property: %s.", propertyId.GetCStr());
                return nullptr;
            }

            return &it->second;
        }
        
        MaterialPropertyGroupDynamicMetadata* MaterialFunctorAPI::EditorContext::QueryMaterialPropertyGroupMetadata(const Name& propertyGroupId) const
        {
            auto it = m_propertyGroupMetadata.find(propertyGroupId);
            if (it == m_propertyGroupMetadata.end())
            {
                AZ_Error("MaterialFunctor", false, "Couldn't find metadata for material property group: %s.", propertyGroupId.GetCStr());
                return nullptr;
            }

            return &it->second;
        }

        MaterialFunctorAPI::ReadMaterialPropertyValues::ReadMaterialPropertyValues(
            const MaterialPropertyCollection& materialProperties,
            const MaterialPropertyFlags* materialPropertyDependencies
        )
            : m_materialProperties(materialProperties)
            , m_materialPropertyDependencies(materialPropertyDependencies)
        { }

        template<typename Type>
        const Type& MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue(const MaterialPropertyIndex& index) const
        {
            return GetMaterialPropertyValue(index).GetValue<Type>();
        }

        // explicit template instantiation
        template AZ_DLL_EXPORT const bool&     MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<bool>     (const Name& propertyId) const;
        template AZ_DLL_EXPORT const int32_t&  MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<int32_t>  (const Name& propertyId) const;
        template AZ_DLL_EXPORT const uint32_t& MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<uint32_t> (const Name& propertyId) const;
        template AZ_DLL_EXPORT const float&    MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<float>    (const Name& propertyId) const;
        template AZ_DLL_EXPORT const Vector2&  MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<Vector2>  (const Name& propertyId) const;
        template AZ_DLL_EXPORT const Vector3&  MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<Vector3>  (const Name& propertyId) const;
        template AZ_DLL_EXPORT const Vector4&  MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<Vector4>  (const Name& propertyId) const;
        template AZ_DLL_EXPORT const Color&    MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<Color>    (const Name& propertyId) const;
        template AZ_DLL_EXPORT const Data::Instance<Image>& MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<Data::Instance<Image>>(const Name& propertyId) const;

        template<typename Type>
        const Type& MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue(const Name& propertyId) const
        {
            return GetMaterialPropertyValue(propertyId).GetValue<Type>();
        }

        // explicit template instantiation
        template AZ_DLL_EXPORT const bool&     MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<bool>     (const MaterialPropertyIndex& index) const;
        template AZ_DLL_EXPORT const int32_t&  MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<int32_t>  (const MaterialPropertyIndex& index) const;
        template AZ_DLL_EXPORT const uint32_t& MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<uint32_t> (const MaterialPropertyIndex& index) const;
        template AZ_DLL_EXPORT const float&    MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<float>    (const MaterialPropertyIndex& index) const;
        template AZ_DLL_EXPORT const Vector2&  MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<Vector2>  (const MaterialPropertyIndex& index) const;
        template AZ_DLL_EXPORT const Vector3&  MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<Vector3>  (const MaterialPropertyIndex& index) const;
        template AZ_DLL_EXPORT const Vector4&  MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<Vector4>  (const MaterialPropertyIndex& index) const;
        template AZ_DLL_EXPORT const Color&    MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<Color>    (const MaterialPropertyIndex& index) const;
        template AZ_DLL_EXPORT const Data::Instance<Image>& MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue<Data::Instance<Image>> (const MaterialPropertyIndex& index) const;

        void CheckPropertyAccess([[maybe_unused]] const MaterialPropertyIndex& index, [[maybe_unused]] const MaterialPropertyFlags& materialPropertyDependencies, [[maybe_unused]] const MaterialPropertiesLayout& materialPropertiesLayout)
        {
#if defined(AZ_ENABLE_TRACING)
            if (!materialPropertyDependencies.test(index.GetIndex()))
            {
                const MaterialPropertyDescriptor* propertyDescriptor = materialPropertiesLayout.GetPropertyDescriptor(index);
                AZ_Error("MaterialFunctor", false, "Material functor accessing an unregistered material property '%s'.",
                    propertyDescriptor ? propertyDescriptor->GetName().GetCStr() : "<unknown>");
            }
#endif
        }

        const MaterialPropertiesLayout* MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertiesLayout() const
        {
            return m_materialProperties.GetMaterialPropertiesLayout().get();
        }

        const MaterialPropertyValue& MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue(const MaterialPropertyIndex& index) const
        {
            CheckPropertyAccess(index, *m_materialPropertyDependencies, *GetMaterialPropertiesLayout());

            return m_materialProperties.GetPropertyValue(index);
        }

        const MaterialPropertyValue& MaterialFunctorAPI::ReadMaterialPropertyValues::GetMaterialPropertyValue(const Name& propertyId) const
        {
            MaterialPropertyIndex index = GetMaterialPropertiesLayout()->FindPropertyIndex(propertyId);
            return GetMaterialPropertyValue(index);
        }

        bool MaterialFunctor::NeedsProcess(const MaterialPropertyFlags& propertyDirtyFlags)
        {
            return (m_materialPropertyDependencies & propertyDirtyFlags).any();
        }

        const MaterialPropertyFlags& MaterialFunctor::GetMaterialPropertyDependencies() const
        {
            return m_materialPropertyDependencies;
        }

    } // namespace RPI
} // namespace AZ
