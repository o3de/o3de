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
#include "RenderDll_precompiled.h"
#include <Common/Memory/VRAMDriller.h>
#include <AzCore/Math/Crc.h>

namespace Render
{
    namespace Debug
    {
        //=========================================================================

        struct VRAMCategoryInfo
        {
            VRAMAllocationCategory m_category = VRAM_CATEGORY_INVALID;
            const char* m_categoryName = nullptr;
            VRAMSubCategoryType m_subcategories;
        };

        struct VRAMAllocationInfo
        {
            void* m_address = nullptr;
            size_t m_byteSize = 0;
            string m_allocationName;
            VRAMAllocationCategory m_category = VRAM_CATEGORY_INVALID;
            VRAMAllocationSubcategory m_subcategory = VRAM_SUBCATEGORY_INVALID;
        };

        typedef AZStd::unordered_map<VRAMAllocationCategory, VRAMCategoryInfo, AZStd::hash<VRAMAllocationCategory>, AZStd::equal_to<VRAMAllocationCategory>, AZ::OSStdAllocator> VRAMCategoryType;
        typedef AZStd::unordered_map<void*, VRAMAllocationInfo, AZStd::hash<void*>, AZStd::equal_to<void*>, AZ::OSStdAllocator> VRAMAllocationRecordsType;

        /**
         * VRAMDrillerAllocations: A class that tracks VRAM allocations and categories/subcategories for the allocations
         */
        class VRAMDrillerAllocations
        {
            friend class VRAMDriller;
            AZ_CLASS_ALLOCATOR(VRAMDrillerAllocations, AZ::OSAllocator, 0)

        public:

            VRAMDrillerAllocations() {}
            ~VRAMDrillerAllocations()
            {
                m_allocations.clear();
            }

            //=========================================================================

            const VRAMCategoryInfo* RegisterCategory(VRAMAllocationCategory category, const char* categoryName, const VRAMSubCategoryType& subcategories)
            {
                AZ_Assert(category < VRAM_CATEGORY_INVALID, ("Error, invalid VRAM category"));

                // Insert and populate the category
                VRAMCategoryType::pair_iter_bool iterBool = m_categories.insert_key(category);
                AZ_Assert(iterBool.second, "VRAM category %u is already registered!", category);

                VRAMCategoryInfo& categoryInfo = iterBool.first->second;
                categoryInfo.m_category = category;
                categoryInfo.m_categoryName = categoryName;
                categoryInfo.m_subcategories = subcategories;

                return &categoryInfo;
            }

            void UnregisterAllCategories(AZ::Debug::DrillerOutputStream* output)
            {
                // Skip if we have no active output
                if (output != nullptr)
                {
                    for (AZStd::pair<VRAMAllocationCategory, VRAMCategoryInfo> currentCategory : m_categories)
                    {
                        output->BeginTag(AZ_CRC("VRAMDriller"));
                        output->BeginTag(AZ_CRC("UnregisterCategory"));
                        output->Write(AZ_CRC("Category"), static_cast<unsigned int>(currentCategory.first));
                        output->EndTag(AZ_CRC("UnregisterCategory"));
                        output->EndTag(AZ_CRC("VRAMDriller"));
                    }
                }
                m_categories.clear();
            }

            const VRAMCategoryType& GetCategoriesMap()
            {
                return m_categories;
            }

            //=========================================================================

            const VRAMAllocationInfo* RegisterAllocation(void* address, size_t byteSize, const char* allocationName, VRAMAllocationCategory category, VRAMAllocationSubcategory subcategory)
            {
                AZ_Assert(address, ("Error, allocation address is null"));

                // Insert and populate the allocation record
                VRAMAllocationRecordsType::pair_iter_bool iterBool = m_allocations.insert_key(address);

                // Turning off the VRAMDriller altogether causes weird allocation errors
                AZ_Warning("Driller", iterBool.second, "VRAM memory address 0x%p is already allocated and being tracked! VRAM memory reporting may now be inaccurate.", address);

                VRAMAllocationInfo& allocationInfo = iterBool.first->second;
                allocationInfo.m_byteSize =  byteSize;
                allocationInfo.m_allocationName = allocationName;
                allocationInfo.m_category = category;
                allocationInfo.m_subcategory = subcategory;
                
                // Update simple tracking statistics
                m_simpleAllocationStatistics[category][subcategory].m_allocatedBytes += byteSize;
                m_simpleAllocationStatistics[category][subcategory].m_numberAllocations++;

                return &allocationInfo;
            }

            void UnregisterAllocation(void* address)
            {
                VRAMAllocationRecordsType::iterator iter = m_allocations.find(address);

                // Turning off the VRAMDriller altogether causes weird allocation errors
                AZ_Warning("Driller", iter != m_allocations.end(), "VRAM memory address 0x%p does not exist in the records. VRAM memory reporting may now be inaccurate.", address);
                
                if ( iter != m_allocations.end() )
                {
                    // Update simple tracking statistics
                    VRAMAllocationInfo& allocationInfo = iter->second;
                    m_simpleAllocationStatistics[allocationInfo.m_category][allocationInfo.m_subcategory].m_allocatedBytes -= allocationInfo.m_byteSize;
                    m_simpleAllocationStatistics[allocationInfo.m_category][allocationInfo.m_subcategory].m_numberAllocations--;

                    m_allocations.erase(iter);
                }
            }

            const VRAMAllocationRecordsType& GetAllocationsMap()
            {
                return m_allocations;
            }

            //=========================================================================

            struct SimpleAllocationStatistics
            {
                size_t m_allocatedBytes = 0;
                size_t m_numberAllocations = 0;
            };

            SimpleAllocationStatistics m_simpleAllocationStatistics[VRAM_CATEGORY_NUMBER_CATEGORIES][VRAM_SUBCATEGORY_NUMBER_SUBCATEGORIES];

        private:

            VRAMCategoryType m_categories;
            VRAMAllocationRecordsType m_allocations;
        };

        //=========================================================================

        VRAMDriller::VRAMDriller()
        {
#if PLATFORM_MEMORY_INSTRUMENTATION_ENABLED
            m_platformMemoryInstrumentationRootGroupId = AZ::PlatformMemoryInstrumentation::GetNextGroupId();
            AZ::PlatformMemoryInstrumentation::RegisterGroup(m_platformMemoryInstrumentationRootGroupId, "VRAM", AZ::PlatformMemoryInstrumentation::m_groupRoot);

            m_platformMemoryInstrumentationCategoryIds[VRAM_CATEGORY_TEXTURE] = AZ::PlatformMemoryInstrumentation::GetNextGroupId();
            AZ::PlatformMemoryInstrumentation::RegisterGroup(m_platformMemoryInstrumentationCategoryIds[VRAM_CATEGORY_TEXTURE], "Texture", m_platformMemoryInstrumentationRootGroupId);
            m_platformMemoryInstrumentationCategoryIds[VRAM_CATEGORY_BUFFER] = AZ::PlatformMemoryInstrumentation::GetNextGroupId();
            AZ::PlatformMemoryInstrumentation::RegisterGroup(m_platformMemoryInstrumentationCategoryIds[VRAM_CATEGORY_BUFFER], "Buffer", m_platformMemoryInstrumentationRootGroupId);
            m_platformMemoryInstrumentationCategoryIds[VRAM_CATEGORY_MISC] = AZ::PlatformMemoryInstrumentation::GetNextGroupId();
            AZ::PlatformMemoryInstrumentation::RegisterGroup(m_platformMemoryInstrumentationCategoryIds[VRAM_CATEGORY_MISC], "Misc", m_platformMemoryInstrumentationRootGroupId);

            m_platformMemoryInstrumentationSubcategoryIds[VRAM_SUBCATEGORY_TEXTURE_RENDERTARGET] = AZ::PlatformMemoryInstrumentation::GetNextGroupId();
            AZ::PlatformMemoryInstrumentation::RegisterGroup(m_platformMemoryInstrumentationSubcategoryIds[VRAM_SUBCATEGORY_TEXTURE_RENDERTARGET], "Render Target", m_platformMemoryInstrumentationCategoryIds[VRAM_CATEGORY_TEXTURE]);
            m_platformMemoryInstrumentationSubcategoryIds[VRAM_SUBCATEGORY_TEXTURE_TEXTURE] = AZ::PlatformMemoryInstrumentation::GetNextGroupId();
            AZ::PlatformMemoryInstrumentation::RegisterGroup(m_platformMemoryInstrumentationSubcategoryIds[VRAM_SUBCATEGORY_TEXTURE_TEXTURE], "Texture", m_platformMemoryInstrumentationCategoryIds[VRAM_CATEGORY_TEXTURE]);
            m_platformMemoryInstrumentationSubcategoryIds[VRAM_SUBCATEGORY_TEXTURE_DYNAMIC] = AZ::PlatformMemoryInstrumentation::GetNextGroupId();
            AZ::PlatformMemoryInstrumentation::RegisterGroup(m_platformMemoryInstrumentationSubcategoryIds[VRAM_SUBCATEGORY_TEXTURE_DYNAMIC], "Dynamic", m_platformMemoryInstrumentationCategoryIds[VRAM_CATEGORY_TEXTURE]);
            m_platformMemoryInstrumentationSubcategoryIds[VRAM_SUBCATEGORY_BUFFER_VERTEX_BUFFER] = AZ::PlatformMemoryInstrumentation::GetNextGroupId();
            AZ::PlatformMemoryInstrumentation::RegisterGroup(m_platformMemoryInstrumentationSubcategoryIds[VRAM_SUBCATEGORY_BUFFER_VERTEX_BUFFER], "Vertex Buffer", m_platformMemoryInstrumentationCategoryIds[VRAM_CATEGORY_BUFFER]);
            m_platformMemoryInstrumentationSubcategoryIds[VRAM_SUBCATEGORY_BUFFER_INDEX_BUFFER] = AZ::PlatformMemoryInstrumentation::GetNextGroupId();
            AZ::PlatformMemoryInstrumentation::RegisterGroup(m_platformMemoryInstrumentationSubcategoryIds[VRAM_SUBCATEGORY_BUFFER_INDEX_BUFFER], "Index Buffer", m_platformMemoryInstrumentationCategoryIds[VRAM_CATEGORY_BUFFER]);
            m_platformMemoryInstrumentationSubcategoryIds[VRAM_SUBCATEGORY_BUFFER_CONSTANT_BUFFER] = AZ::PlatformMemoryInstrumentation::GetNextGroupId();
            AZ::PlatformMemoryInstrumentation::RegisterGroup(m_platformMemoryInstrumentationSubcategoryIds[VRAM_SUBCATEGORY_BUFFER_CONSTANT_BUFFER], "Constant Buffer", m_platformMemoryInstrumentationCategoryIds[VRAM_CATEGORY_BUFFER]);
            m_platformMemoryInstrumentationSubcategoryIds[VRAM_SUBCATEGORY_BUFFER_OTHER_BUFFER] = AZ::PlatformMemoryInstrumentation::GetNextGroupId();
            AZ::PlatformMemoryInstrumentation::RegisterGroup(m_platformMemoryInstrumentationSubcategoryIds[VRAM_SUBCATEGORY_BUFFER_OTHER_BUFFER], "Other Buffer", m_platformMemoryInstrumentationCategoryIds[VRAM_CATEGORY_BUFFER]);
            m_platformMemoryInstrumentationSubcategoryIds[VRAM_SUBCATEGORY_MISC_OTHER] = AZ::PlatformMemoryInstrumentation::GetNextGroupId();
            AZ::PlatformMemoryInstrumentation::RegisterGroup(m_platformMemoryInstrumentationSubcategoryIds[VRAM_SUBCATEGORY_MISC_OTHER], "Misc", m_platformMemoryInstrumentationCategoryIds[VRAM_CATEGORY_MISC]);
#endif
            BusConnect();
        }

        VRAMDriller::~VRAMDriller()
        {
            BusDisconnect();
        }

        const char* VRAMDriller::GroupName() const
        {
            return "RenderingDrillers";
        }

        const char* VRAMDriller::GetName() const
        {
            return "VRAMDriller";
        }

        const char* VRAMDriller::GetDescription() const
        {
            return "Reports all VRAM memory allocations.";
        }

        void VRAMDriller::Start(const Param* params, int numParams)
        {
            (void)params;
            (void)numParams;

            if (m_allocations)
            {
                const VRAMCategoryType& categoriesMap = m_allocations->GetCategoriesMap();
                for (VRAMCategoryType::const_iterator iter = categoriesMap.begin(); iter != categoriesMap.end(); ++iter)
                {
                    RegisterCategoryOutput(iter->first, &iter->second);
                }

                const VRAMAllocationRecordsType& allocationMap = m_allocations->GetAllocationsMap();
                for (VRAMAllocationRecordsType::const_iterator iter = allocationMap.begin(); iter != allocationMap.end(); ++iter)
                {
                    RegisterAllocationOutput(iter->first, &iter->second);
                }
            }
        }

        void VRAMDriller::Stop()
        {
        }

        void VRAMDriller::CreateAllocationRecords([[maybe_unused]] unsigned char stackRecordLevels, [[maybe_unused]] bool isMemoryGuard, [[maybe_unused]] bool isMarkUnallocatedMemory)
        {
            AZ_Assert(m_allocations == nullptr, "Allocation records for the VRAMDriller already exist");
            m_allocations = aznew VRAMDrillerAllocations();
        }

        void VRAMDriller::DestroyAllocationRecords()
        {
            AZ_Assert(m_allocations != nullptr, "Allocation records for the VRAMDriller do not exist");
            delete m_allocations;
            m_allocations = nullptr;
        }

        //=========================================================================

        void VRAMDriller::RegisterAllocation(void* address, size_t byteSize, const char* allocationName, VRAMAllocationCategory category, VRAMAllocationSubcategory subcategory)
        {
            AZ_Assert(category != VRAM_CATEGORY_INVALID, "Invalid VRAM allocation category");
            AZ_Assert(subcategory != VRAM_SUBCATEGORY_INVALID, "No subcategory provided for VRAM Allocation");

#if PLATFORM_MEMORY_INSTRUMENTATION_ENABLED
            AZ::PlatformMemoryInstrumentation::Alloc(address, byteSize, 0, m_platformMemoryInstrumentationSubcategoryIds[subcategory]);
#else
            AZ_Assert(m_allocations != nullptr, "Allocation records for the VRAMDriller do not exist!");

            const VRAMAllocationInfo* info = m_allocations->RegisterAllocation(address, byteSize, allocationName, category, subcategory);
            // Skip if we have no active output
            if (m_output == nullptr)
            {
                return;
            }

            RegisterAllocationOutput(address, info);
#endif
        }

        void VRAMDriller::RegisterAllocationOutput(void* address, const VRAMAllocationInfo* info)
        {
            AZ_Assert(m_output != nullptr, ("The DrillerOutputStream is null"));

            m_output->BeginTag(AZ_CRC("VRAMDriller"));
            m_output->BeginTag(AZ_CRC("RegisterAllocation", 0x992a9780));
            m_output->Write(AZ_CRC("Address", 0x0d4e6f81), address);
            m_output->Write(AZ_CRC("Category"), static_cast<unsigned int>(info->m_category));
            m_output->Write(AZ_CRC("Subcategory"), static_cast<unsigned int>(info->m_subcategory));
            m_output->Write(AZ_CRC("Name", 0x5e237e06), info->m_allocationName.c_str());
            m_output->Write(AZ_CRC("Size", 0xf7c0246a), info->m_byteSize);
            m_output->EndTag(AZ_CRC("RegisterAllocation", 0x992a9780));
            m_output->EndTag(AZ_CRC("VRAMDriller"));
        }

        void VRAMDriller::UnregisterAllocation(void* address)
        {
#if PLATFORM_MEMORY_INSTRUMENTATION_ENABLED
            AZ::PlatformMemoryInstrumentation::Free(address);
#else
            AZ_Assert(m_allocations != nullptr, "Allocation records for the VRAMDriller do not exist!");
            m_allocations->UnregisterAllocation(address);

            // Skip if the driller is not actively capturing
            if (m_output == nullptr)
            {
                return;
            }

            m_output->BeginTag(AZ_CRC("VRAMDriller"));
            m_output->BeginTag(AZ_CRC("UnRegisterAllocation", 0xea5dc4cd));
            m_output->Write(AZ_CRC("Address", 0x0d4e6f81), address);
            m_output->EndTag(AZ_CRC("UnRegisterAllocation", 0xea5dc4cd));
            m_output->EndTag(AZ_CRC("VRAMDriller"));
#endif
        }

        //=========================================================================

        void VRAMDriller::RegisterCategory(VRAMAllocationCategory category, const char* categoryName, const VRAMSubCategoryType& subcategories)
        {
            AZ_Assert(m_allocations != nullptr, "Allocation records for the VRAMDriller do not exist!");
            AZ_Assert(category != VRAM_CATEGORY_INVALID, "Invalid VRAM allocation category");
            AZ_Assert(subcategories.size(), "No subcategory provided for VRAM category");

            const VRAMCategoryInfo* info = m_allocations->RegisterCategory(category, categoryName, subcategories);

            // Skip if the driller is not actively capturing
            if (m_output == nullptr)
            {
                return;
            }

            RegisterCategoryOutput(category, info);
        }

        void VRAMDriller::RegisterCategoryOutput(VRAMAllocationCategory category, const VRAMCategoryInfo* info)
        {
            AZ_Assert(m_output != nullptr, ("The DrillerOutputStream is null"));
            AZ_Assert(info != nullptr, ("The VRAMCategoryInfo is null"));

            m_output->BeginTag(AZ_CRC("VRAMDriller"));
            m_output->BeginTag(AZ_CRC("RegisterCategory"));
            m_output->Write(AZ_CRC("Category"), static_cast<unsigned int>(category));
            m_output->Write(AZ_CRC("CategoryName"), info->m_categoryName);

            for (VRAMSubCategoryType::const_iterator iter = info->m_subcategories.begin(); iter != info->m_subcategories.end(); ++iter)
            {
                const VRAMSubcategory* subcategoryInfo = iter;
                m_output->Write(AZ_CRC("SubcategoryId"), static_cast<unsigned int>(subcategoryInfo->m_subcategoryId));
                m_output->Write(AZ_CRC("SubcategoryName"), subcategoryInfo->m_subcategoryName);
            }

            m_output->EndTag(AZ_CRC("RegisterCategory"));
            m_output->EndTag(AZ_CRC("VRAMDriller"));
        }

        void VRAMDriller::UnregisterAllCategories()
        {
            AZ_Assert(m_allocations != nullptr, "Allocation records for the VRAMDriller do not exist!");
            m_allocations->UnregisterAllCategories(m_output);
        }

        //=========================================================================

        void VRAMDriller::GetCurrentVRAMStats(VRAMAllocationCategory category, VRAMAllocationSubcategory subcategory, AZStd::string& categoryName, AZStd::string& subcategoryName, size_t& numberBytesAllocated, size_t& numberAllocations)
        {
            // Verify the category exists
            const VRAMCategoryType& categoriesMap = m_allocations->GetCategoriesMap();
            auto categoryIter = categoriesMap.find(category);
            if (categoryIter != categoriesMap.end())
            {
                // Get the category and subcategory names
                const VRAMCategoryInfo& categoryInfo = categoryIter->second;
                categoryName = categoryInfo.m_categoryName;

                subcategoryName = "INVALID_SUBCATEGORY";
                for (int subCat=0; subCat<categoryInfo.m_subcategories.size(); ++subCat)
                {
                    if (categoryInfo.m_subcategories[subCat].m_subcategoryId == subcategory)
                    {
                        subcategoryName = categoryInfo.m_subcategories[subCat].m_subcategoryName;
                        break;
                    }
                }
                    
                // Get the basic allocation statistics
                VRAMDrillerAllocations::SimpleAllocationStatistics& stats = m_allocations->m_simpleAllocationStatistics[category][subcategory];
                numberBytesAllocated = stats.m_allocatedBytes;
                numberAllocations = stats.m_numberAllocations;
            }
        }

        //=========================================================================

    }// namespace Debug
} // namespace Render
