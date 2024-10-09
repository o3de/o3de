/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/BehaviorContextUtilities.h>
#include <AzCore/Serialization/Locale.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Debugger/ValidationEvents/DataValidation/ScopedDataConnectionEvent.h>
#include <ScriptCanvas/Debugger/ValidationEvents/GraphTranslationValidation/GraphTranslationValidations.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ParsingValidation/ParsingValidations.h>
#include <ScriptCanvas/Execution/ExecutionState.h>
#include <ScriptCanvas/Grammar/AbstractCodeModel.h>
#include <ScriptCanvas/Grammar/ParsingUtilities.h>
#include <ScriptCanvas/Grammar/Primitives.h>
#include <ScriptCanvas/Translation/GraphToLua.h>

#include <ScriptCanvas/Translation/GraphToLuaUtility.h>


namespace ScriptCanvas
{
    namespace Translation
    {

        void CheckConversionStringPost(Writer& writer, Grammar::VariableConstPtr source, const Grammar::ConversionByIndex& conversions, size_t index)
        {
            auto iter = conversions.find(index);
            if (iter == conversions.end())
            {
                return;
            }

            switch (iter->second.GetType())
            {
            case Data::eType::Boolean:
                writer.Write(" ~= 0");
                break;

            case Data::eType::Number:
                writer.Write(" and 1 or 0");
                break;

            case Data::eType::AABB:
            case Data::eType::BehaviorContextObject:
            case Data::eType::Color:
            case Data::eType::CRC:
            case Data::eType::AssetId:
            case Data::eType::EntityID:
            case Data::eType::NamedEntityID:
            case Data::eType::Matrix3x3:
            case Data::eType::Matrix4x4:
            case Data::eType::OBB:
            case Data::eType::Plane:
            case Data::eType::Quaternion:
            case Data::eType::String:
            case Data::eType::Transform:
            case Data::eType::Vector2:
            case Data::eType::Vector3:
            case Data::eType::Vector4:
                writer.Write(")");
                break;

            case Data::eType::Invalid:
            default:
                AZ_Error("ScriptCanvas", false, "Invalid type found in GraphToLua::ToValueString()!");
            }
        }

        void CheckConversionStringPre(Writer& writer, Grammar::VariableConstPtr source, const Grammar::ConversionByIndex& conversions, size_t index)
        {
            auto iter = conversions.find(index);
            if (iter == conversions.end())
            {
                return;
            }

            switch (iter->second.GetType())
            {
            case Data::eType::Boolean:
                AZ_Error("ScriptCanvas", source->m_datum.GetType().GetType() == Data::eType::Number, "invalid conversion to bool");
                break;

            case Data::eType::Number:
                AZ_Error("ScriptCanvas", source->m_datum.GetType().GetType() == Data::eType::Boolean, "invalid conversion to number");
                break;

            case Data::eType::AABB:
            case Data::eType::BehaviorContextObject:
            case Data::eType::Color:
            case Data::eType::CRC:
            case Data::eType::AssetId:
            case Data::eType::EntityID:
            case Data::eType::NamedEntityID:
            case Data::eType::Matrix3x3:
            case Data::eType::Matrix4x4:
            case Data::eType::OBB:
            case Data::eType::Plane:
            case Data::eType::Quaternion:
            case Data::eType::Transform:
            case Data::eType::Vector2:
            case Data::eType::Vector3:
            case Data::eType::Vector4:
                writer.Write("%s(", Data::GetName(iter->second).data());
                break;
                
            case Data::eType::String:
                writer.Write("tostring(");
                break;

            case Data::eType::Invalid:
            default:
                AZ_Error("ScriptCanvas", false, "Invalid type found in GraphToLua::ToValueString()!");
            }
        }

        bool IsReferenceInLuaAndValueInScriptCanvas(const Data::Type& type)
        {
            switch (type.GetType())
            {
            case Data::eType::Boolean:
            case Data::eType::Number:
            case Data::eType::String:
            case Data::eType::AssetId:
            case Data::eType::EntityID:
            case Data::eType::NamedEntityID:
            case Data::eType::BehaviorContextObject:
                return false;

            case Data::eType::AABB:
            case Data::eType::Color:
            case Data::eType::CRC:
            case Data::eType::Matrix3x3:
            case Data::eType::Matrix4x4:
            case Data::eType::OBB:
            case Data::eType::Plane:
            case Data::eType::Quaternion:
            case Data::eType::Transform:
            case Data::eType::Vector2:
            case Data::eType::Vector3:
            case Data::eType::Vector4:
                return true;

            case Data::eType::Invalid:
            default:
                AZ_Error("ScriptCanvas", false, "Invalid type found in GraphToLua::ToValueString()!");
                // \todo check for behavior context support
                return "";
            }
        }

        AZStd::string ToValueString(const Datum& datum, const Configuration& config)
        {
            AZ::Locale::ScopedSerializationLocale scopedLocale;

            switch (datum.GetType().GetType())
            {
            case Data::eType::AABB:
            {
                const auto value = datum.GetAs<Data::AABBType>();
                const auto valueMin = value->GetMin();
                const auto valueMax = value->GetMax();
                return AZStd::string::format("Aabb.CreateFromMinMaxValues(%f, %f, %f, %f, %f, %f)"
                    , static_cast<float>(valueMin.GetX())
                    , static_cast<float>(valueMin.GetY())
                    , static_cast<float>(valueMin.GetZ())
                    , static_cast<float>(valueMax.GetX())
                    , static_cast<float>(valueMax.GetY())
                    , static_cast<float>(valueMax.GetZ()));
            }

            case Data::eType::BehaviorContextObject:
            {
                if (datum.GetType().GetAZType() != azrtti_typeid<ExecutionState>())
                {
                    return "nil";
                }
                else
                {
                    return config.m_executionStateName;
                }
            }

            case Data::eType::Boolean:
            {
                return *datum.GetAs<Data::BooleanType>() ? "true" : "false";
            }

            case Data::eType::Color:
            {
                if (datum.IsDefaultValue())
                {
                    return "Color()";
                }
                else
                {
                    auto value = datum.GetAs<Data::ColorType>();
                    return AZStd::string::format("Color(%f, %f, %f, %f)", static_cast<float>(value->GetR()), static_cast<float>(value->GetG()), static_cast<float>(value->GetB()), static_cast<float>(value->GetA()));
                }
            }

            case Data::eType::CRC:
                return AZStd::string::format("Crc32(%u)", static_cast<AZ::u32>(*datum.GetAs<Data::CRCType>()));

            case Data::eType::Number:
            {
                return AZStd::to_string(*datum.GetAs<Data::NumberType>());
            }

            case Data::eType::Matrix3x3:
            {
                if (datum.IsDefaultValue())
                {
                    return "Matrix3x3.CreateIdentity()";
                }
                else
                {
                    const Data::Matrix3x3Type m3x3 = *datum.GetAs<Data::Matrix3x3Type>();
                    Data::Vector3Type r0, r1, r2;
                    m3x3.GetRows(&r0, &r1, &r2);
                    return AZStd::string::format("Matrix3x3.ConstructFromValuesNumeric(%f, %f, %f, %f, %f, %f, %f, %f, %f)"
                        , static_cast<float>(r0.GetX()), static_cast<float>(r0.GetY()), static_cast<float>(r0.GetZ())
                        , static_cast<float>(r1.GetX()), static_cast<float>(r1.GetY()), static_cast<float>(r1.GetZ())
                        , static_cast<float>(r2.GetX()), static_cast<float>(r2.GetY()), static_cast<float>(r2.GetZ()));
                }
            }

            case Data::eType::Matrix4x4:
                if (datum.IsDefaultValue())
                {
                    return "Matrix4x4.CreateIdentity()";
                }
                else
                {
                    const Data::Matrix4x4Type m = *datum.GetAs<Data::Matrix4x4Type>();
                    Data::Vector4Type r0, r1, r2, r3;
                    m.GetRows(&r0, &r1, &r2, &r3);
                    return AZStd::string::format("Matrix4x4.ConstructFromValuesNumeric(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)"
                        , static_cast<float>(r0.GetX()), static_cast<float>(r0.GetY()), static_cast<float>(r0.GetZ()), static_cast<float>(r0.GetW())
                        , static_cast<float>(r1.GetX()), static_cast<float>(r1.GetY()), static_cast<float>(r1.GetZ()), static_cast<float>(r1.GetW())
                        , static_cast<float>(r2.GetX()), static_cast<float>(r2.GetY()), static_cast<float>(r2.GetZ()), static_cast<float>(r2.GetW())
                        , static_cast<float>(r3.GetX()), static_cast<float>(r3.GetY()), static_cast<float>(r3.GetZ()), static_cast<float>(r3.GetW())
                    );
                }

            case Data::eType::OBB:
                if (datum.IsDefaultValue())
                {
                    return "Obb()";
                }
                else
                {
                    const Data::OBBType obb = *datum.GetAs<Data::OBBType>();
                    const AZ::Vector3 pos = obb.GetPosition();
                    const AZ::Vector3 x = obb.GetAxisX();
                    const float x2 = obb.GetHalfLengthX();
                    const AZ::Vector3 y = obb.GetAxisY();
                    const float y2 = obb.GetHalfLengthY();
                    const AZ::Vector3 z = obb.GetAxisZ();
                    const float z2 = obb.GetHalfLengthZ();

                    return AZStd::string::format("Obb.ConstructObbFromValues(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)"
                        , static_cast<float>(pos.GetX()), static_cast<float>(pos.GetY()), static_cast<float>(pos.GetZ())
                        , static_cast<float>(x.GetX()), static_cast<float>(x.GetY()), static_cast<float>(x.GetZ()), x2
                        , static_cast<float>(y.GetX()), static_cast<float>(y.GetY()), static_cast<float>(y.GetZ()), y2
                        , static_cast<float>(z.GetX()), static_cast<float>(z.GetY()), static_cast<float>(z.GetZ()), z2);
                }

            case Data::eType::Plane:
                if (datum.IsDefaultValue())
                {
                    return "Plane()";
                }
                else
                {
                    const Data::PlaneType plane = *datum.GetAs<Data::PlaneType>();
                    const AZ::Vector4 coeffs = plane.GetPlaneEquationCoefficients();
                    return AZStd::string::format("Plane.CreateFromCoefficients(%f, %f, %f, %f)"
                        , static_cast<float>(coeffs.GetX())
                        , static_cast<float>(coeffs.GetY())
                        , static_cast<float>(coeffs.GetZ())
                        , static_cast<float>(coeffs.GetW()));
                }

            case Data::eType::Quaternion:
                if (datum.IsDefaultValue())
                {
                    return "Quaternion(0, 0, 0, 1)";
                }
                else
                {
                    const Data::QuaternionType quat = *datum.GetAs<Data::QuaternionType>();
                    return AZStd::string::format("Quaternion(%f, %f, %f, %f)"
                        , static_cast<float>(quat.GetX())
                        , static_cast<float>(quat.GetY())
                        , static_cast<float>(quat.GetZ())
                        , static_cast<float>(quat.GetW()));
                }

            case Data::eType::Transform:
                if (datum.IsDefaultValue())
                {
                    return "Transform.CreateIdentity()";
                }
                else
                {
                    const Data::TransformType t = *datum.GetAs<Data::TransformType>();
                    Data::Vector4Type r0, r1, r2, r3;
                    Data::Matrix4x4Type m = Data::Matrix4x4Type::CreateFromTransform(t);
                    m.GetRows(&r0, &r1, &r2, &r3);
                    
                    return AZStd::string::format("Transform.ConstructFromValuesNumeric(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)"
                        , static_cast<float>(r0.GetX()), static_cast<float>(r0.GetY()), static_cast<float>(r0.GetZ()), static_cast<float>(r0.GetW())
                        , static_cast<float>(r1.GetX()), static_cast<float>(r1.GetY()), static_cast<float>(r1.GetZ()), static_cast<float>(r1.GetW())
                        , static_cast<float>(r2.GetX()), static_cast<float>(r2.GetY()), static_cast<float>(r2.GetZ()), static_cast<float>(r2.GetW())
                        , static_cast<float>(r3.GetX()), static_cast<float>(r3.GetY()), static_cast<float>(r3.GetZ()), static_cast<float>(r3.GetW())
                    );
                }

            case Data::eType::Vector2:
            {
                if (datum.IsDefaultValue())
                {
                    return "Vector2()";
                }
                else
                {
                    auto value = datum.GetAs<Data::Vector2Type>();
                    return AZStd::string::format("Vector2(%f, %f)", static_cast<float>(value->GetX()), static_cast<float>(value->GetY()));
                }
            }

            case Data::eType::Vector3:
            {
                if (datum.IsDefaultValue())
                {
                    return "Vector3()";
                }
                else
                {
                    auto value = datum.GetAs<Data::Vector3Type>();
                    return AZStd::string::format("Vector3(%f, %f, %f)", static_cast<float>(value->GetX()), static_cast<float>(value->GetY()), static_cast<float>(value->GetZ()));
                }
            }

            case Data::eType::Vector4:
            {
                if (datum.IsDefaultValue())
                {
                    return "Vector4()";
                }
                else
                {
                    auto value = datum.GetAs<Data::Vector4Type>();
                    return AZStd::string::format("Vector4(%f, %f, %f, %f)", static_cast<float>(value->GetX()), static_cast<float>(value->GetY()), static_cast<float>(value->GetZ()), static_cast<float>(value->GetW()));
                }
            }

            case Data::eType::String:
            {
                const AZStd::string& formattedString = *datum.GetAs<Data::StringType>();
                return MakeRuntimeSafeStringLiteral(formattedString);
            }

            case Data::eType::AssetId:
            {
                const AZ::Data::AssetId& value = *datum.GetAs<Data::AssetIdType>();
                if (value.IsValid())
                {
                    const AZStd::string valueString = MakeRuntimeSafeStringLiteral(value.ToString<AZStd::string>());
                    return AZStd::string::format("AssetId.CreateString(%s)", valueString.c_str());
                }
                return "AssetId()";
            }

            case Data::eType::EntityID:
                return EntityIdValueToString(*datum.GetAs<Data::EntityIDType>(), config);

            case Data::eType::NamedEntityID:
                return EntityIdValueToString(*datum.GetAs<Data::NamedEntityIDType>(), config);

            case Data::eType::Invalid:
            default:
                AZ_Error("ScriptCanvas", false, "Invalid type found in GraphToLua::ToValueString()!");
                return "";
            }
        }

        AZStd::string EqualSigns(size_t numEqualSignsRequired)
        {
            AZStd::string equalSigns = "";
            while (numEqualSignsRequired--)
            {
                equalSigns += "=";
            }

            return equalSigns;
        }

        AZStd::string MakeLongBracketString(const AZStd::string& formattedString)
        {
            size_t numEqualSignsRequired = 0;

            for (;;)
            {
                auto candidate = AZStd::string::format("]%s", EqualSigns(numEqualSignsRequired).c_str());

                if (formattedString.find(candidate) == AZStd::string::npos)
                {
                    break;
                }

                ++numEqualSignsRequired;
            }

            return EqualSigns(numEqualSignsRequired);
        }

        AZStd::string MakeRuntimeSafeStringLiteral(const AZStd::string& formattedString)
        {
            const AZStd::string bracketString = MakeLongBracketString(formattedString);
            return AZStd::string::format("[%s[%s]%s]", bracketString.c_str(), formattedString.c_str(), bracketString.c_str());
        }

    } 

} 
