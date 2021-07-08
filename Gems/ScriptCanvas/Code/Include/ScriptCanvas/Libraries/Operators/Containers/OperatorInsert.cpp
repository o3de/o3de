/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OperatorInsert.h"
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Contracts/SupportsMethodContract.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            void OperatorInsert::ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs)
            {
                if (sourceType == SourceType::SourceInput)
                {
                    ContractDescriptor supportsMethodContract;
                    supportsMethodContract.m_createFunc = [this]() -> SupportsMethodContract* { return aznew SupportsMethodContract("Insert"); };
                    contractDescs.push_back(AZStd::move(supportsMethodContract));
                }
            }

            void OperatorInsert::OnSourceTypeChanged()
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

                    // Add the INPUT slot for the data to insert
                    {
                        Data::Type type = Data::FromAZType(m_sourceTypes[0]);

                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = Data::GetName(type);
                        slotConfiguration.m_displayGroup = GetSourceDisplayGroup();
                        slotConfiguration.SetType(type);
                        slotConfiguration.SetConnectionType(ConnectionType::Input);

                        m_inputSlots.insert(AddSlot(slotConfiguration));
                    }
                }
                else
                {
                    // Key
                    {
                        Data::Type type = Data::FromAZType(m_sourceTypes[0]);

                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = "Key";
                        slotConfiguration.m_displayGroup = GetSourceDisplayGroup();
                        slotConfiguration.SetType(type);
                        slotConfiguration.SetConnectionType(ConnectionType::Input);

                        m_inputSlots.insert(AddSlot(slotConfiguration));
                    }

                    // Value
                    {
                        Data::Type type = Data::FromAZType(m_sourceTypes[1]);
                        DataSlotConfiguration slotConfiguration;
                        slotConfiguration.m_name = "Value";
                        slotConfiguration.m_displayGroup = GetSourceDisplayGroup();
                        slotConfiguration.SetType(type);
                        slotConfiguration.SetConnectionType(ConnectionType::Input);
                        m_inputSlots.insert(AddSlot(slotConfiguration));
                    }
                }
            }

        }
    }
}
