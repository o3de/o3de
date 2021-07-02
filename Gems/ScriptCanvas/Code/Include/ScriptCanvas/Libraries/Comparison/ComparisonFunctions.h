/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if defined(EXPRESSION_TEMPLATES_ENABLED)

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Datum.h>

#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Component/ComponentApplicationBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Comparison
        {
            enum class OperatorType : AZ::u32
            {
                Equal,
                NotEqual,
                Less,
                Greater,
                LessEqual,
                GreaterEqual
            };

            template<typename NumberType1, typename NumberType2, typename = void>
            struct CompareAction
            {
                // Compares two primitive types each other
                //! \return true if a comparison occurred between both types and stores the result of the comparison in the @result parameter
                inline static bool CompareNumber(bool& result, OperatorType operatorType, NumberType1 leftNumber, NumberType2 rightNumber)
                {
                    switch (operatorType)
                    {
                    case OperatorType::Equal:
                        result = leftNumber == rightNumber;
                        return true;
                    case OperatorType::NotEqual:
                        result = leftNumber != rightNumber;
                        return true;
                    case OperatorType::Less:
                        result = leftNumber < rightNumber;
                        return true;
                    case OperatorType::Greater:
                        result = leftNumber > rightNumber;
                        return true;
                    case OperatorType::LessEqual:
                        result = leftNumber <= rightNumber;
                        return true;
                    case OperatorType::GreaterEqual:
                        result = leftNumber >= rightNumber;
                        return true;
                    default:
                        return false;
                    }
                }
            };

            //! Attempts to cast the second object parameter to a primitive type and compare it against the supplied primitive parameter
            //! \return true if a comparison occurred between both types and stores the result of the comparison in the @result parameter
            template<typename NumberType, typename = AZStd::enable_if_t<AZStd::is_arithmetic<NumberType>::value>>
            inline bool CompareNumberToBehaviorParameter(bool& result, OperatorType operatorType, NumberType leftNumber, AZ::BehaviorValueParameter& rhs)
            {
                if(CanCastToValue<NumberType>(rhs))
                {
                    NumberType convertedParam{};
                    return CastToValue(convertedParam, rhs) && CompareAction<NumberType, NumberType>::CompareNumber(result, operatorType, leftNumber, convertedParam);
                }
                return false;
            }

            //! Attempts to cast the supplied object parameters to a primitive type and compare them
            //! \return true if a comparison occurred between both types and stores the result of the comparison in the @result parameter
            inline bool ComparePrimitive(bool& result, OperatorType operatorType, const Datum& lhs, const Datum& rhs)
            {
                auto leftParam = lhs.Get();
                auto rightParam = rhs.Get();
                if (CanCastToValue<bool>(leftParam))
                {
                    bool convertedParam{};
                    return CastToValue(convertedParam, leftParam) && CompareNumberToBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<double>(leftParam))
                {
                    double convertedParam{};
                    return CastToValue(convertedParam, leftParam) && CompareNumberToBehaviorParameter(result,operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<float>(leftParam))
                {
                    float convertedParam{};
                    return CastToValue(convertedParam, leftParam) && CompareNumberToBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<AZ::u64>(leftParam))
                {
                    AZ::u64 convertedParam{};
                    return CastToValue(convertedParam, leftParam) && CompareNumberToBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<AZ::s64>(leftParam))
                {
                    AZ::s64 convertedParam{};
                    return CastToValue(convertedParam, leftParam) && CompareNumberToBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<unsigned long>(leftParam))
                {
                    unsigned long convertedParam{};
                    return CastToValue(convertedParam, leftParam) && CompareNumberToBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<long>(leftParam))
                {
                    long convertedParam{};
                    return CastToValue(convertedParam, leftParam) && CompareNumberToBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<AZ::u32>(leftParam))
                {
                    AZ::u32 convertedParam{};
                    return CastToValue(convertedParam, leftParam) && CompareNumberToBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<AZ::s32>(leftParam))
                {
                    AZ::s32 convertedParam{};
                    return CastToValue(convertedParam, leftParam) && CompareNumberToBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<AZ::u16>(leftParam))
                {
                    AZ::u16 convertedParam{};
                    return CastToValue(convertedParam, leftParam) && CompareNumberToBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<AZ::s16>(leftParam))
                {
                    AZ::s16 convertedParam{};
                    return CastToValue(convertedParam, leftParam) && CompareNumberToBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<AZ::u8>(leftParam))
                {
                    AZ::u8 convertedParam{};
                    return CastToValue(convertedParam, leftParam) && CompareNumberToBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<AZ::s8>(leftParam))
                {
                    AZ::s8 convertedParam{};
                    return CastToValue(convertedParam, leftParam) && CompareNumberToBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<char>(leftParam))
                {
                    char convertedParam{};
                    return CastToValue(convertedParam, leftParam) && CompareNumberToBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }

                return false;
            }

            /**
            For the record, this is amazing. But, we can't go dumpster diving through behavior context for the right method to call. If there is a proper evaluation to make, we make the ability for people to
            expose to behavior context the correct operations they want used in ScriptCanvas. No matter which route we chose, we should do it at edit time, rather than compile time.
            */

            //! Returns a multimap of methods which match the operatorType prioritized by the by least number of type conversions needed for both parameters to invoke the method
            inline AZStd::multimap<size_t, AZ::BehaviorMethod*> FindOperatorMethod(AZ::Script::Attributes::OperatorType operatorLookupType, AZ::BehaviorValueParameter& leftParameter, AZ::BehaviorValueParameter& rightParameter)
            {
                AZStd::multimap<size_t, AZ::BehaviorMethod*> methodMap;
                AZ::BehaviorContext* behaviorContext{};
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
                if (!behaviorContext)
                {
                    return methodMap;
                }

                auto behaviorClassIt = behaviorContext->m_typeToClassMap.find(leftParameter.m_typeId);
                if (behaviorClassIt != behaviorContext->m_typeToClassMap.end())
                {
                    AZ::BehaviorClass* behaviorClass = behaviorClassIt->second;
                    for (auto& methodPair : behaviorClass->m_methods)
                    {
                        AZ::BehaviorMethod* method = methodPair.second;
                        if (auto* operatorAttr = FindAttribute(AZ::Script::Attributes::Operator, method->m_attributes))
                        {
                            // read the operator type
                            AZ::AttributeReader operatorAttrReader(nullptr, operatorAttr);
                            AZ::Script::Attributes::OperatorType operatorType;
                            operatorAttrReader.Read<decltype(operatorType)>(operatorType);

                            if (operatorType == operatorLookupType
                                && method->HasResult() && method->GetResult()->m_typeId == azrtti_typeid<bool>() && method->GetNumArguments() == 2)
                            {
                                if (behaviorClass->m_typeId == method->GetArgument(0)->m_typeId && rightParameter.m_typeId == method->GetArgument(1)->m_typeId)
                                {
                                    methodMap.emplace(0, method);
                                }
                                else if (behaviorClass->m_typeId == method->GetArgument(0)->m_typeId && rightParameter.m_azRtti && rightParameter.m_azRtti->IsTypeOf(method->GetArgument(1)->m_typeId))
                                {
                                    methodMap.emplace(1, method);
                                }
                                else if (behaviorClass->m_azRtti && behaviorClass->m_azRtti->IsTypeOf(method->GetArgument(0)->m_typeId) && rightParameter.m_typeId == method->GetArgument(1)->m_typeId)
                                {
                                    methodMap.emplace(1, method);
                                }
                                else if (behaviorClass->m_azRtti && behaviorClass->m_azRtti->IsTypeOf(method->GetArgument(0)->m_typeId) && rightParameter.m_azRtti && rightParameter.m_azRtti->IsTypeOf(method->GetArgument(1)->m_typeId))
                                {
                                    methodMap.emplace(2, method);
                                }
                            }
                        }
                    }
                }

                return methodMap;
            }

            inline bool InvokeMethod(AZ::BehaviorMethod* method, AZ::BehaviorValueParameter& resultParam, AZStd::array<AZStd::reference_wrapper<AZ::BehaviorValueParameter>, 2> parameters)
            {
                AZStd::array<void*,2> argAddresses;
                AZStd::array<AZ::BehaviorValueParameter, 2> methodArgs;
                for (size_t i = 0; i < methodArgs.size(); ++i)
                {
                    methodArgs[i].Set(*method->GetArgument(i));
                    argAddresses[i] = parameters[i].get().GetValueAddress();
                    if (methodArgs[i].m_traits & AZ::BehaviorParameter::TR_POINTER)
                    {
                        methodArgs[i].m_value = &argAddresses[i];
                    }
                    else
                    {
                        methodArgs[i].m_value = argAddresses[i];
                    }
                }

                return method->Call(methodArgs.data(), static_cast<unsigned int>(methodArgs.size()), &resultParam);
            }

            //! Compares two object types to each other
            //! \return true if a comparison occurred between both types and stores the result of the comparison in the @result parameter
            inline bool CompareObjects(bool& result, OperatorType operatorType, const Datum& lhs, const Datum& rhs)
            {
                auto leftParameter = lhs.Get();
                auto rightParameter = rhs.Get();
                AZ::BehaviorValueParameter resultParameter(&result);
                AZStd::multimap<size_t, AZ::BehaviorMethod*> methodMap{};
                switch (operatorType)
                {
                case OperatorType::Equal:
                    methodMap = FindOperatorMethod(AZ::Script::Attributes::OperatorType::Equal, leftParameter, rightParameter);
                    if (!methodMap.empty())
                    {
                        if (InvokeMethod(methodMap.begin()->second, resultParameter, { { AZStd::reference_wrapper<AZ::BehaviorValueParameter>(leftParameter), AZStd::reference_wrapper<AZ::BehaviorValueParameter>(rightParameter)} }))
                        {
                            return true;
                        }
                    }
                    else
                    {
                        methodMap = FindOperatorMethod(AZ::Script::Attributes::OperatorType::Equal, rightParameter, leftParameter);
                        if (!methodMap.empty())
                        {
                            if (InvokeMethod(methodMap.begin()->second, resultParameter, { { AZStd::reference_wrapper<AZ::BehaviorValueParameter>(rightParameter), AZStd::reference_wrapper<AZ::BehaviorValueParameter>(leftParameter) } }))
                            {
                                return true;
                            }
                        }
                    }
                    break;
                case OperatorType::NotEqual:
                    methodMap = FindOperatorMethod(AZ::Script::Attributes::OperatorType::Equal, leftParameter, rightParameter);
                    if (!methodMap.empty())
                    {
                        if (InvokeMethod(methodMap.begin()->second, resultParameter, { { AZStd::reference_wrapper<AZ::BehaviorValueParameter>(leftParameter), AZStd::reference_wrapper<AZ::BehaviorValueParameter>(rightParameter) } }))
                        {
                            result = !result;
                            return true;
                        }
                    }
                    else
                    {
                        methodMap = FindOperatorMethod(AZ::Script::Attributes::OperatorType::Equal, rightParameter, leftParameter);
                        if (!methodMap.empty())
                        {
                            if (InvokeMethod(methodMap.begin()->second, resultParameter, { { AZStd::reference_wrapper<AZ::BehaviorValueParameter>(rightParameter), AZStd::reference_wrapper<AZ::BehaviorValueParameter>(leftParameter) } }))
                            {
                                result = !result;
                                return true;
                            }
                        }
                    }
                    break;

                case OperatorType::Less:
                    methodMap = FindOperatorMethod(AZ::Script::Attributes::OperatorType::LessThan, leftParameter, rightParameter);
                    if (!methodMap.empty())
                    {
                        if (InvokeMethod(methodMap.begin()->second, resultParameter, { { AZStd::reference_wrapper<AZ::BehaviorValueParameter>(leftParameter), AZStd::reference_wrapper<AZ::BehaviorValueParameter>(rightParameter) } }))
                        {
                            return true;
                        }
                    }
                    break;
                case OperatorType::Greater:
                    methodMap = FindOperatorMethod(AZ::Script::Attributes::OperatorType::LessThan, leftParameter, rightParameter);
                    if (!methodMap.empty())
                    {
                        if (InvokeMethod(methodMap.begin()->second, resultParameter, { { AZStd::reference_wrapper<AZ::BehaviorValueParameter>(rightParameter), AZStd::reference_wrapper<AZ::BehaviorValueParameter>(leftParameter) } }))
                        {
                            return true;
                        }
                    }
                    break;
                case OperatorType::LessEqual:
                    methodMap = FindOperatorMethod(AZ::Script::Attributes::OperatorType::LessEqualThan, leftParameter, rightParameter);
                    if (!methodMap.empty())
                    {
                        if (InvokeMethod(methodMap.begin()->second, resultParameter, { { AZStd::reference_wrapper<AZ::BehaviorValueParameter>(leftParameter), AZStd::reference_wrapper<AZ::BehaviorValueParameter>(rightParameter) } }))
                        {
                            return true;
                        }
                    }
                    else
                    {
                        methodMap = FindOperatorMethod(AZ::Script::Attributes::OperatorType::LessThan, leftParameter, rightParameter);
                        if (!methodMap.empty())
                        {
                            if (InvokeMethod(methodMap.begin()->second, resultParameter, { { AZStd::reference_wrapper<AZ::BehaviorValueParameter>(rightParameter), AZStd::reference_wrapper<AZ::BehaviorValueParameter>(leftParameter) } }))
                            {
                                result = !result;
                                return true;
                            }
                        }
                    }
                    break;
                case OperatorType::GreaterEqual:
                    methodMap = FindOperatorMethod(AZ::Script::Attributes::OperatorType::LessEqualThan, leftParameter, rightParameter);
                    if (!methodMap.empty())
                    {
                        if (InvokeMethod(methodMap.begin()->second, resultParameter, { { AZStd::reference_wrapper<AZ::BehaviorValueParameter>(rightParameter), AZStd::reference_wrapper<AZ::BehaviorValueParameter>(leftParameter) } }))
                        {
                            return true;
                        }
                    }
                    else
                    {
                        methodMap = FindOperatorMethod(AZ::Script::Attributes::OperatorType::LessThan, leftParameter, rightParameter);
                        if (!methodMap.empty())
                        {
                            if (InvokeMethod(methodMap.begin()->second, resultParameter, { { AZStd::reference_wrapper<AZ::BehaviorValueParameter>(leftParameter), AZStd::reference_wrapper<AZ::BehaviorValueParameter>(rightParameter) } }))
                            {
                                result = !result;
                                return true;
                            }
                        }
                    }
                    break;
                default:
                    break;
                }
                return false;
            }
           
            template<OperatorType operatorType>
            struct ComparisonOperator
            {
                bool operator()(const Datum& lhs, const Datum& rhs) const
                {

                    // If both types sides are primitive types then perform a special case primitive value compare
                    bool result{};
                    if (ComparePrimitive(result, operatorType, lhs, rhs) || CompareObjects(result, operatorType, lhs, rhs))
                    {
                        return result;
                    }

                    return false;
                }

            };
        }
    }
}

#endif // EXPRESSION_TEMPLATES_ENABLED
