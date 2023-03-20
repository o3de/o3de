/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Placeholder.png")
                                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                                ;
                        }
                    }
                }
            };

        }
    }
}

