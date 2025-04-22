/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "./ConvertEmissiveUnitFunctorSourceData.h"
#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        void ConvertEmissiveUnitFunctorSourceData::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ConvertEmissiveUnitFunctorSourceData>()
                    ->Version(5)
                    ->Field("intensityProperty", &ConvertEmissiveUnitFunctorSourceData::m_intensityPropertyName)
                    ->Field("lightUnitProperty", &ConvertEmissiveUnitFunctorSourceData::m_lightUnitPropertyName)
                    ->Field("shaderInput", &ConvertEmissiveUnitFunctorSourceData::m_shaderInputName)
                    ->Field("ev100Index", &ConvertEmissiveUnitFunctorSourceData::m_ev100Index)
                    ->Field("nitIndex", &ConvertEmissiveUnitFunctorSourceData::m_nitIndex)
                    ->Field("ev100MinMax", &ConvertEmissiveUnitFunctorSourceData::m_ev100MinMax)
                    ->Field("nitMinMax", &ConvertEmissiveUnitFunctorSourceData::m_nitMinMax)
                    ;
            }
        }

        RPI::MaterialFunctorSourceData::FunctorResult ConvertEmissiveUnitFunctorSourceData::CreateFunctor(const RuntimeContext& context) const
        {
            RPI::Ptr<ConvertEmissiveUnitFunctor> functor = aznew ConvertEmissiveUnitFunctor;

            functor->m_intensityPropertyIndex = context.FindMaterialPropertyIndex(AZ::Name{ m_intensityPropertyName });
            if (functor->m_intensityPropertyIndex.IsNull())
            {
                return Failure();
            }
            AddMaterialPropertyDependency(functor, functor->m_intensityPropertyIndex);

            functor->m_lightUnitPropertyIndex = context.FindMaterialPropertyIndex(AZ::Name{ m_lightUnitPropertyName });
            if (functor->m_lightUnitPropertyIndex.IsNull())
            {
                return Failure();
            }
            AddMaterialPropertyDependency(functor, functor->m_lightUnitPropertyIndex);

            functor->m_shaderInputIndex = context.FindShaderInputConstantIndex(AZ::Name{ m_shaderInputName });

            if (functor->m_shaderInputIndex.IsNull())
            {
                AZ_Error("ConvertEmissiveUnitFunctorSourceData", false, "Could not find shader input '%s'", m_shaderInputName.data());
                return Failure();
            }

            functor->m_ev100Index = m_ev100Index;
            functor->m_nitIndex = m_nitIndex;

            return Success(RPI::Ptr<RPI::MaterialFunctor>(functor));
        }

        RPI::MaterialFunctorSourceData::FunctorResult ConvertEmissiveUnitFunctorSourceData::CreateFunctor(const EditorContext& context) const
        {
            RPI::Ptr<ConvertEmissiveUnitFunctor> functor = aznew ConvertEmissiveUnitFunctor;

            functor->m_intensityPropertyIndex = context.FindMaterialPropertyIndex(AZ::Name{ m_intensityPropertyName });
            if (functor->m_intensityPropertyIndex.IsNull())
            {
                return Failure();
            }
            AddMaterialPropertyDependency(functor, functor->m_intensityPropertyIndex);

            functor->m_lightUnitPropertyIndex = context.FindMaterialPropertyIndex(AZ::Name{ m_lightUnitPropertyName });
            if (functor->m_lightUnitPropertyIndex.IsNull())
            {
                return Failure();
            }
            AddMaterialPropertyDependency(functor, functor->m_lightUnitPropertyIndex);

            functor->m_ev100Index = m_ev100Index;
            functor->m_nitIndex = m_nitIndex;
            functor->m_ev100Min = m_ev100MinMax.GetX();
            functor->m_ev100Max = m_ev100MinMax.GetY();
            functor->m_nitMin = m_nitMinMax.GetX();
            functor->m_nitMax = m_nitMinMax.GetY();

            return Success(RPI::Ptr<RPI::MaterialFunctor>(functor));
        }
    }
}
