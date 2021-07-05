/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ErrorHandler.h"

#include <ScriptCanvas/Core/Contracts/ConnectionLimitContract.h>
#include <ScriptCanvas/Core/Contracts/ContractRTTI.h>
#include <ScriptCanvas/Core/PureData.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            const char* ErrorHandler::k_sourceName = "Source";
            
            void ErrorHandler::OnInit()
            {
                {
                    ExecutionSlotConfiguration slotConfiguration("Out", ConnectionType::Output);
                    AddSlot(slotConfiguration);
                }
                
                {
                    DynamicDataSlotConfiguration slotConfiguration;

                    slotConfiguration.m_name = k_sourceName;                    
                    slotConfiguration.SetConnectionType(ConnectionType::Input);

                    slotConfiguration.m_dynamicDataType = DynamicDataType::Any;

                    // \todo split node abstractions into executable and !executable
                    AZStd::vector<AZ::Uuid> forbiddenTypes{ azrtti_typeid<PureData>() };
                    auto func = [forbiddenTypes]() { return aznew ContractRTTI(forbiddenTypes, ContractRTTI::Flags::Exclusive); };
                    ContractDescriptor descriptor{ AZStd::move(func) };

                    slotConfiguration.m_contractDescs.emplace_back(descriptor);

                    AddSlot(slotConfiguration);
                }                
            }
            
            AZStd::vector<AZStd::pair<Node*, const SlotId>> ErrorHandler::GetSources() const
            {
                return ModConnectedNodes(*GetSlot(GetSlotId(k_sourceName)));
            }

            void ErrorHandler::Reflect(AZ::ReflectContext* reflectContext)
            {
                if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                {
                    serializeContext->Class<ErrorHandler, Node>()
                        ->Version(0)
                        ;

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<ErrorHandler>("Error Handler", "")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Category, "Utilities/Debug")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/ErrorHandler.png")
                            ;
                    }
                }
            }

        } 
    } 
} 
