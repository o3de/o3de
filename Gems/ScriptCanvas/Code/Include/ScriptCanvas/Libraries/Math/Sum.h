/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Libraries/Core/BinaryOperator.h>

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
                                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Add.png")
                                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                                ;
                        }
                    }
                }

                ScriptCanvas::NodeConfiguration GetReplacementNodeConfiguration() const override
                {
                    ScriptCanvas::NodeConfiguration nodeConfig{};
                    nodeConfig.m_type = AZ::Uuid("C1B42FEC-0545-4511-9FAC-11E0387FEDF0");
                    return nodeConfig;
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
