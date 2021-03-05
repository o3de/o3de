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
            class LessEqual
                : public ComparisonExpression
            {
            public:
                AZ_COMPONENT(LessEqual, "{73F6E302-A2E9-4BE6-A88F-98F81A24100D}", ComparisonExpression);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<LessEqual, ComparisonExpression>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<LessEqual>("Less Than or Equal To (<=)", "Checks if Value A is less than or equal to Value B")
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
                    ComparisonOutcome result(lhs <= rhs);
                    
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
            class LessEqualGeneric
                : public BinaryOperatorGeneric<LessEqualGeneric, ComparisonOperator<OperatorType::LessEqual>>
            {
            public:
                using BaseType = BinaryOperatorGeneric<LessEqualGeneric, ComparisonOperator<OperatorType::LessEqual>>;
                AZ_COMPONENT(LessEqual, "{274EE550-0DB8-4036-8A09-BB1DB82B3D21}", BaseType);

                static const char* GetOperatorName() { return "Less or Equal"; }
                static const char* GetOperatorDesc() { return "Compares if first value is less than or equal to second value"; }
                static const char* GetIconPath() { return "Editor/Icons/ScriptCanvas/Placeholder.png"; } // TODO: Get final icon image
            };
#endif // #if defined(EXPRESSION_TEMPLATES_ENABLED)
        }
    }
}


