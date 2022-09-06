"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class VulkanValidationErrors:
    identifiers = ["vkDebugMessage"]

    # List to add vulkan validation errors that will be ignored.
    # For example, to ignore the following error
    #
    # [vkDebugMessage] [ERROR][Validation] Validation Error: [ VUID-VkMemoryAllocateInfo-flags-03331 ] Object 0: handle = 0x24c6516baa0, 
    # type = VK_OBJECT_TYPE_DEVICE; | MessageID = 0xf972dfbf | If VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT is set, bufferDeviceAddress must be enabled. 
    # The Vulkan spec states: If VkMemoryAllocateFlagsInfo::flags includes VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, the bufferDeviceAddress feature
    # must be enabled (https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VUID-VkMemoryAllocateInfo-flags-03331)
    #
    # The entry "VUID-VkMemoryAllocateInfo-flags-03331" can be added to the list.
    errors_to_skip = [
        ]

    def CheckError(error_id, error_msg):
        if error_id in VulkanValidationErrors.identifiers:
            for error_to_skip in VulkanValidationErrors.errors_to_skip:
                if error_to_skip in error_msg:
                    return False # Skip error
            return True # Found error
        return False # Not a Vulkan error
        