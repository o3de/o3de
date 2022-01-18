/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>

namespace AZ
{
    namespace RPI
    {
        const char* ToString(MaterialPropertyOutputType materialPropertyOutputType)
        {
            switch (materialPropertyOutputType)
            {
            case MaterialPropertyOutputType::ShaderInput:  return "ShaderInput";
            case MaterialPropertyOutputType::ShaderOption: return "ShaderOption";
            default:
                AZ_Assert(false, "Unhandled type");
                return "<Unknown>";
            }
        }

        const char* ToString(MaterialPropertyDataType materialPropertyDataType)
        {
            switch (materialPropertyDataType)
            {
                case MaterialPropertyDataType::Bool:    return "Bool";
                case MaterialPropertyDataType::Int:     return "Int";
                case MaterialPropertyDataType::UInt:    return "UInt";
                case MaterialPropertyDataType::Float:   return "Float";
                case MaterialPropertyDataType::Vector2: return "Vector2";
                case MaterialPropertyDataType::Vector3: return "Vector3";
                case MaterialPropertyDataType::Vector4: return "Vector4";
                case MaterialPropertyDataType::Color:   return "Color";
                case MaterialPropertyDataType::Image:   return "Image";
                case MaterialPropertyDataType::Enum:    return "Enum";
                case MaterialPropertyDataType::Invalid: return "Invalid";
                default: 
                    AZ_Assert(false, "Unhandled type");
                    return "<Unknown>";
            }
        }

        AZStd::string GetMaterialPropertyDataTypeString(AZ::TypeId typeId)
        {
            if (typeId == azrtti_typeid<bool>())
            {
                return ToString(MaterialPropertyDataType::Bool);
            }
            else if (typeId == azrtti_typeid<int32_t>())
            {
                return ToString(MaterialPropertyDataType::Int);
            }
            else if (typeId == azrtti_typeid<uint32_t>())
            {
                return ToString(MaterialPropertyDataType::UInt);
            }
            else if (typeId == azrtti_typeid<float>())
            {
                return ToString(MaterialPropertyDataType::Float);
            }
            else if (typeId == azrtti_typeid<Vector2>())
            {
                return ToString(MaterialPropertyDataType::Vector2);
            }
            else if (typeId == azrtti_typeid<Vector3>())
            {
                return ToString(MaterialPropertyDataType::Vector3);
            }
            else if (typeId == azrtti_typeid<Vector4>())
            {
                return ToString(MaterialPropertyDataType::Vector4);
            }
            else if (typeId == azrtti_typeid<Color>())
            {
                return ToString(MaterialPropertyDataType::Color);
            }
            else if (typeId == azrtti_typeid<Color>())
            {
                return ToString(MaterialPropertyDataType::Color);
            }
            else if (typeId == azrtti_typeid<Data::Instance<Image>>())
            {
                return ToString(MaterialPropertyDataType::Image);
            }
            else
            {
                return AZStd::string::format("<Unkonwn type %s>", typeId.ToString<AZStd::string>().c_str());
            }
        }

        void MaterialPropertyOutputId::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialPropertyOutputId>()
                    ->Version(1)
                    ->Field("m_type", &MaterialPropertyOutputId::m_type)
                    ->Field("m_containerIndex", &MaterialPropertyOutputId::m_containerIndex)
                    ->Field("m_itemIndex", &MaterialPropertyOutputId::m_itemIndex)
                    ;
            }
        }

        void MaterialPropertyDescriptor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Enum<MaterialPropertyOutputType>()
                    ->Value(ToString(MaterialPropertyOutputType::ShaderInput), MaterialPropertyOutputType::ShaderInput)
                    ->Value(ToString(MaterialPropertyOutputType::ShaderOption), MaterialPropertyOutputType::ShaderOption)
                    ;

                serializeContext->Enum<MaterialPropertyDataType>()
                    ->Value(ToString(MaterialPropertyDataType::Invalid), MaterialPropertyDataType::Invalid)
                    ->Value(ToString(MaterialPropertyDataType::Bool), MaterialPropertyDataType::Bool)
                    ->Value(ToString(MaterialPropertyDataType::Int), MaterialPropertyDataType::Int)
                    ->Value(ToString(MaterialPropertyDataType::UInt), MaterialPropertyDataType::UInt)
                    ->Value(ToString(MaterialPropertyDataType::Float), MaterialPropertyDataType::Float)
                    ->Value(ToString(MaterialPropertyDataType::Vector2), MaterialPropertyDataType::Vector2)
                    ->Value(ToString(MaterialPropertyDataType::Vector3), MaterialPropertyDataType::Vector3)
                    ->Value(ToString(MaterialPropertyDataType::Vector4), MaterialPropertyDataType::Vector4)
                    ->Value(ToString(MaterialPropertyDataType::Color), MaterialPropertyDataType::Color)
                    ->Value(ToString(MaterialPropertyDataType::Image), MaterialPropertyDataType::Image)
                    ->Value(ToString(MaterialPropertyDataType::Enum), MaterialPropertyDataType::Enum)
                    ;

                serializeContext->Class<MaterialPropertyDescriptor>()
                    ->Version(2)
                    ->Field("Name", &MaterialPropertyDescriptor::m_nameId)
                    ->Field("DataType", &MaterialPropertyDescriptor::m_dataType)
                    ->Field("OutputConnections", &MaterialPropertyDescriptor::m_outputConnections)
                    ->Field("EnumNames", &MaterialPropertyDescriptor::m_enumNames)
                    ;
            }

            MaterialPropertyIndex::Reflect(context);
        }
        
        MaterialPropertyDataType MaterialPropertyDescriptor::GetDataType() const
        {
            return m_dataType;
        }

        const Name& MaterialPropertyDescriptor::GetName() const
        {
            return m_nameId;
        }

        const MaterialPropertyDescriptor::OutputList& MaterialPropertyDescriptor::GetOutputConnections() const
        {
            return m_outputConnections;
        }

        AZ::TypeId MaterialPropertyDescriptor::GetStorageDataTypeId() const
        {
            switch (m_dataType)
            {
            case MaterialPropertyDataType::Bool:
                return azrtti_typeid<bool>();
            case MaterialPropertyDataType::Int:
                return azrtti_typeid<int32_t>();
            case MaterialPropertyDataType::UInt:
                return azrtti_typeid<uint32_t>();
            case MaterialPropertyDataType::Float:
                return azrtti_typeid<float>();
            case MaterialPropertyDataType::Vector2:
                return azrtti_typeid<Vector2>();
            case MaterialPropertyDataType::Vector3:
                return azrtti_typeid<Vector3>();
            case MaterialPropertyDataType::Vector4:
                return azrtti_typeid<Vector4>();
            case MaterialPropertyDataType::Color:
                return azrtti_typeid<Color>();
            case MaterialPropertyDataType::Enum:
            case MaterialPropertyDataType::Image:
                return azrtti_typeid<AZStd::string>();
            default:
                AZ_Error("MaterialPropertyValueSourceData", false, "Unhandle material property type %s.", ToString(m_dataType));
                return Uuid::CreateNull();
            }
        }

        uint32_t MaterialPropertyDescriptor::GetEnumValue(const AZ::Name& enumName) const
        {
            const uint32_t total = aznumeric_cast<uint32_t>(m_enumNames.size());
            for (uint32_t i = 0; i < total; ++i)
            {
                if (m_enumNames[i] == enumName)
                {
                    return i;
                }
            }

            return InvalidEnumValue;
        }
        
        const AZ::Name& MaterialPropertyDescriptor::GetEnumName(uint32_t enumValue) const
        {
            if (enumValue < m_enumNames.size())
            {
                return m_enumNames.at(enumValue);
            }
            static AZ::Name EmptyName = AZ::Name();
            return EmptyName;
        }

    } // namespace RPI
} // namespace AZ
