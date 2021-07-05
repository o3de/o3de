/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialPropertyValueSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyValueSourceDataSerializer.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/StackedString.h>

namespace AZ
{
    namespace RPI
    {
        void MaterialPropertyValueSourceData::Reflect(ReflectContext* context)
        {
            if (JsonRegistrationContext* jsonContext = azrtti_cast<JsonRegistrationContext*>(context))
            {
                jsonContext->Serializer<JsonMaterialPropertyValueSourceDataSerializer>()->HandlesType<MaterialPropertyValueSourceData>();
            }
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialPropertyValueSourceData>()
                    ->Version(1);
            }
        }

        bool MaterialPropertyValueSourceData::Resolve(const MaterialPropertiesLayout& propertiesLayout, const AZ::Name& materialPropertyName) const
        {
            const MaterialPropertyDescriptor* propertyDescriptor = propertiesLayout.GetPropertyDescriptor(propertiesLayout.FindPropertyIndex(materialPropertyName));
            if (!propertyDescriptor)
            {
                AZ_Error("MaterialPropertyValueSourceData", false, "Material property '%s' can't be found.", materialPropertyName.GetCStr());
                return false;
            }

            AZ::TypeId typeId = propertyDescriptor->GetStorageDataTypeId();
            auto iter = m_possibleValues.find(typeId);
            if (iter != m_possibleValues.end())
            {
                m_resolvedValue = iter->second;
            }

            if (!m_resolvedValue.IsValid())
            {
                AZ_Error("MaterialPropertyValueSourceData", false, "Value for material property '%s' is invalid. %s is required.", materialPropertyName.GetCStr(), ToString(propertyDescriptor->GetDataType()));
                return false;
            }

            return true;
        }

        bool MaterialPropertyValueSourceData::IsResolved() const
        {
            return m_resolvedValue.IsValid();
        }

        const MaterialPropertyValue& MaterialPropertyValueSourceData::GetValue() const
        {
            AZ_Assert(IsResolved(), "Value is not resolved. Resolve() or SetValue() should be called first before GetValue().");
            return m_resolvedValue;
        }

        void MaterialPropertyValueSourceData::SetValue(const MaterialPropertyValue& value)
        {
            m_resolvedValue = AZStd::move(value);
        }

        bool MaterialPropertyValueSourceData::AreSimilar(const MaterialPropertyValueSourceData& lhs, const MaterialPropertyValueSourceData& rhs)
        {
            // Special case where both are empty. They are treated equal.
            if (!lhs.IsResolved() && !rhs.IsResolved() && lhs.m_possibleValues.empty() && rhs.m_possibleValues.empty())
            {
                return true;
            }

            if (lhs.IsResolved() && rhs.IsResolved())
            {
                return lhs.m_resolvedValue == rhs.m_resolvedValue;
            }
            else if (lhs.IsResolved())
            {
                auto possibleValueRhs = rhs.m_possibleValues.find(lhs.m_resolvedValue.GetTypeId());
                return (possibleValueRhs != rhs.m_possibleValues.end())
                    && (lhs.m_resolvedValue == possibleValueRhs->second);
            }
            else if (rhs.IsResolved())
            {
                auto possibleValueLhs = lhs.m_possibleValues.find(rhs.m_resolvedValue.GetTypeId());
                return (possibleValueLhs != lhs.m_possibleValues.end())
                    && (possibleValueLhs->second == rhs.m_resolvedValue);
            }
            else
            {
                if (lhs.m_possibleValues.size() != rhs.m_possibleValues.size())
                {
                    return false;
                }

                // Loop through the entire possible value list.
                for (auto possibleValueLhs : lhs.m_possibleValues)
                {
                    auto possibleValueRhs = rhs.m_possibleValues.find(possibleValueLhs.first);
                    if (possibleValueRhs == rhs.m_possibleValues.end()
                        || (possibleValueLhs.second != possibleValueRhs->second))
                    {
                        return false;
                    }
                }

                return true;
            }
        }
    } // namespace RPI
} // namespace AZ
