/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UnitTesting.h"

#include <AzCore/Serialization/Utils.h>

namespace ScriptCanvas
{
    namespace UnitTesting
    {
        bool ExpectBooleanVersioner(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
        {
            if (rootElement.GetVersion() == 0)
            {
                auto slotContainerElements = AZ::Utils::FindDescendantElements(serializeContext, rootElement, AZStd::vector<AZ::Crc32>{AZ_CRC("BaseClass1", 0xd4925735), AZ_CRC("Slots", 0xc87435d0), AZ_CRC("element", 0x41405e39)});
                for (auto slotElement : slotContainerElements)
                {
                    AZStd::string slotName;
                    if (slotElement->GetChildData(AZ_CRC("slotName", 0x817c3511), slotName))
                    {
                        if (slotName == "Value")
                        {
                            slotName = "Candidate";
                            slotElement->RemoveElementByName(AZ_CRC("slotName", 0x817c3511));

                            if (slotElement->AddElementWithData(serializeContext, "slotName", slotName) == -1)
                            {
                                AZ_Assert(false, "Version Converter failed. Unable to add data element [slotName] with value %s in version %u converter", slotName.data(), rootElement.GetVersion());
                                return false;
                            }
                        }
                    }
                    else
                    {
                        AZ_Assert(false, "Version Converter failed. Unabled to find child data by name 'slotName' in conversion of version %u", rootElement.GetVersion());
                        return false;
                    }
                }
            }

            return true;
        }
        
        bool ExpectComparisonVersioner(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
        {
            if (rootElement.GetVersion() == 0)
            {
                auto slotContainerElements = AZ::Utils::FindDescendantElements(serializeContext, rootElement, AZStd::vector<AZ::Crc32>{AZ_CRC("BaseClass1", 0xd4925735), AZ_CRC("Slots", 0xc87435d0), AZ_CRC("element", 0x41405e39)});
                for (auto slotElement : slotContainerElements)
                {
                    AZStd::string slotName;
                    if (slotElement->GetChildData(AZ_CRC("slotName", 0x817c3511), slotName))
                    {
                        if (slotName == "LHS")
                        {
                            slotName = "Candidate";
                            slotElement->RemoveElementByName(AZ_CRC("slotName", 0x817c3511));

                            if (slotElement->AddElementWithData(serializeContext, "slotName", slotName) == -1)
                            {
                                AZ_Assert(false, "Version Converter failed. Unable to add data element [slotName] with value %s in version %u converter", slotName.data(), rootElement.GetVersion());
                                return false;
                            }
                        }
                        else if (slotName == "RHS")
                        {
                            slotName = "Reference";
                            slotElement->RemoveElementByName(AZ_CRC("slotName", 0x817c3511));

                            if (slotElement->AddElementWithData(serializeContext, "slotName", slotName) == -1)
                            {
                                AZ_Assert(false, "Version Converter failed. Unable to add data element [slotName] with value %s in version %u converter", slotName.data(), rootElement.GetVersion());
                                return false;
                            }
                        }
                    }
                    else
                    {
                        AZ_Assert(false, "Version Converter failed. Unabled to find child data by name 'slotName' in conversion of version %u", rootElement.GetVersion());
                        return false;
                    }
                }
            }

            return true;
        }

    } 

} 
