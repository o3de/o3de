/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OperatorSub.h"
#include <ScriptCanvas/Core/Contracts/MathOperatorContract.h>
#include <AzCore/Math/MathUtils.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            template<typename Type>
            struct OperatorSubImpl
            {
                Type operator()(const Type& a, const Datum& b)
                {
                    const Type* dataB = b.GetAs<Type>();
                    return a - (*dataB);
                }
            };

            template <>
            struct OperatorSubImpl<Data::ColorType>
            {
                Data::ColorType operator()(const Data::ColorType& lhs, const Datum& rhs)
                {
                    const AZ::Color* dataB = rhs.GetAs<AZ::Color>();

                    // TODO: clamping should happen at the AZ::Color level, not here - but it does not.
                    float a = AZ::GetClamp<float>(lhs.GetA(), 0.f, 1.f) - AZ::GetClamp<float>(dataB->GetA(), 0.f, 1.f);
                    float r = AZ::GetClamp<float>(lhs.GetR(), 0.f, 1.f) - AZ::GetClamp<float>(dataB->GetR(), 0.f, 1.f);
                    float g = AZ::GetClamp<float>(lhs.GetG(), 0.f, 1.f) - AZ::GetClamp<float>(dataB->GetG(), 0.f, 1.f);
                    float b = AZ::GetClamp<float>(lhs.GetB(), 0.f, 1.f) - AZ::GetClamp<float>(dataB->GetB(), 0.f, 1.f);

                    return AZ::Color(r, g, b, a);
                }
            };

            template <>
            struct OperatorSubImpl<Data::Matrix3x3Type>
            {
                Data::Matrix3x3Type operator()(const Data::Matrix3x3Type& lhs, const Datum& rhs)
                {
                    const AZ::Matrix3x3* dataRhs = rhs.GetAs<AZ::Matrix3x3>();

                    return AZ::Matrix3x3::CreateFromColumns(lhs.GetColumn(0) - dataRhs->GetColumn(0), lhs.GetColumn(1) - dataRhs->GetColumn(1), lhs.GetColumn(2) - dataRhs->GetColumn(2));
                }
            };

            template <>
            struct OperatorSubImpl<Data::Matrix4x4Type>
            {
                Data::Matrix4x4Type operator()(const Data::Matrix4x4Type& lhs, const Datum& rhs)
                {
                    const AZ::Matrix4x4* dataRhs = rhs.GetAs<AZ::Matrix4x4>();

                    return AZ::Matrix4x4::CreateFromColumns(lhs.GetColumn(0) - dataRhs->GetColumn(0), lhs.GetColumn(1) - dataRhs->GetColumn(1), lhs.GetColumn(2) - dataRhs->GetColumn(2), lhs.GetColumn(3) - dataRhs->GetColumn(3));
                }
            };

            AZStd::unordered_set< Data::Type > OperatorSub::GetSupportedNativeDataTypes() const
            {
                return{
                    Data::Type::Number(),
                    Data::Type::Vector2(),
                    Data::Type::Vector3(),
                    Data::Type::Vector4(),
                    Data::Type::VectorN(),
                    Data::Type::Color(),
                    Data::Type::Matrix3x3(),
                    Data::Type::Matrix4x4(),
                    Data::Type::MatrixMxN()
                };
            }


            void OperatorSub::Operator(Data::eType type, const ArithmeticOperands& operands, Datum& result)
            {
                switch (type)
                {
                case Data::eType::Number:
                    OperatorEvaluator::Evaluate<Data::NumberType>(OperatorSubImpl<Data::NumberType>(), operands, result);
                    break;
                case Data::eType::Color:
                    OperatorEvaluator::Evaluate<Data::ColorType>(OperatorSubImpl<Data::ColorType>(), operands, result);
                    break;
                case Data::eType::Vector2:
                    OperatorEvaluator::Evaluate<Data::Vector2Type>(OperatorSubImpl<Data::Vector2Type>(), operands, result);
                    break;
                case Data::eType::Vector3:
                    OperatorEvaluator::Evaluate<Data::Vector3Type>(OperatorSubImpl<Data::Vector3Type>(), operands, result);
                    break;
                case Data::eType::Vector4:
                    OperatorEvaluator::Evaluate<Data::Vector4Type>(OperatorSubImpl<Data::Vector4Type>(), operands, result);
                    break;
                case Data::eType::VectorN:
                    OperatorEvaluator::Evaluate<Data::VectorNType>(OperatorSubImpl<Data::VectorNType>(), operands, result);
                    break;
                case Data::eType::Matrix3x3:
                    OperatorEvaluator::Evaluate<Data::Matrix3x3Type>(OperatorSubImpl<Data::Matrix3x3Type>(), operands, result);
                    break;
                case Data::eType::Matrix4x4:
                    OperatorEvaluator::Evaluate<Data::Matrix4x4Type>(OperatorSubImpl<Data::Matrix4x4Type>(), operands, result);
                    break;
                case Data::eType::MatrixMxN:
                    OperatorEvaluator::Evaluate<Data::MatrixMxNType>(OperatorSubImpl<Data::MatrixMxNType>(), operands, result);
                    break;
                default:
                    AZ_Assert(false, "Subtraction operator not defined for type: %s", Data::ToAZType(type).ToString<AZStd::string>().c_str());
                    break;
                }
            }
        }
    }
}
