/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OperatorMul.h"
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
            struct OperatorMulImpl
            {
                Type operator()(const Type& a, const Datum& b)
                {
                    const Type* dataB = b.GetAs<Type>();
                    return a * (*dataB);
                }
            };

            template <>
            struct OperatorMulImpl<Data::ColorType>
            {
                Data::ColorType operator()(const Data::ColorType& lhs, const Datum& rhs)
                {
                    const AZ::Color* dataB = rhs.GetAs<AZ::Color>();

                    // TODO: clamping should happen at the AZ::Color level, not here - but it does not.
                    float a = AZ::GetClamp<float>(lhs.GetA(), 0.f, 1.f) * AZ::GetClamp<float>(dataB->GetA(), 0.f, 1.f);
                    float r = AZ::GetClamp<float>(lhs.GetR(), 0.f, 1.f) * AZ::GetClamp<float>(dataB->GetR(), 0.f, 1.f);
                    float g = AZ::GetClamp<float>(lhs.GetG(), 0.f, 1.f) * AZ::GetClamp<float>(dataB->GetG(), 0.f, 1.f);
                    float b = AZ::GetClamp<float>(lhs.GetB(), 0.f, 1.f) * AZ::GetClamp<float>(dataB->GetB(), 0.f, 1.f);

                    return AZ::Color(r, g, b, a);
                }
            };

            AZStd::unordered_set< Data::Type > OperatorMul::GetSupportedNativeDataTypes() const
            {
                return {
                    Data::Type::Number(),
                    Data::Type::Quaternion(),
                    Data::Type::Transform(),
                    Data::Type::Matrix3x3(),
                    Data::Type::Matrix4x4(),
                    Data::Type::MatrixMxN(),
                    Data::Type::Vector2(),
                    Data::Type::Vector3(),
                    Data::Type::Vector4(),
                    Data::Type::VectorN(),
                    Data::Type::Color()
                };
            }

            void OperatorMul::Operator(Data::eType type, const ArithmeticOperands& operands, Datum& result)
            {
                AZ_PROFILE_FUNCTION(ScriptCanvas);

                switch (type)
                {
                case Data::eType::Number:
                    OperatorEvaluator::Evaluate<Data::NumberType>(OperatorMulImpl<Data::NumberType>(), operands, result);
                    break;
                case Data::eType::Quaternion:
                    OperatorEvaluator::Evaluate<Data::QuaternionType>(OperatorMulImpl<Data::QuaternionType>(), operands, result);
                    break;
                case Data::eType::Transform:
                    OperatorEvaluator::Evaluate<Data::TransformType>(OperatorMulImpl<Data::TransformType>(), operands, result);
                    break;
                case Data::eType::Matrix3x3:
                    OperatorEvaluator::Evaluate<Data::Matrix3x3Type>(OperatorMulImpl<Data::Matrix3x3Type>(), operands, result);
                    break;
                case Data::eType::Matrix4x4:
                    OperatorEvaluator::Evaluate<Data::Matrix4x4Type>(OperatorMulImpl<Data::Matrix4x4Type>(), operands, result);
                    break;
                case Data::eType::MatrixMxN:
                    OperatorEvaluator::Evaluate<Data::MatrixMxNType>(OperatorMulImpl<Data::MatrixMxNType>(), operands, result);
                    break;
                case Data::eType::Vector2:
                    OperatorEvaluator::Evaluate<Data::Vector2Type>(OperatorMulImpl<Data::Vector2Type>(), operands, result);
                    break;
                case Data::eType::Vector3:
                    OperatorEvaluator::Evaluate<Data::Vector3Type>(OperatorMulImpl<Data::Vector3Type>(), operands, result);
                    break;
                case Data::eType::Vector4:
                    OperatorEvaluator::Evaluate<Data::Vector4Type>(OperatorMulImpl<Data::Vector4Type>(), operands, result);
                    break;
                case Data::eType::VectorN:
                    OperatorEvaluator::Evaluate<Data::VectorNType>(OperatorMulImpl<Data::VectorNType>(), operands, result);
                    break;
                case Data::eType::Color:
                    OperatorEvaluator::Evaluate<Data::ColorType>(OperatorMulImpl<Data::ColorType>(), operands, result);
                    break;
                default:
                    AZ_Assert(false, "Multiplication operator not defined for type: %s", Data::ToAZType(type).ToString<AZStd::string>().c_str());
                    break;
                }
            }

            bool OperatorMul::IsValidArithmeticSlot(const SlotId& slotId) const
            {
                const Datum* datum = FindDatum(slotId);

                if (datum)
                {
                    switch (datum->GetType().GetType())
                    {
                    case Data::eType::Number:
                        return !AZ::IsClose((*datum->GetAs<Data::NumberType>()), Data::NumberType(1.0), ScriptCanvas::Data::ToleranceEpsilon());
                    case Data::eType::Quaternion:
                        return !datum->GetAs<Data::QuaternionType>()->IsIdentity();
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
