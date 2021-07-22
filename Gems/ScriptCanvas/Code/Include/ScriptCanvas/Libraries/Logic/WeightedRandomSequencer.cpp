/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WeightedRandomSequencer.h"

#include <AzCore/std/string/string.h>

#include <Include/ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

#include <Include/ScriptCanvas/Debugger/ValidationEvents/DataValidation/InvalidRandomSignalEvent.h>

namespace ScriptCanvas
{
    namespace Nodes
    {

        namespace Logic
        {
            ////////////////////////////
            // WeightedRandomSequencer
            ////////////////////////////


            AZ::Outcome<DependencyReport, void> WeightedRandomSequencer::GetDependencies() const
            {
                return AZ::Success(DependencyReport());
            }

            ConstSlotsOutcome WeightedRandomSequencer::GetSlotsInExecutionThreadByTypeImpl(const Slot&, CombinedSlotType targetSlotType, const Slot* /*executionChildSlot*/) const
            {
                return AZ::Success(GetSlotsByType(targetSlotType));
            }

            void WeightedRandomSequencer::ReflectDataTypes(AZ::ReflectContext* reflectContext)
            {
                if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                {
                    if (serializeContext)
                    {
                        serializeContext->Class<WeightedPairing>()
                            ->Version(1)
                            ->Field("WeightSlotId", &WeightedPairing::m_weightSlotId)
                            ->Field("ExecutionSlotId", &WeightedPairing::m_executionSlotId)
                            ;
                    }
                }
            }

            void WeightedRandomSequencer::OnInit()
            {
                for (const WeightedPairing& weightedPairing : m_weightedPairings)
                {
                    EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), weightedPairing.m_weightSlotId });
                    EndpointNotificationBus::MultiHandler::BusConnect({ GetEntityId(), weightedPairing.m_executionSlotId });
                }

                // We always want at least one weighted transition state
                if (m_weightedPairings.empty())
                {
                    AddWeightedPair();
                }
            }

            void WeightedRandomSequencer::ConfigureVisualExtensions()
            {
                {
                    VisualExtensionSlotConfiguration visualExtension(VisualExtensionSlotConfiguration::VisualExtensionType::ExtenderSlot);

                    visualExtension.m_name = "Add State";
                    visualExtension.m_tooltip = "Adds a new weighted state to the node.";

                    visualExtension.m_connectionType = ConnectionType::Input;
                    visualExtension.m_displayGroup = GetDisplayGroup();
                    visualExtension.m_identifier = GetWeightExtensionId();

                    RegisterExtension(visualExtension);
                }

                {
                    VisualExtensionSlotConfiguration visualExtension(VisualExtensionSlotConfiguration::VisualExtensionType::ExtenderSlot);

                    visualExtension.m_name = "Add State";
                    visualExtension.m_tooltip = "Adds a new weighted state to the node.";

                    visualExtension.m_connectionType = ConnectionType::Output;
                    visualExtension.m_displayGroup = GetDisplayGroup();
                    visualExtension.m_identifier = GetExecutionExtensionId();

                    RegisterExtension(visualExtension);
                }
            }

            bool WeightedRandomSequencer::OnValidateNode(ValidationResults& validationResults)
            {
                bool isValid = false;

                for (const Slot& slot : GetSlots())
                {
                    if (!slot.IsData())
                    {
                        continue;
                    }

                    if (slot.IsConnected())
                    {
                        isValid = true;
                        break;
                    }

                    const Datum* datum = FindDatum(slot.GetId());

                    const ScriptCanvas::Data::NumberType* numberType = datum->GetAs<ScriptCanvas::Data::NumberType>();

                    if (numberType && !AZ::IsClose((*numberType), 0.0, DBL_EPSILON))
                    {
                        isValid = true;
                        break;
                    }
                }

                if (!isValid)
                {
                    InvalidRandomSignalEvent* invalidRandomSignalEvent = aznew InvalidRandomSignalEvent(GetEntityId());
                    validationResults.AddValidationEvent(invalidRandomSignalEvent);
                }

                return isValid;
            }

            SlotId WeightedRandomSequencer::HandleExtension(AZ::Crc32 extensionId)
            {
                auto weightedPairing = AddWeightedPair();

                if (extensionId == GetWeightExtensionId())
                {
                    return weightedPairing.m_weightSlotId;
                }
                else if (extensionId == GetExecutionExtensionId())
                {
                    return weightedPairing.m_executionSlotId;
                }

                return SlotId();
            }

            bool WeightedRandomSequencer::CanDeleteSlot([[maybe_unused]] const SlotId& slotId) const
            {
                return m_weightedPairings.size() > 1;
            }

            void WeightedRandomSequencer::OnSlotRemoved(const SlotId& slotId)
            {
                RemoveWeightedPair(slotId);
                FixupStateNames();
            }

            void WeightedRandomSequencer::RemoveWeightedPair(SlotId slotId)
            {
                for (auto pairIter = m_weightedPairings.begin(); pairIter != m_weightedPairings.end(); ++pairIter)
                {

                    if (pairIter->m_executionSlotId == slotId
                        || pairIter->m_weightSlotId == slotId)
                    {
                        SlotId executionSlot = pairIter->m_executionSlotId;
                        SlotId weightSlot = pairIter->m_weightSlotId;

                        m_weightedPairings.erase(pairIter);

                        if (slotId == executionSlot)
                        {
                            RemoveSlot(weightSlot);
                        }
                        else if (slotId == weightSlot)
                        {
                            RemoveSlot(executionSlot);
                        }

                        break;
                    }
                }

                FixupStateNames();
            }

            bool WeightedRandomSequencer::AllWeightsFilled() const
            {
                bool isFilled = true;
                for (const WeightedPairing& weightedPairing : m_weightedPairings)
                {
                    if (!IsConnected(weightedPairing.m_weightSlotId))
                    {
                        isFilled = false;
                        break;
                    }
                }

                return isFilled;
            }

            bool WeightedRandomSequencer::HasExcessEndpoints() const
            {
                bool hasExcess = false;
                bool hasEmpty = false;

                for (const WeightedPairing& weightedPairing : m_weightedPairings)
                {
                    if (!IsConnected(weightedPairing.m_weightSlotId) && !IsConnected(weightedPairing.m_executionSlotId))
                    {
                        if (hasEmpty)
                        {
                            hasExcess = true;
                            break;
                        }
                        else
                        {
                            hasEmpty = true;
                        }
                    }
                }

                return hasExcess;
            }

            WeightedRandomSequencer::WeightedPairing WeightedRandomSequencer::AddWeightedPair()
            {
                int counterWeight = static_cast<int>(m_weightedPairings.size()) + 1;

                WeightedPairing weightedPairing;

                DataSlotConfiguration dataSlotConfiguration;

                dataSlotConfiguration.SetConnectionType(ConnectionType::Input);
                dataSlotConfiguration.m_name = GenerateDataName(counterWeight);
                dataSlotConfiguration.m_toolTip = "The weight associated with the execution state.";
                dataSlotConfiguration.m_addUniqueSlotByNameAndType = false;
                dataSlotConfiguration.SetType(Data::Type::Number());
                dataSlotConfiguration.SetDefaultValue(1.0f);

                dataSlotConfiguration.m_displayGroup = GetDisplayGroup();

                weightedPairing.m_weightSlotId = AddSlot(dataSlotConfiguration);

                ExecutionSlotConfiguration  slotConfiguration;

                slotConfiguration.m_name = GenerateOutName(counterWeight);
                slotConfiguration.m_addUniqueSlotByNameAndType = false;
                slotConfiguration.SetConnectionType(ConnectionType::Output);

                slotConfiguration.m_displayGroup = GetDisplayGroup();

                weightedPairing.m_executionSlotId = AddSlot(slotConfiguration);

                m_weightedPairings.push_back(weightedPairing);

                return weightedPairing;
            }

            void WeightedRandomSequencer::FixupStateNames()
            {
                int counter = 1;
                for (const WeightedPairing& weightedPairing : m_weightedPairings)
                {
                    AZStd::string dataName = GenerateDataName(counter);
                    AZStd::string executionName = GenerateOutName(counter);

                    Slot* dataSlot = GetSlot(weightedPairing.m_weightSlotId);

                    if (dataSlot)
                    {
                        dataSlot->Rename(dataName);
                    }

                    Slot* executionSlot = GetSlot(weightedPairing.m_executionSlotId);

                    if (executionSlot)
                    {
                        executionSlot->Rename(executionName);
                    }

                    ++counter;
                }
            }

            AZStd::string WeightedRandomSequencer::GenerateDataName(int counter)
            {
                return AZStd::string::format("Weight %i", counter);
            }

            AZStd::string WeightedRandomSequencer::GenerateOutName(int counter)
            {
                return AZStd::string::format("Out %i", counter);
            }
        }
    }
}
