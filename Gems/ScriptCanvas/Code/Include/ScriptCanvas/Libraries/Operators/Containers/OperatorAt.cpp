/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OperatorAt.h"

#include <ScriptCanvas/Core/Contracts/SupportsMethodContract.h>
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            void OperatorAt::ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs)
            {
                if (sourceType == SourceType::SourceInput)
                {
                    ContractDescriptor supportsMethodContract;
                    supportsMethodContract.m_createFunc = []() -> SupportsMethodContract* { return aznew SupportsMethodContract("At"); };
                    contractDescs.push_back(AZStd::move(supportsMethodContract));
                }
            }

            void OperatorAt::OnSourceTypeChanged()
            {
                if (Data::IsVectorContainerType(GetSourceAZType()))
                {
                    // Add the INDEX as the INPUT slot
                    {
                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = "Index";
                        slotConfiguration.m_displayGroup = GetSourceDisplayGroup();
                        slotConfiguration.SetType(Data::Type::Number());
                        slotConfiguration.SetConnectionType(ConnectionType::Input);

                        m_inputSlots.insert(AddSlot(slotConfiguration));
                    }

                    // Add the OUTPUT slots, most of the time there will only be one
                    {
                        Data::Type type = Data::FromAZType(m_sourceTypes[0]);

                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = Data::GetName(type);
                        slotConfiguration.m_displayGroup = GetSourceDisplayGroup();
                        slotConfiguration.SetType(type);
                        slotConfiguration.SetConnectionType(ConnectionType::Output);

                        m_outputSlots.insert(AddSlot(slotConfiguration));
                    }
                }
                else if (Data::IsMapContainerType(GetSourceAZType()))
                {
                    AZStd::vector<AZ::Uuid> types = ScriptCanvas::Data::GetContainedTypes(GetSourceAZType());

                    // Only add the KEY as INPUT slot
                    {
                        ScriptCanvas::Data::Type dataType = Data::FromAZType(types[0]);

                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = Data::GetName(dataType);
                        slotConfiguration.SetType(dataType);
                        slotConfiguration.SetConnectionType(ConnectionType::Input);
                        slotConfiguration.m_displayGroup = GetSourceDisplayGroup();

                        m_inputSlots.insert(AddSlot(slotConfiguration));
                    }

                    // Only add the VALUE as the OUTPUT slot
                    {
                        Data::Type type = Data::FromAZType(types[1]);

                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = Data::GetName(type);
                        slotConfiguration.m_toolTip = "The value at the specified index";
                        slotConfiguration.SetType(type);
                        slotConfiguration.SetConnectionType(ConnectionType::Output);
                        slotConfiguration.m_displayGroup = GetSourceDisplayGroup();

                        m_outputSlots.insert(AddSlot(slotConfiguration));
                    }
                }
            }
        }
    }
}
