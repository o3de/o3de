/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <Atom/RHI.Reflect/Bits.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>

#include <vma/vk_mem_alloc.h>

#if !defined(_RELEASE)
    #define AZ_VULKAN_USE_DEBUG_LABELS
#endif

namespace AZ
{
    namespace RHI
    {
        class ScopeAttachment;
        // NOTE: see BufferDescriptor.h, AZ_ENUM... macro wraps enum within an outer inline namespace.
        inline namespace BufferBindFlagsNamespace
        {
            enum class BufferBindFlags : uint32_t;
        }
        class DeviceBufferView;
        class DeviceImageView;
        struct BufferSubresourceRange;
    }

    namespace Vulkan
    {
        using StringList = AZStd::vector<AZStd::string>;
        using RawStringList = AZStd::vector<const char*>;
        using CpuVirtualAddress = uint8_t *;

        struct PipelineAccessFlags
        {
            VkPipelineStageFlags m_pipelineStage = 0;
            VkAccessFlags m_access = 0;

            bool operator==(const PipelineAccessFlags& other) const
            {
                return m_pipelineStage == other.m_pipelineStage && m_access == other.m_access;
            }

            PipelineAccessFlags& operator|=(const PipelineAccessFlags& other)
            {
                m_pipelineStage |= other.m_pipelineStage;
                m_access |= other.m_access;
                return *this;
            }
        };

        //! Flags with the type of barriers used by a Scope.
        enum class BarrierTypeFlags : uint32_t
        {
            None = 0,
            Memory = AZ_BIT(0), // VkMemoryBarrier
            Buffer = AZ_BIT(1), // VkBufferMemoryBarrier
            Image = AZ_BIT(2),  // VkImageMemoryBarrier
            All = Memory | Buffer | Image
        };
        AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::Vulkan::BarrierTypeFlags);

        // Type of optimizations for scope barriers.
        enum class BarrierOptimizationFlags
        {
            None = 0,                       // No optimization.
            UseRenderpassLayout = AZ_BIT(0),// Use renderpass intialLayout and finalLayout for automatic layout transitions.
            RemoveReadAfterRead = AZ_BIT(1),// Remove read after read barriers.
            UseGlobal = AZ_BIT(2),          // Use a global memory barrier per scope instead of resource barriers (except when layout transitions are required)
            All = UseRenderpassLayout | RemoveReadAfterRead | UseGlobal // All optimizations.
        };
        AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::Vulkan::BarrierOptimizationFlags);

        class Device;

        namespace Debug
        {
            /// Default color used for debug labels.
            const AZ::Color DefaultLabelColor = AZ::Color::CreateFromRgba(0, 255, 0, 255);
            
            enum class DebugMessageTypeFlag
            {
                Info = AZ_BIT(0),
                Warning = AZ_BIT(1),
                Error = AZ_BIT(2),
                Debug = AZ_BIT(3),
                Performance = AZ_BIT(4)
            };
            AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::Vulkan::Debug::DebugMessageTypeFlag);

            /// Initializes the debug callback system.
            void InitDebugMessages(const GladVulkanContext& context, VkInstance instance, DebugMessageTypeFlag messageTypeMask);

            /// Shuts down the debug callback system.
            void ShutdownDebugMessages(const GladVulkanContext& context, VkInstance instance);

            /// Returns the instance layers used for Vulkan validation.
            RawStringList GetValidationLayers();

            /// Returns the instance extensions used for Vulkan validation.
            RawStringList GetValidationExtensions();

            /// Set the debug name of an object.
            void SetNameToObject(uint64_t objectHandle, const char* name, VkObjectType objectType, const Device& device);

            /// Begins a command buffer debug label
            void BeginCmdDebugLabel(
                const GladVulkanContext& context, VkCommandBuffer commandBuffer, const char* label, const AZ::Color color);

            /// Ends an open command buffer debug label.
            void EndCmdDebugLabel(const GladVulkanContext& context, VkCommandBuffer commandBuffer);

            /// Begins a queue debug label.
            void BeginQueueDebugLabel(const GladVulkanContext& context, VkQueue queue, const char* label, const AZ::Color color);

            /// Ends an open queue debug label.
            void EndQueueDebugLabel(const GladVulkanContext& context, VkQueue queue);
        }

        const char* GetResultString(const VkResult result);

        #define RETURN_RESULT_IF_UNSUCCESSFUL(result) \
            if (result != RHI::ResultCode::Success) {\
                return result;\
            }

        /// Checks whether the result is successful; if not, it break program execution.
        inline void AssertSuccess([[maybe_unused]] VkResult result)
        {
            if (result != VK_SUCCESS)
            {
                AZ_Assert(false, "ASSERT: Vulkan API method failed: %s", GetResultString(result));
            }
        }

        /// Checks whether the result is successful; if not, reports the error and returns false. Otherwise, returns true.
        bool IsSuccess(VkResult result);

        /// Checks whether the result is an error; if so, reports the error and returns true. Otherwise, returns false.
        bool IsError(VkResult result);

        /// Converts from a vector of AZStd::string to a vector of raw const char* pointers.
        void ToRawStringList(const StringList& source, RawStringList& dest);

        /// Removes items from a RawStringList that are contained in another RawStringList.
        void RemoveRawStringList(RawStringList& removeFrom, const RawStringList& toRemove);

        RawStringList FilterList(const RawStringList& source, const StringList& filter);

        bool ResourceViewOverlaps(const RHI::DeviceBufferView& lhs, const RHI::DeviceBufferView& rhs);

        bool ResourceViewOverlaps(const RHI::DeviceImageView& lhs, const RHI::DeviceImageView& rhs);

        /// Returns true if the lhs device completely contains the rhs resource. 
        bool ResourceViewContains(const RHI::DeviceImageView& lhs, const RHI::DeviceImageView& rhs);

        bool SubresourceRangeOverlaps(const VkImageSubresourceRange& lhs, const VkImageSubresourceRange& rhs);

        bool SubresourceRangeOverlaps(const RHI::BufferSubresourceRange& lhs, const RHI::BufferSubresourceRange& rhs);

        bool IsRenderAttachmentUsage(RHI::ScopeAttachmentUsage usage);

        /// Return true if the flags only included read accesses.
        bool IsReadOnlyAccess(VkAccessFlags access);

        /// Returns a mask for the enabled scope barrier optimizations (CVAR r_vkBarrierOptimizationFlags).
        BarrierOptimizationFlags GetBarrierOptimizationFlags();

        bool operator==(const VkImageSubresourceRange& lhs, const VkImageSubresourceRange& rhs);
        bool operator==(const VkMemoryBarrier& lhs, const VkMemoryBarrier& rhs);
        bool operator==(const VkBufferMemoryBarrier& lhs, const VkBufferMemoryBarrier& rhs);
        bool operator==(const VkImageMemoryBarrier& lhs, const VkImageMemoryBarrier& rhs);

        /// Appends a list of Vulkan structs to end of the "next" chain
        template<class T>
        void AppendVkStruct(T& init, const AZStd::vector<void*>& nextStructs)
        {
            VkBaseOutStructure* baseStruct = reinterpret_cast<VkBaseOutStructure*>(&init);
            // Find the last struct in the chain
            while (baseStruct->pNext)
            {
                baseStruct = baseStruct->pNext;
            }

            // Add the new structs to the chain
            for (void* nextStruct : nextStructs)
            {
                baseStruct->pNext = reinterpret_cast<VkBaseOutStructure*>(nextStruct);
                baseStruct = baseStruct->pNext;
            }
        }

        /// Appends a Vulkan struct to end of the "next" chain
        template<class T>
        void AppendVkStruct(T& init, void* nextStruct)
        {
            AppendVkStruct(init, AZStd::vector<void*>{ nextStruct });
        }

        AZ_DEFINE_ENUM_BITWISE_OPERATORS(VkImageLayout);
    }
}
