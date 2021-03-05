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

#include "ComparisonFunctions.h"
#include <Libraries/Core/BinaryOperator.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Comparison
        {
            class Less
                : public ComparisonExpression
            {
            public:
                AZ_COMPONENT(Less, "{1B93426F-AAA2-4134-BE9A-C33B8F07F867}", ComparisonExpression);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Less, ComparisonExpression>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Less>("Less Than (<)", "Checks if Value A is less than Value B")
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
                    ComparisonOutcome result(lhs < rhs);
                    
                    if (result.IsSuccess())
                    {
                        return Datum(result.GetValue());
                    }
                    else
                    {
                        SCRIPTCANVAS_REPORT_ERROR((*this), result.GetError().c_str());
                        return Datum(false);
                    }
                }
            };

#if defined(EXPRESSION_TEMPLATES_ENABLED)
            class LessGeneric
                : public BinaryOperatorGeneric<Less, ComparisonOperator<OperatorType::Less>>
            {
            public:
                using BaseType = BinaryOperatorGeneric<Less, ComparisonOperator<OperatorType::Less>>;
                AZ_COMPONENT(LessGeneric, "{D1FB2E44-2115-4850-8B34-B2A2EC100FB4}", BaseType);

                static const char* GetOperatorName() { return "Less"; }
                static const char* GetOperatorDesc() { return "Compares if first value is less than second value"; }
                static const char* GetIconPath() { return "Editor/Icons/ScriptCanvas/Placeholder.png"; } // TODO: Get final icon image
            };
#endif // #if defined(EXPRESSION_TEMPLATES_ENABLED)
        }
    }
}


