/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Error.h"

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Contracts/ContractRTTI.h>
#include <ScriptCanvas/Core/Contracts/ConnectionLimitContract.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            void Error::OnInit()
            {
                {
                    ExecutionSlotConfiguration slotConfiguration("In", ConnectionType::Input);
                    AddSlot(slotConfiguration);
                }

                {
                    DynamicDataSlotConfiguration slotConfiguration;

                    slotConfiguration.m_name = "This";
                    slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
                    slotConfiguration.SetConnectionType(ConnectionType::Output);

                    AddSlot(slotConfiguration); // \todo for testing only, we need to ability to arbitrarily connect nodes themeselve to slots (as input to function call or error handling) or directly (as the flow of execution with arrows if easier to read...)
                }

                {
                    DataSlotConfiguration slotConfiguration;

                    slotConfiguration.m_name = "Description";
                    slotConfiguration.SetConnectionType(ConnectionType::Input);
                    slotConfiguration.ConfigureDatum(AZStd::move(Datum(AZStd::move(Data::Type::String()), Datum::eOriginality::Copy)));

                    AddSlot(slotConfiguration);
                }
            }

            void Error::OnInputSignal(const SlotId&)
            {
                const Datum* input = FindDatum(GetSlotId("Description"));
                if (const Data::StringType* desc = input ? input->GetAs<Data::StringType>() : nullptr)
                {
                    SCRIPTCANVAS_REPORT_ERROR((*this), desc->data());
                }
                else
                {
                    // \todo get a more descriptive default error report
                    SCRIPTCANVAS_REPORT_ERROR((*this), "Undescribed error");
                }                
            }

            void Error::Reflect(AZ::ReflectContext* reflectContext)
            {
                if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                {
                    serializeContext->Class<Error, Node>()
                        ->Version(0)
                        ;

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<Error>("Error", "")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Category, "Utilities/Debug")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Error.png")
                                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ;
                    }
                }
            }

        } 
    } 
} 
