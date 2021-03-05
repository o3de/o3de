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
#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_MEMORY_VRAMDRILLERBUS_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_MEMORY_VRAMDRILLERBUS_H 1

#include <AzCore/Driller/DrillerBus.h>

namespace Render
{
    namespace Debug
    {
        enum VRAMAllocationCategory
        {
            VRAM_CATEGORY_TEXTURE,
            VRAM_CATEGORY_BUFFER,
            VRAM_CATEGORY_MISC,

            VRAM_CATEGORY_NUMBER_CATEGORIES,
            VRAM_CATEGORY_INVALID = VRAM_CATEGORY_NUMBER_CATEGORIES
        };

        enum VRAMAllocationSubcategory
        {
            VRAM_SUBCATEGORY_TEXTURE_RENDERTARGET,      // Rendertarget allocations
            VRAM_SUBCATEGORY_TEXTURE_TEXTURE,           // Texture resources loaded from a file
            VRAM_SUBCATEGORY_TEXTURE_DYNAMIC,           // Texture created dynamically at runtime (staging or CPU-updated)

            VRAM_SUBCATEGORY_BUFFER_VERTEX_BUFFER,      // Vertex buffers
            VRAM_SUBCATEGORY_BUFFER_INDEX_BUFFER,       // Index buffers
            VRAM_SUBCATEGORY_BUFFER_CONSTANT_BUFFER,    // Constant buffers
            VRAM_SUBCATEGORY_BUFFER_OTHER_BUFFER,       // Other buffers

            VRAM_SUBCATEGORY_MISC_OTHER,                // Other

            VRAM_SUBCATEGORY_NUMBER_SUBCATEGORIES,
            VRAM_SUBCATEGORY_INVALID = VRAM_SUBCATEGORY_NUMBER_SUBCATEGORIES,
        };

        struct VRAMSubcategory
        {
            VRAMSubcategory(VRAMAllocationSubcategory subcategoryId, const char* subcategoryName)
                : m_subcategoryId(subcategoryId)
                , m_subcategoryName(subcategoryName)
            {}

            VRAMAllocationSubcategory m_subcategoryId = VRAM_SUBCATEGORY_INVALID;
            const char* m_subcategoryName = nullptr;
        };

        typedef AZStd::vector<VRAMSubcategory, AZ::OSStdAllocator> VRAMSubCategoryType;
        
        /**
         * VRAM allocations driller message.
         *
         * We use a driller bus so all messages are sending in exclusive matter no other driller messages
         * can be triggered at that moment, so we already preserve the calling order. You can assume
         * all access code in the driller framework in guarded. You can manually lock the driller mutex are you
         * use by using \ref AZ::Debug::DrillerEBusMutex.
         */
        class VRAMDrillerMessages
            : public AZ::Debug::DrillerEBusTraits
        {
        public:
            virtual ~VRAMDrillerMessages() {}

            // Register a category with a set of subcategories.  A category
            virtual void RegisterCategory(VRAMAllocationCategory category, const char* categoryName, const VRAMSubCategoryType& subcategories) = 0;
            virtual void UnregisterAllCategories() = 0;

            // Functions for registering and unregistering individual VRAM allocations
            virtual void RegisterAllocation(void* address, size_t byteSize, const char* allocationName, VRAMAllocationCategory category, VRAMAllocationSubcategory subcategories) = 0;
            virtual void UnregisterAllocation(void* address) = 0;

            // Query the most up-to-date information about a specific category and subcategory.
            // Returns the category and subcategory names, the number of currently allocated bytes and the current number of allocations
            virtual void GetCurrentVRAMStats(VRAMAllocationCategory category, VRAMAllocationSubcategory subcategory, AZStd::string& categoryName, AZStd::string& subcategoryName, size_t& numberBytesAllocated, size_t& numberAllocations) = 0;
        };

        typedef AZ::EBus<VRAMDrillerMessages> VRAMDrillerBus;
    } // namespace Debug
} // namespace Render

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_MEMORY_VRAMDRILLERBUS_H
#pragma once