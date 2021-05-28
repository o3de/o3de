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
#include <ScriptCanvas/Core/Contracts/TypeContract.h>
#include <ScriptCanvas/Libraries/Core/BinaryOperator.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            class And
                : public BooleanExpression
            {
            public:
                AZ_COMPONENT(And, "{4043C9B7-4ACC-42FE-9C46-EAD7BB718C99}", BooleanExpression);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<And, BooleanExpression>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<And>("And", "An execution flow gate that signals True if both Boolean A and Boolean B are True, otherwise signals False")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Placeholder.png")
                                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                                ;
                        }
                    }
                }

                //////////////////////////////////////////////////////////////////////////
                // Translation
                bool IsLogicalAND() const override
                {
                    return true;
                }
                // Translation
                //////////////////////////////////////////////////////////////////////////

            protected:
                Datum Evaluate(const Datum& lhs, const Datum& rhs) override
                {
                    return Datum(*lhs.GetAs<bool>() && *rhs.GetAs<bool>());
                }

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

