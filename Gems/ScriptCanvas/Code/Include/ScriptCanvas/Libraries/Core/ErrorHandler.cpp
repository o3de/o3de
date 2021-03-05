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
                                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/ErrorHandler.png")
                            ;
                    }
                }
            }

        } // namespace Core
    } // namespace Nodes
} // namespace ScriptCanvas
