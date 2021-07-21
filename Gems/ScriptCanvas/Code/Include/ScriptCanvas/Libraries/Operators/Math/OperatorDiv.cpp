/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OperatorDiv.h"
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
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
            struct OperatorDivImpl
            {
                OperatorDivImpl(Node* node)
                    : m_node(node)
                {}

                Type operator()(const Type& a, const Datum& b)
                {
                    const Type* dataB = b.GetAs<Type>();

                    const Type& divisor = (*dataB);
                    if (AZ::IsClose(divisor, Type(0), std::numeric_limits<Type>::epsilon()))
                    {
                        SCRIPTCANVAS_REPORT_ERROR((*m_node), "Divide by Zero");
                        return Type(0);
                    }

                    return a / divisor;
                }

            private:
                Node* m_node;
            };

            template <typename VectorType, int Elements>
            struct OperatorDivVectorTypes
            {
                using Type = VectorType;

                VectorType operator()(const VectorType& a, const Datum& b)
                {
                    const Type* divisor = b.GetAs<Type>();
                    if (divisor->IsClose(Type::CreateZero()))
                    {
                        // Divide by zero
                        SCRIPTCANVAS_REPORT_ERROR((*m_node), "Divide by Zero");
                        return VectorType(0);
                    }

                    for (int i = 0; i < Elements; ++i)
                    {
                        if (AZ::IsClose((*divisor)(i), 0.0f, std::numeric_limits<float>::epsilon()))
                        {
                            // Divide by zero
                            SCRIPTCANVAS_REPORT_ERROR((*m_node), "Divide by Zero");
                            return VectorType(0);
                        }
                    }

                    return a / (*divisor);
                }

                OperatorDivVectorTypes(Node* node)
                    : m_node(node)
                {}

            private:
                Node* m_node;

            };

            template <>
            struct OperatorDivImpl<Data::Vector2Type> : public OperatorDivVectorTypes<AZ::Vector2, 2>
            {
                OperatorDivImpl(Node* node)
                    : OperatorDivVectorTypes(node)
                {}
            };

            template <>
            struct OperatorDivImpl<Data::Vector3Type> : public OperatorDivVectorTypes<AZ::Vector3, 3>
            {
                OperatorDivImpl(Node* node)
                    : OperatorDivVectorTypes(node)
                {}
            };

            template <>
            struct OperatorDivImpl<Data::Vector4Type> : public OperatorDivVectorTypes<AZ::Vector4, 4>
            {
                OperatorDivImpl(Node* node)
                    : OperatorDivVectorTypes(node)
                {}
            };

            template <>
            struct OperatorDivImpl<Data::ColorType>
            {
                OperatorDivImpl(Node* node)
                    : m_node(node)
                {}

                Datum operator()(const Datum& lhs, const Datum& rhs)
                {
                    const AZ::Color* dataA = lhs.GetAs<AZ::Color>();
                    const AZ::Color* dataB = rhs.GetAs<AZ::Color>();

                    if (dataB->IsClose(AZ::Color(), std::numeric_limits<float>::epsilon()))
                    {
                        // Divide by zero
                        SCRIPTCANVAS_REPORT_ERROR((*m_node), "Divide by Zero");
                        return Datum();
                    }

                    // TODO: clamping should happen at the AZ::Color level, not here - but it does not. 
                    float divisorA = dataB->GetA();
                    if (AZ::IsClose(divisorA, 0.f, std::numeric_limits<float>::epsilon()))
                    {
                        // Divide by zero
                        SCRIPTCANVAS_REPORT_ERROR((*m_node), "Divide by Zero");
                        return Datum();
                    }

                    float divisorR = dataB->GetR();
                    if (AZ::IsClose(divisorR, 0.f, std::numeric_limits<float>::epsilon()))
                    {
                        // Divide by zero
                        SCRIPTCANVAS_REPORT_ERROR((*m_node), "Divide by Zero");
                        return Datum();
                    }

                    float divisorG = dataB->GetG();
                    if (AZ::IsClose(divisorG, 0.f, std::numeric_limits<float>::epsilon()))
                    {
                        // Divide by zero
                        SCRIPTCANVAS_REPORT_ERROR((*m_node), "Divide by Zero");
                        return Datum();
                    }

                    float divisorB = dataB->GetB();
                    if (AZ::IsClose(divisorB, 0.f, std::numeric_limits<float>::epsilon()))
                    {
                        // Divide by zero
                        SCRIPTCANVAS_REPORT_ERROR((*m_node), "Divide by Zero");
                        return Datum();
                    }

                    float a = dataA->GetA() / divisorA;
                    float r = dataA->GetR() / divisorR;
                    float g = dataA->GetG() / divisorG;
                    float b = dataA->GetB() / divisorB;

                    return Datum(AZ::Color(r, g, b, a));
                }

            private:
                Node* m_node;
            };

            AZStd::unordered_set< Data::Type > OperatorDiv::GetSupportedNativeDataTypes() const
            {
                return{
                    Data::Type::Number()
                };
            }

            void OperatorDiv::Operator(Data::eType type, const ArithmeticOperands& operands, Datum& result)
            {
                switch (type)
                {
                case Data::eType::Number:
                    OperatorEvaluator::Evaluate<Data::NumberType>(OperatorDivImpl<Data::NumberType>(this), operands, result);
                    break;
                default:
                    AZ_Assert(false, "Division operator not defined for type: %s", Data::ToAZType(type).ToString<AZStd::string>().c_str());
                    break;
                }
            }

            void OperatorDiv::InitializeSlot(const SlotId& slotId, [[maybe_unused]] const ScriptCanvas::Data::Type& dataType)
            {
                ModifiableDatumView datumView;
                FindModifiableDatumView(slotId, datumView);

                OnResetDatumToDefaultValue(datumView);
            }

            bool OperatorDiv::IsValidArithmeticSlot(const SlotId& slotId) const
            {
                const Datum* datum = FindDatum(slotId);

                // We could do some introspection here to drops 1s, but for now just going to let it perform the pointless math
                // But it gets a bit messy(since x/1 is invalid, but 1/x is very valid).
                return datum != nullptr;
            }

            void OperatorDiv::OnResetDatumToDefaultValue(ModifiableDatumView& datumView)
            {
                Data::Type displayType = GetDisplayType(GetArithmeticDynamicTypeGroup());

                if (datumView.IsValid() && displayType.IsValid())
                {
                    switch (displayType.GetType())
                    {
                    case Data::eType::Number:
                        datumView.SetAs(ScriptCanvas::Data::One());
                        break;
                    case Data::eType::Vector2:
                        datumView.SetAs(ScriptCanvas::Data::Vector2Type::CreateOne());
                        break;
                    case Data::eType::Vector3:
                        datumView.SetAs(ScriptCanvas::Data::Vector3Type::CreateOne());
                        break;
                    case Data::eType::Vector4:
                        datumView.SetAs(ScriptCanvas::Data::Vector4Type::CreateOne());
                        break;
                    case Data::eType::Quaternion:
                        datumView.SetAs(ScriptCanvas::Data::QuaternionType::CreateIdentity());
                        break;
                    case Data::eType::Matrix3x3:
                        datumView.SetAs(ScriptCanvas::Data::Matrix3x3Type::CreateIdentity());
                        break;
                    case Data::eType::Matrix4x4:
                        datumView.SetAs(ScriptCanvas::Data::Matrix4x4Type::CreateIdentity());
                        break;
                    default:
                        break;
                    };
                }
            }
        }
    }
}
