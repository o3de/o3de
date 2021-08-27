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
        class BufferView;
        class ImageView;
        struct BufferSubresourceRange;
    }

    namespace Vulkan
    {
        using StringList = AZStd::vector<AZStd::string>;
        using RawStringList = AZStd::vector<const char*>;
        using CpuVirtualAddress = uint8_t *;

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
            void InitDebugMessages(VkInstance instance, DebugMessageTypeFlag messageTypeMask);

            /// Shuts down the debug callback system.
            void ShutdownDebugMessages(VkInstance instance);

            RawStringList GetValidationLayers();

            /// Set the debug name of an object.
            void SetNameToObject(uint64_t objectHandle, const char* name, VkObjectType objectType, const Device& device);

            /// Begins a command buffer debug label
            void BeginCmdDebugLabel(VkCommandBuffer commandBuffer, const char* label, const AZ::Color color);

            /// Ends an open command buffer debug label.
            void EndCmdDebugLabel(VkCommandBuffer commandBuffer);

            /// Begins a queue debug label.
            void BeginQueueDebugLabel(VkQueue queue, const char* label, const AZ::Color color);

            /// Ends an open queue debug label.
            void EndQueueDebugLabel(VkQueue queue);
        }

        const char* GetResultString(const VkResult result);

        #define RETURN_RESULT_IF_UNSUCCESSFUL(result) \
            if (result != RHI::ResultCode::Success) {\
                return result;\
            }

        /// Checks whether the result is successful; if not, it break program execution.
        inline void AssertSuccess([[maybe_unused]] VkResult result)
        {
            AZ_Assert(result == VK_SUCCESS, "ASSERT: Vulkan API method failed: %s", GetResultString(result));
        }

        /// Checks whether the result is successful; if not, reports the error and returns false. Otherwise, returns true.
        bool IsSuccess(VkResult result);

        /// Checks whether the result is an error; if so, reports the error and returns true. Otherwise, returns false.
        bool IsError(VkResult result);

        /// Converts from a vector of AZStd::string to a vector of raw const char* pointers.
        void ToRawStringList(const StringList& source, RawStringList& dest);

        RawStringList FilterList(const RawStringList& source, const StringList& filter);

        bool ResourceViewOverlaps(const RHI::BufferView& lhs, const RHI::BufferView& rhs);

        bool ResourceViewOverlaps(const RHI::ImageView& lhs, const RHI::ImageView& rhs);

        bool SubresourceRangeOverlaps(const VkImageSubresourceRange& lhs, const VkImageSubresourceRange& rhs);

        bool SubresourceRangeOverlaps(const RHI::BufferSubresourceRange& lhs, const RHI::BufferSubresourceRange& rhs);

        bool IsRenderAttachmentUsage(RHI::ScopeAttachmentUsage usage);

        bool operator==(const VkImageSubresourceRange& lhs, const VkImageSubresourceRange& rhs);

        AZ_DEFINE_ENUM_BITWISE_OPERATORS(VkImageLayout);
    }
}
