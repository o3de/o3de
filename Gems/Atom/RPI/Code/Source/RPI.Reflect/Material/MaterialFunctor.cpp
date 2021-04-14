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

#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
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

        MaterialFunctor::RuntimeContext::RuntimeContext(
            const AZStd::vector<MaterialPropertyValue>& propertyValues,
            RHI::ConstPtr<MaterialPropertiesLayout> materialPropertiesLayout,
            ShaderCollection* shaderCollection,
            ShaderResourceGroup* shaderResourceGroup,
            const MaterialPropertyFlags* materialPropertyDependencies
        )
            : m_materialPropertyValues(propertyValues)
            , m_materialPropertiesLayout(materialPropertiesLayout)
            , m_shaderCollection(shaderCollection)
            , m_shaderResourceGroup(shaderResourceGroup)
            , m_materialPropertyDependencies(materialPropertyDependencies)
        {}

        bool MaterialFunctor::RuntimeContext::SetShaderOptionValue(ShaderCollection::Item& shaderItem, ShaderOptionIndex optionIndex, ShaderOptionValue value)
        {
            ShaderOptionGroup* shaderOptionGroup = shaderItem.GetShaderOptions();
            const ShaderOptionGroupLayout* layout = shaderOptionGroup->GetShaderOptionLayout();

            if (optionIndex.GetIndex() >= layout->GetShaderOptionCount())
            {
                AZ_Error("MaterialFunctor", false, "Shader option index %d is out of range.", optionIndex.GetIndex());
            }
            else if (shaderItem.MaterialOwnsShaderOption(optionIndex))
            {
                return shaderOptionGroup->SetValue(optionIndex, value);
            }
            else
            {
                AZ_Error("MaterialFunctor", false, "Shader option '%s' is not owned by this material.", layout->GetShaderOption(optionIndex).GetName().GetCStr());
            }

            return false;
        }

        bool MaterialFunctor::RuntimeContext::SetShaderOptionValue(AZStd::size_t shaderIndex, ShaderOptionIndex optionIndex, ShaderOptionValue value)
        {
            if (shaderIndex < (*m_shaderCollection).size())
            {
                ShaderCollection::Item& shaderItem = (*m_shaderCollection)[shaderIndex];
                return SetShaderOptionValue(shaderItem, optionIndex, value);
            }
            else
            {
                AZ_Error("MaterialFunctor", false, "Shader index %zu is out of range. There are %zu shaders available.", shaderIndex, (*m_shaderCollection).size());
            }

            return false;
        }

        bool MaterialFunctor::RuntimeContext::SetShaderOptionValue(const AZ::Name& shaderTag, ShaderOptionIndex optionIndex, ShaderOptionValue value)
        {
            if (m_shaderCollection->HasShaderTag(shaderTag))
            {
                ShaderCollection::Item& shaderItem = (*m_shaderCollection)[shaderTag];
                return SetShaderOptionValue(shaderItem, optionIndex, value);
            }
            else
            {
                AZ_Error("MaterialFunctor", false, "Shader tag '%s' is invalid.", shaderTag.GetCStr());
            }

            return false;
        }

        ShaderResourceGroup* MaterialFunctor::RuntimeContext::GetShaderResourceGroup()
        {
            return m_shaderResourceGroup;
        }

        AZStd::size_t MaterialFunctor::RuntimeContext::GetShaderCount() const
        {
            return m_shaderCollection->size();
        }

        void MaterialFunctor::RuntimeContext::SetShaderEnabled(AZStd::size_t shaderIndex, bool enabled)
        {
            (*m_shaderCollection)[shaderIndex].SetEnabled(enabled);
        }

        void MaterialFunctor::RuntimeContext::SetShaderEnabled(const AZ::Name& shaderTag, bool enabled)
        {
            (*m_shaderCollection)[shaderTag].SetEnabled(enabled);
        }

        void MaterialFunctor::RuntimeContext::SetShaderDrawListTagOverride(AZStd::size_t shaderIndex, const Name& drawListTagName)
        {
            (*m_shaderCollection)[shaderIndex].SetDrawListTagOverride(drawListTagName);
        }

        void MaterialFunctor::RuntimeContext::SetShaderDrawListTagOverride(const AZ::Name& shaderTag, const Name& drawListTagName)
        {
            (*m_shaderCollection)[shaderTag].SetDrawListTagOverride(drawListTagName);
        }

        void MaterialFunctor::RuntimeContext::ApplyShaderRenderStateOverlay(AZStd::size_t shaderIndex, const RHI::RenderStates& renderStatesOverlay)
        {
            RHI::MergeStateInto(renderStatesOverlay, *((*m_shaderCollection)[shaderIndex].GetRenderStatesOverlay()));
        }

        void MaterialFunctor::RuntimeContext::ApplyShaderRenderStateOverlay(const AZ::Name& shaderTag, const RHI::RenderStates& renderStatesOverlay)
        {
            RHI::MergeStateInto(renderStatesOverlay, *((*m_shaderCollection)[shaderTag].GetRenderStatesOverlay()));
        }

        MaterialFunctor::EditorContext::EditorContext(
            const AZStd::vector<MaterialPropertyValue>& propertyValues,
            RHI::ConstPtr<MaterialPropertiesLayout> materialPropertiesLayout,
            AZStd::unordered_map<Name, MaterialPropertyDynamicMetadata>& metadata,
            AZStd::unordered_set<Name>& outChangedProperties,
            const MaterialPropertyFlags* materialPropertyDependencies
        )
            : m_materialPropertyValues(propertyValues)
            , m_materialPropertiesLayout(materialPropertiesLayout)
            , m_metadata(metadata)
            , m_outChangedProperties(outChangedProperties)
            , m_materialPropertyDependencies(materialPropertyDependencies)
        {}

        const MaterialPropertyDynamicMetadata* MaterialFunctor::EditorContext::GetMaterialPropertyMetadata(const Name& propertyName) const
        {
            auto it = QueryMaterialMetadata(propertyName);
            if (it == m_metadata.end())
            {
                return nullptr;
            }
            return &(it->second);
        }

        const MaterialPropertyDynamicMetadata* MaterialFunctor::EditorContext::GetMaterialPropertyMetadata(const MaterialPropertyIndex& index) const
        {
            const Name& name = m_materialPropertiesLayout->GetPropertyDescriptor(index)->GetName();
            return GetMaterialPropertyMetadata(name);
        }

        bool MaterialFunctor::EditorContext::SetMaterialPropertyVisibility(const Name& propertyName, MaterialPropertyVisibility visibility)
        {
            auto it = QueryMaterialMetadata(propertyName);
            if (it == m_metadata.end())
            {
                return false;
            }
            MaterialPropertyVisibility originValue = it->second.m_visibility;
            it->second.m_visibility = visibility;
            if (originValue != visibility)
            {
                m_outChangedProperties.insert(propertyName);
            }

            return true;
        }

        bool MaterialFunctor::EditorContext::SetMaterialPropertyVisibility(const MaterialPropertyIndex& index, MaterialPropertyVisibility visibility)
        {
            const Name& name = m_materialPropertiesLayout->GetPropertyDescriptor(index)->GetName();
            return SetMaterialPropertyVisibility(name, visibility);
        }

        bool MaterialFunctor::EditorContext::SetMaterialPropertyDescription(const Name& propertyName, AZStd::string description)
        {
            auto it = QueryMaterialMetadata(propertyName);
            if (it == m_metadata.end())
            {
                return false;
            }

            AZStd::string origin = it->second.m_description;
            it->second.m_description = description;
            if (origin != description)
            {
                m_outChangedProperties.insert(propertyName);
            }

            return true;
        }

        bool MaterialFunctor::EditorContext::SetMaterialPropertyDescription(const MaterialPropertyIndex& index, AZStd::string description)
        {
            const Name& name = m_materialPropertiesLayout->GetPropertyDescriptor(index)->GetName();
            return SetMaterialPropertyDescription(name, description);
        }

        bool MaterialFunctor::EditorContext::SetMaterialPropertyMinValue(const Name& propertyName, const MaterialPropertyValue& min)
        {
            auto it = QueryMaterialMetadata(propertyName);
            if (it == m_metadata.end())
            {
                return false;
            }

            MaterialPropertyValue origin = it->second.m_propertyRange.m_min;
            it->second.m_propertyRange.m_min = min;

            if(origin != min)
            {
                m_outChangedProperties.insert(propertyName);
            }

            return true;
        }

        bool MaterialFunctor::EditorContext::SetMaterialPropertyMinValue(const MaterialPropertyIndex& index, const MaterialPropertyValue& min)
        {
            const Name& name = m_materialPropertiesLayout->GetPropertyDescriptor(index)->GetName();
            return SetMaterialPropertyMinValue(name, min);
        }

        bool MaterialFunctor::EditorContext::SetMaterialPropertyMaxValue(const Name& propertyName, const MaterialPropertyValue& max)
        {
            auto it = QueryMaterialMetadata(propertyName);
            if (it == m_metadata.end())
            {
                return false;
            }

            MaterialPropertyValue origin = it->second.m_propertyRange.m_max;
            it->second.m_propertyRange.m_max = max;

            if (origin != max)
            {
                m_outChangedProperties.insert(propertyName);
            }

            return true;
        }

        bool MaterialFunctor::EditorContext::SetMaterialPropertyMaxValue(const MaterialPropertyIndex& index, const MaterialPropertyValue& max)
        {
            const Name& name = m_materialPropertiesLayout->GetPropertyDescriptor(index)->GetName();
            return SetMaterialPropertyMaxValue(name, max);
        }

        bool MaterialFunctor::EditorContext::SetMaterialPropertySoftMinValue(const Name& propertyName, const MaterialPropertyValue& min)
        {
            auto it = QueryMaterialMetadata(propertyName);
            if (it == m_metadata.end())
            {
                return false;
            }

            MaterialPropertyValue origin = it->second.m_propertyRange.m_softMin;
            it->second.m_propertyRange.m_softMin = min;

            if (origin != min)
            {
                m_outChangedProperties.insert(propertyName);
            }

            return true;
        }

        bool MaterialFunctor::EditorContext::SetMaterialPropertySoftMinValue(const MaterialPropertyIndex& index, const MaterialPropertyValue& min)
        {
            const Name& name = m_materialPropertiesLayout->GetPropertyDescriptor(index)->GetName();
            return SetMaterialPropertySoftMinValue(name, min);
        }

        bool MaterialFunctor::EditorContext::SetMaterialPropertySoftMaxValue(const Name& propertyName, const MaterialPropertyValue& max)
        {
            auto it = QueryMaterialMetadata(propertyName);
            if (it == m_metadata.end())
            {
                return false;
            }

            MaterialPropertyValue origin = it->second.m_propertyRange.m_softMax;
            it->second.m_propertyRange.m_softMax = max;

            if (origin != max)
            {
                m_outChangedProperties.insert(propertyName);
            }

            return true;
        }

        bool MaterialFunctor::EditorContext::SetMaterialPropertySoftMaxValue(const MaterialPropertyIndex& index, const MaterialPropertyValue& max)
        {
            const Name& name = m_materialPropertiesLayout->GetPropertyDescriptor(index)->GetName();
            return SetMaterialPropertySoftMaxValue(name, max);
        }

        AZStd::list_iterator<AZStd::pair<AZ::Name, AZ::RPI::MaterialPropertyDynamicMetadata>> MaterialFunctor::EditorContext::QueryMaterialMetadata(const Name& propertyName) const
        {
            auto it = m_metadata.find(propertyName);
            if (it == m_metadata.end())
            {
                AZ_Error("MaterialFunctor", false, "Couldn't find metadata for material property: %s.", propertyName.GetCStr());
            }

            return it;
        }

        template<typename Type>
        const Type& MaterialFunctor::RuntimeContext::GetMaterialPropertyValue(const MaterialPropertyIndex& index) const
        {
            return GetMaterialPropertyValue(index).GetValue<Type>();
        }

        // explicit template instantiation
        template const bool&     MaterialFunctor::RuntimeContext::GetMaterialPropertyValue<bool>     (const Name& propertyName) const;
        template const int32_t&  MaterialFunctor::RuntimeContext::GetMaterialPropertyValue<int32_t>  (const Name& propertyName) const;
        template const uint32_t& MaterialFunctor::RuntimeContext::GetMaterialPropertyValue<uint32_t> (const Name& propertyName) const;
        template const float&    MaterialFunctor::RuntimeContext::GetMaterialPropertyValue<float>    (const Name& propertyName) const;
        template const Vector2&  MaterialFunctor::RuntimeContext::GetMaterialPropertyValue<Vector2>  (const Name& propertyName) const;
        template const Vector3&  MaterialFunctor::RuntimeContext::GetMaterialPropertyValue<Vector3>  (const Name& propertyName) const;
        template const Vector4&  MaterialFunctor::RuntimeContext::GetMaterialPropertyValue<Vector4>  (const Name& propertyName) const;
        template const Color&    MaterialFunctor::RuntimeContext::GetMaterialPropertyValue<Color>    (const Name& propertyName) const;
        template const Data::Instance<Image>& MaterialFunctor::RuntimeContext::GetMaterialPropertyValue<Data::Instance<Image>>(const Name& propertyName) const;

        template<typename Type>
        const Type& MaterialFunctor::RuntimeContext::GetMaterialPropertyValue(const Name& propertyName) const
        {
            return GetMaterialPropertyValue(propertyName).GetValue<Type>();
        }

        // explicit template instantiation
        template const bool&     MaterialFunctor::RuntimeContext::GetMaterialPropertyValue<bool>     (const MaterialPropertyIndex& index) const;
        template const int32_t&  MaterialFunctor::RuntimeContext::GetMaterialPropertyValue<int32_t>  (const MaterialPropertyIndex& index) const;
        template const uint32_t& MaterialFunctor::RuntimeContext::GetMaterialPropertyValue<uint32_t> (const MaterialPropertyIndex& index) const;
        template const float&    MaterialFunctor::RuntimeContext::GetMaterialPropertyValue<float>    (const MaterialPropertyIndex& index) const;
        template const Vector2&  MaterialFunctor::RuntimeContext::GetMaterialPropertyValue<Vector2>  (const MaterialPropertyIndex& index) const;
        template const Vector3&  MaterialFunctor::RuntimeContext::GetMaterialPropertyValue<Vector3>  (const MaterialPropertyIndex& index) const;
        template const Vector4&  MaterialFunctor::RuntimeContext::GetMaterialPropertyValue<Vector4>  (const MaterialPropertyIndex& index) const;
        template const Color&    MaterialFunctor::RuntimeContext::GetMaterialPropertyValue<Color>    (const MaterialPropertyIndex& index) const;
        template const Data::Instance<Image>& MaterialFunctor::RuntimeContext::GetMaterialPropertyValue<Data::Instance<Image>> (const MaterialPropertyIndex& index) const;

        template<typename Type>
        const Type& MaterialFunctor::EditorContext::GetMaterialPropertyValue(const MaterialPropertyIndex& index) const
        {
            return GetMaterialPropertyValue(index).GetValue<Type>();
        }

        // explicit template instantiation
        template const bool&     MaterialFunctor::EditorContext::GetMaterialPropertyValue<bool>     (const Name& propertyName) const;
        template const int32_t&  MaterialFunctor::EditorContext::GetMaterialPropertyValue<int32_t>  (const Name& propertyName) const;
        template const uint32_t& MaterialFunctor::EditorContext::GetMaterialPropertyValue<uint32_t> (const Name& propertyName) const;
        template const float&    MaterialFunctor::EditorContext::GetMaterialPropertyValue<float>    (const Name& propertyName) const;
        template const Vector2&  MaterialFunctor::EditorContext::GetMaterialPropertyValue<Vector2>  (const Name& propertyName) const;
        template const Vector3&  MaterialFunctor::EditorContext::GetMaterialPropertyValue<Vector3>  (const Name& propertyName) const;
        template const Vector4&  MaterialFunctor::EditorContext::GetMaterialPropertyValue<Vector4>  (const Name& propertyName) const;
        template const Color&    MaterialFunctor::EditorContext::GetMaterialPropertyValue<Color>    (const Name& propertyName) const;

        template<typename Type>
        const Type& MaterialFunctor::EditorContext::GetMaterialPropertyValue(const Name& propertyName) const
        {
            return GetMaterialPropertyValue(propertyName).GetValue<Type>();
        }

        // explicit template instantiation
        template const bool&     MaterialFunctor::EditorContext::GetMaterialPropertyValue<bool>     (const MaterialPropertyIndex& index) const;
        template const int32_t&  MaterialFunctor::EditorContext::GetMaterialPropertyValue<int32_t>  (const MaterialPropertyIndex& index) const;
        template const uint32_t& MaterialFunctor::EditorContext::GetMaterialPropertyValue<uint32_t> (const MaterialPropertyIndex& index) const;
        template const float&    MaterialFunctor::EditorContext::GetMaterialPropertyValue<float>    (const MaterialPropertyIndex& index) const;
        template const Vector2&  MaterialFunctor::EditorContext::GetMaterialPropertyValue<Vector2>  (const MaterialPropertyIndex& index) const;
        template const Vector3&  MaterialFunctor::EditorContext::GetMaterialPropertyValue<Vector3>  (const MaterialPropertyIndex& index) const;
        template const Vector4&  MaterialFunctor::EditorContext::GetMaterialPropertyValue<Vector4>  (const MaterialPropertyIndex& index) const;
        template const Color&    MaterialFunctor::EditorContext::GetMaterialPropertyValue<Color>    (const MaterialPropertyIndex& index) const;
        template const Data::Instance<Image>& MaterialFunctor::EditorContext::GetMaterialPropertyValue<Data::Instance<Image>> (const MaterialPropertyIndex& index) const;

        void CheckPropertyAccess(const MaterialPropertyIndex& index, const MaterialPropertyFlags& materialPropertyDependencies, [[maybe_unused]] const MaterialPropertiesLayout& materialPropertiesLayout)
        {
            if (!materialPropertyDependencies.test(index.GetIndex()))
            {
#if defined(AZ_ENABLE_TRACING)
                const MaterialPropertyDescriptor* propertyDescriptor = materialPropertiesLayout.GetPropertyDescriptor(index);
#endif
                AZ_Error("MaterialFunctor", false, "Material functor accessing an unregistered material property '%s'.",
                    propertyDescriptor ? propertyDescriptor->GetName().GetCStr() : "<unknown>");
            }
        }

        const MaterialPropertyValue& MaterialFunctor::RuntimeContext::GetMaterialPropertyValue(const MaterialPropertyIndex& index) const
        {
            CheckPropertyAccess(index, *m_materialPropertyDependencies, *m_materialPropertiesLayout);

            return m_materialPropertyValues[index.GetIndex()];
        }

        const MaterialPropertyValue& MaterialFunctor::RuntimeContext::GetMaterialPropertyValue(const Name& propertyName) const
        {
            MaterialPropertyIndex index = m_materialPropertiesLayout->FindPropertyIndex(propertyName);
            return GetMaterialPropertyValue(index);
        }

        const MaterialPropertyValue& MaterialFunctor::EditorContext::GetMaterialPropertyValue(const MaterialPropertyIndex& index) const
        {
            CheckPropertyAccess(index, *m_materialPropertyDependencies, *m_materialPropertiesLayout);

            return m_materialPropertyValues[index.GetIndex()];
        }

        const MaterialPropertyValue& MaterialFunctor::EditorContext::GetMaterialPropertyValue(const Name& propertyName) const
        {
            MaterialPropertyIndex index = m_materialPropertiesLayout->FindPropertyIndex(propertyName);
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
