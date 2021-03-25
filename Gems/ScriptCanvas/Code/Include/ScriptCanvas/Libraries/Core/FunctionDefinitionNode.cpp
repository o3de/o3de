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

#include "FunctionDefinitionNode.h"
#include <ScriptCanvas/Core/GraphBus.h>

#include <ScriptCanvas/Core/Contracts/DisallowReentrantExecutionContract.h>
#include <ScriptCanvas/Core/Contracts/DisplayGroupConnectedSlotLimitContract.h>
#include <ScriptCanvas/Libraries/Core/FunctionBus.h>

#include <ScriptCanvas/Debugger/ValidationEvents/DataValidation/InvalidPropertyEvent.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            //////////////////////
            // FunctionDefinitionNode
            //////////////////////

            constexpr const char* s_FunctionDefinitionNodeNameRegex = "^[A-Z|a-z][A-Z|a-z| |0-9|:|_]*";

            bool FunctionDefinitionNode::IsEntryPoint() const
            {
                return m_externalConnectionType == ConnectionType::Input;
            }

            void FunctionDefinitionNode::OnConfigured()
            {
                SetupSlots();
            }

            void FunctionDefinitionNode::OnActivate()
            {
                Nodeling::OnActivate();
            }

            void FunctionDefinitionNode::OnInputSignal(const SlotId&)
            {

            }

            bool FunctionDefinitionNode::OnValidateNode(ValidationResults& validationResults)
            {
                if (!IsValidDisplayName())
                {
                    InvalidPropertyEvent* invalidPropertyEvent = aznew InvalidPropertyEvent(GetEntityId(), GenerateErrorMessage());
                    invalidPropertyEvent->SetTooltip("Execution Nodeling has an invalid value for Display Name.");

                    validationResults.AddValidationEvent(invalidPropertyEvent);

                    return false;
                }

                return true;
            }

            AZ::Outcome<DependencyReport, void> FunctionDefinitionNode::GetDependencies() const
            {
                return AZ::Success(DependencyReport{});
            }

            AZStd::vector<const ScriptCanvas::Slot*> FunctionDefinitionNode::GetEntrySlots() const
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

            void FunctionDefinitionNode::OnEndpointConnected(const ScriptCanvas::Endpoint& endpoint)
            {
                Nodeling::OnEndpointConnected(endpoint);

                ConfigureExternalConnectionType();
            }

            void FunctionDefinitionNode::OnEndpointDisconnected(const ScriptCanvas::Endpoint& endpoint)
            {
                Nodeling::OnEndpointDisconnected(endpoint);

                ConfigureExternalConnectionType();
            }

            void FunctionDefinitionNode::SignalEntrySlots()
            {
                for (const Slot* slot : m_entrySlots)
                {
                    SignalOutput(slot->GetId());
                }
            }

            void FunctionDefinitionNode::OnDisplayNameChanged()
            {
                if (!IsValidDisplayName())
                {
                    GetGraph()->ReportError((*this), "Parse Error", GenerateErrorMessage());
                }
            }

            void FunctionDefinitionNode::ConfigureExternalConnectionType()
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

            void FunctionDefinitionNode::SetupSlots()
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
                            },
                            {
                                [this]()
                                {
                                    DisallowReentrantExecutionContract* reentrantContract = aznew DisallowReentrantExecutionContract();
                                    return reentrantContract;
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

            bool FunctionDefinitionNode::IsValidDisplayName() const
            {
                static AZStd::regex executionNameRegex = AZStd::regex(s_FunctionDefinitionNodeNameRegex);
                return AZStd::regex_match(GetDisplayName(), executionNameRegex);
            }

            AZStd::string FunctionDefinitionNode::GenerateErrorMessage() const
            {
                static AZStd::regex executionNameRegex = AZStd::regex(s_FunctionDefinitionNodeNameRegex);

                AZStd::string displayName = GetDisplayName();
                size_t originalLength = displayName.size();

                if (originalLength == 0)
                {
                    return "Execution Nodeling cannot have an empty display name";
                }

                displayName = AZStd::regex_replace(displayName, executionNameRegex, R"()");

                size_t offset = originalLength - displayName.size();

                return AZStd::string::format("Found invalid character %c in display name", GetDisplayName()[offset]);
            }
        }
    }
}
