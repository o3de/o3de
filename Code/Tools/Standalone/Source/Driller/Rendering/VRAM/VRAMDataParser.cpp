/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "VRAMDataParser.h"
#include "VRAMDataAggregator.hxx"
#include "VRAMEvents.h"

namespace Driller
{
    namespace VRAM
    {
        AZ::Debug::DrillerHandlerParser* VRAMDrillerHandlerParser::OnEnterTag(AZ::u32 tagName)
        {
            AZ_Assert(m_data, "You must set a valid VRAM aggregator before we can process the data!");

            if (tagName == AZ_CRC("RegisterAllocation", 0x992a9780))
            {
                m_subTag = ST_REGISTER_ALLOCATION;
                m_data->AddEvent(aznew VRAMDrillerRegisterAllocationEvent());
                return this;
            }
            else if (tagName == AZ_CRC("UnRegisterAllocation", 0xea5dc4cd))
            {
                m_subTag = ST_UNREGISTER_ALLOCATION;
                m_data->AddEvent(aznew VRAMDrillerUnregisterAllocationEvent());
                return this;
            }
            else if (tagName == AZ_CRC("RegisterCategory"))
            {
                m_subTag = ST_REGISTER_CATEGORY;
                m_data->AddEvent(aznew VRAMDrillerRegisterCategoryEvent());
                return this;
            }
            else if (tagName == AZ_CRC("UnregisterCategory"))
            {
                m_subTag = ST_UNREGISTER_CATEGORY;
                m_data->AddEvent(aznew VRAMDrillerUnregisterCategoryEvent());
                return this;
            }
            else
            {
                m_subTag = ST_NONE;
            }
            return NULL;
        }

        void VRAMDrillerHandlerParser::OnExitTag(DrillerHandlerParser* handler, AZ::u32 tagName)
        {
            (void)tagName;
            if (handler != NULL)
            {
                m_subTag = ST_NONE; // we have only one level just go back to the default state
            }
        }

        void VRAMDrillerHandlerParser::OnData(const AZ::Debug::DrillerSAXParser::Data& dataNode)
        {
            AZ_Assert(m_data, "You must set a valid VRAM aggregator before we can process the data!");

            switch (m_subTag)
            {
            case ST_REGISTER_ALLOCATION:
            {
                VRAMDrillerRegisterAllocationEvent* event = static_cast<VRAMDrillerRegisterAllocationEvent*>(m_data->GetEvents().back());
                if (dataNode.m_name == AZ_CRC("Category"))
                {
                    dataNode.Read(event->m_allocationInfo.m_category);
                }
                else if (dataNode.m_name == AZ_CRC("Subcategory"))
                {
                    dataNode.Read(event->m_allocationInfo.m_subcategory);
                }
                else if (dataNode.m_name == AZ_CRC("Address", 0x0d4e6f81))
                {
                    dataNode.Read(event->m_address);
                }
                else if (dataNode.m_name == AZ_CRC("Size", 0xf7c0246a))
                {
                    dataNode.Read(event->m_allocationInfo.m_size);
                }
                else if (dataNode.m_name == AZ_CRC("Name", 0x5e237e06))
                {
                    event->m_allocationInfo.m_name = dataNode.ReadPooledString();
                }
            }
            break;

            case ST_UNREGISTER_ALLOCATION:
            {
                VRAMDrillerUnregisterAllocationEvent* event = static_cast<VRAMDrillerUnregisterAllocationEvent*>(m_data->GetEvents().back());
                if (dataNode.m_name == AZ_CRC("Address", 0x0d4e6f81))
                {
                    dataNode.Read(event->m_address);
                }
            }
            break;

            case ST_REGISTER_CATEGORY:
            {
                VRAMDrillerRegisterCategoryEvent* event = static_cast<VRAMDrillerRegisterCategoryEvent*>(m_data->GetEvents().back());
                if (dataNode.m_name == AZ_CRC("Category"))
                {
                    dataNode.Read(event->m_categoryId);
                    event->m_categoryInfo.m_categoryId = event->m_categoryId;
                }
                else if (dataNode.m_name == AZ_CRC("CategoryName"))
                {
                    event->m_categoryInfo.m_categoryName = dataNode.ReadPooledString();
                }
                else if (dataNode.m_name == AZ_CRC("SubcategoryId"))
                {
                    // NOTE: "SubcategoryId" and "SubcategoryName" have to be done in two separate Read events.
                    // The SubcategoryId read will create a SubcategoryInfo, and we assume when we hit SubcategoryName that we had just registered a new SubcategoryId on the previous read.
                    unsigned int subcategoryId = 0;
                    dataNode.Read(subcategoryId);
                    event->m_categoryInfo.m_subcategories.push_back(SubcategoryInfo(subcategoryId));
                }
                else if (dataNode.m_name == AZ_CRC("SubcategoryName"))
                {
                    AZ_Assert(event->m_categoryInfo.m_subcategories.size(), "Error: Found a SubcategoryName data tag, but did not find a previous SubcategoryId tag");

                    // Get the most recently registered subcategory
                    SubcategoryInfo& subcategory = event->m_categoryInfo.m_subcategories[event->m_categoryInfo.m_subcategories.size() - 1];
                    AZ_Assert(subcategory.m_subcategoryName == nullptr, "Error: Subcategory 0x%08x already has a SubcategoryName", subcategory.m_subcategoryId);

                    subcategory.m_subcategoryName = dataNode.ReadPooledString();
                }
            }
            break;

            case ST_UNREGISTER_CATEGORY:
            {
                VRAMDrillerUnregisterCategoryEvent* event = static_cast<VRAMDrillerUnregisterCategoryEvent*>(m_data->GetEvents().back());
                if (dataNode.m_name == AZ_CRC("Category"))
                {
                    dataNode.Read(event->m_categoryId);
                }
            }
            break;
            }
        }
    } // namespace VRAM
} // namespace Driller
