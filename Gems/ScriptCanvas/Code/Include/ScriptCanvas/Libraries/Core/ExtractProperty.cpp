/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Libraries/Core/ExtractProperty.h>

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
                    rootElement.FindSubElementAndGetData(AZ_CRC_CE("m_sourceAccount"), metadata);

                    rootElement.RemoveElementByName(AZ_CRC_CE("m_sourceAccount"));
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

            AZ::Outcome<DependencyReport, void> ExtractProperty::GetDependencies() const
            {
                return AZ::Success(DependencyReport::NativeLibrary(Data::GetName(m_dataType)));
            }

            PropertyFields ExtractProperty::GetPropertyFields() const
            {
                PropertyFields propertyFields;
                for (auto&& propertyAccount : m_propertyAccounts)
                {
                    propertyFields.emplace_back(propertyAccount.m_propertyName, propertyAccount.m_propertySlotId);
                }

                return propertyFields;
            }

            bool ExtractProperty::IsOutOfDate(const VersionData& graphVersion) const
            {
                AZ_UNUSED(graphVersion);

                int numOutOfDate = 0;
                for (auto propertyAccount : m_propertyAccounts)
                {
                    if (!propertyAccount.m_getterFunction)
                    {
                        // Print out the error message for each property that is out of date
                        AZ_Warning("ScriptCanvas", false,"Node '%s':  Property (%s : %s) getter method could not be found in Data::PropertyMetadata.",
                            this->GetDebugName().c_str(),
                            propertyAccount.m_propertyName.c_str(),
                            Data::GetName(propertyAccount.m_propertyType).c_str());

                        numOutOfDate++;
                    }
                }

                AZ_Error("ScriptCanvas", numOutOfDate == 0, "Node '%s':  Out of date.  (%d/%d) properties are missing a getter function.", this->GetDebugName().c_str(), numOutOfDate, m_propertyAccounts.size());
                return (numOutOfDate > 0);

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
                            AZ_Error("Script Canvas", true, "Property (%s : %s) getter method could not be found in Data::PropertyTraits or the property type has changed."
                                " Output will not be pushed on the property's slot.",
                                propertyAccount.m_propertyName.c_str(), Data::GetName(propertyAccount.m_propertyType).data());
                        }
                    }
                }

                UpdatePropertyVersion();
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
                    if (!getterWrapper.m_displayName.empty())
                    {
                        slotName = getterWrapper.m_displayName;
                    }

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
