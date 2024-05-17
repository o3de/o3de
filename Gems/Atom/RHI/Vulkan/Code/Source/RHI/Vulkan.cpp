/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/Vulkan.h>
#include <RHI/Device.h>
#include <RHI/CommandQueueContext.h>
#include <RHI/BufferView.h>
#include <RHI/Buffer.h>
#include <RHI/ImageView.h>
#include <RHI/Image.h>
#include <RHI/Instance.h>
#include <Vulkan_Traits_Platform.h>
#include <Atom/RHI.Reflect/VkAllocator.h>
#include <Atom/RHI.Reflect/Bits.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <AzCore/StringFunc/StringFunc.h>

#define VMA_IMPLEMENTATION

#include <vma/vk_mem_alloc.h>

namespace AZ
{
    namespace Vulkan
    {
        bool IsSuccess(VkResult result)
        {
            if (result != VK_SUCCESS)
            {
                AZ_Error("Vulkan", false, "ERROR: Vulkan API method failed: %s", GetResultString(result));
                return false;
            }
            return true;
        }

        bool IsError(VkResult result)
        {
            return IsSuccess(result) == false;
        }
        
        const char* GetResultString(const VkResult result)
        {
            switch (result)
            {
            case VK_SUCCESS:
                return "Success";
            case VK_NOT_READY:
                return "Not ready";
            case VK_TIMEOUT:
                return "Timeout";
            case VK_EVENT_SET:
                return "Event set";
            case VK_EVENT_RESET:
                return "Event reset";
            case VK_INCOMPLETE:
                return "Incomplete";
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                return "Out of host memory";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return "Out of device memory";
            case VK_ERROR_INITIALIZATION_FAILED:
                return "Initialization failed";
            case VK_ERROR_DEVICE_LOST:
                return "Device lost";
            case VK_ERROR_MEMORY_MAP_FAILED:
                return "Memory map failed";
            case VK_ERROR_LAYER_NOT_PRESENT:
                return "Layer not present";
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                return "Extension not present";
            case VK_ERROR_FEATURE_NOT_PRESENT:
                return "Feature not present";
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                return "Incompatible driver";
            case VK_ERROR_TOO_MANY_OBJECTS:
                return "Too many objects";
            case VK_ERROR_FORMAT_NOT_SUPPORTED:
                return "Format not supported";
            case VK_ERROR_SURFACE_LOST_KHR:
                return "Surface lost";
            case VK_SUBOPTIMAL_KHR:
                return "Suboptimal";
            case VK_ERROR_OUT_OF_DATE_KHR:
                return "Out of date";
            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
                return "Incompatible display";
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                return "Native window in use";
            case VK_ERROR_VALIDATION_FAILED_EXT:
                return "Validation failed";
            case VK_ERROR_OUT_OF_POOL_MEMORY:
                return "Pool is out of memory";
            case VK_ERROR_FRAGMENTED_POOL:
                return "Fragmented pool";
            default:
                return "Unknown error";
            }
        }    

        void ToRawStringList(const StringList& source, RawStringList& dest)
        {
            dest.clear();
            dest.reserve(source.size());
            for (const AZStd::string& s : source)
            {
                dest.push_back(s.c_str());
            }
        }

        void RemoveRawStringList(RawStringList& removeFrom, const RawStringList& toRemove)
        {
            AZStd::erase_if(
                removeFrom,
                [&](const auto& x)
                {
                    return AZStd::find_if(
                               toRemove.begin(),
                               toRemove.end(),
                               [&](const auto& y)
                               {
                                   return AZ::StringFunc::Equal(x, y);
                               }
                    ) != toRemove.end();
                });
        }

        RawStringList FilterList(const RawStringList& source, const StringList& filter)
        {
            RawStringList filteredList;
            for (auto& item : source)
            {
                if (AZStd::find(filter.begin(), filter.end(), item) != filter.end())
                {
                    filteredList.push_back(item);
                }
            }
            return filteredList;
        }

        template<class T>
        bool Overlaps(const T x1, const T x2, const T y1, const T y2)
        {
            return x1 < y2 && y1 < x2;
        }

        bool ResourceViewOverlaps(const RHI::SingleDeviceBufferView& lhs, const RHI::SingleDeviceBufferView& rhs)
        {
            auto const& lhsBufferMemoryView = static_cast<const Buffer&>(lhs.GetBuffer()).GetBufferMemoryView();
            auto const& rhsBufferMemoryView = static_cast<const Buffer&>(rhs.GetBuffer()).GetBufferMemoryView();
            if (lhsBufferMemoryView->GetNativeBuffer() != rhsBufferMemoryView->GetNativeBuffer())
            {
                return false;
            }

            const auto& lhsDesc = lhs.GetDescriptor();
            const auto& rhsDesc = rhs.GetDescriptor();
            size_t lhsBegin = lhsBufferMemoryView->GetOffset() + lhsDesc.m_elementOffset * lhsDesc.m_elementSize;
            size_t lhsEnd = lhsBegin + (lhsDesc.m_elementCount * lhsDesc.m_elementSize);
            size_t rhsBegin = rhsBufferMemoryView->GetOffset() + rhsDesc.m_elementOffset * rhsDesc.m_elementSize;
            size_t rhsEnd = rhsBegin + (rhsDesc.m_elementCount * rhsDesc.m_elementSize);
            return Overlaps(lhsBegin, lhsEnd, rhsBegin, rhsEnd);
        }

        bool ResourceViewOverlaps(const RHI::SingleDeviceImageView& lhs, const RHI::SingleDeviceImageView& rhs)
        {
            auto const& lhsImage = static_cast<const Image&>(lhs.GetImage());
            auto const& rhsImage = static_cast<const Image&>(rhs.GetImage());
            if (lhsImage.GetNativeImage() != rhsImage.GetNativeImage())
            {
                return false;
            }

            return SubresourceRangeOverlaps(
                static_cast<const ImageView&>(lhs).GetVkImageSubresourceRange(),
                static_cast<const ImageView&>(rhs).GetVkImageSubresourceRange());
        }

        bool SubresourceRangeOverlaps(const VkImageSubresourceRange& lhs, const VkImageSubresourceRange& rhs)
        {
            return
                RHI::CheckBitsAny(lhs.aspectMask, rhs.aspectMask) &&
                Overlaps(lhs.baseArrayLayer, lhs.baseArrayLayer + lhs.layerCount, rhs.baseArrayLayer, rhs.baseArrayLayer + rhs.layerCount) &&
                Overlaps(lhs.baseMipLevel, lhs.baseMipLevel + lhs.levelCount, rhs.baseMipLevel, rhs.baseMipLevel + rhs.levelCount);
        }

        bool SubresourceRangeOverlaps(const RHI::BufferSubresourceRange& lhs, const RHI::BufferSubresourceRange& rhs)
        {
            return Overlaps(lhs.m_byteOffset, lhs.m_byteOffset + lhs.m_byteSize, rhs.m_byteOffset, rhs.m_byteOffset + rhs.m_byteSize);
        }

        bool IsRenderAttachmentUsage(RHI::ScopeAttachmentUsage usage)
        {
            switch (usage)
            {
            case RHI::ScopeAttachmentUsage::RenderTarget:
            case RHI::ScopeAttachmentUsage::DepthStencil:
            case RHI::ScopeAttachmentUsage::Resolve:
            case RHI::ScopeAttachmentUsage::SubpassInput:
                return true;
            default:
                return false;
            }
        }

        bool operator==(const VkImageSubresourceRange& lhs, const VkImageSubresourceRange& rhs)
        {
            return
                lhs.aspectMask == rhs.aspectMask &&
                lhs.baseArrayLayer == rhs.baseArrayLayer &&
                lhs.baseMipLevel == rhs.baseMipLevel &&
                lhs.layerCount == rhs.layerCount &&
                lhs.levelCount == rhs.levelCount;
        }

        namespace Debug
        {
            const char* s_debugMessageLabel = "vkDebugMessage";            

            VkDebugUtilsMessengerEXT s_messageCallback = VK_NULL_HANDLE;
            VKAPI_ATTR VkBool32 VKAPI_CALL MessageCallbak(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, [[maybe_unused]] void* pUserData)
            {
                const char* severtityString = "";
                if (RHI::CheckBitsAny(messageSeverity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT))
                {
                    severtityString = "[VERBOSE]";
                } 
                else if (RHI::CheckBitsAny(messageSeverity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT))
                {
                    severtityString = "[INFO]";
                }
                else if (RHI::CheckBitsAny(messageSeverity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT))
                {
                    severtityString = "[WARNING]";
                }
                else if (RHI::CheckBitsAny(messageSeverity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT))
                {
                    severtityString = "[ERROR]";
                }

                const char* typeString = "";
                if (RHI::CheckBitsAny(static_cast<VkDebugUtilsMessageTypeFlagBitsEXT>(messageType), VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT))
                {
                    typeString = "[General]";
                }
                else if (RHI::CheckBitsAny(static_cast<VkDebugUtilsMessageTypeFlagBitsEXT>(messageType), VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT))
                {
                    typeString = "[Validation]";
                }
                else if (RHI::CheckBitsAny(static_cast<VkDebugUtilsMessageTypeFlagBitsEXT>(messageType), VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
                {
                    typeString = "[Performance]";
                }

                if (RHI::CheckBitsAny(messageSeverity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT))
                {
                    AZ_Error(s_debugMessageLabel, false, "%s%s %s\n", severtityString, typeString, pCallbackData->pMessage);
                }
                else if (RHI::CheckBitsAny(messageSeverity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT))
                {
                    AZ_Warning(s_debugMessageLabel, false, "%s%s %s\n", severtityString, typeString, pCallbackData->pMessage);
                }
                else
                {
                    AZ_Printf(s_debugMessageLabel, "%s%s %s\n", severtityString, typeString, pCallbackData->pMessage);
                }

                return VK_FALSE;
            }

            void InitDebugMessages(const GladVulkanContext& context, VkInstance instance, DebugMessageTypeFlag messageTypeMask)
            {
                if (VK_INSTANCE_EXTENSION_SUPPORTED(context, EXT_debug_utils))
                {
                    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
                    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
                    createInfo.pNext = nullptr;
                    createInfo.flags = 0;
                    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
                    createInfo.pfnUserCallback = MessageCallbak;
                    createInfo.pUserData = nullptr;

                    if (RHI::CheckBitsAny(messageTypeMask, DebugMessageTypeFlag::Warning))
                    {
                        createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                    }
                    if (RHI::CheckBitsAny(messageTypeMask, DebugMessageTypeFlag::Error))
                    {
                        createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                    }
                    if (RHI::CheckBitsAny(messageTypeMask, DebugMessageTypeFlag::Performance))
                    {
                        createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                        createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                    }
                    if (RHI::CheckBitsAny(messageTypeMask, DebugMessageTypeFlag::Info))
                    {
                        createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
                    }
                    if (RHI::CheckBitsAny(messageTypeMask, DebugMessageTypeFlag::Debug))
                    {
                        createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
                    }

                    [[maybe_unused]] VkResult result =
                        context.CreateDebugUtilsMessengerEXT(instance, &createInfo, VkSystemAllocator::Get(), &s_messageCallback);

                    AZ_Error("Vulkan", !result, "Failed to initialize the debug messaging system");
                }
            }

            void ShutdownDebugMessages(const GladVulkanContext& context, VkInstance instance)
            {
                if (s_messageCallback != VK_NULL_HANDLE)
                {
                    context.DestroyDebugUtilsMessengerEXT(instance, s_messageCallback, VkSystemAllocator::Get());
                }
            }

            RawStringList GetValidationLayers()
            {
                if (Instance::GetInstance().GetValidationMode() != RHI::ValidationMode::Disabled)
                {
                    return
                    {
                        "VK_LAYER_KHRONOS_validation"
                    };
                }
                return RawStringList{};
            }

            RawStringList GetValidationExtensions()
            {
                if (Instance::GetInstance().GetValidationMode() != RHI::ValidationMode::Disabled)
                {
                    return
                    {
                            VK_EXT_DEBUG_REPORT_EXTENSION_NAME
                    };
                }
                return RawStringList();
            }

            VkDebugUtilsLabelEXT CreateVkDebugUtilLabel(const char* label, const AZ::Color color)
            {
                VkDebugUtilsLabelEXT info = {};
                info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
                info.pLabelName = label;
                info.color[0] = color.GetR();
                info.color[1] = color.GetG();
                info.color[2] = color.GetB();
                info.color[3] = color.GetA();
                return info;
            }

            void SetNameToObject([[maybe_unused]] uint64_t objectHandle, [[maybe_unused]] const char* name, [[maybe_unused]] VkObjectType objectType, [[maybe_unused]] const Device& device)
            {
#if defined(AZ_VULKAN_USE_DEBUG_LABELS)
                AZ_Assert(objectHandle != reinterpret_cast<uint64_t>(VK_NULL_HANDLE), "objectHandle is null.");
                if (VK_DEVICE_EXTENSION_SUPPORTED(device.GetContext(), EXT_debug_utils))
                {
                    VkDebugUtilsObjectNameInfoEXT info{};
                    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
                    info.objectType = objectType;
                    info.objectHandle = objectHandle;
                    info.pObjectName = name;
                    AssertSuccess(device.GetContext().SetDebugUtilsObjectNameEXT(device.GetNativeDevice(), &info));
                }
#endif
            }

            void BeginCmdDebugLabel(
                [[maybe_unused]] const GladVulkanContext& context,
                [[maybe_unused]] VkCommandBuffer commandBuffer,
                [[maybe_unused]] const char* label,
                [[maybe_unused]] const AZ::Color color)
            {
#if defined(AZ_VULKAN_USE_DEBUG_LABELS)
                if (VK_DEVICE_EXTENSION_SUPPORTED(context, EXT_debug_utils))
                {
                    VkDebugUtilsLabelEXT info = CreateVkDebugUtilLabel(label, color);
                    context.CmdBeginDebugUtilsLabelEXT(commandBuffer, &info);
                }
#endif
            }

            void EndCmdDebugLabel([[maybe_unused]] const GladVulkanContext& context, [[maybe_unused]] VkCommandBuffer commandBuffer)
            {
#if defined(AZ_VULKAN_USE_DEBUG_LABELS)
                if (VK_DEVICE_EXTENSION_SUPPORTED(context, EXT_debug_utils))
                {
                    context.CmdEndDebugUtilsLabelEXT(commandBuffer);
                }
#endif
            }

            void BeginQueueDebugLabel(
                [[maybe_unused]] const GladVulkanContext& context,
                [[maybe_unused]] VkQueue queue,
                [[maybe_unused]] const char* label,
                [[maybe_unused]] const AZ::Color color)
            {
#if defined(AZ_VULKAN_USE_DEBUG_LABELS)
                if (VK_DEVICE_EXTENSION_SUPPORTED(context, EXT_debug_utils))
                {
                    VkDebugUtilsLabelEXT info = CreateVkDebugUtilLabel(label, color);
                    context.QueueBeginDebugUtilsLabelEXT(queue, &info);
                }
#endif
            }

            void EndQueueDebugLabel([[maybe_unused]] const GladVulkanContext& context, [[maybe_unused]] VkQueue queue)
            {
#if defined(AZ_VULKAN_USE_DEBUG_LABELS)
                if (VK_DEVICE_EXTENSION_SUPPORTED(context, EXT_debug_utils))
                {
                    context.QueueEndDebugUtilsLabelEXT(queue);
                }
#endif
            }


        } // namespace Debug

    } // namespace Vulkan
} // namespace AZ
