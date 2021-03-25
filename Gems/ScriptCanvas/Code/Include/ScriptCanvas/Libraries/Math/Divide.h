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

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Libraries/Core/BinaryOperator.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Math
        {
            class Divide
                : public ArithmeticExpression
            {
            public:
                AZ_COMPONENT(Divide, "{7379D5B4-787B-4C46-9394-288F16E5BF3A}", ArithmeticExpression);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Divide, ArithmeticExpression>()
                            ->Version(0)
                            ->Attribute(AZ::Script::Attributes::Deprecated, true)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Divide>("Divide", "Divide")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "This node is deprecated use the Divide (/) node instead, it provides contextual type and slot configurations.")
                                    ->Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "DeprecatedNodeTitlePalette")
                                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                                    ->Attribute(AZ::Edit::Attributes::Category, "Math/Number/Deprecated")
                                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Placeholder.png")
                                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                                ;
                        }
                    }
                }

                ScriptCanvas::NodeConfiguration GetReplacementNodeConfiguration() const override
                {
                    ScriptCanvas::NodeConfiguration nodeConfig{};
                    nodeConfig.m_type = AZ::Uuid("DC17E19F-3829-410D-9A0B-AD60C6066DAA");
                    return nodeConfig;
                }

            protected:
                Datum Evaluate(const Datum& lhs, const Datum& rhs) override
                {
                    const Data::NumberType lhsValue = *lhs.GetAs<Data::NumberType>();
                    const Data::NumberType rhsValue = *rhs.GetAs<Data::NumberType>();

                    if (AZ::IsClose(rhsValue, 0.0, 0.0001))
                    {
                        SCRIPTCANVAS_REPORT_ERROR((*this), "Divide by zero");
                        return Datum();
                    }

                    return Datum(lhsValue / rhsValue);
                }
            };

        }
    }
}

