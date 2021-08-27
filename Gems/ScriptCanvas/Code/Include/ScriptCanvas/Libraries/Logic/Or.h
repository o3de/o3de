/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Libraries/Core/BinaryOperator.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            class Or
                : public BooleanExpression
            {
            public:
                AZ_COMPONENT(Or, "{363F9994-8D55-4117-BE94-EFF536BDAC17}", BooleanExpression);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Or, BooleanExpression>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Or>("Or", "An execution flow gate that continues True if either Boolean A or Boolean B are True, otherwise continues False")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Placeholder.png")
                                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                                ;
                        }
                    }
                }

                //////////////////////////////////////////////////////////////////////////
                // Translation
                bool IsLogicalOR() const override
                {
                    return true;
                }
                // Translation
                //////////////////////////////////////////////////////////////////////////

            protected:
                void InitializeBooleanExpression() override
                {
                    {
                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = k_lhsName;
                        slotConfiguration.SetType(Data::Type::Boolean());
                        slotConfiguration.SetConnectionType(ConnectionType::Input);

                        AddSlot(slotConfiguration);
                    }

                    {
                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = k_rhsName;
                        slotConfiguration.SetType(Data::Type::Boolean());
                        slotConfiguration.SetConnectionType(ConnectionType::Input);

                        AddSlot(slotConfiguration);
                    }
                }
            };

        }
    }
}


