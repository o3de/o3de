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

#pragma once


#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Math/Random.h>
#include <Libraries/Math/MathNodeUtilities.h>
#include <random>

namespace ScriptCanvas
{
#if defined(EXPRESSION_TEMPLATES_ENABLED)

    namespace Nodes
    {
        namespace Math
        {
            // Performs arithmetic random math on two primitive integers as ranges
            //! \return a random integer between two numbers
            template<typename NumberType, typename = AZStd::enable_if_t<AZStd::is_integral<NumberType>::value>>
            inline static NumberType PerformArithmeticRandom(NumberType leftNumber, NumberType rightNumber)
            {
                return MathNodeUtilities::GetRandomIntegral(leftNumber, rightNumber);
            }

            // Performs arithmetic random math on two real numbers as ranges
            //! \return a random real number between two real numbers
            template<typename NumberType, typename = AZStd::enable_if_t<AZStd::is_floating_point<NumberType>::value>>
            inline static NumberType PerformArithmeticRandomReal(NumberType leftNumber, NumberType rightNumber)
            {
                return MathNodeUtilities::GetRandomReal(leftNumber, rightNumber);
            }

            enum class OperatorType : AZ::u32
            {
                Add,
                Sub,
                Mul,
                Div,
                Mod,
                Concat,
                Random,
                Xor
            };

            template<typename NumberType1, typename NumberType2, typename = void>
            struct ArithmeticAction;

            template<typename NumberType1, typename NumberType2>
            struct ArithmeticAction<NumberType1, NumberType2, AZStd::enable_if_t<AZStd::is_integral<NumberType1>::value && !AZStd::is_same<NumberType1, bool>::value 
                && AZStd::is_integral<NumberType2>::value && !AZStd::is_same<NumberType2, bool>::value>>
            {
                // Performs arithmetic action on two primitive types each other
                //! \return true if an arithmetic operation occurred between both types and stores the result of the operation in the @result parameter
                inline static bool Perform(Datum& result, OperatorType operatorType, NumberType1 leftNumber, NumberType2 rightNumber)
                {
                    switch (operatorType)
                    {
                    case OperatorType::Add:
                        result = Datum(AZStd::any(leftNumber + rightNumber));
                        return true;
                    case OperatorType::Sub:
                        result = Datum(AZStd::any(leftNumber - rightNumber));
                        return true;
                    case OperatorType::Mul:
                        result = Datum(AZStd::any(leftNumber * rightNumber));
                        return true;
                    case OperatorType::Div:
                        result = Datum(AZStd::any(leftNumber / rightNumber));
                        return true;
                    case OperatorType::Mod:
                        result = Datum(AZStd::any(leftNumber % rightNumber));
                        return true;
                    case OperatorType::Random:
                        result = Datum(AZStd::any(PerformArithmeticRandom(leftNumber, rightNumber)));
                        return true;
                    case OperatorType::Xor:
                        result = Datum(AZStd::any(leftNumber ^ rightNumber));
                        return true;
                    default:
                        return false;
                    }
                }
            };

            template<typename NumberType1, typename NumberType2>
            struct ArithmeticAction<NumberType1, NumberType2, AZStd::enable_if_t<AZStd::is_floating_point<NumberType1>::value && AZStd::is_floating_point<NumberType2>::value>>
            {
                // Performs arithmetic action on two primitive types each other
                //! \return true if an arithmetic operation occurred between both types and stores the result of the operation in the @result parameter
                inline static bool Perform(Datum& result, OperatorType operatorType, NumberType1 leftNumber, NumberType2 rightNumber)
                {
                    switch (operatorType)
                    {
                    case OperatorType::Add:
                        result = Datum(AZStd::any(leftNumber + rightNumber));
                        return true;
                    case OperatorType::Sub:
                        result = Datum(AZStd::any(leftNumber - rightNumber));
                        return true;
                    case OperatorType::Mul:
                        result = Datum(AZStd::any(leftNumber * rightNumber));
                        return true;
                    case OperatorType::Div:
                        result = Datum(AZStd::any(leftNumber / rightNumber));
                        return true;
                    case OperatorType::Random:
                        result = Datum(AZStd::any(PerformArithmeticRandomReal(leftNumber, rightNumber)));
                        return true;
                    default:
                        return false;
                    }
                }
            };

            template<typename NumberType1, typename NumberType2>
            struct ArithmeticAction<NumberType1, NumberType2, AZStd::enable_if_t<AZStd::is_same<NumberType1, bool>::value && AZStd::is_same<NumberType2, bool>::value>>
            {
                // Performs arithmetic action on two primitive types each other
                //! \return true if an arithmetic operation occurred between both types and stores the result of the operation in the @result parameter
                inline static bool Perform(Datum& result, OperatorType operatorType, NumberType1 leftNumber, NumberType2 rightNumber)
                {
                    switch (operatorType)
                    {
                    case OperatorType::Add:
                        result = Datum(AZStd::any(leftNumber + rightNumber));
                        return true;
                    case OperatorType::Sub:
                        result = Datum(AZStd::any(leftNumber - rightNumber));
                        return true;
                    case OperatorType::Xor:
                        result = Datum(AZStd::any(leftNumber ^ rightNumber));
                        return true;
                    default:
                        return false;
                    }
                }
            };

            // Attempts to cast the second behavior parameter to a primitive type and perform an arithmetic action against the supplied primitive type
            //! \return true if an arithmetic operation occurred between both types and stores the result of the operation in the @result parameter
            template<typename NumberType, typename = AZStd::enable_if_t<AZStd::is_arithmetic<NumberType>::value>>
            inline bool PerformArithmeticNumberBehaviorParameter(Datum& result, OperatorType operatorType, NumberType leftNumber, AZ::BehaviorValueParameter& rhs)
            {
                if (CanCastToValue<NumberType>(rhs))
                {
                    NumberType convertedParam{};
                    return CastToValue(convertedParam, rhs) && ArithmeticAction<NumberType, NumberType>::Perform(result, operatorType, leftNumber, convertedParam);
                }
                return false;
            }

            // Attempts to cast the first behavior parameter to a primitive type and perform an arithmetic action between the casted type and the second behavior parameter
            //! \return true if an arithmetic operation occurred between both types and stores the result of the operation in the @result parameter
            inline bool PerformArithmeticPrimitive(Datum& result, OperatorType operatorType, const Datum& lhs, const Datum& rhs)
            {
                auto leftParam = lhs.Get();
                auto rightParam = rhs.Get();
                if (CanCastToValue<bool>(leftParam))
                {
                    bool convertedParam{};
                    return CastToValue(convertedParam, leftParam) && PerformArithmeticNumberBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if(CanCastToValue<double>(leftParam))
                {
                    double convertedParam{};
                    return CastToValue(convertedParam, leftParam) && PerformArithmeticNumberBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<float>(leftParam))
                {
                    float convertedParam{};
                    return CastToValue(convertedParam, leftParam) && PerformArithmeticNumberBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<AZ::u64>(leftParam))
                {
                    AZ::u64 convertedParam{};
                    return CastToValue(convertedParam, leftParam) && PerformArithmeticNumberBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<AZ::s64>(leftParam))
                {
                    AZ::s64 convertedParam{};
                    return CastToValue(convertedParam, leftParam) && PerformArithmeticNumberBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<unsigned long>(leftParam))
                {
                    unsigned long convertedParam{};
                    return CastToValue(convertedParam, leftParam) && PerformArithmeticNumberBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<long>(leftParam))
                {
                    long convertedParam{};
                    return CastToValue(convertedParam, leftParam) && PerformArithmeticNumberBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<AZ::u32>(leftParam))
                {
                    AZ::u32 convertedParam{};
                    return CastToValue(convertedParam, leftParam) && PerformArithmeticNumberBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<AZ::s32>(leftParam))
                {
                    AZ::s32 convertedParam{};
                    return CastToValue(convertedParam, leftParam) && PerformArithmeticNumberBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<AZ::u16>(leftParam))
                {
                    AZ::u16 convertedParam{};
                    return CastToValue(convertedParam, leftParam) && PerformArithmeticNumberBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<AZ::s16>(leftParam))
                {
                    AZ::s16 convertedParam{};
                    return CastToValue(convertedParam, leftParam) && PerformArithmeticNumberBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<AZ::u8>(leftParam))
                {
                    AZ::u8 convertedParam{};
                    return CastToValue(convertedParam, leftParam) && PerformArithmeticNumberBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<AZ::s8>(leftParam))
                {
                    AZ::s8 convertedParam{};
                    return CastToValue(convertedParam, leftParam) && PerformArithmeticNumberBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }
                else if (CanCastToValue<char>(leftParam))
                {
                    char convertedParam{};
                    return CastToValue(convertedParam, leftParam) && PerformArithmeticNumberBehaviorParameter(result, operatorType, convertedParam, rightParam);
                }

                return false;
            }

            /**
            For the record, this is amazing. But, we can't go dumpster diving through behavior context for the right method to call. If there is a proper evaluation to make, we make the ability for people to 
            expose to behavior context the corrector operations they want used in ScriptCanvas
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
                                && method->HasResult() && method->GetNumArguments() == 2)
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

            inline bool PerformArithmeticObject(Datum& result, OperatorType operatorType, const Datum& lhs, const Datum& rhs)
            {
                auto leftParameter = lhs.Get();
                auto rightParameter = rhs.Get();
                AZStd::multimap<size_t, AZ::BehaviorMethod*> methodMap{};
                switch (operatorType)
                {
                case OperatorType::Add:
                    methodMap = FindOperatorMethod(AZ::Script::Attributes::OperatorType::Add, leftParameter, rightParameter);
                    break;
                case OperatorType::Sub:
                    methodMap = FindOperatorMethod(AZ::Script::Attributes::OperatorType::Sub, leftParameter, rightParameter);
                    break;
                case OperatorType::Mul:
                    methodMap = FindOperatorMethod(AZ::Script::Attributes::OperatorType::Mul, leftParameter, rightParameter);
                    break;
                case OperatorType::Div:
                    methodMap = FindOperatorMethod(AZ::Script::Attributes::OperatorType::Div, leftParameter, rightParameter);
                    break;
                case OperatorType::Mod:
                    methodMap = FindOperatorMethod(AZ::Script::Attributes::OperatorType::Mod, leftParameter, rightParameter);
                    break;
                case OperatorType::Concat:
                    methodMap = FindOperatorMethod(AZ::Script::Attributes::OperatorType::Concat, leftParameter, rightParameter);
                    break;
                default:
                    break;
                }

                if (!methodMap.empty())
                {
                    auto& method = methodMap.begin()->second;
                    auto* methodResultParam = method->GetResult();
                    AZ::BehaviorValueParameter resultParameter;
                    resultParameter.Set(*methodResultParam);
                    auto* resultClassData = AZ::Utils::GetApplicationSerializeContext()->FindClassData(resultParameter.m_typeId);
                    if (resultClassData && resultClassData->m_factory)
                    {
                        resultParameter.m_value = resultClassData->m_factory->Create("ArithmeticResult");
                        if (InvokeMethod(method, resultParameter, { { AZStd::reference_wrapper<AZ::BehaviorValueParameter>(leftParameter), AZStd::reference_wrapper<AZ::BehaviorValueParameter>(rightParameter) } }))
                        {
                            ConvertToValue(result, resultParameter);
                            resultClassData->m_factory->Destroy(resultParameter.GetValueAddress());
                            return true;
                        }
                        resultClassData->m_factory->Destroy(resultParameter.GetValueAddress());
                    }
                }
                return false;
            }
           
            template<OperatorType operatorType>
            struct ArithmeticOperator
            {
                Datum operator()(const Datum& lhs, const Datum& rhs) const
                {
                    // If both types sides are primitive types then perform a special case primitive value compare
                    Datum result;
                    if (PerformArithmeticPrimitive(result, operatorType, lhs, rhs) || PerformArithmeticObject(result, operatorType, lhs, rhs))
                    {
                        return result;
                    }

                    return{};
                }
            };
        } // Math
    } // Nodes

#endif // defined(EXPRESSION_TEMPLATES_ENABLED)

} // ScriptCanvas
