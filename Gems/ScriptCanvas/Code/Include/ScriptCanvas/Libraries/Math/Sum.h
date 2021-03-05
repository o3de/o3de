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

#include <AzCore/Script/ScriptContextAttributes.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Math
        {
            class Sum
                : public ArithmeticExpression
            {
            public:
                AZ_COMPONENT(Sum, "{6C52B2D1-3526-4855-A217-5106D54F6B90}", ArithmeticExpression);

                static void Reflect(AZ::ReflectContext* reflection)
                { 
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Sum, ArithmeticExpression>()
                            ->Version(0)
                            ->Attribute(AZ::Script::Attributes::Deprecated, true)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Sum>("Add", "Add")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "This node is deprecated use the Add (+) node instead, it provides contextual type and slot configurations.")
                                    ->Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "DeprecatedNodeTitlePalette")
                                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                                    ->Attribute(AZ::Edit::Attributes::Category, "Math/Number/Deprecated")
                                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Add.png")
                                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                                ;
                        }
                    }
                }
                
            protected:
                Datum Evaluate(const Datum& lhs, const Datum& rhs) override
                {
                    return Datum(*lhs.GetAs<Data::NumberType>() + *rhs.GetAs<Data::NumberType>());
                }
            };
        }
    }
}
