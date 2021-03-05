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
            class NotEqualTo
                : public EqualityExpression
            {
            public:
                AZ_COMPONENT(NotEqualTo, "{C8D7A10F-A919-4467-96B1-F1852C282628}", EqualityExpression);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<NotEqualTo, EqualityExpression>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<NotEqualTo>("Not Equal To (!=)", "Checks if Value A is not equal to Value B")
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
                    ComparisonOutcome result(lhs != rhs);
                    
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
            class NotEqualToGeneric
                : public BinaryOperatorGeneric<NotEqualToGeneric, ComparisonOperator<OperatorType::NotEqual>>
            {
            public:
                using BaseType = BinaryOperatorGeneric<NotEqualToGeneric, ComparisonOperator<OperatorType::NotEqual>>;
                AZ_COMPONENT(NotEqualTo, "{FE95C7F6-D262-400E-83D9-AE22406FA0FB}", BaseType);

                static const char* GetOperatorName() { return "Not EqualTo"; }
                static const char* GetOperatorDesc() { return "Compares two values for inequality"; }
                static const char* GetIconPath() { return "Editor/Icons/ScriptCanvas/Placeholder.png"; } // TODO: Get final icon image
            };
#endif // #if defined(EXPRESSION_TEMPLATES_ENABLED)
        }
    }
}


