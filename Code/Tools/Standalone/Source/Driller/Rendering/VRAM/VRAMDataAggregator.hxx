/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_VRAM_DATAAGGREGATOR_H
#define DRILLER_VRAM_DATAAGGREGATOR_H

#if !defined(Q_MOC_RUN)
#include "Source/Driller/DrillerAggregator.hxx"
#include "Source/Driller/DrillerAggregatorOptions.hxx"
#include "Source/Driller/GenericCustomizeCSVExportWidget.hxx"
#include "VRAMDataParser.h"
#include "AzCore/std/string/string.h"
#include "AzCore/RTTI/RTTI.h"
#endif

namespace Driller
{
    namespace VRAM
    {
        //=========================================================================

        struct CategoryInfo;
        typedef AZStd::list<CategoryInfo*> CategoryInfoArrayType;

        /**
         * VRAM data drilling aggregator
         */
        class VRAMDataAggregator
            : public Aggregator
        {
            Q_OBJECT;
        public:
            AZ_RTTI(VRAMDataAggregator, "{D17F2623-A980-4A08-9CEB-B8F89C811C1C}");
            AZ_CLASS_ALLOCATOR(VRAMDataAggregator, AZ::SystemAllocator, 0);

            VRAMDataAggregator(int identity = 0);
            virtual ~VRAMDataAggregator();

            static AZ::u32 DrillerId()
            {
                return VRAMDrillerHandlerParser::GetDrillerId();
            }

            AZ::u32 GetDrillerId() const override
            {
                return DrillerId();
            }

            static const char* ChannelName()
            {
                return "VRAM";
            }

            AZ::Crc32 GetChannelId() const override
            {
                return AZ::Crc32(ChannelName());
            }

            AZ::Debug::DrillerHandlerParser* GetDrillerDataParser() override
            {
                return &m_parser;
            }

            bool CanExportToCSV() const override
            {
                return true;
            }

            CustomizeCSVExportWidget* CreateCSVExportCustomizationWidget();

            bool RegisterCategory(AZ::u32 categoryId, CategoryInfo* categoryInfo);
            bool UnregisterCategory(AZ::u32 categoryId);
            CategoryInfo* FindCategory(AZ::u32 categoryId);

            // Search all categories for this address, remove it from the hash table and return its allocation info.
            struct AllocationInfo* FindAndRemoveAllocation( AZ::u64 address );

            void ApplySettingsFromWorkspace(WorkspaceSettingsProvider*) override {}
            void ActivateWorkspaceSettings(WorkspaceSettingsProvider*) override {}
            void SaveSettingsToWorkspace(WorkspaceSettingsProvider*) override {}

            void Reset() override;

        public slots:
            float ValueAtFrame(FrameNumberType frame) override;
            QColor GetColor() const override;
            QString GetChannelName() const override;
            QString GetName() const override;
            QString GetDescription() const override;
            QString GetToolTip() const override;    
            AZ::Uuid GetID() const override;
            void OptionsRequest()  override {}
            QWidget* DrillDownRequest(FrameNumberType frame) override
            {
                // Create a Qt view window to show a graph view of the VRAM usage
                (void)frame;
                return nullptr;
            }

        protected:
            void ExportColumnDescriptorToCSV(AZ::IO::SystemFile& file, CSVExportSettings* exportSettings) override;
            void ExportEventToCSV(AZ::IO::SystemFile& file, const DrillerEvent* drillerEvent, CSVExportSettings* exportSettings) override;
            void ExportCategoryHeaderToCSV(AZ::IO::SystemFile& file);

            class VRAMExportSettings* m_csvExportSettings;
            VRAMDrillerHandlerParser m_parser;

            // Different categories of VRAM allocations and all of the allocations that live in that category.
            CategoryInfoArrayType m_categories;
        };

        //=========================================================================
    } // namespace VRAM
} // namespace Driller

#endif
