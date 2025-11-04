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
                serializeContext->Class<MaterialFunctorSourceData>()->Version(1)->Field(
                    "shaderParameters", &MaterialFunctorSourceData::m_shaderParameters);
                ;
            }
        }

        void MaterialFunctorSourceData::SetFunctorShaderParameter(
            Ptr<MaterialFunctor> functor, const AZStd::vector<MaterialFunctorShaderParameter>& shaderParameters)
        {
            functor->m_functorShaderParameters = shaderParameters;
        }

        void MaterialFunctorSourceData::AddMaterialPropertyDependency(Ptr<MaterialFunctor> functor, MaterialPropertyIndex index) const
        {
            functor->m_materialPropertyDependencies.set(index.GetIndex());
        }

        const MaterialPropertiesLayout* MaterialFunctorSourceData::RuntimeContext::GetMaterialPropertiesLayout() const
        {
            return m_materialPropertiesLayout;
        }

        MaterialPropertyIndex MaterialFunctorSourceData::RuntimeContext::FindMaterialPropertyIndex(Name propertyId) const
        {
            m_materialNameContext->ContextualizeProperty(propertyId);
            MaterialPropertyIndex propertyIndex = m_materialPropertiesLayout->FindPropertyIndex(propertyId);

            AZ_Error("MaterialFunctorSourceData", propertyIndex.IsValid(), "Could not find property '%s'.", propertyId.GetCStr());

            return propertyIndex;
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

        const AZStd::vector<MaterialFunctorShaderParameter> MaterialFunctorSourceData::GetMaterialShaderParameters(
            const MaterialNameContext* nameContext) const
        {
            AZStd::vector<MaterialFunctorShaderParameter> result{};
            if (nameContext && nameContext->HasContextForSrgInputs())
            {
                for (const auto& param : m_shaderParameters)
                {
                    AZStd::string contextualizedName{ param.m_name };
                    nameContext->ContextualizeSrgInput(contextualizedName);
                    result.emplace_back(MaterialFunctorShaderParameter{ contextualizedName, param.m_typeName, param.m_typeSize });
                }
            }
            else
            {
                result = m_shaderParameters;
            }
            return result;
        }

    } // namespace RPI
} // namespace AZ
