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

#include "ExecutionNode.h"
#include <ScriptCanvas/Core/GraphBus.h>

#include <ScriptCanvas/Core/Contracts/DisplayGroupConnectedSlotLimitContract.h>
#include <ScriptCanvas/Libraries/Core/FunctionBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            namespace Internal
            {
                /////////////
                // Nodeling
                /////////////

                Nodeling::Nodeling()
                    : m_identifier(AZ::Uuid::CreateRandom())
                    , m_displayName(" ")
                {
                }

                void Nodeling::OnInit()
                {
                    m_displayNameInterface.SetPropertyReference(&m_displayName);
                    m_displayNameInterface.RegisterListener(this);

                    if (GetOwningScriptCanvasId().IsValid())
                    {
                        NodelingRequestBus::Handler::BusConnect(GetScopedNodeId());
                    }

                    m_previousName = m_displayName;
                }

                void Nodeling::OnGraphSet()
                {
                    if (GetEntity() != nullptr)
                    {
                        NodelingRequestBus::Handler::BusConnect(GetScopedNodeId());
                    }
                }
                
                void Nodeling::ConfigureVisualExtensions()
                {
                    {
                        VisualExtensionSlotConfiguration visualExtensions(VisualExtensionSlotConfiguration::VisualExtensionType::PropertySlot);

                        visualExtensions.m_name = "";
                        visualExtensions.m_tooltip = "";

                        // Should be centered. But we'll sort that out in specialized display pass.
                        visualExtensions.m_connectionType = ConnectionType::Input;

                        visualExtensions.m_identifier = GetPropertyId();

                        RegisterExtension(visualExtensions);
                    }
                }

                NodePropertyInterface* Nodeling::GetPropertyInterface(AZ::Crc32 propertyId)
                {
                    if (propertyId == GetPropertyId())
                    {
                        return &m_displayNameInterface;
                    }

                    return nullptr;
                }

                void Nodeling::SetDisplayName(const AZStd::string& displayName)
                {
                    m_displayName = displayName;

                    m_displayNameInterface.SignalDataChanged();                    
                }

                void Nodeling::OnPropertyChanged()
                {
                    if (m_displayName.empty())
                    {
                        m_displayName = m_previousName;

                        if (!m_previousName.empty())
                        {
                            m_displayNameInterface.SignalDataChanged();
                        }

                        return;
                    }
                    else
                    {
                        m_previousName = m_displayName;
                    }

                    NodelingNotificationBus::Event(GetScopedNodeId(), &NodelingNotifications::OnNameChanged, m_displayName);
                }

                void Nodeling::RemapId()
                {
                    m_identifier = AZ::Uuid::CreateRandom();
                }
            }

            //////////////////////
            // ExecutionNodeling
            //////////////////////

            bool ExecutionNodeling::IsEntryPoint() const
            {
                return m_externalConnectionType == ConnectionType::Input;
            }

            void ExecutionNodeling::OnConfigured()
            {
                SetupSlots();
            }

            void ExecutionNodeling::OnActivate()
            {
                Nodeling::OnActivate();

                if (GetExecutionType() == ExecutionType::Runtime)
                {
                    if (IsEntryPoint())
                    {
                        m_entrySlots = GetAllSlotsByDescriptor(ScriptCanvas::SlotDescriptors::ExecutionOut());
                    }
                }
            }

            void ExecutionNodeling::OnInputSignal(const SlotId& slotId)
            {
                auto scriptCanvasId = GetOwningScriptCanvasId();
                FunctionRequestBus::Event(scriptCanvasId, &FunctionRequests::OnSignalOut, GetEntityId(), slotId);
            }

            AZStd::vector<const ScriptCanvas::Slot*> ExecutionNodeling::GetEntrySlots() const
            {
                if (GetExecutionType() == ExecutionType::Runtime)
                {
                    return m_entrySlots;
                }
                else
                {
                    if (IsEntryPoint())
                    {
                        return GetAllSlotsByDescriptor(ScriptCanvas::SlotDescriptors::ExecutionOut());
                    }
                    else
                    {
                        return GetAllSlotsByDescriptor(ScriptCanvas::SlotDescriptors::ExecutionIn());
                    }
                }
            }

            void ExecutionNodeling::OnEndpointConnected(const ScriptCanvas::Endpoint& endpoint)
            {
                Nodeling::OnEndpointConnected(endpoint);

                ConfigureExternalConnectionType();
            }

            void ExecutionNodeling::OnEndpointDisconnected(const ScriptCanvas::Endpoint& endpoint)
            {
                Nodeling::OnEndpointDisconnected(endpoint);

                ConfigureExternalConnectionType();
            }

            void ExecutionNodeling::SignalEntrySlots()
            {
                for (const Slot* slot : m_entrySlots)
                {
                    SignalOutput(slot->GetId());
                }
            }

            void ExecutionNodeling::ConfigureExternalConnectionType()
            {
                // Invert the logic. If we have a connection to an in, we are an output for the function
                if (HasConnectionForDescriptor(ScriptCanvas::SlotDescriptors::ExecutionIn()))
                {
                    m_externalConnectionType = ConnectionType::Output;
                }
                else if (HasConnectionForDescriptor(ScriptCanvas::SlotDescriptors::ExecutionOut()))
                {
                    m_externalConnectionType = ConnectionType::Input;
                }
                else
                {
                    m_externalConnectionType = ConnectionType::Unknown;
                }
            }

            void ExecutionNodeling::SetupSlots()
            {
                auto groupedSlots = GetSlotsWithDisplayGroup(GetSlotDisplayGroup());

                if (groupedSlots.empty())
                {
                    {
                        ExecutionSlotConfiguration slotConfiguration;
                        slotConfiguration.SetConnectionType(ConnectionType::Input);
                        slotConfiguration.m_displayGroup = GetSlotDisplayGroup();
                        slotConfiguration.m_name = " ";
                        slotConfiguration.m_addUniqueSlotByNameAndType = false;

                        slotConfiguration.m_contractDescs = 
                        { 
                            { 
                                [this]() 
                                { 
                                    DisplayGroupConnectedSlotLimitContract* limitContract = aznew DisplayGroupConnectedSlotLimitContract(GetSlotDisplayGroup(), 1);

                                    limitContract->SetCustomErrorMessage("Execution nodes can only be connected to either the Input or Output, and not both at the same time.");
                                    return limitContract;
                                } 
                            }
                        };

                        AddSlot(slotConfiguration);
                    }

                    {
                        ExecutionSlotConfiguration slotConfiguration;
                        slotConfiguration.SetConnectionType(ConnectionType::Output);
                        slotConfiguration.m_displayGroup = GetSlotDisplayGroup();
                        slotConfiguration.m_name = " ";
                        slotConfiguration.m_addUniqueSlotByNameAndType = false;

                        slotConfiguration.m_contractDescs = 
                        { 
                            { 
                                [this]() 
                                { 
                                    DisplayGroupConnectedSlotLimitContract* limitContract = aznew DisplayGroupConnectedSlotLimitContract(GetSlotDisplayGroup(), 1);

                                    limitContract->SetCustomErrorMessage("Execution nodes can only be connected to either the Input or Output, and not both at the same time.");
                                    return limitContract;
                                } 
                            }
                        };

                        AddSlot(slotConfiguration);
                    }
                }
            }
        }
    }
}
