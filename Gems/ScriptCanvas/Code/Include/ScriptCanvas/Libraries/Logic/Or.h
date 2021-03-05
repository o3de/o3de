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

#include <Libraries/Core/BinaryOperator.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            class Or
                : public BooleanExpression
            {
            public:
                AZ_COMPONENT(Or, "{363F9994-8D55-4117-BE94-EFF536BDAC17}", BooleanExpression);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Or, BooleanExpression>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Or>("Or", "An execution flow gate that continues True if either Boolean A or Boolean B are True, otherwise continues False")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Placeholder.png")
                                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                                ;
                        }
                    }
                }

            protected:
                Datum Evaluate(const Datum& lhs, const Datum& rhs) override
                {
                    return Datum(*lhs.GetAs<bool>() || *rhs.GetAs<bool>());
                }

                void InitializeBooleanExpression() override
                {
                    {
                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = k_lhsName;
                        slotConfiguration.SetType(Data::Type::Boolean());
                        slotConfiguration.SetConnectionType(ConnectionType::Input);

                        AddSlot(slotConfiguration);
                    }

                    {
                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = k_rhsName;
                        slotConfiguration.SetType(Data::Type::Boolean());
                        slotConfiguration.SetConnectionType(ConnectionType::Input);

                        AddSlot(slotConfiguration);
                    }
                }
            };

#if defined(EXPRESSION_TEMPLATES_ENABLED)
            class Or
                : public BinaryOperatorGeneric<Or, AZStd::logical_or<bool>>
            {
            public:
                using BaseType = BinaryOperatorGeneric<Or, AZStd::logical_or<bool>>;
                AZ_COMPONENT(Or, "{B892B000-22BB-41FC-BE5C-A8B8798BCB01}", BaseType);

                static const char* GetOperatorName() { return "Or"; }
                static const char* GetOperatorDesc() { return "Logical OR between two boolean values"; }
                static const char* GetIconPath() { return "Editor/Icons/ScriptCanvas/Or.png"; }
                static AZStd::vector<ContractDescriptor> GetFirstArgContractDesc()
                {
                    ContractDescriptor typeIdContractDesc;
                    typeIdContractDesc.m_createFunc = []() -> ScriptCanvas::TypeContract* {return aznewScriptCanvas::TypeContract{ azrtti_typeid<bool>() }; };
                    AZStd::vector<ContractDescriptor> contractDescs;
                    contractDescs.push_back(AZStd::move(typeIdContractDesc));
                    return contractDescs;
                }
                static AZStd::vector<ContractDescriptor> GetSecondArgContractDesc() { return GetFirstArgContractDesc(); }

                
            };
#endif // #if defined(EXPRESSION_TEMPLATES_ENABLED)
        }
    }
}


