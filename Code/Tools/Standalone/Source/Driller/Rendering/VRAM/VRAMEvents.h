/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_VRAM_EVENTS_H
#define DRILLER_VRAM_EVENTS_H

#include "Source/Driller/DrillerEvent.h"
#include <AzCore/Memory/AllocationRecords.h>

namespace Driller
{
    namespace VRAM
    {
        //=========================================================================

        struct AllocationInfo
        {
            // This is an index for which category this allocation belongs to
            AZ::u32     m_category = 0;

            // This is an index for which subcategory this allocation belongs to
            AZ::u32     m_subcategory = 0;

            const char* m_name = nullptr;
            AZ::u64     m_size = 0;
        };

        struct SubcategoryInfo
        {
            SubcategoryInfo(AZ::u32 subcategoryId)
                : m_subcategoryId(subcategoryId)
            {}

            SubcategoryInfo(AZ::u32 subcategoryId, const char* subcategoryName)
                : m_subcategoryId(subcategoryId)
                , m_subcategoryName(subcategoryName)
            {}

            AZ::u32     m_subcategoryId = 0;
            const char* m_subcategoryName = nullptr;
        };

        typedef AZStd::unordered_map<AZ::u64, AllocationInfo*> AllocationMapType;
        typedef AZStd::vector<SubcategoryInfo> SubcategoryVectorType;

        struct CategoryInfo
        {
            const char* m_categoryName = nullptr;
            AZ::u32     m_categoryId = 0;

            // The total amount of memory allocated for this category.
            // Note that this amount may be different
            size_t      m_allocatedMemory = 0;

            // Map of all allocations
            AllocationMapType       m_allocations;

            // Container of all subcategories
            SubcategoryVectorType   m_subcategories;
        };

        enum VRAMEventType
        {
            ET_REGISTER_ALLOCATION,
            ET_UNREGISTER_ALLOCATION,
            ET_REGISTER_CATEGORY,
            ET_UNREGISTER_CATEGORY
        };

        //=========================================================================

        class VRAMDrillerRegisterAllocationEvent
            : public DrillerEvent
        {
        public:
            AZ_CLASS_ALLOCATOR(VRAMDrillerRegisterAllocationEvent, AZ::SystemAllocator, 0)
            AZ_RTTI(VRAMDrillerRegisterAllocationEvent, "{458DE527-390F-479E-A5AA-408EF44DB93F}", DrillerEvent);

            VRAMDrillerRegisterAllocationEvent()
                : DrillerEvent(VRAM::ET_REGISTER_ALLOCATION)
            {}

            virtual void StepForward(Aggregator* data);
            virtual void StepBackward(Aggregator* data);

            AZ::u64 m_address = 0;
            VRAM::AllocationInfo m_allocationInfo;
        };

        //=========================================================================

        class VRAMDrillerUnregisterAllocationEvent
            : public DrillerEvent
        {
        public:
            AZ_CLASS_ALLOCATOR(VRAMDrillerUnregisterAllocationEvent, AZ::SystemAllocator, 0)
            AZ_RTTI(VRAMDrillerUnregisterAllocationEvent, "{674F8DE3-11C1-4B1E-B0A5-EB45B5F72F68}", DrillerEvent);

            VRAMDrillerUnregisterAllocationEvent()
                : DrillerEvent(VRAM::ET_UNREGISTER_ALLOCATION)
            {}

            virtual void StepForward(Aggregator* data);
            virtual void StepBackward(Aggregator* data);

            AZ::u64 m_address = 0;
            AllocationInfo* m_removedAllocationInfo = nullptr;
        };

        //=========================================================================

        class VRAMDrillerRegisterCategoryEvent
            : public DrillerEvent
        {
        public:
            AZ_CLASS_ALLOCATOR(VRAMDrillerRegisterCategoryEvent, AZ::SystemAllocator, 0)
            AZ_RTTI(VRAMDrillerUnregisterAllocationEvent, "{F024BA49-E8C9-4699-B999-9E6F988CFF8E}", DrillerEvent);

            VRAMDrillerRegisterCategoryEvent()
                : DrillerEvent(VRAM::ET_REGISTER_CATEGORY)
            {}

            virtual void StepForward(Aggregator* data);
            virtual void StepBackward(Aggregator* data);

            AZ::u32 m_categoryId = 0;
            CategoryInfo m_categoryInfo;
        };

        //=========================================================================

        class VRAMDrillerUnregisterCategoryEvent
            : public DrillerEvent
        {
        public:
            AZ_CLASS_ALLOCATOR(VRAMDrillerUnregisterCategoryEvent, AZ::SystemAllocator, 0)
            AZ_RTTI(VRAMDrillerUnregisterCategoryEvent, "{6549C4A4-70E4-47AD-8688-47C00543197A}", DrillerEvent);

            VRAMDrillerUnregisterCategoryEvent()
                : DrillerEvent(VRAM::ET_UNREGISTER_CATEGORY)
            {}

            virtual void StepForward(Aggregator* data);
            virtual void StepBackward(Aggregator* data);

            AZ::u32 m_categoryId = 0;
            CategoryInfo m_unregisteredCategoryInfo;
        };

        //=========================================================================
    } // namespace VRAM
} // namespace Driller

#endif
