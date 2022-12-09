/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataHolder.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>

namespace AZ
{
    namespace RPI
    {
        MaterialFunctorSourceDataHolder::MaterialFunctorSourceDataHolder(Ptr<MaterialFunctorSourceData> actualSourceData)
            : m_actualSourceData(actualSourceData)
        {
        }

        void MaterialFunctorSourceDataHolder::Reflect(AZ::ReflectContext* context)
        {
            if (JsonRegistrationContext* jsonContext = azrtti_cast<JsonRegistrationContext*>(context))
            {
                jsonContext->Serializer<JsonMaterialFunctorSourceDataSerializer>()->HandlesType<MaterialFunctorSourceDataHolder>();
            }
            else if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<MaterialFunctorSourceDataHolder>();
            }
        }

        MaterialFunctorSourceData::FunctorResult MaterialFunctorSourceDataHolder::CreateFunctor(const MaterialFunctorSourceData::RuntimeContext& runtimeContext) const
        {
            return m_actualSourceData ? m_actualSourceData->CreateFunctor(runtimeContext) : Failure();
        }

        MaterialFunctorSourceData::FunctorResult MaterialFunctorSourceDataHolder::CreateFunctor(const MaterialFunctorSourceData::EditorContext& editorContext) const
        {
            return m_actualSourceData ? m_actualSourceData->CreateFunctor(editorContext) : Failure();
        }

        Ptr<MaterialFunctorSourceData> MaterialFunctorSourceDataHolder::GetActualSourceData() const
        {
            return m_actualSourceData;
        }
    } // namespace RPI
} // namespace AZ
