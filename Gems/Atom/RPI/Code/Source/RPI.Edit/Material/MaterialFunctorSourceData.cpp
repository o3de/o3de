/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>
#include <Atom/RPI.Reflect/Material/MaterialNameContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>

namespace AZ
{
    namespace RPI
    {
        void MaterialFunctorSourceData::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialFunctorSourceData>();
            }
        }

        void MaterialFunctorSourceData::AddMaterialPropertyDependency(Ptr<MaterialFunctor> functor, MaterialPropertyIndex index) const
        {
            functor->m_materialPropertyDependencies.set(index.GetIndex());
        }

        const MaterialPropertiesLayout* MaterialFunctorSourceData::RuntimeContext::GetMaterialPropertiesLayout() const
        {
            return m_materialPropertiesLayout;
        }

        const RHI::ShaderResourceGroupLayout* MaterialFunctorSourceData::RuntimeContext::GetShaderResourceGroupLayout() const
        {
            return m_shaderResourceGroupLayout;
        }

        MaterialPropertyIndex MaterialFunctorSourceData::RuntimeContext::FindMaterialPropertyIndex(Name propertyId) const
        {
            m_materialNameContext->ContextualizeProperty(propertyId);
            MaterialPropertyIndex propertyIndex = m_materialPropertiesLayout->FindPropertyIndex(propertyId);

            AZ_Error("MaterialFunctorSourceData", propertyIndex.IsValid(), "Could not find property '%s'.", propertyId.GetCStr());

            return propertyIndex;
        }

        RHI::ShaderInputConstantIndex MaterialFunctorSourceData::RuntimeContext::FindShaderInputConstantIndex(Name inputName) const
        {
            m_materialNameContext->ContextualizeSrgInput(inputName);
            return m_shaderResourceGroupLayout->FindShaderInputConstantIndex(inputName);
        }

        RHI::ShaderInputImageIndex MaterialFunctorSourceData::RuntimeContext::FindShaderInputImageIndex(Name inputName) const
        {
            m_materialNameContext->ContextualizeSrgInput(inputName);
            return m_shaderResourceGroupLayout->FindShaderInputImageIndex(inputName);
        }

        const MaterialPropertiesLayout* MaterialFunctorSourceData::EditorContext::GetMaterialPropertiesLayout() const
        {
            return m_materialPropertiesLayout;
        }

        MaterialPropertyIndex MaterialFunctorSourceData::EditorContext::FindMaterialPropertyIndex(Name propertyId) const
        {
            m_materialNameContext->ContextualizeProperty(propertyId);
            MaterialPropertyIndex propertyIndex = m_materialPropertiesLayout->FindPropertyIndex(propertyId);

            AZ_Error("MaterialFunctorSourceData", propertyIndex.IsValid(), "Could not find property '%s'", propertyId.GetCStr());

            return propertyIndex;
        }

    } // namespace RPI
} // namespace AZ
