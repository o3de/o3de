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

#include <Libraries/Core/UnaryOperator.h>
#include <Libraries/Logic/Boolean.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            class Not
                : public UnaryExpression
            {
            public:
                AZ_COMPONENT(Not, "{EF6BA813-9AF9-45CF-A8A4-7F800D7B7CB0}", UnaryExpression);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Not, UnaryExpression>()
                            ->Version(1)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Not>("Not", "An execution flow gate that continues True if the Boolean is False, otherwise continues False if the Boolean is True")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Placeholder.png")
                                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                                ;
                        }
                    }
                }

            protected:

                Datum Evaluate(const Datum& value) override
                {
                    const bool* boolValue = value.GetAs<bool>();
                    return Datum(boolValue && (!(*boolValue)));
                }

                
            }; 

#if defined(EXPRESSION_TEMPLATES_ENABLED)
            class Not 
                : public UnaryOperator<Not, AZStd::logical_not<bool>>
            {
            public:
                using BaseType = UnaryOperator<Not, AZStd::logical_not<bool>>;
                AZ_COMPONENT(Not, "{23029BBB-210E-4610-AC41-DF32F4721C2A}", BaseType);

                static const char* GetOperatorName() { return "Not"; }
                static const char* GetOperatorDesc() { return "Logical NOT of a boolean value"; }
                static const char* GetIconPath() { return "Editor/Icons/ScriptCanvas/Not.png"; }
                static AZStd::vector<ContractDescriptor> GetFirstArgContractDesc()
                {
                    ContractDescriptor typeIdContractDesc;
                    typeIdContractDesc.m_createFunc = []() -> ScriptCanvas::TypeContract* {return aznewScriptCanvas::TypeContract{ azrtti_typeid<bool>() }; };
                    AZStd::vector<ContractDescriptor> contractDescs;
                    contractDescs.push_back(AZStd::move(typeIdContractDesc));
                    return contractDescs;
                }

                
            };
#endif // #if defined(EXPRESSION_TEMPLATES_ENABLED)
        }
    }
}


