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
#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_MEMORY_VRAMDRILLER_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_MEMORY_VRAMDRILLER_H 1

#include <AzCore/Driller/Driller.h>
#include <Common/Memory/VRAMDrillerBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Memory/PlatformMemoryInstrumentation.h>

namespace Render
{
    namespace Debug
    {
        /**
         * VRAMDriller: A class that tracks VRAM allocations and communicates with the Driller
         * to log and generate reports for the allocations.
         */
        class VRAMDriller
            : public AZ::Debug::Driller
            , public VRAMDrillerBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(VRAMDriller, AZ::OSAllocator, 0)

            VRAMDriller();
            ~VRAMDriller();

            void CreateAllocationRecords(unsigned char stackRecordLevels, bool isMemoryGuard, bool isMarkUnallocatedMemory);
            void DestroyAllocationRecords();

        protected:
            //////////////////////////////////////////////////////////////////////////
            // Driller
            virtual const char*  GroupName() const;
            virtual const char*  GetName() const;
            virtual const char*  GetDescription() const;
            virtual void         Start(const Param* params = NULL, int numParams = 0);
            virtual void         Stop();
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // VRAMDrillerBus
            virtual void RegisterCategory(VRAMAllocationCategory category, const char* categoryName, const VRAMSubCategoryType& subcategories);
            virtual void UnregisterAllCategories();
            virtual void RegisterAllocation(void* address, size_t byteSize, const char* allocationName, VRAMAllocationCategory category, VRAMAllocationSubcategory subcategory);
            virtual void UnregisterAllocation(void* address);
            virtual void GetCurrentVRAMStats(VRAMAllocationCategory category, VRAMAllocationSubcategory subcategory, AZStd::string& categoryName, AZStd::string& subcategoryName, size_t& numberBytesAllocated, size_t& numberAllocations);
            //////////////////////////////////////////////////////////////////////////

            // Subfunctions of RegisterCategory and RegisterAllocation.
            // Split out due to these parts being called in from both of those functions and the Start function
            void RegisterCategoryOutput(VRAMAllocationCategory category, const struct VRAMCategoryInfo* info);
            void RegisterAllocationOutput(void* address, const struct VRAMAllocationInfo* info);

        private:

#if PLATFORM_MEMORY_INSTRUMENTATION_ENABLED
            uint16_t m_platformMemoryInstrumentationRootGroupId = 0;
            uint16_t m_platformMemoryInstrumentationCategoryIds[Render::Debug::VRAM_CATEGORY_NUMBER_CATEGORIES] = { 0 };
            uint16_t m_platformMemoryInstrumentationSubcategoryIds[Render::Debug::VRAM_SUBCATEGORY_NUMBER_SUBCATEGORIES] = { 0 };
#endif
            class VRAMDrillerAllocations* m_allocations = nullptr;
        };
    } // namespace Debug
} // namespace Render

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_MEMORY_VRAMDRILLER_H
#pragma once
