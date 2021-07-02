/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "VRAMDataAggregator.hxx"
#include <Source/Driller/Rendering/VRAM/moc_VRAMDataAggregator.cpp>
#include "VRAMEvents.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UserSettings/UserSettings.h>
#include "Source/Driller/Workspaces/Workspace.h"

namespace Driller
{
    namespace VRAM
    {
        //=========================================================================

        enum class ExportField
        {
            RESOURCE_NAME,
            ALLOCATION_SIZE,
            UNKNOWN
        };

        /**
         * VRAM CSV export settings
         */
        class VRAMExportSettings
            : public GenericCSVExportSettings
        {
        public:
            AZ_CLASS_ALLOCATOR(VRAMExportSettings, AZ::SystemAllocator, 0);

            VRAMExportSettings()
            {
                m_columnDescriptors =
                {
                    {ExportField::RESOURCE_NAME, "Resource Name"},
                    {ExportField::ALLOCATION_SIZE, "VRAM Allocation Size"},
                };

                m_exportOrdering =
                {
                    ExportField::RESOURCE_NAME,
                    ExportField::ALLOCATION_SIZE,
                };

                for (const AZStd::pair< ExportField, AZStd::string >& item : m_columnDescriptors)
                {
                    m_stringToExportEnum[item.second] = item.first;
                }
            }

            virtual void GetExportItems(QStringList& items) const
            {
                for (const AZStd::pair< ExportField, AZStd::string>& item : m_columnDescriptors)
                {
                    items.push_back(QString(item.second.c_str()));
                }
            }

            virtual void GetActiveExportItems(QStringList& items) const
            {
                for (ExportField currentField : m_exportOrdering)
                {
                    if (currentField != ExportField::UNKNOWN)
                    {
                        items.push_back(QString(FindColumnDescriptor(currentField).c_str()));
                    }
                }
            }

            const AZStd::vector< ExportField >& GetExportOrder() const
            {
                return m_exportOrdering;
            }

            const AZStd::string& FindColumnDescriptor(ExportField exportField) const
            {
                static const AZStd::string emptyDescriptor;

                AZStd::unordered_map<ExportField, AZStd::string>::const_iterator descriptorIter = m_columnDescriptors.find(exportField);

                if (descriptorIter == m_columnDescriptors.end())
                {
                    AZ_Warning("Standalone Tools", false, "Unknown column descriptor in VRAM CSV Export");
                    return emptyDescriptor;
                }
                else
                {
                    return descriptorIter->second;
                }
            }

        protected:
            virtual void UpdateExportOrdering(const QStringList& activeItems)
            {
                m_exportOrdering.clear();

                for (const QString& activeItem : activeItems)
                {
                    ExportField field = FindExportFieldFromDescriptor(activeItem.toStdString().c_str());

                    AZ_Warning("Standalone Tools", field != ExportField::UNKNOWN, "Unknown descriptor %s", activeItem.toStdString().c_str());
                    if (field != ExportField::UNKNOWN)
                    {
                        m_exportOrdering.push_back(field);
                    }
                }
            }

        private:
            ExportField FindExportFieldFromDescriptor(const char* columnDescriptor) const
            {
                AZStd::unordered_map<AZStd::string, ExportField>::const_iterator exportIter = m_stringToExportEnum.find(columnDescriptor);

                ExportField retVal = ExportField::UNKNOWN;

                if (exportIter != m_stringToExportEnum.end())
                {
                    retVal = exportIter->second;
                }

                return retVal;
            }

            AZStd::unordered_map< ExportField, AZStd::string > m_columnDescriptors;
            AZStd::unordered_map< AZStd::string, ExportField > m_stringToExportEnum;
            AZStd::vector< ExportField > m_exportOrdering;
        };

        //=========================================================================

        VRAMDataAggregator::VRAMDataAggregator(int identity)
            : Aggregator(identity)
        {
            m_parser.SetAggregator(this);
            m_csvExportSettings = aznew VRAMExportSettings();
        }

        VRAMDataAggregator::~VRAMDataAggregator()
        {
            delete m_csvExportSettings;
        }

        float VRAMDataAggregator::ValueAtFrame(FrameNumberType frame)
        {
            const float maxEventsPerFrame = 1000.0f; // just a scale number
            float numEventsPerFrame = static_cast<float>(NumOfEventsAtFrame(frame));
            return AZStd::GetMin(numEventsPerFrame / maxEventsPerFrame, 1.0f) * 2.0f - 1.0f;
        }

        QColor VRAMDataAggregator::GetColor() const
        {
            return QColor(0, 255, 255);
        }

        QString VRAMDataAggregator::GetName() const
        {
            return "VRAM";
        }

        QString VRAMDataAggregator::GetChannelName() const
        {
            return ChannelName();
        }

        QString VRAMDataAggregator::GetDescription() const
        {
            return "VRAM allocations driller";
        }

        QString VRAMDataAggregator::GetToolTip() const
        {
            return "Information about VRAM allocations";
        }

        AZ::Uuid VRAMDataAggregator::GetID() const
        {
            return AZ::Uuid("{9D895E46-6CF7-4AA1-AC8F-79D8B6FB202E}");
        }

        bool VRAMDataAggregator::RegisterCategory(AZ::u32 categoryId, CategoryInfo* categoryInfo)
        {
            for (CategoryInfoArrayType::iterator iter = m_categories.begin(); iter != m_categories.end(); ++iter)
            {
                if ((*iter)->m_categoryId == categoryId)
                {
                    AZ_Assert(0, "Category %u has already been registered", categoryId);
                    return false;
                }
            }

            m_categories.push_back(categoryInfo);
            return true;
        }

        bool VRAMDataAggregator::UnregisterCategory(AZ::u32 categoryId)
        {
            for (CategoryInfoArrayType::iterator iter = m_categories.begin(); iter != m_categories.end(); ++iter)
            {
                if ((*iter)->m_categoryId == categoryId)
                {
                    m_categories.erase(iter);
                    return true;
                }
            }

            AZ_Assert(0, "Attempting to unregister a category %u which has not been registered", categoryId);
            return false;
        }

        CategoryInfo* VRAMDataAggregator::FindCategory(AZ::u32 categoryId)
        {
            for (CategoryInfoArrayType::iterator iter = m_categories.begin(); iter != m_categories.end(); ++iter)
            {
                if ((*iter)->m_categoryId == categoryId)
                {
                    return (*iter);
                }
            }
            return nullptr;
        }

        AllocationInfo* VRAMDataAggregator::FindAndRemoveAllocation(AZ::u64 address)
        {
            for (CategoryInfoArrayType::iterator iter = m_categories.begin(); iter != m_categories.end(); ++iter)
            {
                CategoryInfo* category = *iter;
                AllocationMapType::iterator allocIt = category->m_allocations.find(address);
                if (allocIt != category->m_allocations.end())
                {
                    AllocationInfo* allocInfo = allocIt->second;

                    // Deallocation, so subtract and remove from the allocation table
                    category->m_allocatedMemory -= allocInfo->m_size;
                    category->m_allocations.erase(address);

                    return allocInfo;
                }
            }

            return nullptr;
        }

        void VRAMDataAggregator::Reset()
        {
            m_categories.clear();
        }

        //=========================================================================

        CustomizeCSVExportWidget* VRAMDataAggregator::CreateCSVExportCustomizationWidget()
        {
            return aznew GenericCustomizeCSVExportWidget(*m_csvExportSettings);
        }

        void VRAMDataAggregator::ExportCategoryHeaderToCSV(AZ::IO::SystemFile& file)
        {
            const AZStd::string categoryHeader = AZStd::string("Category,Number of Allocations, Memory Usage,\n");
            file.Write(categoryHeader.c_str(), categoryHeader.size());

            for (VRAM::CategoryInfo* currentCategory : m_categories)
            {
                const AZStd::string categoryInfo = AZStd::string::format("%s,%zu,%zu,\n", currentCategory->m_categoryName, currentCategory->m_allocations.size(), currentCategory->m_allocatedMemory);
                file.Write(categoryInfo.c_str(), categoryInfo.size());
            }

            file.Write("\n", 1);
        }

        void VRAMDataAggregator::ExportColumnDescriptorToCSV(AZ::IO::SystemFile& file, CSVExportSettings* exportSettings)
        {
            // Write the category information at the top of the file
            ExportCategoryHeaderToCSV(file);

            VRAMExportSettings* vramExportSettings = static_cast<VRAMExportSettings*>(exportSettings);
            const AZStd::vector< ExportField >& exportOrdering = vramExportSettings->GetExportOrder();

            bool addComma = false;

            // Now export all of our VRAM allocations
            for (ExportField currentField : exportOrdering)
            {
                if (addComma)
                {
                    file.Write(",", 1);
                }

                const AZStd::string& columnDescriptor = vramExportSettings->FindColumnDescriptor(currentField);
                file.Write(columnDescriptor.c_str(), columnDescriptor.size());
                addComma = true;
            }

            file.Write("\n", 1);
        }

        void VRAMDataAggregator::ExportEventToCSV(AZ::IO::SystemFile& file, const DrillerEvent* drillerEvent, CSVExportSettings* exportSettings)
        {
            // We don't care about logging the category registration events
            if (azrtti_istypeof<VRAMDrillerRegisterCategoryEvent>(drillerEvent))
            {
                return;
            }

            bool isDeallocation = azrtti_istypeof<VRAMDrillerUnregisterAllocationEvent>(drillerEvent);
            AZ_Assert(azrtti_istypeof<VRAMDrillerRegisterAllocationEvent>(drillerEvent) || isDeallocation, "Invalid Event");

            const VRAM::AllocationInfo* allocationInformation = nullptr;
            if (isDeallocation)
            {
                const VRAMDrillerUnregisterAllocationEvent* vramDeallocationEvent = static_cast<const VRAMDrillerUnregisterAllocationEvent*>(drillerEvent);
                allocationInformation = vramDeallocationEvent->m_removedAllocationInfo;
            }
            else
            {
                const VRAMDrillerRegisterAllocationEvent* vramAllocationEvent = static_cast<const VRAMDrillerRegisterAllocationEvent*>(drillerEvent);
                allocationInformation = &vramAllocationEvent->m_allocationInfo;
            }

            if (allocationInformation == nullptr)
            {
                AZ_Warning("System", 0, "Error: Allocation information not found for VRAM tracking event");
                return;
            }

            VRAMExportSettings* vramExportSettings = static_cast<VRAMExportSettings*>(exportSettings);
            const AZStd::vector< ExportField >& exportOrdering = vramExportSettings->GetExportOrder();

            bool addComma = false;
            AZStd::string field;

            for (ExportField currentField : exportOrdering)
            {
                if (addComma)
                {
                    file.Write(",", 1);
                }

                switch (currentField)
                {
                case ExportField::RESOURCE_NAME:
                {
                    field = allocationInformation->m_name;
                    break;
                }
                case ExportField::ALLOCATION_SIZE:
                {
                    if (isDeallocation)
                    {
                        field = AZStd::string::format("-%llu", allocationInformation->m_size);
                    }
                    else
                    {
                        field = AZStd::string::format("%llu", allocationInformation->m_size);
                    }
                    break;
                }
                default:
                    AZ_Warning("Standalone Tools", false, "Unknown Export Field for VRAMDataAggreagtor");
                    break;
                }

                file.Write(field.c_str(), field.length());
                addComma = true;
            }

            file.Write("\n", 1);
        }

        //=========================================================================
    } // namespace VRAM
} // namespace Driller
