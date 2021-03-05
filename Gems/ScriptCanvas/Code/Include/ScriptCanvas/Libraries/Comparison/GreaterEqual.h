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
            class GreaterEqual
                : public ComparisonExpression
            {
            public:
                AZ_COMPONENT(GreaterEqual, "{8CA0C442-9139-4180-96EC-300FF888C35A}", ComparisonExpression);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<GreaterEqual, ComparisonExpression>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<GreaterEqual>("Greater Than or Equal To (>=)", "Checks if Value A is greater than or equal to Value B")
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
                    ComparisonOutcome result(lhs >= rhs);
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
            class GreaterEqualGeneric
                : public BinaryOperatorGeneric<GreaterEqualGeneric, ComparisonOperator<OperatorType::GreaterEqual>>
            {
            public:
                using BaseType = BinaryOperatorGeneric<GreaterEqualGeneric, ComparisonOperator<OperatorType::GreaterEqual>>;
                AZ_COMPONENT(GreaterEqual, "{974AC852-E75E-40D5-B1A1-0D70FFE3D608}", BaseType);

                static const char* GetOperatorName() { return "Greater or Equal"; }
                static const char* GetOperatorDesc() { return "Compares if first value is greater than or equal to second value"; }
                static const char* GetIconPath() { return "Editor/Icons/ScriptCanvas/Placeholder.png"; } // TODO: Get final icon image
            };
#endif // #if defined(EXPRESSION_TEMPLATES_ENABLED)
        }
    }
}


