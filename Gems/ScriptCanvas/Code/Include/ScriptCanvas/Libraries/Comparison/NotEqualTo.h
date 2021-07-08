/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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


