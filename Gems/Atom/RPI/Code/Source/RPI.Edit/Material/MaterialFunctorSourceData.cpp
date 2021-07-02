/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/ShaderCollection.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>

namespace AZ
{
    namespace RPI
    {
        void MaterialFunctorSourceData::AddMaterialPropertyDependency(Ptr<MaterialFunctor> functor, MaterialPropertyIndex index) const
        {
            functor->m_materialPropertyDependencies.set(index.GetIndex());
        }

        const MaterialPropertiesLayout* MaterialFunctorSourceData::RuntimeContext::GetMaterialPropertiesLayout() const
        {
            return m_materialPropertiesLayout;
        }

        AZStd::size_t MaterialFunctorSourceData::RuntimeContext::GetShaderCount() const
        {
            return m_shaderCollection->size();
        }

        AZStd::vector<AZ::Name> MaterialFunctorSourceData::RuntimeContext::GetShaderTags() const
        {
            AZStd::vector<AZ::Name> keys;
            keys.reserve(m_shaderCollection->size());
            for (auto it = m_shaderCollection->begin(); it != m_shaderCollection->end(); ++it)
            {
                keys.push_back(it->GetShaderTag());
            }
            return keys;
        }

        const ShaderOptionGroupLayout* MaterialFunctorSourceData::RuntimeContext::GetShaderOptionGroupLayout(AZStd::size_t shaderIndex) const
        {
            if (!CheckShaderIndexValid(shaderIndex))
            {
                return nullptr;
            }
            return (*m_shaderCollection)[shaderIndex].GetShaderAsset()->GetShaderOptionGroupLayout();
        }

        const ShaderOptionGroupLayout* MaterialFunctorSourceData::RuntimeContext::GetShaderOptionGroupLayout(const AZ::Name& shaderTag) const
        {
            if (!CheckShaderTagValid(shaderTag))
            {
                return nullptr;
            }
            return (*m_shaderCollection)[shaderTag].GetShaderAsset()->GetShaderOptionGroupLayout();
        }

        const RHI::ShaderResourceGroupLayout* MaterialFunctorSourceData::RuntimeContext::GetShaderResourceGroupLayout() const
        {
            return m_shaderResourceGroupLayout;
        }

        MaterialPropertyIndex MaterialFunctorSourceData::RuntimeContext::FindMaterialPropertyIndex(const Name& propertyName) const
        {
            MaterialPropertyIndex propertyIndex = m_materialPropertiesLayout->FindPropertyIndex(propertyName);

            AZ_Error("MaterialFunctorSourceData", propertyIndex.IsValid(), "Could not find property '%s'.", propertyName.GetCStr());

            return propertyIndex;
        }

        ShaderOptionIndex MaterialFunctorSourceData::RuntimeContext::FindShaderOptionIndex(AZStd::size_t shaderIndex, const Name& optionName, bool reportErrors) const
        {
            const ShaderOptionGroupLayout* shaderOptionGroupLayout = GetShaderOptionGroupLayout(shaderIndex);

            if (shaderOptionGroupLayout)
            {
                ShaderOptionIndex optionIndex = shaderOptionGroupLayout->FindShaderOptionIndex(optionName);
                if (reportErrors)
                {
                    AZ_Error("MaterialFunctorSourceData", optionIndex.IsValid(), "Could not find shader option '%s' in shader[%zu].", optionName.GetCStr(), shaderIndex);
                }
                return optionIndex;
            }

            return ShaderOptionIndex();
        }

        AZ::RPI::ShaderOptionIndex MaterialFunctorSourceData::RuntimeContext::FindShaderOptionIndex(const AZ::Name& shaderTag, const Name& optionName, bool reportErrors /*= true*/) const
        {
            const ShaderOptionGroupLayout* shaderOptionGroupLayout = GetShaderOptionGroupLayout(shaderTag);

            if (shaderOptionGroupLayout)
            {
                ShaderOptionIndex optionIndex = shaderOptionGroupLayout->FindShaderOptionIndex(optionName);
                if (reportErrors)
                {
                    AZ_Error("MaterialFunctorSourceData", optionIndex.IsValid(), "Could not find shader option '%s' in shader['%s'].", optionName.GetCStr(), shaderTag.GetCStr());
                }
                return optionIndex;
            }

            return ShaderOptionIndex();
        }

        bool MaterialFunctorSourceData::RuntimeContext::CheckShaderIndexValid(AZStd::size_t shaderIndex) const
        {
            const bool valid = (m_shaderCollection->size() > shaderIndex);
            AZ_Error("MaterialFunctorSourceData", valid, "Shader index %zu is invalid", shaderIndex);
            return valid;
        }

        bool MaterialFunctorSourceData::RuntimeContext::CheckShaderTagValid(const AZ::Name& shaderTag) const
        {
            const bool valid = m_shaderCollection->HasShaderTag(shaderTag);
            AZ_Error("MaterialFunctorSourceData", valid, "Shader tag '%s' is invalid", shaderTag.GetCStr());
            return valid;
        }

        const MaterialPropertiesLayout* MaterialFunctorSourceData::EditorContext::GetMaterialPropertiesLayout() const
        {
            return m_materialPropertiesLayout;
        }

        MaterialPropertyIndex MaterialFunctorSourceData::EditorContext::FindMaterialPropertyIndex(const Name& propertyName) const
        {
            MaterialPropertyIndex propertyIndex = m_materialPropertiesLayout->FindPropertyIndex(propertyName);

            AZ_Error("MaterialFunctorSourceData", propertyIndex.IsValid(), "Could not find property '%s'", propertyName.GetCStr());

            return propertyIndex;
        }
    } // namespace RPI
} // namespace AZ
