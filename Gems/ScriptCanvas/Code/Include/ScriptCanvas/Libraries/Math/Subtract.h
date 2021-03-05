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
#include <Libraries/Math/ArithmeticFunctions.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Math
        {
            class Subtract
                : public ArithmeticExpression
            {
            public:
                AZ_COMPONENT(Subtract, "{A10AD4C7-B633-4A75-8210-1353A87441E4}", ArithmeticExpression);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Subtract, ArithmeticExpression>()
                            ->Version(0)
                            ->Attribute(AZ::Script::Attributes::Deprecated, true)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Subtract>("Subtract", "Subtract")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "This node is deprecated use the Subtract (-) node instead, it provides contextual type and slot configurations.")
                                    ->Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "DeprecatedNodeTitlePalette")
                                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                                    ->Attribute(AZ::Edit::Attributes::Category, "Math/Number/Deprecated")
                                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Placeholder.png")
                                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                                ;
                        }
                    }
                }
                
            protected:
                Datum Evaluate(const Datum& lhs, const Datum& rhs) override
                {
                    return Datum(*lhs.GetAs<Data::NumberType>() - *rhs.GetAs<Data::NumberType>());
                }
            };

#if defined(EXPRESSION_TEMPLATES_ENABLED)

            class Subtract 
                : public BinaryOperatorGeneric<Subtract, ArithmeticOperator<OperatorType::Sub>>
            {
            public:
                using BaseType = BinaryOperatorGeneric<Subtract, ArithmeticOperator<OperatorType::Sub>>;
                AZ_COMPONENT(Subtract, "{A4532B9D-FFD8-43E0-A359-E1AB11208E11}", BaseType);

                static const char* GetOperatorName() { return "Subtract"; }
                static const char* GetOperatorDesc() { return "Perform subtraction between two numbers"; }
                static const char* GetIconPath() { return "Editor/Icons/ScriptCanvas/Subtract.png"; }

                

            };
#endif // #if defined(EXPRESSION_TEMPLATES_ENABLED)
        }
    }
}
