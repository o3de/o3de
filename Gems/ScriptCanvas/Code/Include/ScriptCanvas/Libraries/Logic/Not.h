/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Libraries/Core/UnaryOperator.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            class Not
                : public UnaryExpression
            {
            public:
                AZ_COMPONENT(Not, "{EF6BA813-9AF9-45CF-A8A4-7F800D7B7CB0}", UnaryExpression);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Not, UnaryExpression>()
                            ->Version(1)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Not>("Not", "An execution flow gate that continues True if the Boolean is False, otherwise continues False if the Boolean is True")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Placeholder.png")
                                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                                ;
                        }
                    }
                }

                //////////////////////////////////////////////////////////////////////////
                // Translation
                AZ::Outcome<DependencyReport, void> GetDependencies() const override
                {
                    return AZ::Success(DependencyReport{});
                }

                bool IsIfBranch() const override
                {
                    return true;
                }

                bool IsIfBranchPrefacedWithBooleanExpression() const override
                {
                    return true;
                }

                bool IsLogicalNOT() const override
                {
                    return true;
                }
                // Translation
                //////////////////////////////////////////////////////////////////////////
            };

        }
    }
}


