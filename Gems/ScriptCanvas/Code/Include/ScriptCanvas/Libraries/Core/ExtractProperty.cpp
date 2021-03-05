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
#include <ScriptCanvas/Libraries/Core/ExtractProperty.h>
#include <ScriptCanvas/Libraries/Core/BehaviorContextObjectNode.h>

#include <Libraries/Core/MethodUtility.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            bool ExtractProperty::VersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
            {
                if (rootElement.GetVersion() < 1)
                {
                    SlotMetadata metadata;
                    rootElement.FindSubElementAndGetData(AZ_CRC("m_sourceAccount", 0x25f29920), metadata);

                    rootElement.RemoveElementByName(AZ_CRC("m_sourceAccount", 0x25f29920));
                    rootElement.AddElementWithData(serializeContext, "m_dataType", metadata.m_dataType);
                }

                return true;
            }

            void ExtractProperty::OnInit()
            {
                // DYNAMIC_SLOT_VERSION_CONVERTER
                Slot* sourceSlot = ExtractPropertyProperty::GetSourceSlot(this);

                if (sourceSlot && !sourceSlot->IsDynamicSlot())
                {
                    sourceSlot->SetDynamicDataType(DynamicDataType::Value);
                }
                ////

                RefreshGetterFunctions();
            }

            void ExtractProperty::OnSlotDisplayTypeChanged(const SlotId& slotId, const Data::Type& dataType)
            {
                if (!dataType.IsValid() || dataType == m_dataType)
                {
                    return;
                }

                if (slotId == ExtractPropertyProperty::GetSourceSlotId(this))
                {
                    m_dataType = dataType;

                    ClearPropertySlots();
                    AddPropertySlots(dataType);
                }
            }
            
            Data::Type ExtractProperty::GetSourceSlotDataType() const
            {
                return m_dataType;
            }

            void ExtractProperty::OnInputSignal(const SlotId& slotID)
            {
                if (slotID == GetSlotId(GetInputSlotName()))
                {
                    if (auto input = FindDatum(ExtractPropertyProperty::GetSourceSlotId(this)))
                    {
                        for(auto&& propertyAccount : m_propertyAccounts)
                        {
                            Slot* propertySlot = GetSlot(propertyAccount.m_propertySlotId);
                            if (propertySlot && propertyAccount.m_getterFunction)
                            {
                                auto outputOutcome = propertyAccount.m_getterFunction(*input);
                                if (!outputOutcome)
                                {
                                    SCRIPTCANVAS_REPORT_ERROR((*this), outputOutcome.TakeError().data());
                                    return;
                                }
                                PushOutput(outputOutcome.TakeValue(), *propertySlot);
                            }
                        }
                    }
                    SignalOutput(GetSlotId(GetOutputSlotName()));
                }
            }

            void ExtractProperty::AddPropertySlots(const Data::Type& type)
            {
                AZStd::unordered_map<AZStd::string, SlotId> versionedInfo;
                AddPropertySlots(type, versionedInfo);
            }

            void ExtractProperty::ClearPropertySlots()
            {
                for (auto&& propertyAccount : m_propertyAccounts)
                {
                    RemoveSlot(propertyAccount.m_propertySlotId);
                }
                m_propertyAccounts.clear();
            }

            AZStd::vector<AZStd::pair<AZStd::string_view, SlotId>> ExtractProperty::GetPropertyFields() const
            {
                AZStd::vector<AZStd::pair<AZStd::string_view, SlotId>> propertyFields;
                for (auto&& propertyAccount : m_propertyAccounts)
                {
                    propertyFields.emplace_back(propertyAccount.m_propertyName, propertyAccount.m_propertySlotId);
                }

                return propertyFields;
            }

            bool ExtractProperty::IsOutOfDate() const
            {
                bool isOutOfDate = false;
                for (auto propertyAccount : m_propertyAccounts)
                {
                    if (!propertyAccount.m_getterFunction)
                    {
                        isOutOfDate = true;
                        break;
                    }
                }

                return isOutOfDate;
            }

            UpdateResult ExtractProperty::OnUpdateNode()
            {
                for (auto propertyAccount : m_propertyAccounts)
                {
                    if (!propertyAccount.m_getterFunction)
                    {
                        RemoveSlot(propertyAccount.m_propertySlotId);
                    }
                }

                return UpdateResult::DirtyGraph;
            }

            void ExtractProperty::RefreshGetterFunctions()
            {
                Data::Type sourceType = GetSourceSlotDataType();
                if (!sourceType.IsValid())
                {
                    return;
                }

                auto getterWrapperMap = Data::ExplodeToGetters(sourceType);

                for (auto&& propertyAccount : m_propertyAccounts)
                {
                    if (!propertyAccount.m_getterFunction)
                    {
                        auto foundPropIt = getterWrapperMap.find(propertyAccount.m_propertyName);
                        if (foundPropIt != getterWrapperMap.end() && propertyAccount.m_propertyType.IS_A(foundPropIt->second.m_propertyType))
                        {
                            propertyAccount.m_getterFunction = foundPropIt->second.m_getterFunction;
                        }
                        else
                        {
                            AZ_Error("Script Canvas", GetExecutionType() == ExecutionType::Editor, "Property (%s : %s) getter method could not be found in Data::PropertyTraits or the property type has changed."
                                " Output will not be pushed on the property's slot.",
                                propertyAccount.m_propertyName.c_str(), Data::GetName(propertyAccount.m_propertyType).data());
                        }
                    }
                }

                if (GetExecutionType() == ExecutionType::Editor)
                {
                    UpdatePropertyVersion();
                }
            }

            void ExtractProperty::UpdatePropertyVersion()
            {
                AZStd::unordered_map<AZStd::string, SlotId> previousSlots;

                {
                    auto outputSlots = GetAllSlotsByDescriptor(SlotDescriptors::DataOut());

                    for (const Slot* slot : outputSlots)
                    {
                        previousSlots[slot->GetName()] = slot->GetId();
                    }
                }

                AddPropertySlots(GetSourceSlotDataType(), previousSlots);
            }

            void ExtractProperty::AddPropertySlots(const Data::Type& dataType, const AZStd::unordered_map<AZStd::string, SlotId>& existingSlots)
            {
                Data::GetterContainer getterFunctions = Data::ExplodeToGetters(dataType);

                for (const auto& getterWrapperPair : getterFunctions)
                {
                    const AZStd::string& propertyName = getterWrapperPair.first;
                    const Data::GetterWrapper& getterWrapper = getterWrapperPair.second;

                    DataSlotConfiguration config;

                    AZStd::string slotName = AZStd::string::format("%s: %s", propertyName.data(), Data::GetName(getterWrapper.m_propertyType).data());

                    if (existingSlots.find(slotName) == existingSlots.end())
                    {
                        Data::PropertyMetadata propertyAccount;
                        propertyAccount.m_propertyType = getterWrapper.m_propertyType;
                        propertyAccount.m_propertyName = propertyName;

                        config.m_name = slotName;
                        config.m_toolTip = "";

                        config.SetType(getterWrapper.m_propertyType);
                        config.SetConnectionType(ConnectionType::Output);

                        propertyAccount.m_propertySlotId = AddSlot(config);

                        propertyAccount.m_getterFunction = getterWrapper.m_getterFunction;
                        m_propertyAccounts.push_back(propertyAccount);
                    }
                }
            }
        }
    }
}
