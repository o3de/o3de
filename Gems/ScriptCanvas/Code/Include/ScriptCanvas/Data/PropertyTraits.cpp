/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    } 
}
