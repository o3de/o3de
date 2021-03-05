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
            class EqualTo
                : public EqualityExpression
            {
            public:
                AZ_COMPONENT(EqualTo, "{02A3A3E6-9D80-432B-8AF5-F3AF24CF6959}", EqualityExpression);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<EqualTo, EqualityExpression>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<EqualTo>("Equal To (==)", "Checks if Value A and Value B are equal to each other")
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
                    ComparisonOutcome result(lhs == rhs);
                    
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
            class EqualToGeneric
                : public BinaryOperatorGeneric<EqualToGeneric, ComparisonOperator<OperatorType::Equal>>
            {
            public:
                using BaseType = BinaryOperatorGeneric<EqualToGeneric, ComparisonOperator<OperatorType::Equal>>;
                AZ_COMPONENT(EqualTo, "{68775617-0C31-4E25-8C5D-362B152FA2FD}", BaseType);

                static const char* GetOperatorName() { return "EqualTo"; }
                static const char* GetOperatorDesc() { return "Compares two values for equality"; }
                static const char* GetIconPath() { return "Editor/Icons/ScriptCanvas/Placeholder.png"; } // TODO: Get final icon image
            };
#endif // defined(EXPRESSION_TEMPLATES_ENABLED)        
        }
    }
}

