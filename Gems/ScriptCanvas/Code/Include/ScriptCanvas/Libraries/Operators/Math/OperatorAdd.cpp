/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OperatorAdd.h"
#include <ScriptCanvas/Core/Contracts/MathOperatorContract.h>
#include <AzCore/Math/MathUtils.h>
#include <ScriptCanvas/Data/NumericData.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            template<typename Type>
            struct OperatorAddImpl
            {
                Type operator()(const Type& a, const Datum& b)
                {
                    const Type* dataB = b.GetAs<Type>();
                    return a + (*dataB);
                }
            };

            template<>
            struct OperatorAddImpl<Data::AABBType>
            {
                Data::AABBType operator()(const Data::AABBType& lhs, const Datum& rhs)
                {
                    Data::AABBType retVal = lhs;
                    retVal.AddAabb((*rhs.GetAs<AZ::Aabb>()));

                    return retVal;
                }
            };

            template <>
            struct OperatorAddImpl<Data::ColorType>
            {
                Data::ColorType operator()(const Data::ColorType& lhs, const Datum& rhs)
                {
                    const AZ::Color* dataRhs = rhs.GetAs<AZ::Color>();

                    float a = AZ::GetClamp<float>(lhs.GetA(), 0.f, 1.f) + AZ::GetClamp<float>(dataRhs->GetA(), 0.f, 1.f);
                    float r = AZ::GetClamp<float>(lhs.GetR(), 0.f, 1.f) + AZ::GetClamp<float>(dataRhs->GetR(), 0.f, 1.f);
                    float g = AZ::GetClamp<float>(lhs.GetG(), 0.f, 1.f) + AZ::GetClamp<float>(dataRhs->GetG(), 0.f, 1.f);
                    float b = AZ::GetClamp<float>(lhs.GetB(), 0.f, 1.f) + AZ::GetClamp<float>(dataRhs->GetB(), 0.f, 1.f);

                    return AZ::Color(r, g, b, a);
                }
            };

            template <>
            struct OperatorAddImpl<Data::Matrix3x3Type>
            {
                Data::Matrix3x3Type operator()(const Data::Matrix3x3Type& lhs, const Datum& rhs)
                {
                    const AZ::Matrix3x3* dataRhs = rhs.GetAs<AZ::Matrix3x3>();
                    return AZ::Matrix3x3::CreateFromColumns(dataRhs->GetColumn(0) + lhs.GetColumn(0), dataRhs->GetColumn(1) + lhs.GetColumn(1), dataRhs->GetColumn(2) + lhs.GetColumn(2));
                }
            };

            template <>
            struct OperatorAddImpl<Data::Matrix4x4Type>
            {
                Data::Matrix4x4Type operator()(const Data::Matrix4x4Type& lhs, const Datum& rhs)
                {
                    const AZ::Matrix4x4* dataRhs = rhs.GetAs<AZ::Matrix4x4>();
                    return AZ::Matrix4x4::CreateFromColumns(dataRhs->GetColumn(0) + lhs.GetColumn(0), dataRhs->GetColumn(1) + lhs.GetColumn(1), dataRhs->GetColumn(2) + lhs.GetColumn(2), dataRhs->GetColumn(3) + lhs.GetColumn(3));
                }
            };

            void OperatorAdd::Operator(Data::eType type, const ArithmeticOperands& operands, Datum& result)
            {
                switch (type)
                {
                case Data::eType::Number:
                    OperatorEvaluator::Evaluate<Data::NumberType>(OperatorAddImpl<Data::NumberType>(), operands, result);
                    break;
                case Data::eType::Color:
                    OperatorEvaluator::Evaluate<Data::ColorType>(OperatorAddImpl<Data::ColorType>(), operands, result);
                    break;
                case Data::eType::Vector2:
                    OperatorEvaluator::Evaluate<Data::Vector2Type>(OperatorAddImpl<Data::Vector2Type>(), operands, result);
                    break;
                case Data::eType::Vector3:
                    OperatorEvaluator::Evaluate<Data::Vector3Type>(OperatorAddImpl<Data::Vector3Type>(), operands, result);
                    break;
                case Data::eType::Vector4:
                    OperatorEvaluator::Evaluate<Data::Vector4Type>(OperatorAddImpl<Data::Vector4Type>(), operands, result);
                    break;
                case Data::eType::VectorN:
                    OperatorEvaluator::Evaluate<Data::VectorNType>(OperatorAddImpl<Data::VectorNType>(), operands, result);
                    break;
                case Data::eType::String:
                    OperatorEvaluator::Evaluate<Data::StringType>(OperatorAddImpl<Data::StringType>(), operands, result);
                    break;
                case Data::eType::Quaternion:
                    OperatorEvaluator::Evaluate<Data::QuaternionType>(OperatorAddImpl<Data::QuaternionType>(), operands, result);
                    break;
                case Data::eType::AABB:
                    OperatorEvaluator::Evaluate<Data::AABBType>(OperatorAddImpl<Data::AABBType>(), operands, result);
                    break;
                case Data::eType::Matrix3x3:
                    OperatorEvaluator::Evaluate<Data::Matrix3x3Type>(OperatorAddImpl<Data::Matrix3x3Type>(), operands, result);
                    break;
                case Data::eType::Matrix4x4:
                    OperatorEvaluator::Evaluate<Data::Matrix4x4Type>(OperatorAddImpl<Data::Matrix4x4Type>(), operands, result);
                    break;
                case Data::eType::MatrixMxN:
                    OperatorEvaluator::Evaluate<Data::MatrixMxNType>(OperatorAddImpl<Data::MatrixMxNType>(), operands, result);
                    break;
                default:
                    AZ_Assert(false, "Addition operator not defined for type: %s", Data::ToAZType(type).ToString<AZStd::string>().c_str());
                    break;
                }
            }

            AZStd::unordered_set< Data::Type > OperatorAdd::GetSupportedNativeDataTypes() const
            {
                return{
                    Data::Type::Number(),
                    Data::Type::Vector2(),
                    Data::Type::Vector3(),
                    Data::Type::Vector4(),
                    Data::Type::VectorN(),
                    Data::Type::Color(),
                    Data::Type::Quaternion(),
                    Data::Type::AABB(),
                    Data::Type::Matrix3x3(),
                    Data::Type::Matrix4x4(),
                    Data::Type::MatrixMxN()
                };
            }

            bool OperatorAdd::IsValidArithmeticSlot(const SlotId& slotId) const
            {
                const Datum* datum = FindDatum(slotId);

                if (datum)
                {
                    switch (datum->GetType().GetType())
                    {
                    case Data::eType::Number:
                        return !AZ::IsClose((*datum->GetAs<Data::NumberType>()), Data::NumberType(0.0), ScriptCanvas::Data::ToleranceEpsilon());
                    case Data::eType::Quaternion:
                        return !datum->GetAs<Data::QuaternionType>()->IsIdentity();
                    case Data::eType::String:
                        return !datum->GetAs<Data::StringType>()->empty();
                    case Data::eType::Matrix3x3:
                        return !datum->GetAs<Data::Matrix3x3Type>()->IsClose(Data::Matrix3x3Type::CreateIdentity());
                    case Data::eType::Matrix4x4:
                        return !datum->GetAs<Data::Matrix4x4Type>()->IsClose(Data::Matrix4x4Type::CreateIdentity());
                    case Data::eType::MatrixMxN:
                        return true;
                    default:
                        break;
                    }
                }

                return (datum != nullptr);
            }
        }
    }
}
