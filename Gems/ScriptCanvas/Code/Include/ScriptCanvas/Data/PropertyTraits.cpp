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

#include <ScriptCanvas/Data/PropertyTraits.h>
#include <ScriptCanvas/Data/DataRegistry.h>

namespace ScriptCanvas
{
    namespace Data
    {
        void PropertyMetadata::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PropertyMetadata>()
                    ->Field("m_propertySlotId", &PropertyMetadata::m_propertySlotId)
                    ->Field("m_propertyType", &PropertyMetadata::m_propertyType)
                    ->Field("m_propertyName", &PropertyMetadata::m_propertyName)
                    ;
            }
        }

        GetterContainer ExplodeToGetters(const Data::Type& type)
        {
            auto& typeIdTraitMap = GetDataRegistry()->m_typeIdTraitMap;
            auto dataTraitIt = typeIdTraitMap.find(type.GetType());
            if (dataTraitIt != typeIdTraitMap.end())
            {
                return dataTraitIt->second.m_propertyTraits.GetGetterWrappers(type);
            }
            return {};
        }

        SetterContainer ExplodeToSetters(const Data::Type& type)
        {
            auto& typeIdTraitMap = GetDataRegistry()->m_typeIdTraitMap;
            auto dataTraitIt = typeIdTraitMap.find(type.GetType());
            if (dataTraitIt != typeIdTraitMap.end())
            {
                return dataTraitIt->second.m_propertyTraits.GetSetterWrappers(type);
            }

            return {};
        }
    } // namespace Data
} // namespace ScriptCanvas