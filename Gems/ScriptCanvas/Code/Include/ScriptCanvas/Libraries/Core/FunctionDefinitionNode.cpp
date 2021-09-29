/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FunctionDefinitionNode.h"
#include <ScriptCanvas/Core/GraphBus.h>

#include <ScriptCanvas/Core/Contracts/DisallowReentrantExecutionContract.h>
#include <ScriptCanvas/Core/Contracts/DisplayGroupConnectedSlotLimitContract.h>
#include <ScriptCanvas/Libraries/Core/FunctionBus.h>

#include <ScriptCanvas/Debugger/ValidationEvents/DataValidation/InvalidPropertyEvent.h>
#include <AzCore/std/string/regex.h>

namespace FunctionDefinitionNodeCpp
{
    void VersionUpdateRemoveDefaultDisplayGroup(ScriptCanvas::Nodes::Core::FunctionDefinitionNode& node)
    {
        using namespace ScriptCanvas;
        using namespace ScriptCanvas::Nodes::Core;

        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        if (serializeContext)
        {
            const auto& classData = serializeContext->FindClassData(azrtti_typeid<FunctionDefinitionNode>());
            if (classData && classData->m_version < FunctionDefinitionNode::NodeVersion::RemoveDefaultDisplayGroup)
            {
                for (auto& slot : node.ModAllSlots())
                {
                    if (slot->GetType() == CombinedSlotType::DataIn || slot->GetType() == CombinedSlotType::DataOut)
                    {
                        slot->ClearDynamicGroup();
                    }
                }
            }
        }
    }
}

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

            bool FunctionDefinitionNode::IsExecutionEntry() const
            {
                return m_isExecutionEntry;
            }

            bool FunctionDefinitionNode::IsExecutionExit() const
            {
                return !m_isExecutionEntry;
            }

            void FunctionDefinitionNode::OnConfigured()
            {
                SetupSlots();
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

            const Slot* FunctionDefinitionNode::GetEntrySlot() const
            {
                if (IsExecutionExit())
                {
                    return nullptr;
                }

                auto slots = GetAllSlotsByDescriptor(ScriptCanvas::SlotDescriptors::ExecutionOut());
                if (slots.size() != 1 || slots.front() == nullptr)
                {
                    AZ_Warning("ScriptCanvas", false, "FunctionDefinitionNode did not have a required, single Out slot.");
                    return nullptr;
                }

                return slots.front();
            }

            const Slot* FunctionDefinitionNode::GetExitSlot() const
            {
                if (IsExecutionEntry())
                {
                    return nullptr;
                }

                auto slots = GetAllSlotsByDescriptor(ScriptCanvas::SlotDescriptors::ExecutionIn());
                if (slots.size() != 1 || slots.front() == nullptr)
                {
                    AZ_Warning("ScriptCanvas", false, "FunctionDefinitionNode did not have a required, single In slot.");
                    return nullptr;
                }

                return slots.front();
            }

            void FunctionDefinitionNode::MarkExecutionExit()
            {
                m_isExecutionEntry = false;
            }

            void FunctionDefinitionNode::OnDisplayNameChanged()
            {
                if (!IsValidDisplayName())
                {
                    GetGraph()->ReportError((*this), "Parse Error", GenerateErrorMessage());
                }
            }

            void FunctionDefinitionNode::OnInit()
            {
                Nodeling::OnInit();
                FunctionDefinitionNodeCpp::VersionUpdateRemoveDefaultDisplayGroup(*this);
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
                                []()
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

            SlotId FunctionDefinitionNode::CreateDataSlot(AZStd::string_view name, AZStd::string_view toolTip, ConnectionType connectionType)
            {
                DynamicDataSlotConfiguration slotConfiguration;

                slotConfiguration.m_name = name;
                slotConfiguration.m_toolTip = toolTip;
                slotConfiguration.SetConnectionType(connectionType);

                slotConfiguration.m_displayGroup = GetDataDisplayGroup();
                slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
                slotConfiguration.m_isUserAdded = true;

                slotConfiguration.m_addUniqueSlotByNameAndType = false;

                SlotId slotId = AddSlot(slotConfiguration);

                return slotId;
            }

            SlotId FunctionDefinitionNode::HandleExtension(AZ::Crc32 extensionId)
            {
                if (extensionId == GetAddNodelingInputDataSlot())
                {
                    SlotId retVal = CreateDataSlot("Value", "", ConnectionType::Output);
                    return retVal;
                }

                if (extensionId == GetAddNodelingOutputDataSlot())
                {
                    SlotId retVal = CreateDataSlot("Value", "", ConnectionType::Input);
                    return retVal;
                }

                return SlotId();
            }

            void FunctionDefinitionNode::OnSetup()
            {
                m_configureVisualExtensions = true;
                SetupSlots();
            }

            void FunctionDefinitionNode::ConfigureVisualExtensions()
            {
                {
                    VisualExtensionSlotConfiguration visualExtensions(VisualExtensionSlotConfiguration::VisualExtensionType::ExtenderSlot);

                    visualExtensions.m_name = "Add Data Input";
                    visualExtensions.m_tooltip = "Adds a new operand for the operator";

                    visualExtensions.m_displayGroup = GetDataDisplayGroup();
                    visualExtensions.m_identifier = GetAddNodelingInputDataSlot();

                    visualExtensions.m_connectionType = ConnectionType::Output;

                    RegisterExtension(visualExtensions);
                }

                {
                    VisualExtensionSlotConfiguration visualExtensions(VisualExtensionSlotConfiguration::VisualExtensionType::ExtenderSlot);

                    visualExtensions.m_name = "Add Data Output";
                    visualExtensions.m_tooltip = "";

                    visualExtensions.m_displayGroup = GetDataDisplayGroup();
                    visualExtensions.m_identifier = GetAddNodelingOutputDataSlot();

                    visualExtensions.m_connectionType = ConnectionType::Input;

                    RegisterExtension(visualExtensions);
                }
            }

            bool FunctionDefinitionNode::CanDeleteSlot(const SlotId&) const
            {
                // Allow slots to be deleted by users
                return true;
            }
        }
    }
}
