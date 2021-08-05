/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

